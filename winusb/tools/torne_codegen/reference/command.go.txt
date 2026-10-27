// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package mb86h47

import (
	"encoding/binary"
	"errors"
	"fmt"
	"time"

	"github.com/Till0196/coax/pkg/usbio"
)

// The command channel carries the ring's accesses over the bulk endpoints
// instead of memory requests, which take about 3 ms each on some hosts while
// a packet out and its answer in take about half a millisecond.
//
// A packet is commandSize bytes on endpoint 0x02, which the endpoint's DMA
// channel writes into the slots at commandBase, one after the other; the
// memory request's handler moves the channel on a slot at a time. A packet
// holds
//
//	word 0      sequence number
//	word 1      bit 31 set when an answer is wanted; bits 7-0 the number of
//	            access words
//	words 2-    accesses, as ring slots (a modify's mask in the word after
//	            it), without the lap bit
//	last word   the sequence number again
//
// Besides the ring's accesses a packet carries memory accesses, marked by bit
// 27 alone: bit 30 makes one a read, bit 29 a FIFO access of the byte count in
// bits 7-0 to or from one register, and the address follows in the next word.
// A write brings its value, or its bytes four to a word, after that; a read
// answers with the word, or with the bytes four to a word.
//
// Codec core 1 takes the slot when both sequence numbers are the one it
// expects, runs the accesses and, when asked, answers on endpoint 0x83 with
// the sequence number, the first error since its last answer (bits 23-16)
// with the number of values read (bits 7-0), and the values. The host waits
// for an answer at the last slot before it moves the channel back to the
// first.
const (
	commandBase   uint32 = 0x82100000
	commandSlots         = 16
	commandSize          = 512
	commandWords         = commandSize/4 - 3
	commandAnswer uint32 = 1 << 31
	commandReads         = 14

	memoryAccess = ringMask
	memoryRead   = ringRead
	memoryFIFO   = 1 << 29

	endpointCommand = 0x02
	endpointAnswer  = 0x83

	// Core 1's variables for the channel, in its data RAM.
	commandVars    = 0xa3c00
	commandExpect  = 0x00
	commandSlot    = 0x04
	commandPointer = 0x08
	commandLeft    = 0x0c
	commandCount   = 0x10
	commandError   = 0x14
	commandFlags   = 0x18
	commandMask    = 0x1c
	commandOp      = 0x20
	commandAnswers = 0xa3c40

	// Endpoint 0x83's FIFO and the register that sends what it holds.
	answerFIFO = 0xfdbd8c
	answerSend = 0xfdbd1c

	commandTimeoutMS = 500
)

// commandSetupLocked points endpoint 0x02's DMA channel at the slots and
// checks that core 1 answers a packet; without an answer the ring over memory
// requests stays in use. Core 1 expects the sequence number the host starts
// from, which is new every time so that no packet of an earlier run matches.
func (bridge *Bridge) commandSetupLocked() {
	bridge.command = false
	// Channel 0xFD8300 would take what arrives on the endpoint; bit 18 of
	// 0xFDBC14 puts the endpoint in DMA mode; the handler moves the DMA
	// channel on after every packet.
	bridge.write32(0xfd8300, 1)
	bridge.write32(0xfd8304, 0x00008004)
	bridge.write32(0xfdbc14, bridge.read32(0xfdbc14)|0x00040000)
	bridge.write32(0xfd80b4, bridge.read32(0xfd80b4)|4)
	bridge.write32(0xfd8510, commandSize)
	bridge.write32(0xfd8508, commandBase)
	bridge.write32(0xfd8504, 1)
	if bridge.err != nil {
		bridge.err = nil
		return
	}
	// Whatever an earlier run left on the answer endpoint goes.
	stale := make([]byte, 64)
	for index := 0; index < 8; index++ {
		if n, err := bridge.device.Bulk(endpointAnswer, stale, 5); err != nil || n == 0 {
			break
		}
	}
	bridge.commandHeld = bridge.commandHeld[:0]
	bridge.command = true
	if _, err := bridge.commandSendLocked(true); err != nil {
		bridge.command = false
	}
}

// commandSeed is the first sequence number of a run.
func commandSeed() uint32 {
	return uint32(time.Now().UnixNano())&0x7fffffff | 1
}

// commandPushLocked adds accesses to the next packet, sending the packet when
// they would not fit.
func (bridge *Bridge) commandPushLocked(words ...uint32) error {
	if len(bridge.commandHeld)+len(words) > commandWords {
		if _, err := bridge.commandSendLocked(false); err != nil {
			return err
		}
	}
	bridge.commandHeld = append(bridge.commandHeld, words...)
	return nil
}

// errCommandAnswer is an answer that never came or does not fit the packet.
var errCommandAnswer = errors.New("MB86H47A command channel: no answer from codec core 1")

