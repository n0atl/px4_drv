// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fc

import (
	"sort"

	. "github.com/Till0196/coax/pkg/fr30"
)

// tempOrder is the order temporaries are taken in; the argument registers
// come last so that they are rarely live across a call.
var tempOrder = []Reg{R0, R1, R2, R3, R5, R6, R7, R4}

type loopLabels struct{ brk, cont *Label }

type gen struct {
	f        *Func
	a        *Assembler
	used     [16]bool
	kept     [16]bool // registers locals keep: not temporaries
	loops    []loopLabels
	frame    bool // the function makes a frame
	returned bool // a return went to the epilogue
	// The last store of a frame local: the register still holds it while
	// nothing went in after it.
	stored struct {
		at, slot int
		reg      Reg
	}
	// The register the Z flag was last set by, while nothing went in
	// after it.
	zero struct {
		at  int
		reg Reg
	}
}

// setZ notes that the last instruction set the Z flag by r.
func (g *gen) setZ(r Reg) { g.zero.at, g.zero.reg = g.a.Emitted(), r }

// zeroTest compares r with zero unless the Z flag already tells.
func (g *gen) zeroTest(r Reg) {
	if g.zero.at != g.a.Emitted() || g.zero.reg != r {
		g.a.CMPI(0, r)
	}
}

func (g *gen) fail(format string, args ...any) { g.f.p.fail(g.f.Name+": "+format, args...) }

func (g *gen) alloc() Reg {
	for _, r := range tempOrder {
		if !g.used[r] && !g.kept[r] {
			g.used[r] = true
			return r
		}
	}
	g.fail("expression needs more than %d temporaries", len(tempOrder))
	return R0
}

func (g *gen) free(r Reg) { g.used[r] = false }

// spare counts the temporaries free.
func (g *gen) spare() int {
	n := 0
	for _, r := range tempOrder {
		if !g.used[r] && !g.kept[r] {
			n++
		}
	}
	return n
}

func (g *gen) live() []Reg {
	var regs []Reg
	for _, r := range tempOrder {
		if g.used[r] {
			regs = append(regs, r)
		}
	}
	return regs
}

func (g *gen) function() {
	f, a := g.f, g.a
	if f.splice {
		g.splice()
		return
	}
	// A leaf function keeps its arguments where they came, in R4-R7,
	// out of the temporaries' way.
	inRegs := !f.isr && g.leaf()
	if inRegs {
		for i := 0; i < f.nparams; i++ {
			v := f.params[i]
			v.pinned, v.reg = true, R4+Reg(i)
			g.kept[v.reg] = true
		}
	}
	a.Bind(f.Label)
	f.epilogue = a.Label(f.Name + ".return")
	if f.isr {
		a.STM(R0, R1, R2, R3, R4, R5, R6, R7, R12, R13)
		a.PUSHS(RP)
		a.PUSHS(MDH)
		a.PUSHS(MDL)
	} else {
		if !g.leaf() || f.stack != 0 {
			a.PUSHS(RP)
		}
		if f.stack != 0 {
			a.MOV(R15, R13)
			a.LDI32(f.stack, R15)
			a.PUSH(R13)
		}
		if len(f.pinned) > 0 {
			a.STM(f.pinned...)
		}
	}
	// A leaf keeping its arguments in registers needs no frame when they
	// are all its locals.
	g.frame = f.slots > 0 && !(inRegs && f.slots == f.nparams)
	if g.frame {
		if f.slots > 254 {
			g.fail("frame of %d words", f.slots)
		}
		a.ENTER(4 * (f.slots + 1))
	}
	for i := 0; i < min(f.nparams, 4) && !inRegs; i++ {
		g.storeVar(f.params[i], R4+Reg(i))
	}
	// Arguments after the fourth are where the caller left them: above
	// the return address, the saved registers and the saved R14.
	for i := 4; i < f.nparams; i++ {
		g.a.LDR14(8+4*len(f.pinned)+4*(i-4), R12)
		g.storeVar(f.params[i], R12)
	}
	g.stmts(f.body)
	a.Bind(f.epilogue)
	// A body that ends in a tail call and never returns has no epilogue.
	if n := len(f.body); n > 0 && f.body[n-1].kind == sTail && !g.returned {
		return
	}
	g.unwind()
	if f.isr {
		a.POPS(MDL)
		a.POPS(MDH)
		a.POPS(RP)
		a.LDM(R0, R1, R2, R3, R4, R5, R6, R7, R12, R13)
		a.RETI()
		return
	}
	a.RET()
}

