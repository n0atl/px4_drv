// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package mb86h47

import (
	"encoding/binary"
	"fmt"
	"time"

	"github.com/Till0196/coax/internal/coaxd/usb/bridge/mb86h47/regs"
)

// I2C master (regs.USB.I2C): status, control, clock, data. The host sets it
// up; the transfers themselves are run by codec core 1, because run from the
// host every byte costs four memory requests, and a request takes about 3 ms
// on some hosts.
var (
	i2cBase    = regs.USB.I2C.Status.Addr()
	i2cStatus  = regs.USB.I2C.Status.Addr()  // bit 7 busy, bit 4 no acknowledge
	i2cControl = regs.USB.I2C.Control.Addr() // bit 0 done, bit 7 bus error
	i2cClock   = regs.USB.I2C.Clock.Addr()
	i2cData    = regs.USB.I2C.Data.Addr()
)

const i2cTimeout = 500 * time.Millisecond

func (bridge *Bridge) i2cInitLocked() {
	usb := regs.USB
	bridge.set32(usb.Enable.Addr(), regs.USBBlockI2C)
	bridge.clear32(usb.Reset.Addr(), regs.USBBlockI2C)
	bridge.write32(usb.Unk0C0.Addr(), 1)
	bridge.write32(usb.Unk0C0.Addr(), 0)
	bridge.write32(usb.Unk0C4.Addr(), 0)
	bridge.write16(0x90114, 0x8008)
	bridge.write32(usb.I2C.Unk18.Addr(), 0x06)
	// The bus clock's divider. 0x10 is about 100 kHz; 0x08 doubles it,
	// a register access taking 0.3-0.45 ms instead of 0.5-0.75. With 0x05
	// the DiB8000 already loses reads while it initializes.
	bridge.write32(usb.I2C.Divider.Addr(), 0x08)
	bridge.write32(i2cClock, 0x3c)
	bridge.write32(i2cControl, 0x40)
}

// Codec core 1 serves two queues, both in its data RAM.
//
// Register accesses go through a ring of sixteen one-word slots: a write of
// two bytes after a one- or two-byte register number, or a read of two bytes
// after one. A write costs the host one request and is not waited for, a read
// one request and one read of the answer. A slot holds
//
//	bit  31     lap: the slot's number divided by sixteen, modulo two
//	bit  30     1 for a read
//	bit  29     1 for a one-byte register number
//	bit  28     1 for a modify: a two-byte register is read, its bits
//	            outside the next slot's mask cleared, the value or'ed in,
//	            and the result written back
//	bit  27     with bit 28, the slot after a modify: bits 15-0 are its mask
//	bits 26-16  register number
//	bits 15-0   value to write
//
// Accesses with a two-byte register number go to the slave address in byte 0
// of ringAddresses, those with a one-byte number to the one in byte 1.
//
// A read can also be one request of its own: the memory request's handler
// puts it into the ring and returns the answer (ringExecute).
// and after each one the core leaves the slot's number, an error and the
// value read in ringAnswer. The first error of an access nobody waited for is
// kept in ringFailure until the host takes it.
//
// Anything else goes through the mailbox:
//
//	+0x00  command: sequence, 8-bit slave address, bytes to write, bytes to
//	       read; the sequence is written last and starts the transfer
//	+0x04  bytes to write
//	+0x7C  a counter the program advances while it waits
//	+0x80  answer: sequence, 0, 0, error
//	+0x84  bytes read
//
// A transfer writes, if there is anything to write, and then reads, if there
// is anything to read; a read after a write follows a repeated start.
const (
	core1Base   = 0x1700000
	core1Origin = 0xa0000

	mailbox        = 0xa3f00
	mailboxWrite   = 0x04
	mailboxAlive   = 0x7c
	mailboxAnswer  = 0x80
	mailboxRead    = 0x84
	i2cMaxTransfer = 32

	// ringScratch is the mailbox layout core 1 fills in for a ring access.
	ringScratch          = 0xa3d00
	ringAddresses        = 0xa3d40
	ring                 = 0xa3e00
	ringSlots            = 16
	ringAnswer           = 0xa3e40
	ringFailure          = 0xa3e44
	ringLap       uint32 = 1 << 31
	ringRead             = 1 << 30
	ringShort            = 1 << 29
	ringModify           = 1 << 28
	ringMask             = 1 << 27
	ringRegister         = 0x7ff

	// Where a modify keeps its value and mask while it reads, in the scratch
	// mailbox.
	modifyValue = 0x60
	modifyMask  = 0x62

	// ringExecute in wValue's low byte makes a read a ring read of its own:
	// the request carries the access and returns the answer (addressing.go).
	ringExecute = 0xfe
)

