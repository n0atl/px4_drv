// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fr30

import (
	"bytes"
	"math/rand"
	"testing"

	"github.com/Till0196/coax/pkg/fr30/sim"
)

// randomRun emits a run of straight code that works on R0-R7, the flags,
// MDH/MDL, a frame at R14 and RAM at a few places that overlap.
func randomRun(r *rand.Rand, a *Assembler, length int) {
	reg := func() Reg { return Reg(r.Intn(8)) }
	places := []uint32{0x20000, 0x20002, 0x20004, 0x20008, 0x20009}
	frame := func(size int) int { return -4*(1+r.Intn(6)) + r.Intn(4)&^(size-1) }
	for n := 0; n < length; n++ {
		switch r.Intn(22) {
		case 0:
			a.LDI8(uint32(r.Intn(256)), reg())
		case 1:
			a.LDI20(uint32(r.Intn(1<<20)), reg())
		case 2:
			a.LDI32(r.Uint32(), reg())
		case 3:
			a.MOV(reg(), reg())
		case 4:
			a.ADD(reg(), reg())
		case 5:
			a.SUB(reg(), reg())
		case 6:
			a.ADDC(reg(), reg())
		case 7:
			a.EOR(reg(), reg())
		case 8:
			a.LSL(r.Intn(16), reg())
		case 9:
			a.CMP(reg(), reg())
		case 10:
			a.MUL(reg(), reg())
		case 11:
			a.MOVFromS(MDL, reg())
		case 12:
			a.EXTUB(reg())
		case 13:
			a.LDR14(frame(4), reg())
		case 14:
			a.STR14(reg(), frame(4))
		case 15:
			a.LDUHR14(frame(2), reg())
		case 16:
			a.STBR14(reg(), frame(1))
		case 17, 18:
			// An access to a place the run puts in a register.
			at, i := reg(), reg()
			for i == at {
				i = reg()
			}
			place := places[r.Intn(len(places))]
			a.LDI20(place, at)
			switch r.Intn(4) {
			case 0:
				a.LD(at, i)
			case 1:
				a.ST(i, at)
			case 2:
				a.LDUB(at, i)
			case 3:
				a.STH(i, at)
			}
		case 19:
			// Through R13, @(R13,Rj).
			j := reg()
			a.LDI20(0x20000, R13)
			a.LDI8(uint32(r.Intn(3)*4), j)
			if r.Intn(2) == 0 {
				a.LDR13(j, reg())
			} else {
				a.STR13(reg(), j)
			}
		case 20:
			a.NOP()
		case 21:
			a.ANDM(reg(), reg()) // AND Rj,@Ri: not known, stays put
		}
	}
}

// Scheduled runs leave the registers, the flags and memory as they are
// without it.
func TestScheduleKeepsMeaning(t *testing.T) {
	const origin, frameAt = 0x10000, 0x21000
	r := rand.New(rand.NewSource(7))
	moved := 0
	for round := 0; round < 3000; round++ {
		seed := r.Int63()
		var codes [2][]byte
		var cpus [2]*sim.CPU
		for s := range 2 {
			a := New(origin)
			a.SetSchedule(s == 1)
			randomRun(rand.New(rand.NewSource(seed)), a, 40)
			a.Here("end") // a NOP apart, to stop at
			a.NOP()
			code, err := a.Assemble()
			if err != nil {
				t.Fatal(err)
			}
			codes[s] = code
			c := sim.New()
			c.Load(origin, code)
			init := rand.New(rand.NewSource(seed + 1))
			for i := range 8 {
				c.R[i] = init.Uint32()
			}
			// Addresses the run may put in R0-R7 point at RAM.
			for at := uint32(0x20000); at < 0x22000; at += 4 {
				c.Write(at, 4, init.Uint32())
			}
			c.R[14] = frameAt
			c.PS = init.Uint32() & 0xf
			c.PC = origin
			c.StopAt = origin + uint32(len(code)) - 2
			if err := c.Run(10000); err != nil {
				t.Fatal(err)
			}
			cpus[s] = c
		}
		if !bytes.Equal(codes[0], codes[1]) {
			moved++
		}
		a, b := cpus[0], cpus[1]
		bad := a.R != b.R || a.PS&0xf != b.PS&0xf || a.MDH != b.MDH || a.MDL != b.MDL ||
			!bytes.Equal(a.Bytes(0x20000, 0x2000), b.Bytes(0x20000, 0x2000))
		if bad {
			m0, m1 := a.Bytes(0x20000, 0x2000), b.Bytes(0x20000, 0x2000)
			for k := range m0 {
				if m0[k] != m1[k] {
					t.Logf("%x: %02x want %02x", 0x20000+k, m1[k], m0[k])
				}
			}
			t.Fatalf("round %d (seed %d): scheduled run differs\nregs %x ps %x md %x %x\nwant %x ps %x md %x %x", round, seed, b.R, b.PS&0xf, b.MDH, b.MDL, a.R, a.PS&0xf, a.MDH, a.MDL)
		}
	}
	t.Logf("%d of 3000 runs reordered", moved)
	if moved == 0 {
		t.Error("nothing was reordered")
	}
}