// splice emits a splice: no prologue, the temporaries only the registers it
// was given, the end a jump.
func (g *gen) splice() {
	f := g.f
	for _, r := range tempOrder {
		if !f.free[r] {
			g.kept[r] = true
		}
	}
	for _, r := range f.named {
		g.kept[r] = true
	}
	if f.slots > 0 {
		g.fail("a splice has no frame for locals")
	}
	g.a.Bind(f.Label)
	g.stmts(f.body)
	if n := len(f.body); n == 0 || f.body[n-1].kind != sJump {
		g.fail("a splice must end in a jump")
	}
}

// unwind undoes the prologue, short of the return: the frame, the kept
// registers, the own stack and the return address.
func (g *gen) unwind() {
	f, a := g.f, g.a
	if g.frame {
		a.LEAVE()
	}
	if f.isr {
		a.POPS(MDL)
		a.POPS(MDH)
		a.POPS(RP)
		a.LDM(R0, R1, R2, R3, R4, R5, R6, R7, R12, R13)
		return
	}
	if len(f.pinned) > 0 {
		a.LDM(f.pinned...)
	}
	if f.stack != 0 {
		a.POP(R15)
	}
	if !g.leaf() || f.stack != 0 {
		a.POPS(RP)
	}
}

// leaf tells a function that calls nothing and takes its arguments in
// registers: it need not keep its return address.
func (g *gen) leaf() bool {
	if g.f.nparams > 4 {
		return false
	}
	var node func(n *Node) bool
	node = func(n *Node) bool {
		if n == nil {
			return true
		}
		if n.kind == nCall {
			return false
		}
		for _, x := range n.args {
			if !node(x) {
				return false
			}
		}
		return node(n.l) && node(n.r)
	}
	var stmts func(list []stmt) bool
	stmts = func(list []stmt) bool {
		for i := range list {
			s := &list[i]
			if s.kind == sTail {
				for _, x := range s.value.args {
					if !node(x) {
						return false
					}
				}
				continue
			}
			if s.kind == sAsm || !node(s.place) || !node(s.rem) || !node(s.value) || !stmts(s.then) || !stmts(s.orelse) {
				return false
			}
		}
		return true
	}
	return stmts(g.f.body)
}

func (g *gen) frameDisp(slot int) int {
	if slot > 128 {
		g.fail("local in frame word %d is out of reach", slot)
	}
	return -4 * slot
}

func (g *gen) loadVar(v *Node, dst Reg) {
	if v.f != g.f {
		g.fail("local %s of %s used in %s", v.name, v.f.Name, g.f.Name)
	}
	if v.pinned {
		g.a.MOV(v.reg, dst)
		return
	}
	if g.stored.at == g.a.Emitted() && g.stored.slot == v.slot {
		if g.stored.reg != dst {
			g.a.MOV(g.stored.reg, dst)
		}
		return
	}
	g.a.LDR14(g.frameDisp(v.slot), dst)
}

func (g *gen) storeVar(v *Node, src Reg) {
	if v.f != g.f {
		g.fail("local %s of %s used in %s", v.name, v.f.Name, g.f.Name)
	}
	if v.pinned {
		g.a.MOV(src, v.reg)
		return
	}
	g.a.STR14(src, g.frameDisp(v.slot))
	g.stored.at, g.stored.slot, g.stored.reg = g.a.Emitted(), v.slot, src
}

// constant is n's value when it is a constant.
func constant(n *Node) (uint32, bool) {
	if n.kind == nConst {
		return n.value, true
	}
	return 0, false
}

// address evaluates a memory address as a base register plus a constant
// offset. With temp false the base is R13, holding a constant address, and
// the offset is 0.
func (g *gen) address(n *Node) (base Reg, offset uint32, temp bool) {
	if c, ok := constant(n); ok {
		g.a.LDI(c, R13)
		return R13, 0, false
	}
	x := n
	if n.kind == nBin && n.op == opAdd {
		if c, ok := constant(n.r); ok {
			x, offset = n.l, c
		}
	}
	// A local kept in a register is the base as it is.
	if x.kind == nVar && x.pinned && x.f == g.f {
		return x.reg, offset, false
	}
	return g.eval(x), offset, true
}

