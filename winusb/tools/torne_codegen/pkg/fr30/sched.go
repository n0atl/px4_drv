// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fr30

// The core stalls about two cycles when an instruction uses a register the
// load just before it loads; one instruction in between hides it. With
// scheduling on (SetSchedule), Assemble moves instructions within each run
// of straight code (between labels, branches and calls) so that a load's
// result is not used by the next instruction, where another instruction
// that does not depend on it can go first. What each instruction reads and
// writes is kept in order: registers, the flags, MDH/MDL and memory (loads
// and stores keep their order among themselves, but where they are to
// places apart: slots of the frame (R14), or RAM at addresses the run
// loads into registers); anything not known here stays where it is, and
// nothing moves across it.

// SetSchedule turns the reordering of straight code on or off.
func (a *Assembler) SetSchedule(on bool) { a.schedule = on }

// Resources an instruction may read or write, as bits: R0-R15, then these.
const (
	resFlags = 1 << 16
	resMD    = 1 << 17 // MDH, MDL
)

// insn is one instruction of a run, as bytes and what it touches.
type insn struct {
	code        []byte
	uses, defs  uint32
	load, store bool
	barrier     bool
	loads       uint32 // the registers it loads from memory
	// Where a load or store is: in the frame (at an offset from R14), at
	// a known address, or not known.
	place place
	at    int64
	size  int64
}

type place uint8

const (
	placeUnknown place = iota
	placeFrame
	placeFixed
)

// ioFrom is where the registers of the units start: accesses there keep
// their order.
const ioFrom = 0x1000000

// ram tells whether an access is to memory that reads do not change.
func (in insn) ram() bool {
	return in.place == placeFrame || in.place == placeFixed && in.at >= 0 && in.at+in.size <= ioFrom
}

// apart tells whether two accesses are to places that do not overlap.
func apart(a, b insn) bool {
	if a.place != b.place || a.place == placeUnknown || !a.ram() || !b.ram() {
		return false
	}
	return a.at+a.size <= b.at || b.at+b.size <= a.at
}

func reg(r uint16) uint32 { return 1 << (r & 15) }

