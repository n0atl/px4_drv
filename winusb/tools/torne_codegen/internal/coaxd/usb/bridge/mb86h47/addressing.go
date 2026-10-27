// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package mb86h47

// Codegen adaptation: runtime USB methods removed; reference retained separately.

import (
	"fmt"

	. "github.com/Till0196/coax/internal/coaxd/usb/bridge/mb86h47/fr30/fc"
	"github.com/Till0196/coax/pkg/fr30"
)

// The memory request reaches 24-bit addresses at power-up; codec core 0 and
// some of the clock registers lie above. Its handler runs from SRAM, so the
// host widens it: two routines, one for reads and one for writes, go into
// SRAM the handler leaves unused (one of addressingBanks), and the handler's address
// check is replaced by a jump to them. After that the low byte of wValue is
// address bits 24-31.
//
// The handler's write path also keeps only the first word of a write: it
// buffers word n of the data stage at 16n rather than 4n and then copies the
// buffer out word by word. One instruction, the extra shift, is replaced, and
// a write then carries up to writeLimit bytes.
var addressingBanks = [2]uint32{0xe1000, 0xe1800}

const addressingBankSize = 0x800

// handlerWord is a word of the handler that is replaced: its address, what
// it holds at power-up and what it holds once widened.
type handlerWord struct{ address, original, widened uint32 }

// addressing is the two routines for one bank, and the handler words that
// jump to them, in the order they are switched. A write stores one word, so
// each switch is atomic, and the order keeps the handler's own check in
// place until the jump that replaces it is in.
type addressing struct {
	code  []uint32
	words []handlerWord
}

var addressings = [2]addressing{assembleAddressing(addressingBanks[0]), assembleAddressing(addressingBanks[1])}

// assembleAddressing builds the routines. Where they are entered, the
// handler holds the request's 24-bit address in R4 and its request state
// (0xC4600) in R8 on the read path, R10 on the write path; state byte 0x40 is
// wValue's low byte and word 0x44 the address the transfer then uses. Each
// routine stands in for the handler's check "ldi:20 0x80000,r0; cmp r0,r4;
// bc" and goes on where that branch goes:
//
//   - with a low byte of 0, as the handler does itself, 0x80000-0x9FFFF maps
//     to 0xF00000 and every other address is left alone;
//   - otherwise the low byte becomes address bits 24-31. The write path
//     clears it once used.
func assembleAddressing(base uint32) addressing {
	code, entries, err := buildAddressing(base)
	if err != nil {
		panic(err)
	}
	if len(code)*4 > addressingBankSize {
		panic(fmt.Sprintf("addressing routines take %d words", len(code)))
	}
	var words []handlerWord
	for _, check := range []struct {
		address  uint32
		original [2]uint32
		routine  uint32
	}{
		{handlerReadCheck, [2]uint32{0x9b800000, 0xaa04e40f}, entries["read"]},
		{handlerWriteCheck, [2]uint32{0x9b800000, 0xaa04e40d}, entries["write"]},
	} {
		// ldi:20 routine,r12; jmp @r12; nop in place of
		// ldi:20 0x80000,r0; cmp r0,r4; bc.
		jump, err := handlerJump(check.address, check.routine)
		if err != nil {
			panic(err)
		}
		words = append(words,
			handlerWord{check.address, check.original[0], jump[0]},
			handlerWord{check.address + 4, check.original[1], jump[1]})
	}
	// lsl #2,r7; ldi:32 0xc4600,r13 -> nop; ldi:32 0xc4600,r13
	const shift = 0xb4279f8d
	words = append(words, handlerWord{handlerWriteShift, shift, shift&0xffff | fr30.NOPCode<<16})
	return addressing{code, words}
}

// Where the handler's code is replaced, and where the routines go back to.
const (
	handlerReadCheck   = 0xe9ac0
	handlerReadResume  = 0xe9ae6
	handlerWriteCheck  = 0xe9e0c
	handlerWriteResume = 0xe9e2e
	handlerWriteShift  = 0xe9d94

	// The handler's request state: wValue's low byte, the address the
	// transfer uses.
	requestHighByte = 0x40
	requestAddress  = 0x44
)

