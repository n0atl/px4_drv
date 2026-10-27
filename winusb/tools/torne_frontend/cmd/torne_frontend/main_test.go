// SPDX-License-Identifier: GPL-2.0-only
package main

import (
	"bytes"
	"encoding/binary"
	"io"
	"testing"
)

type shortWriter struct{}
func (shortWriter) Write(data []byte)(int,error){return len(data)-1,nil}

func TestFrames(t *testing.T) {
	for _,size:=range []int{0,1,32,4096} {
		var b bytes.Buffer
		want:=frame{kind:0x11,body:bytes.Repeat([]byte{0x47},size)}
		if err:=send(&b,want);err!=nil {t.Fatal(err)}
		got,err:=receive(&b);if err!=nil || got.kind!=want.kind || !bytes.Equal(got.body,want.body) {t.Fatalf("round trip %d: %v",size,err)}
	}
	h:=make([]byte,8);binary.LittleEndian.PutUint32(h[4:],4097)
	if _,err:=receive(bytes.NewReader(h));err==nil {t.Fatal("oversized frame accepted")}
	if _,err:=receive(bytes.NewReader([]byte{1}));err==nil {t.Fatal("truncated header accepted")}
	if err:=send(shortWriter{},frame{kind:0x10});err!=io.ErrShortWrite {t.Fatal("short write accepted",err)}
}

func TestProxyBus(t *testing.T) {
	var response,requests bytes.Buffer
	if err:=send(&response,frame{kind:0x81,body:[]byte{1,2}});err!=nil {t.Fatal(err)}
	bus:=&proxyBus{input:&response,output:&requests}
	got,err:=bus.WriteRead(0x12,[]byte{3,0x80},2)
	if err!=nil || !bytes.Equal(got,[]byte{1,2}) {t.Fatal(got,err)}
	f,err:=receive(&requests)
	if err!=nil || f.kind!=1 || f.address!=0x12 || f.writes!=2 || f.reads!=2 || !bytes.Equal(f.body,[]byte{3,0x80}) {t.Fatal(f,err)}
	if _,err:=bus.WriteRead(0x13,nil,2);err==nil {t.Fatal("odd address accepted")}
	if _,err:=bus.WriteRead(0x12,nil,33);err==nil {t.Fatal("oversized read accepted")}
}

func TestInitializationRequired(t *testing.T) {
	var input,output bytes.Buffer
	if err:=send(&input,frame{kind:0x13});err!=nil {t.Fatal(err)}
	if err:=run(&input,&output);err!=nil && err!=io.EOF {t.Fatal(err)}
	f,err:=receive(&output)
	if err!=nil || f.kind!=0xff {t.Fatal("status before initialization accepted",f,err)}
}
