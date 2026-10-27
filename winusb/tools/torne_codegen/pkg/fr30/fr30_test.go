// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fr30

import (
	"encoding/binary"
	"testing"
)

// Each case is an instruction as fr30-elf-as encodes it.
func TestEncodings(t *testing.T) {
	cases := []struct {
		name string
		emit func(a *Assembler)
		want []uint16
	}{
		{"ldi:8 0x40,r13", func(a *Assembler) { a.LDI8(0x40, R13) }, []uint16{0xc40d}},
		{"ldi:20 0xa3f00,r0", func(a *Assembler) { a.LDI20(0xa3f00, R0) }, []uint16{0x9ba0, 0x3f00}},
		{"ldi:32 0x600df00d,r1", func(a *Assembler) { a.LDI32(0x600df00d, R1) }, []uint16{0x9f81, 0x600d, 0xf00d}},
		{"ld @r12,r0", func(a *Assembler) { a.LD(R12, R0) }, []uint16{0x04c0}},
		{"ld @(r13,r7),r3", func(a *Assembler) { a.LDR13(R7, R3) }, []uint16{0x0073}},
		{"lduh @r12,r0", func(a *Assembler) { a.LDUH(R12, R0) }, []uint16{0x05c0}},
		{"ldub @r8,r0", func(a *Assembler) { a.LDUB(R8, R0) }, []uint16{0x0680}},
		{"ldub @(r13,r8),r0", func(a *Assembler) { a.LDUBR13(R8, R0) }, []uint16{0x0280}},
		{"st r1,@r0", func(a *Assembler) { a.ST(R1, R0) }, []uint16{0x1401}},
		{"st r4,@(r13,r8)", func(a *Assembler) { a.STR13(R4, R8) }, []uint16{0x1084}},
		{"sth r1,@r0", func(a *Assembler) { a.STH(R1, R0) }, []uint16{0x1501}},
		{"stb r0,@r8", func(a *Assembler) { a.STB(R0, R8) }, []uint16{0x1680}},
		{"stb r0,@(r13,r10)", func(a *Assembler) { a.STBR13(R0, R10) }, []uint16{0x12a0}},
		{"mov r4,r2", func(a *Assembler) { a.MOV(R4, R2) }, []uint16{0x8b42}},
		{"add r0,r4", func(a *Assembler) { a.ADD(R0, R4) }, []uint16{0xa604}},
		{"add 4,r1", func(a *Assembler) { a.ADDI(4, R1) }, []uint16{0xa441}},
		{"add2 -4,r3", func(a *Assembler) { a.ADD2(-4, R3) }, []uint16{0xa5c3}},
		{"sub r2,r4", func(a *Assembler) { a.SUB(R2, R4) }, []uint16{0xac24}},
		{"and r0,r4", func(a *Assembler) { a.AND(R0, R4) }, []uint16{0x8204}},
		{"or r0,r4", func(a *Assembler) { a.OR(R0, R4) }, []uint16{0x9204}},
		{"cmp r0,r4", func(a *Assembler) { a.CMP(R0, R4) }, []uint16{0xaa04}},
		{"cmp 1,r5", func(a *Assembler) { a.CMPI(1, R5) }, []uint16{0xa815}},
		{"lsl2 8,r0", func(a *Assembler) { a.LSL2(8, R0) }, []uint16{0xb580}},
		{"lsl 2,r0", func(a *Assembler) { a.LSL(2, R0) }, []uint16{0xb420}},
		{"lsr 4,r1", func(a *Assembler) { a.LSR(4, R1) }, []uint16{0xb041}},
		{"lsr2 12,r1", func(a *Assembler) { a.LSR2(12, R1) }, []uint16{0xb1c1}},
		{"jmp @r12", func(a *Assembler) { a.JMP(R12) }, []uint16{0x970c}},
		{"call @r12", func(a *Assembler) { a.CALL(R12) }, []uint16{0x971c}},
		{"ret", func(a *Assembler) { a.RET() }, []uint16{0x9720}},
		{"nop", func(a *Assembler) { a.NOP() }, []uint16{0x9fa0}},
		{"andccr 0xef", func(a *Assembler) { a.ANDCCR(0xef) }, []uint16{0x83ef}},
	}
	for _, test := range cases {
		a := New(0xa0000)
		test.emit(a)
		words, err := a.Words()
		if err != nil {
			t.Errorf("%s: %v", test.name, err)
			continue
		}
		code := make([]byte, 0, len(words)*4)
		for _, word := range words {
			code = binary.BigEndian.AppendUint32(code, word)
		}
		for index, want := range test.want {
			if got := binary.BigEndian.Uint16(code[index*2:]); got != want {
				t.Errorf("%s: halfword %d = %04x, want %04x", test.name, index, got, want)
			}
		}
	}
}

// Branches count halfwords from the next instruction, both ways: the
// copy loop of a program as fr30-elf-as laid it out.
func TestBranches(t *testing.T) {
	a := New(0xa0000)
	loop, done := a.Label("loop"), a.Label("done")
	a.Bind(loop)
	a.LD(R1, R4)      // 0x0414
	a.ST(R4, R2)      // 0x1424
	a.ADDI(4, R1)     // 0xa441
	a.ADDI(4, R2)     // 0xa442
	a.ADD2(-4, R3)    // 0xa5c3
	a.B(NE, loop)     // 0xe3fa
	a.B(Always, done) // 0xe000
	a.Bind(done)
	a.LDIAddr(loop, R12)
	words, err := a.Words()
	if err != nil {
		t.Fatal(err)
	}
	want := []uint32{0x04141424, 0xa441a442, 0xa5c3e3fa, 0xe0009f8c, 0x000a0000}
	if len(words) != len(want) {
		t.Fatalf("got %d words %08x, want %08x", len(words), words, want)
	}
	for index := range want {
		if words[index] != want[index] {
			t.Errorf("word %d = %08x, want %08x", index, words[index], want[index])
		}
	}
}

// With a scratch register, a branch or call that does not reach goes
// through it, and the labels after it move.
func TestFarBranches(t *testing.T) {
	a := New(0x1000)
	a.SetScratch(R12)
	far, near := a.Label("far"), a.Label("near")
	a.B(EQ, far)     // 0x1000: bne +4; ldi:32 far,r12; jmp @r12
	a.B(Always, far) // 0x100a: ldi:32 far,r12; jmp @r12
	a.BD(NE, near)   // 0x1012: short
	a.NOP()
	a.CALLL(far) // 0x1016: ldi:32 far,r12; call @r12
	a.Bind(near)
	for index := 0; index < 1200; index++ {
		a.NOP()
	}
	a.Bind(far)
	a.RET()
	code, err := a.Assemble()
	if err != nil {
		t.Fatal(err)
	}
	farAddress, _ := a.Address(far)
	if want := uint32(0x101e + 2400); farAddress != want {
		t.Fatalf("far at %#x, want %#x", farAddress, want)
	}
	want := []uint16{
		0xe304, 0x9f8c, uint16(farAddress >> 16), uint16(farAddress), 0x970c,
		0x9f8c, uint16(farAddress >> 16), uint16(farAddress), 0x970c,
		0xf305, 0x9fa0,
		0x9f8c, uint16(farAddress >> 16), uint16(farAddress), 0x971c,
	}
	for index, word := range want {
		if got := binary.BigEndian.Uint16(code[index*2:]); got != word {
			t.Errorf("halfword %d = %04x, want %04x", index, got, word)
		}
	}
}