// decode tells what the instruction at code does, and its length; ok is
// false for what is not an instruction this knows the length of.
func decode(code []byte) (in insn, ok bool) {
	if len(code) < 2 {
		return in, false
	}
	op := uint16(code[0])<<8 | uint16(code[1])
	hi, j, i := op>>8, op>>4&15, op&15
	n := 2
	switch {
	case hi == 0x9b:
		n = 4
	case op&0xfff0 == 0x9f80:
		n = 6
	}
	if len(code) < n {
		return in, false
	}
	in.code = code[:n]
	load := func(uses uint32, dest uint16) {
		in.uses, in.defs, in.load, in.loads = uses, reg(dest), true, reg(dest)
	}
	store := func(uses uint32) { in.uses, in.store = uses, true }
	alu := func(uses uint32, flags bool) {
		in.uses, in.defs = uses, reg(i)
		if flags {
			in.defs |= resFlags
		}
	}
	switch {
	case hi <= 0x02:
		load(reg(13)|reg(j), i)
	case hi == 0x03:
		load(reg(15), i)
	case hi >= 0x04 && hi <= 0x06:
		load(reg(j), i)
	case hi == 0x07 && j == 0: // LD @R15+,Ri
		load(reg(15), i)
		in.defs |= reg(15)
	case hi >= 0x10 && hi <= 0x12:
		store(reg(i) | reg(13) | reg(j))
	case hi == 0x13:
		store(reg(i) | reg(15))
	case hi >= 0x14 && hi <= 0x16:
		store(reg(i) | reg(j))
	case hi == 0x17 && j == 0: // ST Ri,@-R15
		store(reg(i) | reg(15))
		in.defs |= reg(15)
	case hi >= 0x20 && hi <= 0x2f, hi >= 0x40 && hi <= 0x4f, hi >= 0x60 && hi <= 0x6f:
		load(reg(14), i)
	case hi >= 0x30 && hi <= 0x3f, hi >= 0x50 && hi <= 0x5f, hi >= 0x70 && hi <= 0x7f:
		store(reg(i) | reg(14))
	case hi == 0x80 || hi == 0x81 || hi == 0x90 || hi == 0x91 || hi == 0x98 || hi == 0x99: // bit ops on @Ri
		store(reg(i))
		in.load = true
	case hi == 0x84 || hi == 0x85 || hi == 0x86 || hi == 0x94 || hi == 0x95 || hi == 0x96 || hi == 0x9c || hi == 0x9d || hi == 0x9e: // op Rj,@Ri
		store(reg(i) | reg(j))
		in.load = true
		in.defs |= resFlags
	case hi == 0x88 || hi == 0x89: // BTST #u4,@Ri
		in.uses, in.defs, in.load = reg(i), resFlags, true
	case hi == 0x82 || hi == 0x92 || hi == 0x9a || hi == 0xa6 || hi == 0xac || hi == 0xb2 || hi == 0xb6 || hi == 0xba:
		alu(reg(i)|reg(j), true)
	case hi == 0xa7 || hi == 0xad: // ADDC, SUBC
		alu(reg(i)|reg(j)|resFlags, true)
	case hi == 0xa2 || hi == 0xae: // ADDN, SUBN
		alu(reg(i)|reg(j), false)
	case hi == 0xa0 || hi == 0xa1: // ADDN #i4
		alu(reg(i), false)
	case hi == 0xa4 || hi == 0xa5 || hi == 0xb0 || hi == 0xb1 || hi == 0xb4 || hi == 0xb5 || hi == 0xb8 || hi == 0xb9:
		alu(reg(i), true)
	case hi == 0xa8 || hi == 0xa9: // CMP #i4,Ri
		in.uses, in.defs = reg(i), resFlags
	case hi == 0xaa: // CMP Rj,Ri
		in.uses, in.defs = reg(i)|reg(j), resFlags
	case hi == 0x8b: // MOV Rj,Ri
		in.uses, in.defs = reg(j), reg(i)
	case hi == 0xab || hi == 0xaf || hi == 0xbb || hi == 0xbf: // MUL
		in.uses, in.defs = reg(i)|reg(j), resMD|resFlags
	case hi == 0xb3 && (j == 4 || j == 5): // MOV Ri,MDH/MDL
		in.uses, in.defs = reg(i)|resMD, resMD
	case hi == 0xb7 && (j == 4 || j == 5): // MOV MDH/MDL,Ri
		in.uses, in.defs = resMD, reg(i)
	case hi == 0x97 && j >= 8 && j <= 11: // EXTSB, EXTUB, EXTSH, EXTUH
		alu(reg(i), false)
	case hi == 0x9b, hi >= 0xc0 && hi <= 0xcf, op&0xfff0 == 0x9f80: // LDI
		in.defs = reg(i)
	case op == 0x9fa0: // NOP
	default:
		in.barrier = true
	}
	// The frame's accesses are known apart while R14 stays.
	if in.defs&reg(14) != 0 {
		in.barrier = true
	}
	return in, true
}

// scheduleRuns reorders the instructions of each fixed piece.
func (a *Assembler) scheduleRuns() {
	for index := range a.pieces {
		p := &a.pieces[index]
		if p.kind != pieceFixed || p.data {
			continue
		}
		var run []insn
		ok := true
		var known [16]int64 // the registers' values the run set, or -1
		for r := range known {
			known[r] = -1
		}
		for at := 0; at < len(p.code); {
			in, good := decode(p.code[at:])
			if !good {
				ok = false
				break
			}
			locate(&in, &known)
			run = append(run, in)
			at += len(in.code)
		}
		if !ok || len(run) < 3 {
			continue
		}
		// A delay slot stays where it is.
		if index > 0 && a.pieces[index-1].delay && (a.pieces[index-1].kind == pieceBranch || a.pieces[index-1].kind == pieceCall) {
			run[0].barrier = true
		}
		code := make([]byte, 0, len(p.code))
		for _, in := range schedule(tidy(run)) {
			code = append(code, in.code...)
		}
		p.code = code
	}
}