func (g *gen) eval(n *Node) Reg {
	a := g.a
	switch n.kind {
	case nConst:
		r := g.alloc()
		a.LDI(n.value, r)
		return r
	case nLabel:
		r := g.alloc()
		a.LDIAddr(n.label, r)
		return r
	case nVar:
		r := g.alloc()
		g.loadVar(n, r)
		return r
	case nFrameAddr:
		r := g.alloc()
		a.MOV(R14, r)
		a.LDI(uint32(4*n.slot), R13)
		a.SUB(R13, r)
		return r
	case nMDH:
		r := g.alloc()
		a.MOVFromS(MDH, r)
		return r
	case nEntrySP, nEntryRP:
		if g.f.isr || g.f.stack != 0 || g.frame || len(g.f.pinned) > 0 || !g.leaf() {
			g.fail("the caller's stack and return address are read only in a leaf function without a frame")
		}
		r := g.alloc()
		if n.kind == nEntrySP {
			a.MOV(R15, r)
		} else {
			a.MOVFromS(RP, r)
		}
		return r
	case nLoad:
		base, offset, temp := g.address(n.l)
		r := base
		if !temp {
			r = g.alloc()
		}
		if offset == 0 {
			switch n.size {
			case 4:
				a.LD(base, r)
			case 2:
				a.LDUH(base, r)
			default:
				a.LDUB(base, r)
			}
		} else {
			a.LDI(offset, R13)
			switch n.size {
			case 4:
				a.LDR13(base, r)
			case 2:
				a.LDUHR13(base, r)
			default:
				a.LDUBR13(base, r)
			}
		}
		if n.signed {
			if n.size == 2 {
				a.EXTSH(r)
			} else if n.size == 1 {
				a.EXTSB(r)
			}
		}
		return r
	case nCall:
		return g.call(n)
	case nNeg:
		r := g.eval(n.l)
		a.MOV(r, R13)
		a.LDI8(0, r)
		a.SUB(R13, r)
		return r
	case nCompl:
		r := g.eval(n.l)
		a.LDI32(0xffffffff, R13)
		a.EOR(R13, r)
		return r
	case nNot:
		return g.boolean(n)
	case nBin:
		if n.op.isCompare() || n.op == opLAnd || n.op == opLOr {
			return g.boolean(n)
		}
		return g.arith(n)
	}
	g.fail("expression kind %d", n.kind)
	return R0
}

// boolean materializes a condition as 1 or 0.
func (g *gen) boolean(n *Node) Reg {
	a := g.a
	r := g.alloc()
	no, done := a.Label("false"), a.Label("done")
	g.jump(n, false, no)
	a.LDI8(1, r)
	a.B(Always, done)
	a.Bind(no)
	a.LDI8(0, r)
	a.Bind(done)
	return r
}

// operand puts the right-hand side of a binary operator in a register: R13
// for a constant, else a temporary (temp true). l holds the left-hand side;
// when few temporaries are left it waits on the stack meanwhile, and it may
// come back in another register.
func (g *gen) operand(l *Reg, n *Node) (Reg, bool) {
	if c, ok := constant(n); ok {
		g.a.LDI(c, R13)
		return R13, false
	}
	if n.kind == nVar && n.pinned && n.f == g.f {
		return n.reg, false
	}
	if g.spare() <= 4 && g.used[*l] {
		g.a.PUSH(*l)
		g.free(*l)
		r := g.eval(n)
		*l = g.alloc()
		g.a.POP(*l)
		return r, true
	}
	return g.eval(n), true
}