// Errors core 1 answers with.
const (
	i2cOK = iota
	i2cBusError
	i2cNoAddressAck
	i2cNoDataAck
	i2cStuck
)

var core1Program = mustBuild(buildCore1)

// core1StartLocked loads core1Program and starts it, then waits to see it
// run. The queues are cleared first: the data RAM holds whatever it powered
// up with.
func (bridge *Bridge) core1StartLocked() error {
	bridge.loadLocked(core1Base, core1Origin, core1Program)
	for _, address := range []uint32{mailbox, mailbox + mailboxAnswer, mailbox + mailboxAlive, ringScratch, ringAddresses, ringFailure} {
		bridge.write32(core1Base+address, 0)
	}
	bridge.writeBlock(core1Base+ring, make([]byte, ringSlots*4))
	// The command channel starts from a new sequence number at the first
	// slot.
	bridge.commandSequence, bridge.commandSlot, bridge.command = commandSeed(), 0, false
	bridge.write32(core1Base+commandVars+commandSlot, commandBase)
	bridge.write32(core1Base+commandVars+commandError, 0)
	bridge.write32(core1Base+commandVars+commandExpect, bridge.commandSequence)
	// The ring starts at slot 16, whose lap is 1; the answer names the slot
	// before.
	bridge.write32(core1Base+ringAnswer, (ringSlots-1)<<24)
	bridge.write32(core1Base, 0xfffcffff)
	bridge.sequence = 0
	bridge.ringNext, bridge.ringDone, bridge.ringAddresses = ringSlots, ringSlots-1, [2]byte{}
	bridge.ringHeld = bridge.ringHeld[:0]
	before := bridge.read32(core1Base + mailbox + mailboxAlive)
	time.Sleep(time.Millisecond)
	after := bridge.read32(core1Base + mailbox + mailboxAlive)
	if bridge.err != nil {
		return bridge.err
	}
	if before == after {
		return fmt.Errorf("MB86H47A codec core 1 did not start")
	}
	return nil
}

func i2cError(address byte, code byte) error {
	switch code {
	case i2cOK:
		return nil
	case i2cBusError:
		return fmt.Errorf("MB86H47A I2C bus error at %#02x", address)
	case i2cNoAddressAck:
		return fmt.Errorf("MB86H47A I2C address %#02x not acknowledged", address)
	case i2cNoDataAck:
		return fmt.Errorf("MB86H47A I2C data to %#02x not acknowledged", address)
	case i2cStuck:
		return fmt.Errorf("MB86H47A I2C timeout at %#02x", address)
	}
	return fmt.Errorf("MB86H47A I2C transfer to %#02x: error %d", address, code)
}

// ringPollLocked reads the ring's answer and failure, and learns from the
// answer how far core 1 has come.
func (bridge *Bridge) ringPollLocked() (answer uint32, err error) {
	var status [8]byte
	bridge.readBlock(core1Base+ringAnswer, status[:])
	if bridge.err != nil {
		return 0, bridge.err
	}
	answer = binary.BigEndian.Uint32(status[:])
	last := bridge.ringNext - 1
	bridge.ringDone = last - uint32(byte(last)-byte(answer>>24))
	if failure := binary.BigEndian.Uint32(status[4:]); failure != 0 {
		bridge.write32(core1Base+ringFailure, 0)
		return answer, i2cError(bridge.ringSent[byte(failure>>24)], byte(failure>>16))
	}
	return answer, nil
}

// ringWaitLocked waits for core 1 to finish the access numbered done.
func (bridge *Bridge) ringWaitLocked(done uint32) (answer uint32, err error) {
	if int32(bridge.ringDone-done) >= 0 {
		// Known finished already; only the waits of queued writes get here.
		return 0, nil
	}
	deadline := time.Now().Add(i2cTimeout)
	for {
		answer, err := bridge.ringPollLocked()
		if err != nil || int32(bridge.ringDone-done) >= 0 {
			return answer, err
		}
		if time.Now().After(deadline) {
			return 0, fmt.Errorf("MB86H47A I2C: no answer from codec core 1")
		}
	}
}

// ringPushLocked queues a register access on the device at address; short
// accesses have a one-byte register number. The slot is held back until
// ringFlushLocked, which sends everything held in as few writes as the ring
// allows.
func (bridge *Bridge) ringPushLocked(address byte, short bool, access uint32) error {
	if short {
		access |= ringShort
	}
	if err := bridge.ringAddressLocked(address, short); err != nil {
		return err
	}
	if bridge.command {
		return bridge.commandPushLocked(access)
	}
	next := bridge.ringNext
	if next/ringSlots%2 == 1 {
		access |= ringLap
	}
	bridge.ringHeld = append(bridge.ringHeld, access)
	bridge.ringSent[byte(next)] = address
	bridge.ringNext++
	if len(bridge.ringHeld) == ringSlots {
		return bridge.ringFlushLocked()
	}
	return bridge.err
}

