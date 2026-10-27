// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

// Package fr30 assembles Fujitsu FR (FR30/FR80) code for the programs the
// MB86H47A driver places on the chip's CPUs, so that they are written and
// built as Go source.
//
// The whole FR30 instruction set is provided (isa.go), each encoding checked
// against fr30-elf-as. Code is big-endian and made of 16-bit halfwords.
//
// A program is collected as a sequence of pieces and laid out when it is
// finished. A conditional branch reaches 256 bytes either way and a relative
// call 2 KiB; with a scratch register set (SetScratch), a branch or call that
// does not reach is rewritten to go through the scratch register instead.
package fr30

import (
	"encoding/binary"
	"fmt"
)

// Reg is a general register R0-R15.
type Reg uint8

// Registers. R13 is the index register of the @(R13,Rj) forms, R14 the base
// of the @(R14,disp) forms and R15 the stack pointer.
const (
	R0 Reg = iota
	R1
	R2
	R3
	R4
	R5
	R6
	R7
	R8
	R9
	R10
	R11
	R12
	R13
	R14
	R15
)

// Cond is a branch condition.
type Cond uint8

// Branch conditions, by their code in the Bcc encoding.
const (
	Always Cond = 0x0 // BRA
	Never  Cond = 0x1 // BNO
	EQ     Cond = 0x2 // BEQ: Z
	NE     Cond = 0x3 // BNE: !Z
	C      Cond = 0x4 // BC: carry (unsigned lower on CMP)
	NC     Cond = 0x5 // BNC: no carry (unsigned higher or same)
	N      Cond = 0x6 // BN: negative
	P      Cond = 0x7 // BP: positive or zero
	V      Cond = 0x8 // BV: overflow
	NV     Cond = 0x9 // BNV: no overflow
	LT     Cond = 0xa // BLT: signed less
	GE     Cond = 0xb // BGE: signed greater or equal
	LE     Cond = 0xc // BLE: signed less or equal
	GT     Cond = 0xd // BGT: signed greater
	LS     Cond = 0xe // BLS: unsigned lower or same
	HI     Cond = 0xf // BHI: unsigned higher
)

// Not is the opposite condition.
func (c Cond) Not() Cond { return c ^ 1 }

// Label names a place in the program; it is bound with Bind.
type Label struct {
	name  string
	piece int
	bound bool
}

type pieceKind uint8

const (
	pieceFixed  pieceKind = iota // bytes as they are
	pieceAbs                     // bytes with a label's address at absAt
	pieceBranch                  // Bcc / Bcc:D to a label
	pieceCall                    // CALL / CALL:D to a label
	pieceAlign                   // zero padding to a multiple of align
)

type piece struct {
	kind  pieceKind
	code  []byte
	label *Label
	absAt int
	cond  Cond
	delay bool
	align int
	far   bool // a branch or call rewritten through the scratch register
	data  bool // a fixed piece with data in it, not only instructions
	slot  bool // a delay slot's instruction, put apart by fillSlots
}

// Assembler collects the program. The zero value is not usable; call New.
type Assembler struct {
	origin   uint32
	pieces   []piece
	open     bool // the last piece is a fixed one still taking bytes
	scratch  Reg
	hasFar   bool // a scratch register is set
	pcSeen   bool
	err      error
	emitted  int      // things put in so far: instructions, data, labels
	schedule bool     // reorder straight code (sched.go)
	labels   []*Label // the labels bound

	// addresses of each piece after layout
	laidOut   bool
	addresses []uint32
}

// New starts a program that will be placed at origin.
func New(origin uint32) *Assembler { return &Assembler{origin: origin} }

// SetScratch lets branches and calls that do not reach go through register r,
// which the program must then leave free for that at every branch and call.
func (a *Assembler) SetScratch(r Reg) { a.scratch, a.hasFar = r, true }

// Err is the first error so far.
func (a *Assembler) Err() error { return a.err }

// Label makes a label; name is only for error messages.
func (a *Assembler) Label(name string) *Label { return &Label{name: name} }

// Bind puts the label at the current position.
func (a *Assembler) Bind(label *Label) {
	if label.bound {
		a.fail("label %s bound twice", label.name)
	}
	a.open = false
	a.emitted++
	label.piece, label.bound = len(a.pieces), true
	a.labels = append(a.labels, label)
}