// constant is the value an LDI loads.
func constant(in insn) (int64, bool) {
	c := in.code
	op := uint16(c[0])<<8 | uint16(c[1])
	switch hi := op >> 8; {
	case hi >= 0xc0 && hi <= 0xcf:
		return int64(op >> 4 & 0xff), true
	case hi == 0x9b:
		return int64(uint32(op>>4&15)<<16 | uint32(c[2])<<8 | uint32(c[3])), true
	case op&0xfff0 == 0x9f80:
		return int64(uint32(c[2])<<24 | uint32(c[3])<<16 | uint32(c[4])<<8 | uint32(c[5])), true
	}
	return 0, false
}

// tidy leaves out what a run does again for nothing: a constant loaded
// into a register that has it, and a load of a slot of the frame whose
// value a register still has (from a store there or a load of it), which
// becomes a move, or nothing.
func tidy(run []insn) []insn {
	var known [16]int64
	holder := map[int64]int{} // a frame slot (a word) -> the register that has it
	forget := func() {
		for r := range known {
			known[r] = -1
		}
		clear(holder)
	}
	forget()
	out := make([]insn, 0, len(run))
	for k, in := range run {
		i := int(in.code[1] & 15)
		if in.barrier {
			forget()
			out = append(out, in)
			continue
		}
		fixed := k == 0 // a delay slot stays
		if v, ok := constant(in); ok && !fixed && known[i] == v {
			continue
		}
		word := in.place == placeFrame && in.size == 4
		if in.load && word && !fixed {
			if h, ok := holder[in.at]; ok {
				if h == i {
					continue
				}
				mov, _ := decode([]byte{0x8b, byte(h<<4 | i)})
				in = mov
			}
		}
		// What it changes.
		if in.store {
			for at := range holder {
				if in.place != placeFrame || at < in.at+in.size && in.at < at+4 {
					delete(holder, at)
				}
			}
		}
		value := int64(-1)
		if v, ok := constant(in); ok {
			value = v
		} else if in.code[0] == 0x8b {
			value = known[in.code[1]>>4]
		}
		for r := 0; r < 16; r++ {
			if in.defs&(1<<r) == 0 {
				continue
			}
			known[r] = -1
			for at, h := range holder {
				if h == r {
					delete(holder, at)
				}
			}
		}
		if in.defs&reg(uint16(i)) != 0 {
			known[i] = value
		}
		if word && (in.load && in.defs&reg(uint16(i)) != 0 || in.store && in.code[0]>>4 == 3) {
			holder[in.at] = i
		}
		out = append(out, in)
	}
	return out
}

// schedule orders a run: each instruction after those it depends on,
// loads first, the rest in the order given, but for one that would use what
// a load just before it loads when another could go first.
func schedule(run []insn) []insn {
	n := len(run)
	// after[k]: the instructions k must follow.
	after := make([][]int, n)
	for k := range run {
		for m := 0; m < k; m++ {
			if depends(run[m], run[k]) {
				after[k] = append(after[k], m)
			}
		}
	}
	done := make([]bool, n)
	out := make([]insn, 0, n)
	var loaded uint32 // what the last instruction placed loads
	ready := func(k int) bool {
		for _, m := range after[k] {
			if !done[m] {
				return false
			}
		}
		return true
	}
	for len(out) < n {
		// Loads as soon as they can go (their results are then ready
		// when wanted), then the rest in order; but not what would use
		// what the last one loads, when something else can go.
		pick, first := -1, -1
		for k := range run {
			if done[k] || !ready(k) {
				continue
			}
			if first < 0 {
				first = k
			}
			if run[k].uses&loaded != 0 {
				continue
			}
			if run[k].load {
				pick = k
				break
			}
			if pick < 0 {
				pick = k
			}
		}
		if pick < 0 {
			pick = first
		}
		done[pick] = true
		out = append(out, run[pick])
		loaded = run[pick].loads
	}
	return out
}