func (g *gen) arith(n *Node) Reg {
	a := g.a
	l := g.eval(n.l)
	c, isConst := constant(n.r)
	sc := int32(c)
	switch n.op {
	case opAdd, opSub:
		if n.op == opSub {
			sc = -sc
		}
		if isConst {
			switch {
			case sc == 0:
				return l
			case sc > 0 && sc <= 15:
				a.ADDI(int(sc), l)
				g.setZ(l)
				return l
			case sc < 0 && sc >= -16:
				a.ADD2(int(sc), l)
				g.setZ(l)
				return l
			}
		}
	case opShl, opShr, opSar:
		if isConst {
			switch n.op {
			case opShl:
				a.Shl(int(c&31), l)
			case opShr:
				a.Shr(int(c&31), l)
			default:
				a.Sar(int(c&31), l)
			}
			return l
		}
	}
	r, temp := g.operand(&l, n.r)
	switch n.op {
	case opAdd:
		a.ADD(r, l)
		g.setZ(l)
	case opSub:
		a.SUB(r, l)
		g.setZ(l)
	case opMul:
		a.MUL(r, l)
		a.MOVFromS(MDL, l)
	case opAnd:
		a.AND(r, l)
		g.setZ(l)
	case opOr:
		a.OR(r, l)
		g.setZ(l)
	case opXor:
		a.EOR(r, l)
		g.setZ(l)
	case opShl:
		a.LSLR(r, l)
	case opShr:
		a.LSRR(r, l)
	case opSar:
		a.ASRR(r, l)
	case opDivU, opModU:
		g.divide(l, r, n.op == opModU)
	case opDivUShort:
		g.divideShort(l, r, int(n.value))
	case opDiv, opMod:
		// The signed step sequence (DIV0S, 32 DIV1, DIV2, DIV3, DIV4S):
		// truncating, the remainder with the dividend's sign; a zero
		// divisor is not checked for and gives whatever the steps leave.
		a.MOVToS(l, MDL)
		a.DIV0S(r)
		for step := 0; step < 32; step++ {
			a.DIV1(r)
		}
		a.DIV2(r)
		a.DIV3()
		a.DIV4S()
		if n.op == opMod {
			a.MOVFromS(MDH, l)
		} else {
			a.MOVFromS(MDL, l)
		}
	default:
		g.fail("operator %d", n.op)
	}
	if temp {
		g.free(r)
	}
	return l
}

// divide sets l to l / r unsigned, or l % r.
func (g *gen) divide(l, r Reg, remainder bool) {
	a := g.a
	a.MOVToS(l, MDL)
	a.DIV0U(r)
	for step := 0; step < 32; step++ {
		a.DIV1(r)
	}
	if remainder {
		a.MOVFromS(MDH, l)
	} else {
		a.MOVFromS(MDL, l)
	}
}

// divideShort sets l to l / r unsigned, the quotient below 1<<bits: the
// steps that would only shift the dividend's top bits into MDH are done
// at once.
func (g *gen) divideShort(l, r Reg, bits int) {
	if bits < 1 || bits > 31 {
		g.fail("quotient of %d bits", bits)
		return
	}
	a := g.a
	t := g.alloc()
	a.MOV(l, t)
	a.Shr(bits, t)
	a.Shl(32-bits, l)
	a.MOVToS(l, MDL)
	a.DIV0U(r)
	a.MOVToS(t, MDH)
	for step := 0; step < bits; step++ {
		a.DIV1(r)
	}
	a.MOVFromS(MDL, l)
	g.free(t)
}

// jump branches to target when n's truth equals when, and falls through
// otherwise.
func (g *gen) jump(n *Node, when bool, target *Label) {
	a := g.a
	switch {
	case n.kind == nConst:
		if (n.value != 0) == when {
			a.B(Always, target)
		}
		return
	case n.kind == nNot:
		g.jump(n.l, !when, target)
		return
	case n.kind == nBin && n.op == opLAnd:
		if when {
			skip := a.Label("and")
			g.jump(n.l, false, skip)
			g.jump(n.r, true, target)
			a.Bind(skip)
		} else {
			g.jump(n.l, false, target)
			g.jump(n.r, false, target)
		}
		return
	case n.kind == nBin && n.op == opLOr:
		if when {
			g.jump(n.l, true, target)
			g.jump(n.r, true, target)
		} else {
			skip := a.Label("or")
			g.jump(n.l, true, skip)
			g.jump(n.r, false, target)
			a.Bind(skip)
		}
		return
	case n.kind == nBin && n.op.isCompare():
		var l Reg
		lTemp := true
		if x := n.l; x.kind == nVar && x.pinned && x.f == g.f {
			l, lTemp = x.reg, false
		} else {
			l = g.eval(n.l)
		}
		c, isConst := constant(n.r)
		switch {
		case isConst && c == 0 && (n.op == opEq || n.op == opNe):
			g.zeroTest(l)
		case isConst && c <= 15:
			a.CMPI(int(c), l)
		case isConst && int32(c) < 0 && int32(c) >= -16:
			a.CMP2(int(int32(c)), l)
		default:
			r, temp := g.operand(&l, n.r)
			a.CMP(r, l)
			if temp {
				g.free(r)
			}
		}
		if lTemp {
			g.free(l)
		}
		cond := map[binOp]Cond{
			opEq: EQ, opNe: NE, opLtU: C, opGeU: NC, opLeU: LS, opGtU: HI,
			opLt: LT, opGe: GE, opLe: LE, opGt: GT,
		}[n.op]
		if !when {
			cond = cond.Not()
		}
		a.B(cond, target)
		return
	}
	r := g.eval(n)
	g.zeroTest(r)
	g.free(r)
	if when {
		a.B(NE, target)
	} else {
		a.B(EQ, target)
	}
}