// Emitted counts what went in so far (instructions, data and labels): two
// equal counts mean nothing went in between.
func (a *Assembler) Emitted() int { return a.emitted }

// Here makes a label bound at the current position.
func (a *Assembler) Here(name string) *Label {
	label := a.Label(name)
	a.Bind(label)
	return label
}

// PC is the address of the next instruction, as long as no branch before it
// has to be rewritten; programs that set a scratch register use labels
// instead.
func (a *Assembler) PC() uint32 {
	a.pcSeen = true
	address := a.origin
	for _, p := range a.pieces {
		address += uint32(a.size(p, address))
	}
	return address
}

// Align pads with zero bytes to a multiple of n bytes from address 0.
func (a *Assembler) Align(n int) {
	a.emitted++
	a.pieces = append(a.pieces, piece{kind: pieceAlign, align: n})
	a.open = false
}

func (a *Assembler) fail(format string, args ...any) {
	if a.err == nil {
		a.err = fmt.Errorf("fr30: "+format, args...)
	}
}

func (a *Assembler) bytes(b ...byte) {
	a.emitted++
	if !a.open {
		a.pieces = append(a.pieces, piece{kind: pieceFixed})
		a.open = true
	}
	last := &a.pieces[len(a.pieces)-1]
	last.code = append(last.code, b...)
}

func (a *Assembler) half(value uint16) { a.bytes(byte(value>>8), byte(value)) }

func (a *Assembler) op(opcode uint8, j, i Reg) {
	a.half(uint16(opcode)<<8 | uint16(j&15)<<4 | uint16(i&15))
}

func (a *Assembler) imm4(opcode uint8, value int, low, high int, i Reg) {
	if value < low || value > high {
		a.fail("immediate %d out of range %d..%d", value, low, high)
	}
	a.op(opcode, Reg(value&15), i)
}

// absolute adds a piece whose bytes hold a label's address at offset at.
func (a *Assembler) absolute(code []byte, at int, label *Label) {
	a.emitted++
	a.pieces = append(a.pieces, piece{kind: pieceAbs, code: code, absAt: at, label: label})
	a.open = false
}

// B branches to label on cond: Bcc label (no delay slot).
func (a *Assembler) B(cond Cond, label *Label) {
	a.emitted++
	a.pieces = append(a.pieces, piece{kind: pieceBranch, cond: cond, label: label})
	a.open = false
}

// BD branches to label on cond after the next instruction, which runs either
// way: Bcc:D label. The delay-slot instruction must not use the scratch
// register.
func (a *Assembler) BD(cond Cond, label *Label) {
	a.emitted++
	a.pieces = append(a.pieces, piece{kind: pieceBranch, cond: cond, label: label, delay: true})
	a.open = false
}

// CALLL calls a label, keeping the return address in RP: CALL label12.
func (a *Assembler) CALLL(label *Label) {
	a.emitted++
	a.pieces = append(a.pieces, piece{kind: pieceCall, label: label})
	a.open = false
}

// CALLLD calls a label after the next instruction: CALL:D label12.
func (a *Assembler) CALLLD(label *Label) {
	a.emitted++
	a.pieces = append(a.pieces, piece{kind: pieceCall, label: label, delay: true})
	a.open = false
}

// size is a piece's length when it starts at address.
func (a *Assembler) size(p piece, address uint32) int {
	switch p.kind {
	case pieceBranch:
		switch {
		case !p.far:
			return 2
		case p.cond == Always:
			return 8 // LDI:32 target,scratch; JMP @scratch
		default:
			return 10 // Bncc over; LDI:32; JMP
		}
	case pieceCall:
		if p.far {
			return 8 // LDI:32 target,scratch; CALL @scratch
		}
		return 2
	case pieceAlign:
		return int((uint32(p.align) - address%uint32(p.align)) % uint32(p.align))
	}
	return len(p.code)
}