// locate finds where a load or store of a run is, from what the run put
// in registers before it, and notes what it puts in them.
func locate(in *insn, known *[16]int64) {
	c := in.code
	op := uint16(c[0])<<8 | uint16(c[1])
	hi, j, i := op>>8, op>>4&15, op&15
	s8 := int64(int8(op >> 4 & 0xff)) // the frame forms' disp8
	sizes := map[uint16]int64{0: 4, 1: 2, 2: 1, 4: 4, 5: 2, 6: 1, 0x10: 4, 0x11: 2, 0x12: 1, 0x14: 4, 0x15: 2, 0x16: 1}
	switch {
	case hi <= 0x02 || hi >= 0x10 && hi <= 0x12: // @(R13,Rj)
		if known[13] >= 0 && known[j] >= 0 {
			in.place, in.at, in.size = placeFixed, known[13]+known[j], sizes[hi]
		}
	case hi >= 0x04 && hi <= 0x06 || hi >= 0x14 && hi <= 0x16: // @Rj
		if known[j] >= 0 {
			in.place, in.at, in.size = placeFixed, known[j], sizes[hi]
		}
	case hi>>4 == 2 || hi>>4 == 3:
		in.place, in.at, in.size = placeFrame, s8*4, 4
	case hi>>4 == 4 || hi>>4 == 5:
		in.place, in.at, in.size = placeFrame, s8*2, 2
	case hi>>4 == 6 || hi>>4 == 7:
		in.place, in.at, in.size = placeFrame, s8, 1
	}
	if in.place == placeFixed && (in.at < 0 || in.at > 0xffffffff) {
		in.place = placeUnknown
	}
	// The core drops the low bits of an address a size does not fit.
	if in.place == placeFixed {
		in.at &^= in.size - 1
	}
	// What it leaves in registers.
	value := int64(-1)
	switch {
	case hi >= 0xc0 && hi <= 0xcf:
		value = int64(op >> 4 & 0xff)
	case hi == 0x9b:
		value = int64(uint32(op>>4&15)<<16 | uint32(c[2])<<8 | uint32(c[3]))
	case op&0xfff0 == 0x9f80:
		value = int64(uint32(c[2])<<24 | uint32(c[3])<<16 | uint32(c[4])<<8 | uint32(c[5]))
	case hi == 0x8b:
		value = known[j]
	}
	if in.barrier {
		for r := range known {
			known[r] = -1
		}
		return
	}
	for r := 0; r < 16; r++ {
		if in.defs&(1<<r) != 0 {
			known[r] = -1
		}
	}
	if value >= 0 && in.defs&reg(i) != 0 {
		known[i] = value
	}
}

// depends tells whether b, after a, must stay after it.
func depends(a, b insn) bool {
	if a.barrier || b.barrier {
		return true
	}
	if a.defs&b.uses != 0 || a.uses&b.defs != 0 || a.defs&b.defs != 0 {
		return true
	}
	// Loads keep their order with stores to where they may be, and
	// among themselves but in RAM (a unit's registers may count their
	// reads).
	if (a.load || a.store) && (b.load || b.store) {
		if a.store || b.store {
			return !apart(a, b)
		}
		return !a.ram() || !b.ram()
	}
	return false
}

// Delay slots. A branch or call taken costs a cycle more than one with a
// delay slot; the instruction before it goes in the slot when it can: one
// halfword, not what the branch's condition is from, not the scratch
// register (a branch rewritten through it loads it before the slot), and
// no label at the branch (what comes there would run it too). RET and
// JMP @Ri at the end of straight code take the one before them likewise.