// ringAddressLocked has the ring serve address for accesses of the kind
// given, once what it holds for the address before is done.
func (bridge *Bridge) ringAddressLocked(address byte, short bool) error {
	kind := 0
	if short {
		kind = 1
	}
	if bridge.ringAddresses[kind] == address {
		return nil
	}
	if err := bridge.ringDrainLocked(); err != nil {
		return err
	}
	bridge.ringAddresses[kind] = address
	bridge.write32(core1Base+ringAddresses, uint32(bridge.ringAddresses[0])<<24|uint32(bridge.ringAddresses[1])<<16)
	return bridge.err
}

// ringFlushLocked sends the slots held back, once core 1 is done with the
// ones they replace: one write up to the end of the ring, and one from its
// start.
func (bridge *Bridge) ringFlushLocked() error {
	if bridge.command {
		_, err := bridge.commandSendLocked(false)
		return err
	}
	if len(bridge.ringHeld) == 0 {
		return bridge.err
	}
	if last := bridge.ringNext - 1; last-bridge.ringDone > ringSlots {
		if _, err := bridge.ringWaitLocked(last - ringSlots); err != nil {
			return err
		}
	}
	first := bridge.ringNext - uint32(len(bridge.ringHeld))
	held := bridge.ringHeld
	for len(held) > 0 {
		slot := first % ringSlots
		count := min(len(held), int(ringSlots-slot))
		data := make([]byte, 0, count*4)
		for _, word := range held[:count] {
			data = binary.BigEndian.AppendUint32(data, word)
		}
		bridge.writeBlock(core1Base+ring+slot*4, data)
		held, first = held[count:], first+uint32(count)
	}
	bridge.ringHeld = bridge.ringHeld[:0]
	return bridge.err
}

// ringDrainLocked sends what is held back and waits for core 1 to finish it.
func (bridge *Bridge) ringDrainLocked() error {
	if bridge.command {
		_, err := bridge.commandSendLocked(true)
		return err
	}
	if err := bridge.ringFlushLocked(); err != nil {
		return err
	}
	_, err := bridge.ringWaitLocked(bridge.ringNext - 1)
	return err
}

// ringReadLocked reads a register through the ring: what is held back goes
// first, then the read itself as one request that returns its answer.
func (bridge *Bridge) ringReadLocked(address byte, short bool, number uint32) (uint32, error) {
	if err := bridge.ringAddressLocked(address, short); err != nil {
		return 0, err
	}
	if bridge.command {
		access := ringRead | number<<16
		if short {
			access |= ringShort
		}
		if err := bridge.commandPushLocked(access); err != nil {
			return 0, err
		}
		values, err := bridge.commandSendLocked(true)
		if err != nil {
			return 0, err
		}
		if len(values) == 0 {
			return 0, errCommandAnswer
		}
		return uint32(values[len(values)-1]), nil
	}
	if err := bridge.ringFlushLocked(); err != nil {
		return 0, err
	}
	next := bridge.ringNext
	if next-bridge.ringDone > ringSlots {
		if _, err := bridge.ringWaitLocked(next - ringSlots); err != nil {
			return 0, err
		}
	}
	slot := next%ringSlots | next/ringSlots%2<<4
	if short {
		slot |= 1 << 5
	}
	bridge.ringSent[byte(next)] = address
	bridge.ringNext++
	var status [8]byte
	bridge.readBlock(ringExecute<<24|slot<<16|number, status[:])
	if bridge.err != nil {
		return 0, bridge.err
	}
	if answer := binary.BigEndian.Uint32(status[:]); byte(answer>>24) == byte(next) {
		bridge.ringDone = next
		if failure := binary.BigEndian.Uint32(status[4:]); failure != 0 {
			bridge.write32(core1Base+ringFailure, 0)
			return answer, i2cError(bridge.ringSent[byte(failure>>24)], byte(failure>>16))
		}
		return answer, nil
	}
	// The handler gave up waiting; the access is in the ring all the same.
	return bridge.ringWaitLocked(next)
}

// ringNumber reads the register number that starts a ring access: two
// bytes, or one for a short access.
func ringNumber(data []byte, short bool) (uint32, bool) {
	if short {
		return uint32(data[0]), true
	}
	number := uint32(binary.BigEndian.Uint16(data))
	return number, number <= ringRegister
}

