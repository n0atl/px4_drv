// SPDX-License-Identifier: GPL-2.0-only
// Diagnostic reference frontend. USB is owned exclusively by the native parent.
package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"os"

	"github.com/Till0196/coax/internal/coaxd/demod/dib8096"
)

const maxBody = 4096

type frame struct {
	kind, address, writes, reads byte
	body []byte
}

func receive(input io.Reader) (frame, error) {
	var h [8]byte
	if _, err := io.ReadFull(input, h[:]); err != nil { return frame{}, err }
	n := binary.LittleEndian.Uint32(h[4:])
	if n > maxBody { return frame{}, errors.New("oversized protocol frame") }
	f := frame{kind:h[0], address:h[1], writes:h[2], reads:h[3], body:make([]byte,n)}
	_, err := io.ReadFull(input, f.body)
	return f, err
}

func send(output io.Writer, f frame) error {
	if len(f.body) > maxBody { return errors.New("oversized protocol frame") }
	h := [8]byte{f.kind,f.address,f.writes,f.reads}
	binary.LittleEndian.PutUint32(h[4:],uint32(len(f.body)))
	if n, err := output.Write(h[:]); err != nil { return err } else if n!=len(h) { return io.ErrShortWrite }
	if len(f.body) != 0 { n, err := output.Write(f.body); if err!=nil{return err};if n!=len(f.body){return io.ErrShortWrite} }
	return nil
}

type proxyBus struct { input io.Reader; output io.Writer }

func (bus *proxyBus) WriteRead(address byte, write []byte, readLength int) ([]byte,error) {
	if len(write)>32 || readLength<0 || readLength>32 || address&1 != 0 { return nil,errors.New("invalid I2C request") }
	if err := send(bus.output,frame{kind:1,address:address,writes:byte(len(write)),reads:byte(readLength),body:write}); err != nil { return nil,err }
	f,err := receive(bus.input)
	if err != nil { return nil,err }
	if f.kind == 0xff { return nil,fmt.Errorf("native I2C: %s",f.body) }
	if f.kind!=0x81 || len(f.body)!=readLength { return nil,errors.New("invalid I2C response") }
	return f.body,nil
}
func (bus *proxyBus) Write(address byte,data []byte) error { _,err:=bus.WriteRead(address,data,0);return err }
func (bus *proxyBus) Read(address byte,length int)([]byte,error) { return bus.WriteRead(address,nil,length) }

// execute has no USB access. A single persistent demodulator retains calibration.
func execute(demod *dib8096.Demod, f frame) ([]byte,error) {
	switch f.kind {
	case 0x10:
		if len(f.body)!=0 { return nil,errors.New("invalid initialize command") }
		if err:=demod.Initialize();err!=nil { return nil,err }
		// CECH-ZD1J: 元coaxのチューナーOpenと同じGPIOで使用中表示にする。
		return nil,demod.SetGPIO(1,true)
	case 0x11:
		if len(f.body)!=4 { return nil,errors.New("invalid tune command") }
		if err:=demod.Tune(binary.LittleEndian.Uint32(f.body));err!=nil { return nil,err }
		// Transport failureとは分け、元の探索処理が失敗を確定した場合だけ通知する。
		if demod.SearchFailed() { return []byte{1},nil }
		return nil,nil
	case 0x12:
		if len(f.body)!=1 || f.body[0]>byte(dib8096.OutputFIFO) { return nil,errors.New("invalid output command") }
		return nil,demod.SetOutputMode(dib8096.OutputMode(f.body[0]))
	case 0x13:
		if len(f.body)!=0 { return nil,errors.New("invalid status command") }
		status,err:=demod.Status()
		if err!=nil { return nil,err }
		body:=make([]byte,24)
		if status.Locked { body[0]=1 }; if status.HasCNR { body[1]=1 }; if status.HasStrength { body[2]=1 }
		binary.LittleEndian.PutUint64(body[8:],math.Float64bits(status.CNR))
		binary.LittleEndian.PutUint64(body[16:],math.Float64bits(status.StrengthDBm))
		return body,nil
	case 0x14:
		if len(f.body)!=0 { return nil,errors.New("invalid sleep command") }
		if err:=demod.SetGPIO(1,false);err!=nil { return nil,err }
		return nil,demod.Sleep()
	default: return nil,errors.New("unknown frontend command")
	}
}

func run(input io.Reader,output io.Writer) error {
	bus:=&proxyBus{input:input,output:output}
	demod:=dib8096.New(bus)
	initialized:=false
	for {
		f,err:=receive(input)
		if errors.Is(err,io.EOF) { return nil }; if err!=nil { return err }
		var body []byte
		if !initialized && f.kind!=0x10 { err=errors.New("frontend is not initialized") } else { body,err=execute(demod,f) }
		if f.kind==0x10 { initialized=err==nil }
		response:=frame{kind:0x80,body:body}
		if err!=nil { response.kind=0xff; response.body=[]byte(err.Error()); if len(response.body)>maxBody { response.body=response.body[:maxBody] } }
		if err:=send(output,response); err!=nil { return err }
	}
}

func main() {
	if len(os.Args)!=2 || os.Args[1]!="--stdio-diagnostic-v1" { fmt.Fprintln(os.Stderr,"Diagnostic helper: use the native torne frontend probe");os.Exit(2) }
	if err:=run(os.Stdin,os.Stdout);err!=nil {fmt.Fprintln(os.Stderr,err);os.Exit(1)}
}