// handlerJump is the two words that go to routine from at: a jump and a
// NOP to fill the second word.
func handlerJump(at, routine uint32) ([]uint32, error) {
	p := NewProgram(at)
	jump := p.Splice("jump")
	jump.Define(func(b *Block) { b.Jump(routine) })
	words, err := p.Words()
	if err != nil {
		return nil, err
	}
	if len(words) != 2 || words[1]&0xffff != 0 {
		return nil, fmt.Errorf("jump to %#x takes %d words", routine, len(words))
	}
	words[1] |= fr30.NOPCode
	return words, nil
}

// buildAddressing is the two routines at base, and where each starts
// ("read", "write").
func buildAddressing(base uint32) ([]uint32, map[string]uint32, error) {
	p := NewProgram(base)
	execute := p.Func("execute", 1)

	// widen does the mapping on address, the request's R4, and notes the
	// address the transfer uses when it changed; clear takes the high byte
	// once used.
	widen := func(b *Block, address, state *Node, clear bool) {
		high := Mem8(state.Add(requestHighByte))
		b.If(high.Ne(0), func(b *Block) {
			b.Set(address, address.Or(high.Shl(24)))
			if clear {
				b.Store8(state.Add(requestHighByte), 0)
			}
			b.Store32(state.Add(requestAddress), address)
		}).ElseIf(address.GeU(0x80000).AndAlso(address.LtU(0xa0000)), func(b *Block) {
			b.Set(address, address.And(0x3ffff).Add(0xf00000))
			b.Store32(state.Add(requestAddress), address)
		})
	}

	// Only R0, R1 and R4 of R0-R7 are free where the handler is left.
	read := p.Splice("read", fr30.R0, fr30.R1, fr30.R4)
	read.Define(func(b *Block) {
		address, state := read.Register("address", fr30.R4), read.Register("state", fr30.R8)
		b.If(Mem8(state.Add(requestHighByte)).Eq(ringExecute), func(b *Block) {
			b.Do(execute.Call(address))
			b.Store32(state.Add(requestAddress), core1Base+ringAnswer)
			b.Jump(handlerReadResume)
		})
		widen(b, address, state, false)
		b.Jump(handlerReadResume)
	})

	write := p.Splice("write", fr30.R0, fr30.R1, fr30.R4)
	write.Define(func(b *Block) {
		address, state := write.Register("address", fr30.R4), write.Register("state", fr30.R10)
		widen(b, address, state, true)
		b.Jump(handlerWriteResume)
	})

	// execute serves a read whose wValue low byte is ringExecute: it puts
	// the ring read the rest of the address describes into codec core 1's
	// ring and waits for core 1's answer, which the handler then returns.
	// Address bits 16-23 are the slot (bits 0-3), its lap (bit 4) and
	// whether the register number is one byte (bit 5); bits 0-10 are the
	// register number.
	execute.Define(func(b *Block) {
		address := execute.Param(0)
		slot := b.Tmp("slot", address.Field(16, 8))
		access := b.Tmp("access", K(ringRead).Or(address.And(ringRegister).Shl(16)))
		b.If(slot.Field(4, 1).Ne(0), func(b *Block) { b.Set(access, access.Or(ringLap)) })
		b.If(slot.Field(5, 1).Ne(0), func(b *Block) { b.Set(access, access.Or(ringShort)) })
		b.Store32(K(core1Base+ring).Add(slot.And(ringSlots-1).Shl(2)), access)
		// The answer names the slot's number, whose low five bits are
		// the slot byte's.
		polls := b.Tmp("polls", i2cPolls)
		b.While(Mem8(K(core1Base+ringAnswer)).And(0x1f).Ne(slot.And(0x1f)).AndAlso(polls.Ne(0)), func(b *Block) {
			b.Inc(polls, -1)
		})
	})

	code, err := p.Words()
	if err != nil {
		return nil, nil, err
	}
	return code, p.FuncAddresses(), nil
}

// jumpTarget is where a handler word written by handlerJump, in this build
// or another, sends the handler: the word is ldi:20 target,r12.
func jumpTarget(word uint32) (uint32, bool) {
	if word>>16&0xff0f != 0x9b0c {
		return 0, false
	}
	return word>>20&0xf<<16 | word&0xffff, true
}

// addressingBank is the bank the routines at address are in, or -1.
func addressingBank(address uint32) int {
	for bank, base := range addressingBanks {
		if address >= base && address < base+addressingBankSize {
			return bank
		}
	}
	return -1
}

