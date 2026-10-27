// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package mb86h47

import (
	"errors"
	"fmt"
	"time"

	"github.com/Till0196/coax/internal/coaxd/usb/bridge/mb86h47/regs"
	"github.com/Till0196/coax/pkg/usbio"
)

// Endpoint 0x81 carries 192-byte units: a four-byte arrival timestamp in
// front of each transport packet.
const (
	sourcePacketSize = 192
	transportSize    = 188
	timestampSize    = sourcePacketSize - transportSize
	syncByte         = 0x47
)

// Stream read sizing; see ReadStream.
const (
	streamReadUnit = 1536
	maxStreamRead  = 64 * 1024
)

// errNotStreaming is ReadStream on a bridge that is not streaming.
var errNotStreaming = errors.New("MB86H47A is not streaming")

// framer turns the endpoint's units into transport packets.
type framer struct {
	// pending holds bytes of a unit that the last read cut short.
	pending []byte
}

// feed takes raw bytes off the stream endpoint and appends whole transport
// packets to out. A read that does not start on a unit boundary, which the
// first one after a start usually does not, is realigned on the sync byte.
func (framer *framer) feed(raw []byte, out []byte) []byte {
	data := append(framer.pending, raw...)
	offset := 0
	for offset+sourcePacketSize <= len(data) {
		packet := data[offset+timestampSize : offset+sourcePacketSize]
		if packet[0] != syncByte {
			offset = framer.resync(data, offset+1)
			continue
		}
		out = append(out, packet...)
		offset += sourcePacketSize
	}
	framer.pending = append(framer.pending[:0], data[offset:]...)
	return out
}

// resync finds the next offset from which a unit starts: a sync byte where
// the transport packet begins and, when the data reaches that far, another
// one a unit later. A lone 0x47 in the payload is too common to trust on its
// own.
func (framer *framer) resync(data []byte, from int) int {
	for offset := from; offset+sourcePacketSize <= len(data); offset++ {
		if data[offset+timestampSize] != syncByte {
			continue
		}
		next := offset + sourcePacketSize + timestampSize
		if next >= len(data) || data[next] == syncByte {
			return offset
		}
	}
	return max(from, len(data)-sourcePacketSize+1)
}

// The stream path. The ring is in SDRAM, which the TS engine and core 0 see
// at bus address 0.
const (
	ringBase = 0x00ae4000
	ringSize = 0x00600000 // 32768 units of 192 bytes

	engineBase = 0xd00000  // TS engine; its own core stays in reset
	core0Base  = 0x1600000 // codec core 0
)

// core0Program is the program for codec core 0, placed at its address
// core0Origin (host core0Base+core0Origin). Addresses below 0x20000 are the
// core's own registers.
var core0Program = mustBuild(buildCore0(core0Ring{
	base: ringBase, size: ringSize,
	writeOffset: engineBase + 0x2038, readLimit: engineBase + 0x202c, margin: 8,
}))

// core0Ring is the ring core 0 sends to endpoint 0x81, in 192-byte units:
// where it is, the word telling how far it is written (bytes from base) and
// the word core 0 tells how far it has read, less margin.
type core0Ring struct {
	base, size             uint32
	writeOffset, readLimit uint32
	margin                 uint32
}

const (
	core0Origin = 0xa0000
	// Counters for debugging: +0 loops, +4 units sent, +8 FIFO refills, +0xC
	// last event word.
	core0Counters = 0xa3f00
)

// A stream step is one piece of the setup StartStream needs, a request or a
// few.
type streamStep func()

