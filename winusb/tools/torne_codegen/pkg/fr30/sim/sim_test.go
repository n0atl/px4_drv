// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package sim_test

import (
	"testing"

	. "github.com/Till0196/coax/pkg/fr30"
	"github.com/Till0196/coax/pkg/fr30/sim"
)

const (
	origin = 0x10000
	stack  = 0x8000
)

// run assembles a function body (ended by RET by the caller of run) and calls
// it with args.
func run(t *testing.T, body func(a *Assembler), args ...uint32) (*sim.CPU, uint32) {
	t.Helper()
	a := New(origin)
	a.SetScratch(R12)
	body(a)
	code, err := a.Assemble()
	if err != nil {
		t.Fatal(err)
	}
	cpu := sim.New()
	cpu.Load(origin, code)
	result, err := cpu.Call(origin, stack, args...)
	if err != nil {
		t.Fatal(err)
	}
	return cpu, result
}

// Compare-and-branch for each condition against Go's own comparisons.
func TestConditions(t *testing.T) {
	values := []uint32{0, 1, 2, 0x7fffffff, 0x80000000, 0xffffffff, 0xfffffffe, 100}
	type check struct {
		cond Cond
		want func(a, b uint32) bool
	}
	checks := []check{
		{EQ, func(a, b uint32) bool { return a == b }},
		{NE, func(a, b uint32) bool { return a != b }},
		{C, func(a, b uint32) bool { return a < b }},
		{NC, func(a, b uint32) bool { return a >= b }},
		{LT, func(a, b uint32) bool { return int32(a) < int32(b) }},
		{GE, func(a, b uint32) bool { return int32(a) >= int32(b) }},
		{LE, func(a, b uint32) bool { return int32(a) <= int32(b) }},
		{GT, func(a, b uint32) bool { return int32(a) > int32(b) }},
		{LS, func(a, b uint32) bool { return a <= b }},
		{HI, func(a, b uint32) bool { return a > b }},
	}
	for _, c := range checks {
		for _, x := range values {
			for _, y := range values {
				_, got := run(t, func(a *Assembler) {
					yes := a.Label("yes")
					a.CMP(R5, R4) // flags of R4 - R5
					a.B(c.cond, yes)
					a.LDI8(0, R4)
					a.RET()
					a.Bind(yes)
					a.LDI8(1, R4)
					a.RET()
				}, x, y)
				if (got == 1) != c.want(x, y) {
					t.Errorf("cond %#x on %#x, %#x: %d", c.cond, x, y, got)
				}
			}
		}
	}
}

// A frame, pushes and pops, calls, delay slots and memory widths.
func TestFrameAndCalls(t *testing.T) {
	cpu, got := run(t, func(a *Assembler) {
		double := a.Label("double")
		a.PUSHS(RP)
		a.STM(R8, R9)
		a.ENTER(8)
		a.MOV(R4, R8)
		a.STR14(R8, -4)
		a.CALLLD(double)
		a.MOV(R8, R4) // delay slot: the argument
		a.LDR14(-4, R9)
		a.ADD(R9, R4) // 3x
		a.LDI20(0x9000, R1)
		a.STH(R4, R1)
		a.LDUB(R1, R0) // high byte of the halfword
		a.MOV(R0, R5)
		a.LEAVE()
		a.LDM(R8, R9)
		a.POPS(RP)
		a.RET()
		a.Bind(double)
		a.RETD()
		a.ADD(R4, R4)
	}, 0x1234)
	if got != 0x369c || cpu.R[5] != 0x36 || cpu.R[15] != stack {
		t.Errorf("got %#x, r5 %#x, sp %#x", got, cpu.R[5], cpu.R[15])
	}
}

// Shifts, extensions and multiplies.
func TestArithmetic(t *testing.T) {
	cpu, _ := run(t, func(a *Assembler) {
		a.MOV(R4, R0)
		a.Sar(20, R0) // 0xfffff800 >> 20 arithmetic
		a.MOV(R4, R1)
		a.Shr(20, R1)
		a.MOV(R4, R2)
		a.EXTSB(R2)
		a.MUL(R5, R4) // signed 0x80000080 * 3
		a.MOVFromS(MDL, R3)
		a.MOVFromS(MDH, R6)
		a.RET()
	}, 0x80000080, 3)
	want := [...]uint32{0xfffff800, 0x800, 0xffffff80, 0x80000180, 0xfffffffe}
	got := [...]uint32{cpu.R[0], cpu.R[1], cpu.R[2], cpu.R[3], cpu.R[6]}
	if got != want {
		t.Errorf("got %08x, want %08x", got, want)
	}
}

// An interrupt runs its handler and RETI goes back to where the CPU was,
// with the PS it had.
func TestInterrupt(t *testing.T) {
	cpu := sim.New()
	cpu.TBR = 0xffc00
	a := New(0x1000)
	a.ORCCR(0x10)
	loop := a.Here("loop")
	a.ADDI(1, R0)
	a.B(Always, loop)
	main, _ := a.Words()
	cpu.LoadWords(0x1000, main)
	h := New(0x2000)
	h.LDI8(7, R1)
	h.RETI()
	handler, _ := h.Words()
	cpu.LoadWords(0x2000, handler)
	cpu.Write(0xffc00+0x3fc-4*16, 4, 0x2000)
	cpu.PC, cpu.R[15] = 0x1000, 0x8000
	for i := 0; i < 5; i++ {
		cpu.Step()
	}
	pc := cpu.PC
	if !cpu.Interrupt(16) {
		t.Fatal("not taken")
	}
	cpu.Step()
	cpu.Step()
	if cpu.R[1] != 7 || cpu.PC != pc || cpu.R[15] != 0x8000 || cpu.PS&0x10 == 0 {
		t.Fatalf("after RETI: R1 %d PC %#x (want %#x) R15 %#x PS %#x", cpu.R[1], cpu.PC, pc, cpu.R[15], cpu.PS)
	}
}
