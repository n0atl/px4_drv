// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

// Package fc builds FR30 programs from structured Go: functions with locals,
// expressions, conditions and loops, compiled with the fr30 assembler. It is
// what the larger programs the driver places on the MB86H47A are written in.
//
// Every value is 32 bits. A function takes up to four arguments in R4-R7 and
// returns one in R4, as the chip's own code does; R8-R11, R14 and R15 survive
// a call. Locals live in the function's frame (R14) unless declared as
// register locals, which take R8-R11. R0-R7 hold temporaries, R12 is the
// assembler's far-branch scratch and R13 the index and immediate scratch.
package fc

import (
	"fmt"
	"sort"

	"github.com/Till0196/coax/pkg/fr30"
)

// Program is a set of functions and data assembled at one origin.
type Program struct {
	A     *fr30.Assembler
	err   error
	funcs map[string]*Func
	data  []func()
}

// NewProgram starts a program placed at origin.
func NewProgram(origin uint32) *Program {
	a := fr30.New(origin)
	a.SetScratch(fr30.R12)
	a.SetSchedule(true)
	return &Program{A: a, funcs: map[string]*Func{}}
}

func (p *Program) fail(format string, args ...any) {
	if p.err == nil {
		p.err = fmt.Errorf("fc: "+format, args...)
	}
}

// Assemble emits the data declared with Table and returns the program.
func (p *Program) Assemble() ([]byte, error) {
	for _, emit := range p.data {
		emit()
	}
	p.data = nil
	for name, f := range p.funcs {
		if !f.defined && !f.external {
			p.fail("function %s called but not defined", name)
		}
	}
	if p.err != nil {
		return nil, p.err
	}
	return p.A.Assemble()
}

// Words is Assemble as big-endian words.
func (p *Program) Words() ([]uint32, error) {
	code, err := p.Assemble()
	if err != nil {
		return nil, err
	}
	for len(code)%4 != 0 {
		code = append(code, 0)
	}
	words := make([]uint32, len(code)/4)
	for i := range words {
		words[i] = uint32(code[4*i])<<24 | uint32(code[4*i+1])<<16 | uint32(code[4*i+2])<<8 | uint32(code[4*i+3])
	}
	return words, nil
}

// Address is where a function or table ended up, after Assemble.
func (p *Program) Address(label *fr30.Label) uint32 {
	address, err := p.A.Address(label)
	if err != nil {
		p.fail("%v", err)
	}
	return address
}

// Table places constant data after the code and gives its label; Addr(label)
// is its address in expressions.
func (p *Program) Table(name string, data []byte) *fr30.Label {
	label := p.A.Label(name)
	p.data = append(p.data, func() {
		p.A.Align(4)
		p.A.Bind(label)
		p.A.Byte(data...)
	})
	return label
}

// TableWords is Table for big-endian words.
func (p *Program) TableWords(name string, words []uint32) *fr30.Label {
	data := make([]byte, 0, len(words)*4)
	for _, w := range words {
		data = append(data, byte(w>>24), byte(w>>16), byte(w>>8), byte(w))
	}
	return p.Table(name, data)
}

// Align pads the program to a multiple of n bytes: the next function starts
// there.
func (p *Program) Align(n int) { p.A.Align(n) }

// Func is a function of the program.
type Func struct {
	p        *Program
	Name     string
	Label    *fr30.Label
	params   []*Node
	slots    int // frame words
	pinned   []fr30.Reg
	defined  bool
	nparams  int
	isr      bool
	external bool // code the program does not hold, at address
	address  uint32
	stack    uint32 // the function's own stack, if not 0
	splice   bool   // entered from other code, in its registers
	free     map[fr30.Reg]bool
	named    []fr30.Reg
	body     []stmt
	epilogue *fr30.Label
}

// Entry is where a core starts its program: the stack at stack, interrupts
// off, then main called; were it to return, the core stays in a loop. It
// goes first in the program, at the program's origin.
func (p *Program) Entry(stack uint32, main *Func) {
	a := p.A
	a.LDI32(stack, fr30.R15)
	a.ANDCCR(0xef)
	a.LDIAddr(main.Label, fr30.R12)
	a.CALL(fr30.R12)
	halt := a.Here("halt")
	a.B(fr30.Always, halt)
}

// Func declares a function with nparams arguments (0..4); it can be called
// before Define gives its body.
func (p *Program) Func(name string, nparams int) *Func {
	if f, ok := p.funcs[name]; ok {
		if f.nparams != nparams {
			p.fail("function %s declared with %d and %d parameters", name, f.nparams, nparams)
		}
		return f
	}
	if nparams > 8 {
		p.fail("function %s has more than 8 parameters", name)
	}
	f := &Func{p: p, Name: name, Label: p.A.Label(name), nparams: nparams}
	p.funcs[name] = f
	return f
}

// At declares a function the program does not hold, found at address; it
// follows the same convention.
func (p *Program) At(name string, nparams int, address uint32) *Func {
	f := p.Func(name, nparams)
	f.external, f.address = true, address
	return f
}

