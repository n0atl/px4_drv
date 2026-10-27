// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fc

import "github.com/Till0196/coax/pkg/fr30"

type stmtKind uint8

const (
	sSet stmtKind = iota
	sIf
	sWhile
	sLoop
	sBreak
	sContinue
	sReturn
	sExpr
	sAsm
	sLabel
	sGoto
	sDivMod
	sEnable
	sTail
	sJump
)

type stmt struct {
	kind   stmtKind
	place  *Node
	value  *Node
	then   []stmt
	orelse []stmt
	asm    func(a *fr30.Assembler)
	label  *fr30.Label
	rem    *Node  // sDivMod: where the remainder goes
	target uint32 // sJump
}

// Block is a sequence of statements being built.
type Block struct {
	f     *Func
	stmts []stmt
}

// F is the function the block belongs to.
func (b *Block) F() *Func { return b.f }

// Set assigns value to a local or a memory place.
func (b *Block) Set(place *Node, value any) {
	if place.kind != nVar && place.kind != nLoad {
		b.f.p.fail("%s: assignment to something that is not a place", b.f.Name)
	}
	b.stmts = append(b.stmts, stmt{kind: sSet, place: place, value: node(value)})
}

// DivModU sets quotient to x / d and remainder to x % d, unsigned, with the
// one division (both must be locals).
func (b *Block) DivModU(quotient, remainder *Node, x, d any) {
	if quotient.kind != nVar || remainder.kind != nVar {
		b.f.p.fail("%s: DivModU into something that is not a local", b.f.Name)
	}
	b.stmts = append(b.stmts, stmt{kind: sDivMod, place: quotient, rem: remainder, value: node(x).DivU(d)})
}

// Inc adds x to a place.
func (b *Block) Inc(place *Node, x any) { b.Set(place, place.Add(x)) }

// Store32 writes a word to address.
func (b *Block) Store32(address, value any) { b.Set(Mem32(address), value) }

// Store16 writes a halfword to address.
func (b *Block) Store16(address, value any) { b.Set(Mem16(address), value) }

// Store8 writes a byte to address.
func (b *Block) Store8(address, value any) { b.Set(Mem8(address), value) }

// Do evaluates an expression for its effects, a call usually.
func (b *Block) Do(x *Node) { b.stmts = append(b.stmts, stmt{kind: sExpr, value: x}) }

func (b *Block) sub(body func(b *Block)) []stmt {
	if body == nil {
		return nil
	}
	inner := &Block{f: b.f}
	body(inner)
	return inner.stmts
}

// If is a conditional statement; Else adds its other branch.
type If struct {
	s *stmt
	b *Block
}

// If runs then when cond is not 0.
func (b *Block) If(cond any, then func(b *Block)) *If {
	b.stmts = append(b.stmts, stmt{kind: sIf, value: node(cond), then: b.sub(then)})
	return &If{s: &b.stmts[len(b.stmts)-1], b: b}
}

// Else gives the statement's other branch. It must be called right after If.
func (i *If) Else(orelse func(b *Block)) {
	s := &i.b.stmts[len(i.b.stmts)-1]
	s.orelse = i.b.sub(orelse)
}

// ElseIf chains another condition.
func (i *If) ElseIf(cond any, then func(b *Block)) *If {
	var next *If
	i.Else(func(b *Block) { next = b.If(cond, then) })
	return next
}

// While runs body as long as cond holds, testing it first.
func (b *Block) While(cond any, body func(b *Block)) {
	b.stmts = append(b.stmts, stmt{kind: sWhile, value: node(cond), then: b.sub(body)})
}

// For runs init, then body and step while cond holds. Continue goes to step.
func (b *Block) For(init func(b *Block), cond any, step func(b *Block), body func(b *Block)) {
	if init != nil {
		init(b)
	}
	b.stmts = append(b.stmts, stmt{kind: sWhile, value: node(cond), then: b.sub(body), orelse: b.sub(step)})
}

// Range runs body with v going from start up to (not including) end.
func (b *Block) Range(v *Node, start, end any, body func(b *Block)) {
	b.For(func(b *Block) { b.Set(v, start) }, v.Lt(end), func(b *Block) { b.Inc(v, 1) }, body)
}

// Loop runs body until Break or Return.
func (b *Block) Loop(body func(b *Block)) {
	b.stmts = append(b.stmts, stmt{kind: sLoop, then: b.sub(body)})
}

// Break leaves the innermost loop.
func (b *Block) Break() { b.stmts = append(b.stmts, stmt{kind: sBreak}) }

// Continue goes to the next pass of the innermost loop.
func (b *Block) Continue() { b.stmts = append(b.stmts, stmt{kind: sContinue}) }

// Return leaves the function, with a value or without.
func (b *Block) Return(value ...any) {
	s := stmt{kind: sReturn}
	if len(value) > 0 {
		s.value = node(value[0])
	}
	b.stmts = append(b.stmts, s)
}

// Asm emits instructions as they are. R0-R7 hold nothing then, but locals in
// R8-R11 and the frame in R14 must be kept.
func (b *Block) Asm(emit func(a *fr30.Assembler)) {
	b.stmts = append(b.stmts, stmt{kind: sAsm, asm: emit})
}

// TailCall leaves the function by jumping to f with args (at most four): f
// returns to this function's caller.
func (b *Block) TailCall(f *Func, args ...any) {
	if len(args) > 4 {
		b.f.p.fail("%s: tail call with %d arguments", b.f.Name, len(args))
	}
	b.stmts = append(b.stmts, stmt{kind: sTail, value: f.Call(args...)})
}

// Jump goes on at address, for good: how a splice ends.
func (b *Block) Jump(address uint32) {
	b.stmts = append(b.stmts, stmt{kind: sJump, target: address})
}

// EnableInterrupts accepts interrupts of every level.
func (b *Block) EnableInterrupts() {
	b.stmts = append(b.stmts, stmt{kind: sEnable})
}

// Mark binds a label here, for Goto.
func (b *Block) Mark(label *fr30.Label) { b.stmts = append(b.stmts, stmt{kind: sLabel, label: label}) }

// Goto jumps to a label bound with Mark in the same function.
func (b *Block) Goto(label *fr30.Label) { b.stmts = append(b.stmts, stmt{kind: sGoto, label: label}) }

// sel is a local that holds a when cond holds and x otherwise.
func (b *Block) sel(cond *Node, a, x any) *Node {
	t := b.f.Local("sel")
	b.If(cond, func(b *Block) { b.Set(t, a) }).Else(func(b *Block) { b.Set(t, x) })
	return t
}

// Tmp makes a local set to value.
func (b *Block) Tmp(name string, value any) *Node {
	t := b.f.Local(name)
	b.Set(t, value)
	return t
}