// call evaluates a call: the live temporaries are saved around it, the
// arguments go through the stack into R4-R7.
func (g *gen) call(n *Node) Reg {
	a := g.a
	saved := g.live()
	// A splice keeps the code around it whole: its return address and
	// the registers it was not given.
	var kept []Reg
	if g.f.splice {
		a.PUSHS(RP)
		for r := R0; r <= R7; r++ {
			if g.kept[r] && !g.used[r] {
				kept = append(kept, r)
			}
		}
	}
	if all := append(append([]Reg{}, saved...), kept...); len(all) > 0 {
		sort.Slice(all, func(i, j int) bool { return all[i] < all[j] })
		a.STM(all...)
		defer func() {
			a.LDM(all...)
			if g.f.splice {
				a.POPS(RP)
			}
		}()
	} else if g.f.splice {
		defer a.POPS(RP)
	}
	for _, r := range saved {
		g.free(r)
	}
	// The arguments after the fourth stay on the stack, the fifth at
	// the stack pointer; the first four go in R4-R7.
	for i := len(n.args) - 1; i >= 4; i-- {
		r := g.eval(n.args[i])
		a.PUSH(r)
		g.free(r)
	}
	g.arguments(n.args[:min(len(n.args), 4)])
	if n.callee.external {
		a.LDI32(n.callee.address, R12)
		a.CALL(R12)
	} else {
		a.CALLL(n.callee.Label)
	}
	if len(n.args) > 4 {
		a.ADDSP(4 * (len(n.args) - 4))
	}
	for _, r := range saved {
		g.used[r] = true
	}
	result := g.alloc()
	if result != R4 {
		a.MOV(R4, result)
	}
	return result
}

// arguments puts the first four arguments in R4-R7: each worked out into
// a temporary and moved, the constants, labels and locals loaded straight
// in last. When a temporary is one of R4-R7 the moves could overwrite one
// another: then the values go through the stack.
func (g *gen) arguments(args []*Node) {
	a := g.a
	simple := func(x *Node) bool {
		_, ok := constant(x)
		return ok || x.kind == nLabel || x.kind == nVar
	}
	regs := make([]Reg, len(args))
	clash := false
	for i, x := range args {
		if !simple(x) {
			regs[i] = g.eval(x)
			clash = clash || regs[i] >= R4 && regs[i] <= R7
		}
	}
	if clash {
		for i, x := range args {
			if simple(x) {
				regs[i] = g.eval(x)
			}
			a.PUSH(regs[i])
			g.free(regs[i])
		}
		for i := len(args) - 1; i >= 0; i-- {
			a.POP(R4 + Reg(i))
		}
		return
	}
	for i, x := range args {
		if !simple(x) {
			a.MOV(regs[i], R4+Reg(i))
			g.free(regs[i])
		}
	}
	for i, x := range args {
		r := R4 + Reg(i)
		switch {
		case x.kind == nLabel:
			a.LDIAddr(x.label, r)
		case x.kind == nVar:
			g.loadVar(x, r)
		case simple(x):
			v, _ := constant(x)
			a.LDI(v, r)
		}
	}
}

func (g *gen) stmts(list []stmt) {
	for i := range list {
		g.stmt(&list[i])
		if live := g.live(); len(live) > 0 {
			g.fail("temporaries %v left live", live)
			for _, r := range live {
				g.free(r)
			}
		}
	}
}