// layout places the pieces, rewriting branches that do not reach until
// every one does.
func (a *Assembler) layout() {
	for {
		addresses := make([]uint32, len(a.pieces)+1)
		address := a.origin
		for index, p := range a.pieces {
			addresses[index] = address
			address += uint32(a.size(p, address))
		}
		addresses[len(a.pieces)] = address
		changed := false
		for index := range a.pieces {
			p := &a.pieces[index]
			if (p.kind != pieceBranch && p.kind != pieceCall) || p.far || !p.label.bound {
				continue
			}
			displacement := (int64(addresses[p.label.piece]) - int64(addresses[index]+2)) / 2
			limit := int64(128)
			if p.kind == pieceCall {
				limit = 1024
			}
			if displacement >= -limit && displacement < limit {
				continue
			}
			if !a.hasFar {
				a.fail("branch to %s is %d bytes away", p.label.name, displacement*2)
				continue
			}
			if a.pcSeen {
				a.fail("branch to %s rewritten after PC was taken", p.label.name)
			}
			p.far, changed = true, true
		}
		if !changed || a.err != nil {
			a.addresses, a.laidOut = addresses, true
			return
		}
	}
}

// Assemble finishes the program: it lays it out, resolves labels and returns
// the code.
func (a *Assembler) Assemble() ([]byte, error) {
	for _, p := range a.pieces {
		if p.label != nil && !p.label.bound {
			a.fail("label %s not bound", p.label.name)
		}
	}
	if a.err != nil {
		return nil, a.err
	}
	if a.schedule {
		a.scheduleRuns()
		if !a.pcSeen {
			a.fillSlots()
		}
	}
	a.layout()
	if a.err != nil {
		return nil, a.err
	}
	var code []byte
	for index, p := range a.pieces {
		address := a.addresses[index]
		switch p.kind {
		case pieceFixed:
			code = append(code, p.code...)
		case pieceAbs:
			at := len(code) + p.absAt
			code = append(code, p.code...)
			binary.BigEndian.PutUint32(code[at:], a.addresses[p.label.piece])
		case pieceAlign:
			code = append(code, make([]byte, a.size(p, address))...)
		case pieceBranch, pieceCall:
			target := a.addresses[p.label.piece]
			if !p.far {
				displacement := (int64(target) - int64(address+2)) / 2
				if p.kind == pieceBranch {
					word := uint16(0xe000) | uint16(p.cond&15)<<8 | uint16(uint8(int8(displacement)))
					if p.delay {
						word |= 0x1000
					}
					code = binary.BigEndian.AppendUint16(code, word)
				} else {
					word := uint16(0xd000) | uint16(displacement)&0x7ff
					if p.delay {
						word |= 0x0800
					}
					code = binary.BigEndian.AppendUint16(code, word)
				}
				continue
			}
			s := uint16(a.scratch & 15)
			if p.kind == pieceBranch && p.cond != Always {
				// Skip the LDI:32 and JMP (8 bytes = 4 halfwords).
				code = binary.BigEndian.AppendUint16(code, 0xe000|uint16(p.cond.Not()&15)<<8|4)
			}
			code = binary.BigEndian.AppendUint16(code, 0x9f80|s)
			code = binary.BigEndian.AppendUint32(code, target)
			var jump uint16
			switch {
			case p.kind == pieceBranch && !p.delay:
				jump = 0x9700 | s // JMP @s
			case p.kind == pieceBranch:
				jump = 0x9f00 | s // JMP:D @s
			case !p.delay:
				jump = 0x9710 | s // CALL @s
			default:
				jump = 0x9f10 | s // CALL:D @s
			}
			code = binary.BigEndian.AppendUint16(code, jump)
		}
	}
	return code, nil
}

// Words finishes the program like Assemble and returns the code as
// big-endian words, the last one padded with zeros.
func (a *Assembler) Words() ([]uint32, error) {
	code, err := a.Assemble()
	if err != nil {
		return nil, err
	}
	for len(code)%4 != 0 {
		code = append(code, 0)
	}
	words := make([]uint32, len(code)/4)
	for index := range words {
		words[index] = binary.BigEndian.Uint32(code[index*4:])
	}
	return words, nil
}

// Address is where a label ended up; it is known once the program has been
// assembled.
func (a *Assembler) Address(label *Label) (uint32, error) {
	if !a.laidOut {
		return 0, fmt.Errorf("fr30: %s asked for before assembly", label.name)
	}
	if !label.bound {
		return 0, fmt.Errorf("fr30: label %s not bound", label.name)
	}
	return a.addresses[label.piece], nil
}