// streamSteps is the stream setup as steps, run by StartStream or, ahead of
// it, in the waits of a tune (Idle):
//
//   - the TS engine's input port and ring writer only (d01xxx, d02xxx): raw
//     TS from the demodulator's parallel output, 188-byte packets on sync byte
//     0x47, with a four-byte arrival timestamp, into the ring. The PID filter
//     behind it stays unused. The input and the writer stay off: while the
//     demodulator is tuning, its output is not a stream;
//   - core 0 loaded and started;
//   - endpoint 0x81: the stream DMA channels, then the endpoint reset and the
//     switch of EP1 to DMA mode. EP1 comes up in interrupt mode; without the
//     switch the packetizer stalls as soon as the endpoint buffer is full.
func (bridge *Bridge) streamSteps() []streamStep {
	var steps []streamStep
	write := func(address, value uint32) {
		steps = append(steps, func() { bridge.write32(address, value) })
	}
	run := func(step streamStep) { steps = append(steps, step) }
	usb := regs.USB

	write(0xd000c4, 0) // no interrupts to the main CPU
	write(0xd00000, 0xffffffff)
	// Pulse the sub-block resets while keeping the engine's core (bits 16-17)
	// in reset.
	write(0xd00004, 0x001f33ff)
	write(0xd00000, 0x30030fff)
	write(0xd00000, 0x00030000)
	write(0xd00004, 0x00003305)
	for _, address := range []uint32{0xd01040, 0xd01064, 0xd010a0, 0xd010a4, 0xd010a8} {
		write(address, 0)
	}
	write(0xd01020, 0x02000000) // hold the input
	write(0xd01004, 0)
	write(0xd01044, 0x00bc0000) // 188-byte packets
	write(0xd01048, 0x47000000) // sync byte
	write(0xd0104c, 0x00ffffff)
	write(0xd01050, 0)
	write(0xd01060, 0)
	write(0xd01000, 0x06070000) // port mode
	write(0xd01080, 0x01000000) // timestamp: 192-byte units
	write(0xd01088, 0)
	write(0xd0108c, 0xff)
	write(0xd01084, 0)
	write(0xd01090, 0x01000000)
	for _, address := range []uint32{0xd02028, 0xd0202c, 0xd02030, 0xd02038} {
		write(address, 0)
	}
	write(0xd02000, 0x03020300)
	write(0xd02020, ringBase)
	write(0xd02024, ringSize)
	write(0xd00004, 0x00043305)

	run(func() {
		if !bridge.core0Loaded {
			bridge.loadLocked(core0Base, core0Origin, core0Program)
			bridge.core0Loaded = bridge.err == nil
		} else {
			bridge.write32(core0Base, 0xffffffff)
		}
	})
	write(core0Base, 0xfffcffff)

	run(func() { bridge.set32(usb.Enable.Addr(), regs.USBBlockStream) })
	write(usb.DMA[0].Control.Addr(), 1)
	write(usb.DMA[1].Control.Addr(), 1)
	run(func() { bridge.clear32(usb.Reset.Addr(), regs.USBBlockStream) })
	write(usb.DMA[2].Control.Addr(), 0)
	write(usb.DMA[5].Control.Addr(), 0)
	write(usb.DMA[0].Control.Addr(), 0)
	write(usb.DMA[0].Config.Addr(), 0xb09f0000)
	write(usb.DMA[0].Control.Addr(), 2)
	write(usb.DMA[1].Control.Addr(), 0)
	write(usb.DMA[1].Config.Addr(), 0xb09f2000)
	write(usb.DMA[1].Control.Addr(), 2)
	// Channel DMA[8] would take what arrives on endpoint 0x02, which the
	// command channel needs for itself (command.go); it stays held.
	write(usb.DMA[8].Control.Addr(), 1)
	write(usb.DMA[8].Config.Addr(), 0x00008004)
	// Unk3C0C stays as it is: 0x72 there would keep the power-up code's
	// USB interrupt from giving the main CPU back to its wait loop, and the
	// endpoint goes without.
	// USB-core registers take byte-lane enables from usb.Lanes, which is
	// reset to all lanes on every request, so whole words are written.
	run(func() {
		bridge.write32(usb.Endpoint[1].Control.Addr(), 0x00ff6001) // EP1 reset
		for index := 0; index < 100 && bridge.read32(usb.Endpoint[1].Control.Addr())&1 != 0; index++ {
		}
		bridge.write32(usb.Endpoint[1].Control.Addr(), 0x00ff6000)
	})
	write(usb.EPMode.Addr(), 0x00060030) // bit 16+n: EPn in DMA mode (EP2: command.go); bit n: EPn interrupt
	return steps
}

// streamOnLocked lets the demodulator's output in: the input, its timestamp clock
// and the ring writer.
func (bridge *Bridge) streamOnLocked() {
	bridge.write32(0xd01020, 0)
	bridge.write32(0xd01040, 0x03000000) // input on
	bridge.write32(0xd010a4, 0x13170400)
	bridge.write32(0xd00484, 0x01000000) // timestamp clock
	bridge.write32(0xd00488, 0x206c0000)
	bridge.write32(0xd02010, 0x02000000) // writer on
}