// runBoth assembles a program twice, as it is and scheduled, runs both
// from origin to the label end and tells whether they agree.
func runBoth(t *testing.T, seed int64, build func(a *Assembler) *Label) (same, moved bool) {
	const origin, frameAt = 0x10000, 0x21000
	var codes [2][]byte
	var cpus [2]*sim.CPU
	for s := range 2 {
		a := New(origin)
		a.SetScratch(R12)
		a.SetSchedule(s == 1)
		end := build(a)
		code, err := a.Assemble()
		if err != nil {
			t.Fatal(err)
		}
		stop, err := a.Address(end)
		if err != nil {
			t.Fatal(err)
		}
		codes[s] = code
		c := sim.New()
		c.Load(origin, code)
		init := rand.New(rand.NewSource(seed + 1))
		for i := range 8 {
			c.R[i] = init.Uint32()
		}
		for at := uint32(0x20000); at < 0x22000; at += 4 {
			c.Write(at, 4, init.Uint32())
		}
		c.R[14], c.R[15] = frameAt, frameAt-0x100
		c.PS = init.Uint32() & 0xf
		c.PC = origin
		c.StopAt = stop
		if err := c.Run(100000); err != nil {
			t.Fatal(err)
		}
		cpus[s] = c
	}
	a, b := cpus[0], cpus[1]
	a.R[12], b.R[12] = 0, 0 // the scratch register: addresses of the code
	same = a.R == b.R && a.PS&0xf == b.PS&0xf && a.MDH == b.MDH && a.MDL == b.MDL &&
		bytes.Equal(a.Bytes(0x20000, 0x2000), b.Bytes(0x20000, 0x2000))
	if !same {
		t.Logf("regs %x ps %x md %x %x pc %x\nwant %x ps %x md %x %x pc %x", b.R, b.PS&0xf, b.MDH, b.MDL, b.PC, a.R, a.PS&0xf, a.MDH, a.MDL, a.PC)
	}
	return same, !bytes.Equal(codes[0], codes[1])
}

// With branches, calls and returns, scheduled programs (their delay slots
// filled too) do as they do without it.
func TestSlotsKeepMeaning(t *testing.T) {
	r := rand.New(rand.NewSource(11))
	moved := 0
	for round := 0; round < 3000; round++ {
		seed := r.Int63()
		same, m := runBoth(t, seed, func(a *Assembler) *Label {
			g := rand.New(rand.NewSource(seed))
			const segments = 6
			labels := make([]*Label, segments+1)
			for k := range labels {
				labels[k] = a.Label("l")
			}
			fn := a.Label("fn")
			for k := 0; k < segments; k++ {
				a.Bind(labels[k])
				randomRun(g, a, 1+g.Intn(8))
				switch g.Intn(7) {
				case 0:
					a.B(Cond(2+g.Intn(14)), labels[k+1+g.Intn(segments-k)])
				case 1:
					a.B(Always, labels[k+1+g.Intn(segments-k)])
				case 2:
					a.CALLL(fn)
				case 3:
					// A branch over the next instruction, to a label at the
					// one after it.
					skip := a.Label("skip")
					a.B(Cond(2+g.Intn(14)), skip)
					a.ADDI(1+g.Intn(15), Reg(g.Intn(8)))
					a.Bind(skip)
				case 5:
					// Branches one after another.
					a.B(Cond(2+g.Intn(14)), labels[k+1+g.Intn(segments-k)])
					a.B(Always, labels[k+1+g.Intn(segments-k)])
				case 4:
					if g.Intn(4) == 0 {
						// Far enough that branches over it go through the
						// scratch register.
						for range 150 {
							a.NOP()
						}
					}
				}
			}
			a.Bind(labels[segments])
			end := a.Here("end")
			a.NOP()
			a.Bind(fn)
			randomRun(g, a, 1+g.Intn(6))
			a.RET()
			return end
		})
		if !same {
			t.Fatalf("round %d (seed %d): scheduled program differs", round, seed)
		}
		if m {
			moved++
		}
	}
	t.Logf("%d of 3000 programs changed", moved)
}