// slotted tells whether an instruction may go in a delay slot, before a
// branch on a condition or not.
func slotted(in insn, cond bool, scratch Reg) bool {
	if in.barrier || len(in.code) != 2 || cond && in.defs&resFlags != 0 {
		return false
	}
	return (in.uses|in.defs)&reg(uint16(scratch)) == 0
}

// pickSlot finds, in straight code, the last instruction that can go after
// all those after it and into a delay slot; avoid is what it must not
// write. It returns the code without it, and it.
func pickSlot(code []byte, pinned, cond bool, scratch Reg, avoid uint32) (rest, slot []byte, ok bool) {
	var run []insn
	var starts []int
	var known [16]int64
	for r := range known {
		known[r] = -1
	}
	for k := 0; k < len(code); {
		in, good := decode(code[k:])
		if !good {
			return nil, nil, false
		}
		locate(&in, &known)
		run = append(run, in)
		starts = append(starts, k)
		k += len(in.code)
	}
	for k := len(run) - 1; k >= 0; k-- {
		if k == 0 && pinned {
			break
		}
		in := run[k]
		if in.barrier {
			break // nothing before it moves past it
		}
		if !slotted(in, cond, scratch) || in.defs&avoid != 0 {
			continue
		}
		free := true
		for m := k + 1; m < len(run) && free; m++ {
			free = !depends(in, run[m])
		}
		if free {
			rest = append(append([]byte(nil), code[:starts[k]]...), code[starts[k]+2:]...)
			return rest, in.code, true
		}
	}
	return nil, nil, false
}

// lastIs tells whether an instruction starts at at, the last of code.
func lastIs(code []byte, at int) bool {
	for k := 0; k < len(code); {
		in, ok := decode(code[k:])
		if !ok {
			return false
		}
		if k == at {
			return k+len(in.code) == len(code)
		}
		k += len(in.code)
	}
	return false
}

func (a *Assembler) fillSlots() {
	labelled := map[int]bool{}
	for _, l := range a.labels {
		labelled[l.piece] = true
	}
	// A piece's first instruction is a delay slot already.
	inSlot := func(index int) bool {
		if index == 0 {
			return false
		}
		q := a.pieces[index-1]
		return q.delay && (q.kind == pieceBranch || q.kind == pieceCall)
	}
	var out []piece
	shift := make([]int, len(a.pieces)+1) // pieces put in before each one
	added := 0
	for index := range a.pieces {
		shift[index] = added
		p := a.pieces[index]
		// RET, JMP @Ri at the end of straight code.
		if p.kind == pieceFixed && !p.data && len(p.code) >= 4 {
			at := len(p.code) - 2
			op := uint16(p.code[at])<<8 | uint16(p.code[at+1])
			var avoid uint32
			if op&0xfff0 == 0x9700 {
				avoid = reg(op & 15)
			}
			if _, ok := decode(p.code[at:]); ok && (op == 0x9720 || op&0xfff0 == 0x9700) && lastIs(p.code, at) {
				if rest, slot, ok := pickSlot(p.code[:at], inSlot(index), false, a.scratch, avoid); ok {
					code := append(rest, 0x9f, p.code[at+1]) // RET:D, JMP:D @Ri
					p.code = append(code, slot...)
				}
			}
		}
		if (p.kind == pieceBranch || p.kind == pieceCall) && !p.delay && index > 0 && !labelled[index] && len(out) > 0 {
			q := &out[len(out)-1]
			if q.kind == pieceFixed && !q.data && !q.slot {
				cond := p.kind == pieceBranch && p.cond != Always
				if rest, slot, ok := pickSlot(q.code, inSlot(index-1), cond, a.scratch, 0); ok {
					q.code = rest
					p.delay = true
					out = append(out, p, piece{kind: pieceFixed, code: slot, slot: true})
					added++
					continue
				}
			}
		}
		out = append(out, p)
	}
	shift[len(a.pieces)] = added
	for _, l := range a.labels {
		l.piece += shift[l.piece]
	}
	a.pieces = out
}