// idleMargin is how much of a wait Idle leaves unused: about what a step
// takes on a slow host.
const idleMargin = 5 * time.Millisecond

// PrepareStream queues the stream setup, for Idle to run in the waits of the
// tune that follows and StartStream to finish. On hosts where a request takes
// milliseconds it is a quarter of a second that the tune would otherwise
// spend asleep.
func (bridge *Bridge) PrepareStream() {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.steps = bridge.streamSteps()
	bridge.stepsErr = nil
}

// Idle runs queued stream setup until shortly before until.
func (bridge *Bridge) Idle(until time.Time) {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	saved := bridge.err
	bridge.err = nil
	for len(bridge.steps) > 0 && time.Until(until) > idleMargin && bridge.err == nil {
		bridge.steps[0]()
		bridge.steps = bridge.steps[1:]
	}
	if bridge.err != nil && bridge.stepsErr == nil {
		bridge.stepsErr = bridge.err
	}
	bridge.err = saved
}

// StartStream finishes the stream setup, or runs all of it when nothing
// prepared it, and lets the stream in. The demodulator has to be driving its
// parallel output already.
func (bridge *Bridge) StartStream() error {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.err = nil
	if err := bridge.ringFlushLocked(); err != nil {
		return err
	}
	if bridge.steps == nil {
		bridge.steps = bridge.streamSteps()
	}
	for _, step := range bridge.steps {
		step()
	}
	bridge.steps = nil
	bridge.streamOnLocked()
	if err := bridge.stepsErr; err != nil {
		bridge.stepsErr = nil
		return err
	}
	if bridge.err != nil {
		return bridge.err
	}
	bridge.streamMu.Lock()
	bridge.framer = &framer{}
	bridge.streaming = true
	bridge.streamMu.Unlock()
	return nil
}

// StopStream holds core 0 and the engine's input, so nothing more reaches the
// endpoint, and drops any setup still queued.
func (bridge *Bridge) StopStream() error {
	bridge.streamMu.Lock()
	bridge.streaming = false
	if bridge.reader != nil {
		bridge.reader.Stop()
		bridge.reader = nil
	}
	bridge.streamMu.Unlock()
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.err = nil
	bridge.steps = nil
	if err := bridge.ringFlushLocked(); err != nil {
		return err
	}
	bridge.write32(core0Base, 0xffffffff)
	bridge.write32(0xd01040, 0)
	bridge.write32(0xd01020, 0x02000000)
	bridge.write32(0xd02010, 0x04000000)
	return bridge.err
}

// ReadStream reads transport stream packets, 188 bytes each. It returns
// usbio.ErrTimeout when nothing arrived in time, and may return 0 bytes
// without an error when what arrived was less than a packet.
//
// The endpoint is read by a ring of transfers kept in flight
// (usbio.ReaderOf), started with the first read: what the chip sends while
// the caller is busy waits in the ring rather than in the chip, and a read
// that times out loses nothing.
func (bridge *Bridge) ReadStream(buffer []byte, timeoutMS uint32) (int, error) {
	bridge.streamMu.Lock()
	defer bridge.streamMu.Unlock()
	if !bridge.streaming {
		return 0, errNotStreaming
	}
	if bridge.reader == nil {
		bridge.reader = usbio.ReaderOf(bridge.device, endpointStream)
		if err := bridge.reader.Start(); err != nil {
			bridge.reader = nil
			return 0, err
		}
	}
	// What arrives yields at most as many packets as fit, one packet of
	// room kept for the piece the last read left over.
	rawSize := min((len(buffer)/transportSize-1)*sourcePacketSize, maxStreamRead)
	rawSize -= rawSize % streamReadUnit
	if rawSize <= 0 {
		return 0, fmt.Errorf("MB86H47A stream buffer of %d bytes is too short", len(buffer))
	}
	if cap(bridge.raw) < rawSize {
		bridge.raw = make([]byte, rawSize)
	}
	raw := bridge.raw[:rawSize]
	got, err := bridge.reader.Read(raw, timeoutMS)
	if got == 0 {
		return 0, err
	}
	plain := bridge.framer.feed(raw[:got], buffer[:0])
	if len(plain) == 0 {
		// Less than a packet arrived; to the caller that is no data yet.
		return 0, usbio.ErrTimeout
	}
	return len(plain), nil
}