// commandSendLocked sends the accesses held as a packet and, when answer is
// set or the packet takes the last slot, waits for core 1's answer and returns
// the values read.
func (bridge *Bridge) commandSendLocked(answer bool) ([]uint32, error) {
	if len(bridge.commandHeld) == 0 && !answer {
		return nil, nil
	}
	if bridge.commandSlot == commandSlots-1 {
		answer = true
	}
	sequence := bridge.commandSequence
	packet := make([]byte, commandSize)
	binary.BigEndian.PutUint32(packet, sequence)
	flags := uint32(len(bridge.commandHeld))
	if answer {
		flags |= commandAnswer
	}
	binary.BigEndian.PutUint32(packet[4:], flags)
	for index, word := range bridge.commandHeld {
		binary.BigEndian.PutUint32(packet[8+index*4:], word)
	}
	binary.BigEndian.PutUint32(packet[commandSize-4:], sequence)
	bridge.commandHeld = bridge.commandHeld[:0]
	if _, err := bridge.device.Bulk(endpointCommand, packet, commandTimeoutMS); err != nil {
		bridge.command = false
		return nil, fmt.Errorf("MB86H47A command channel: %w", err)
	}
	bridge.commandSequence++
	bridge.commandSlot++
	if !answer {
		return nil, nil
	}

	message := make([]byte, 64)
	deadline := time.Now().Add(commandTimeoutMS * time.Millisecond)
	for {
		n, err := bridge.device.Bulk(endpointAnswer, message, commandTimeoutMS)
		if err != nil && !errors.Is(err, usbio.ErrTimeout) {
			bridge.command = false
			return nil, fmt.Errorf("MB86H47A command channel: %w", err)
		}
		if n >= 8 && binary.BigEndian.Uint32(message) == sequence {
			message = message[:n]
			break
		}
		if time.Now().After(deadline) {
			bridge.command = false
			return nil, errCommandAnswer
		}
	}
	if bridge.commandSlot == commandSlots {
		// Core 1 is done with every slot: the channel goes back to the
		// first.
		bridge.commandSlot = 0
		bridge.write32(0xfd8508, commandBase)
		bridge.write32(0xfd8504, 1)
	}
	status := binary.BigEndian.Uint32(message[4:])
	count := int(status & 0xff)
	if 8+count*4 > len(message) {
		return nil, errCommandAnswer
	}
	values := make([]uint32, count)
	for index := range values {
		values[index] = binary.BigEndian.Uint32(message[8+index*4:])
	}
	if failure := byte(status >> 16); failure != i2cOK {
		return values, i2cError(bridge.ringAddresses[0], failure)
	}
	return values, bridge.err
}

// registerWriteLocked writes a chip register, held back in the next command
// packet when the channel is up.
func (bridge *Bridge) registerWriteLocked(address, value uint32) {
	if !bridge.command {
		bridge.write32(address, value)
		return
	}
	bridge.keepLocked(bridge.commandPushLocked(memoryAccess, address, value))
}

// registerReadLocked reads a chip register after whatever is held back.
func (bridge *Bridge) registerReadLocked(address uint32) uint32 {
	if !bridge.command {
		bridge.flushLocked()
		return bridge.read32(address)
	}
	if !bridge.keepLocked(bridge.commandPushLocked(memoryAccess|memoryRead, address)) {
		return 0
	}
	values, err := bridge.commandSendLocked(true)
	if !bridge.keepLocked(err) || len(values) == 0 {
		bridge.keepLocked(errCommandAnswer)
		return 0
	}
	return values[len(values)-1]
}

// fifoWriteLocked writes bytes one after the other to a FIFO register.
func (bridge *Bridge) fifoWriteLocked(address uint32, data []byte) {
	if !bridge.command {
		for _, value := range data {
			bridge.write32(address, uint32(value))
		}
		return
	}
	for len(data) > 0 {
		chunk := data[:min(len(data), 0xff)]
		data = data[len(chunk):]
		words := []uint32{memoryAccess | memoryFIFO | uint32(len(chunk)), address}
		for offset := 0; offset < len(chunk); offset += 4 {
			var word [4]byte
			copy(word[:], chunk[offset:])
			words = append(words, binary.BigEndian.Uint32(word[:]))
		}
		if !bridge.keepLocked(bridge.commandPushLocked(words...)) {
			return
		}
	}
}

// fifoReadLocked reads length bytes one after the other from a FIFO
// register, after whatever is held back.
func (bridge *Bridge) fifoReadLocked(address uint32, length int) []byte {
	data := make([]byte, 0, length)
	if !bridge.command {
		bridge.flushLocked()
		for len(data) < length && bridge.err == nil {
			data = append(data, byte(bridge.read32(address)))
		}
		return data
	}
	for len(data) < length {
		chunk := min(length-len(data), commandReads*4)
		if !bridge.keepLocked(bridge.commandPushLocked(memoryAccess|memoryRead|memoryFIFO|uint32(chunk), address)) {
			return data
		}
		values, err := bridge.commandSendLocked(true)
		if !bridge.keepLocked(err) {
			return data
		}
		for _, value := range values {
			data = binary.BigEndian.AppendUint32(data, value)
		}
		if len(data) > length {
			data = data[:length]
		}
	}
	return data
}

// flushLocked sends what is held back, keeping the first error.
func (bridge *Bridge) flushLocked() { bridge.keepLocked(bridge.ringFlushLocked()) }

// keepLocked keeps err as bridge.err unless one is kept already, and reports
// whether there was none.
func (bridge *Bridge) keepLocked(err error) bool {
	if err != nil && bridge.err == nil {
		bridge.err = err
	}
	return err == nil
}