func (g *gen) stmt(s *stmt) {
	a := g.a
	switch s.kind {
	case sDivMod:
		// The quotient from MDL, the remainder left in MDH.
		l := g.eval(s.value.l)
		d := g.eval(s.value.r)
		g.divide(l, d, false)
		g.free(d)
		g.storeVar(s.place, l)
		a.MOVFromS(MDH, l)
		g.storeVar(s.rem, l)
		g.free(l)
	case sSet:
		if v := s.place; v.kind == nVar && v.pinned && v.f == g.f {
			// place = place + small constant, in the register.
			if x := s.value; x.kind == nBin && (x.op == opAdd || x.op == opSub) && x.l == v {
				if c, ok := constant(x.r); ok {
					sc := int32(c)
					if x.op == opSub {
						sc = -sc
					}
					switch {
					case sc >= 0 && sc <= 15:
						a.ADDI(int(sc), v.reg)
						g.setZ(v.reg)
						return
					case sc < 0 && sc >= -16:
						a.ADD2(int(sc), v.reg)
						g.setZ(v.reg)
						return
					}
				}
			}
		}
		if s.place.kind == nVar {
			r := g.eval(s.value)
			g.storeVar(s.place, r)
			g.free(r)
			return
		}
		v := g.eval(s.value)
		base, offset, temp := g.address(s.place.l)
		if offset == 0 {
			switch s.place.size {
			case 4:
				a.ST(v, base)
			case 2:
				a.STH(v, base)
			default:
				a.STB(v, base)
			}
		} else {
			a.LDI(offset, R13)
			switch s.place.size {
			case 4:
				a.STR13(v, base)
			case 2:
				a.STHR13(v, base)
			default:
				a.STBR13(v, base)
			}
		}
		if temp {
			g.free(base)
		}
		g.free(v)
	case sIf:
		end := a.Label("endif")
		if s.orelse == nil {
			g.jump(s.value, false, end)
			g.stmts(s.then)
		} else {
			other := a.Label("else")
			g.jump(s.value, false, other)
			g.stmts(s.then)
			a.B(Always, end)
			a.Bind(other)
			g.stmts(s.orelse)
		}
		a.Bind(end)
	case sWhile:
		top, cont, end := a.Label("while"), a.Label("continue"), a.Label("endwhile")
		a.Bind(top)
		g.jump(s.value, false, end)
		g.loops = append(g.loops, loopLabels{end, cont})
		g.stmts(s.then)
		g.loops = g.loops[:len(g.loops)-1]
		a.Bind(cont)
		g.stmts(s.orelse)
		a.B(Always, top)
		a.Bind(end)
	case sLoop:
		top, end := a.Label("loop"), a.Label("endloop")
		a.Bind(top)
		g.loops = append(g.loops, loopLabels{end, top})
		g.stmts(s.then)
		g.loops = g.loops[:len(g.loops)-1]
		a.B(Always, top)
		a.Bind(end)
	case sBreak, sContinue:
		if len(g.loops) == 0 {
			g.fail("break or continue outside a loop")
			return
		}
		loop := g.loops[len(g.loops)-1]
		if s.kind == sBreak {
			a.B(Always, loop.brk)
		} else {
			a.B(Always, loop.cont)
		}
	case sReturn:
		if s.value != nil {
			r := g.eval(s.value)
			if r != R4 {
				a.MOV(r, R4)
			}
			g.free(r)
		}
		if g.f.splice {
			g.fail("a splice ends in a jump, not a return")
		}
		g.returned = true
		a.B(Always, g.f.epilogue)
	case sTail:
		n := s.value
		if g.f.isr {
			g.fail("tail call from an interrupt handler")
		}
		// Arguments kept in R4-R7 would be moved among themselves in
		// an order that could lose one.
		for i, x := range n.args {
			if x.kind == nVar && x.pinned && x.reg >= R4 && x.reg <= R7 && x.reg != R4+Reg(i) {
				g.fail("tail call passes argument %s on as argument %d", x.name, i)
			}
		}
		g.arguments(n.args)
		g.unwind()
		if n.callee.external {
			if n.callee.address < 1<<20 {
				a.LDI20(n.callee.address, R12)
			} else {
				a.LDI32(n.callee.address, R12)
			}
		} else {
			a.LDIAddr(n.callee.Label, R12)
		}
		a.JMP(R12)
	case sExpr:
		g.free(g.eval(s.value))
	case sAsm:
		s.asm(a)
	case sJump:
		if s.target < 1<<20 {
			a.LDI20(s.target, R12)
		} else {
			a.LDI32(s.target, R12)
		}
		a.JMP(R12)
	case sEnable:
		a.STILM(0x1f)
		a.ORCCR(0x10)
	case sLabel:
		a.Bind(s.label)
	case sGoto:
		a.B(Always, s.label)
	}
}