// transferLocked runs one transfer through the mailbox, after whatever the
// ring still holds.
func (bridge *Bridge) transferLocked(address byte, write []byte, readLength int) ([]byte, error) {
	if len(write) > i2cMaxTransfer || readLength > i2cMaxTransfer {
		return nil, fmt.Errorf("MB86H47A I2C transfer of %d+%d bytes is longer than %d", len(write), readLength, i2cMaxTransfer)
	}
	bridge.err = nil
	if err := bridge.ringDrainLocked(); err != nil {
		return nil, err
	}
	bridge.sequence++
	if bridge.sequence == 0 {
		bridge.sequence = 1
	}
	for offset := 0; offset < len(write); offset += 4 {
		var word [4]byte
		copy(word[:], write[offset:])
		bridge.write32(core1Base+mailbox+mailboxWrite+uint32(offset), binary.BigEndian.Uint32(word[:]))
	}
	bridge.write32(core1Base+mailbox, uint32(bridge.sequence)<<24|uint32(address)<<16|uint32(len(write))<<8|uint32(readLength))
	answer := make([]byte, 4+(readLength+3)&^3)
	deadline := time.Now().Add(i2cTimeout)
	for {
		bridge.readBlock(core1Base+mailbox+mailboxAnswer, answer)
		if bridge.err != nil {
			return nil, bridge.err
		}
		if answer[0] == bridge.sequence {
			break
		}
		if time.Now().After(deadline) {
			return nil, fmt.Errorf("MB86H47A I2C transfer to %#02x: no answer from codec core 1", address)
		}
	}
	if err := i2cError(address, answer[3]); err != nil {
		return nil, err
	}
	return answer[4 : 4+readLength], nil
}

// Write writes data to the slave at the (8-bit) address. A register write,
// three or four bytes, is held back until the next read, Flush or stream
// start or stop, and a failure of it is reported by a later call.
func (bridge *Bridge) Write(address byte, data []byte) error {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.err = nil
	if len(data) == 3 || len(data) == 4 {
		short := len(data) == 3
		if number, ok := ringNumber(data, short); ok {
			return bridge.ringPushLocked(address, short, number<<16|uint32(binary.BigEndian.Uint16(data[len(data)-2:])))
		}
	}
	_, err := bridge.transferLocked(address, data, 0)
	return err
}

// Modify16 changes a 16-bit register with a two-byte number on codec core 1:
// the bits outside keep are cleared and set is or'ed in. It is held back like
// a register write.
func (bridge *Bridge) Modify16(address byte, register, keep, set uint16) error {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.err = nil
	if uint32(register) > ringRegister {
		value, err := bridge.transferLocked(address, []byte{byte(register >> 8), byte(register)}, 2)
		if err != nil {
			return err
		}
		value16 := (uint16(value[0])<<8|uint16(value[1]))&keep | set
		_, err = bridge.transferLocked(address, []byte{byte(register >> 8), byte(register), byte(value16 >> 8), byte(value16)}, 0)
		return err
	}
	if err := bridge.ringAddressLocked(address, false); err != nil {
		return err
	}
	if bridge.command {
		return bridge.commandPushLocked(ringModify|uint32(register)<<16|uint32(set), ringModify|ringMask|uint32(keep))
	}
	// The two slots go out together: core 1 waits for the mask.
	if len(bridge.ringHeld) > ringSlots-2 {
		if err := bridge.ringFlushLocked(); err != nil {
			return err
		}
	}
	if err := bridge.ringPushLocked(address, false, ringModify|uint32(register)<<16|uint32(set)); err != nil {
		return err
	}
	return bridge.ringPushLocked(address, false, ringModify|ringMask|uint32(keep))
}

// Read reads length bytes from the slave at the (8-bit) address.
func (bridge *Bridge) Read(address byte, length int) ([]byte, error) {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	return bridge.transferLocked(address, nil, length)
}

// WriteRead writes and then reads after a repeated start.
func (bridge *Bridge) WriteRead(address byte, write []byte, readLength int) ([]byte, error) {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.err = nil
	if (len(write) == 1 || len(write) == 2) && readLength == 2 {
		short := len(write) == 1
		if number, ok := ringNumber(write, short); ok {
			answer, err := bridge.ringReadLocked(address, short, number)
			if err != nil {
				return nil, err
			}
			if err := i2cError(address, byte(answer>>16)); err != nil {
				return nil, err
			}
			return []byte{byte(answer >> 8), byte(answer)}, nil
		}
	}
	return bridge.transferLocked(address, write, readLength)
}

// Flush sends the register writes held back.
func (bridge *Bridge) Flush() error {
	bridge.mu.Lock()
	defer bridge.mu.Unlock()
	bridge.err = nil
	return bridge.ringFlushLocked()
}