// OwnStack has the function run on its own stack, whose top is stack: it
// changes to it on entry and back to the caller's on return. The function
// takes at most four arguments.
func (f *Func) OwnStack(stack uint32) {
	if f.nparams > 4 || f.isr {
		f.p.fail("%s: own stack with stack arguments or as an interrupt handler", f.Name)
	}
	f.stack = stack
}

// Splice declares code that other code jumps into partway through: it has
// no prologue or frame, works in the registers free only (R12 and R13 are
// always free), reaches the other code's values through Register and ends
// with Jump. A call from it keeps its return address and every register
// of R0-R7 it was not given.
func (p *Program) Splice(name string, free ...fr30.Reg) *Func {
	f := p.Func(name, 0)
	f.splice, f.free = true, map[fr30.Reg]bool{}
	for _, r := range free {
		f.free[r] = true
	}
	return f
}

// Register is a value the code around a splice keeps in r, as a local.
func (f *Func) Register(name string, r fr30.Reg) *Node {
	if !f.splice {
		f.p.fail("%s: Register outside a splice", f.Name)
	}
	f.named = append(f.named, r)
	return &Node{kind: nVar, f: f, pinned: true, reg: r, name: name}
}

// Interrupt declares an interrupt handler: it keeps every register it uses
// and ends with RETI.
func (p *Program) Interrupt(name string) *Func {
	f := p.Func(name, 0)
	f.isr = true
	return f
}

// Param is the i-th argument, a local of the function.
func (f *Func) Param(i int) *Node {
	if i >= f.nparams {
		f.p.fail("%s: parameter %d of %d", f.Name, i, f.nparams)
	}
	for len(f.params) <= i {
		f.params = append(f.params, f.Local(fmt.Sprintf("param%d", len(f.params))))
	}
	return f.params[i]
}

// Local makes a local variable in the frame.
func (f *Func) Local(name string) *Node {
	f.slots++
	return &Node{kind: nVar, f: f, slot: f.slots, name: name}
}

// RegLocal makes a local variable kept in one of R8-R11.
func (f *Func) RegLocal(name string) *Node {
	if len(f.pinned) == 4 {
		f.p.fail("%s: more than four register locals", f.Name)
	}
	reg := fr30.R8 + fr30.Reg(len(f.pinned))
	f.pinned = append(f.pinned, reg)
	return &Node{kind: nVar, f: f, pinned: true, reg: reg, name: name}
}

// Array reserves size bytes in the frame; the node is its address.
func (f *Func) Array(name string, size int) *Node {
	words := (size + 3) / 4
	f.slots += words
	return &Node{kind: nFrameAddr, f: f, slot: f.slots, name: name}
}

// Define gives the function its body and emits it.
func (f *Func) Define(body func(b *Block)) {
	if f.defined || f.external {
		f.p.fail("function %s defined twice", f.Name)
	}
	f.defined = true
	for i := 0; i < f.nparams; i++ {
		f.Param(i)
	}
	b := &Block{f: f}
	body(b)
	f.body = b.stmts
	g := &gen{f: f, a: f.p.A}
	if !f.isr && !f.splice && !g.leaf() {
		f.autoPin()
	}
	g.function()
}

// FuncAddresses tells where each function is, once assembled.
func (p *Program) FuncAddresses() map[string]uint32 {
	out := map[string]uint32{}
	for name, f := range p.funcs {
		if f.external {
			out[name] = f.address
			continue
		}
		out[name] = p.Address(f.Label)
	}
	return out
}

// autoPin keeps the most used locals in the registers R8-R11 left over:
// uses in a loop count eight times those outside it.
func (f *Func) autoPin() {
	uses := map[*Node]int{}
	var order []*Node
	var node func(n *Node, w int)
	node = func(n *Node, w int) {
		if n == nil {
			return
		}
		if n.kind == nVar && n.f == f && !n.pinned {
			if _, ok := uses[n]; !ok {
				order = append(order, n)
			}
			uses[n] += w
		}
		node(n.l, w)
		node(n.r, w)
		for _, x := range n.args {
			node(x, w)
		}
	}
	var stmts func(list []stmt, w int)
	stmts = func(list []stmt, w int) {
		for i := range list {
			s := &list[i]
			inner := w
			if s.kind == sWhile || s.kind == sLoop {
				inner = min(w*8, 1<<24)
			}
			node(s.place, w)
			node(s.rem, w)
			node(s.value, inner)
			stmts(s.then, inner)
			stmts(s.orelse, inner)
		}
	}
	stmts(f.body, 1)
	sort.SliceStable(order, func(i, j int) bool { return uses[order[i]] > uses[order[j]] })
	for _, v := range order {
		if len(f.pinned) == 4 || uses[v] < pinUses {
			break
		}
		v.pinned = true
		v.reg = fr30.R8 + fr30.Reg(len(f.pinned))
		f.pinned = append(f.pinned, v.reg)
	}
}

// pinUses is the least weighted use count a local is kept in a register
// for.
var pinUses = 8
