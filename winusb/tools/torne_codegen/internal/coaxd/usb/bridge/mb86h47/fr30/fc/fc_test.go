// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fc

import (
	"fmt"
	"math/rand"
	"testing"

	"github.com/Till0196/coax/pkg/fr30/sim"
)

const (
	testOrigin = 0x10000
	testStack  = 0x80000
	testData   = 0x40000
)

// build assembles the program and loads it into a simulator.
func build(t *testing.T, p *Program) *sim.CPU {
	t.Helper()
	code, err := p.Assemble()
	if err != nil {
		t.Fatal(err)
	}
	cpu := sim.New()
	cpu.Load(testOrigin, code)
	return cpu
}

func call(t *testing.T, cpu *sim.CPU, p *Program, f *Func, args ...uint32) uint32 {
	t.Helper()
	result, err := cpu.Call(p.Address(f.Label), testStack, args...)
	if err != nil {
		t.Fatal(err)
	}
	if cpu.R[15] != testStack {
		t.Fatalf("%s left the stack at %#x", f.Name, cpu.R[15])
	}
	return result
}

// expression is a random expression over the parameters a, b, c together
// with its value as Go computes it.
type expression struct {
	node  *Node
	value func(a, b, c uint32) uint32
	text  string
}

func randomExpression(r *rand.Rand, f *Func, helper *Func, depth int) expression {
	if depth == 0 || r.Intn(4) == 0 {
		switch r.Intn(5) {
		case 0, 1:
			i := r.Intn(3)
			return expression{f.Param(i), func(a, b, c uint32) uint32 { return [3]uint32{a, b, c}[i] }, fmt.Sprintf("p%d", i)}
		case 2:
			v := uint32(r.Intn(40)) - 20
			return expression{K(v), func(a, b, c uint32) uint32 { return v }, fmt.Sprint(int32(v))}
		case 3:
			v := r.Uint32()
			return expression{K(v), func(a, b, c uint32) uint32 { return v }, fmt.Sprintf("%#x", v)}
		default:
			v := uint32(r.Intn(0x100000))
			return expression{K(v), func(a, b, c uint32) uint32 { return v }, fmt.Sprintf("%#x", v)}
		}
	}
	x := randomExpression(r, f, helper, depth-1)
	y := randomExpression(r, f, helper, depth-1)
	b2u := func(v bool) uint32 {
		if v {
			return 1
		}
		return 0
	}
	type op struct {
		name  string
		build func(x, y *Node) *Node
		eval  func(x, y uint32) uint32
	}
	ops := []op{
		{"+", func(x, y *Node) *Node { return x.Add(y) }, func(x, y uint32) uint32 { return x + y }},
		{"-", func(x, y *Node) *Node { return x.Sub(y) }, func(x, y uint32) uint32 { return x - y }},
		{"*", func(x, y *Node) *Node { return x.Mul(y) }, func(x, y uint32) uint32 { return x * y }},
		{"&", func(x, y *Node) *Node { return x.And(y) }, func(x, y uint32) uint32 { return x & y }},
		{"|", func(x, y *Node) *Node { return x.Or(y) }, func(x, y uint32) uint32 { return x | y }},
		{"^", func(x, y *Node) *Node { return x.Xor(y) }, func(x, y uint32) uint32 { return x ^ y }},
		{"<<", func(x, y *Node) *Node { return x.Shl(y) }, func(x, y uint32) uint32 { return x << (y & 31) }},
		{">>", func(x, y *Node) *Node { return x.Shr(y) }, func(x, y uint32) uint32 { return x >> (y & 31) }},
		{">>s", func(x, y *Node) *Node { return x.Sar(y) }, func(x, y uint32) uint32 { return uint32(int32(x) >> (y & 31)) }},
		{"/u", func(x, y *Node) *Node { return x.DivU(y.Or(1)) }, func(x, y uint32) uint32 { return x / (y | 1) }},
		{"%u", func(x, y *Node) *Node { return x.ModU(y.Or(1)) }, func(x, y uint32) uint32 { return x % (y | 1) }},
		{"/s", func(x, y *Node) *Node { return x.Div(y.Or(1)) }, func(x, y uint32) uint32 {
			if int32(x) == -1<<31 && int32(y|1) == -1 {
				return x
			}
			return uint32(int32(x) / int32(y|1))
		}},
		{"%s", func(x, y *Node) *Node { return x.Mod(y.Or(1)) }, func(x, y uint32) uint32 {
			if int32(y|1) == -1 {
				return 0
			}
			return uint32(int32(x) % int32(y|1))
		}},
		{"==", func(x, y *Node) *Node { return x.Eq(y) }, func(x, y uint32) uint32 { return b2u(x == y) }},
		{"!=", func(x, y *Node) *Node { return x.Ne(y) }, func(x, y uint32) uint32 { return b2u(x != y) }},
		{"<u", func(x, y *Node) *Node { return x.LtU(y) }, func(x, y uint32) uint32 { return b2u(x < y) }},
		{"<=u", func(x, y *Node) *Node { return x.LeU(y) }, func(x, y uint32) uint32 { return b2u(x <= y) }},
		{">u", func(x, y *Node) *Node { return x.GtU(y) }, func(x, y uint32) uint32 { return b2u(x > y) }},
		{">=u", func(x, y *Node) *Node { return x.GeU(y) }, func(x, y uint32) uint32 { return b2u(x >= y) }},
		{"<", func(x, y *Node) *Node { return x.Lt(y) }, func(x, y uint32) uint32 { return b2u(int32(x) < int32(y)) }},
		{"<=", func(x, y *Node) *Node { return x.Le(y) }, func(x, y uint32) uint32 { return b2u(int32(x) <= int32(y)) }},
		{">", func(x, y *Node) *Node { return x.Gt(y) }, func(x, y uint32) uint32 { return b2u(int32(x) > int32(y)) }},
		{">=", func(x, y *Node) *Node { return x.Ge(y) }, func(x, y uint32) uint32 { return b2u(int32(x) >= int32(y)) }},
		{"&&", func(x, y *Node) *Node { return x.AndAlso(y) }, func(x, y uint32) uint32 { return b2u(x != 0 && y != 0) }},
		{"||", func(x, y *Node) *Node { return x.OrElse(y) }, func(x, y uint32) uint32 { return b2u(x != 0 || y != 0) }},
		{"call", func(x, y *Node) *Node { return helper.Call(x, y) }, func(x, y uint32) uint32 { return x*3 - y }},
	}
	switch r.Intn(len(ops) + 3) {
	case len(ops):
		return expression{x.node.Not(), func(a, b, c uint32) uint32 { return b2u(x.value(a, b, c) == 0) }, "!(" + x.text + ")"}
	case len(ops) + 1:
		return expression{x.node.Neg(), func(a, b, c uint32) uint32 { return -x.value(a, b, c) }, "-(" + x.text + ")"}
	case len(ops) + 2:
		return expression{x.node.Compl(), func(a, b, c uint32) uint32 { return ^x.value(a, b, c) }, "^(" + x.text + ")"}
	}
	o := ops[r.Intn(len(ops))]
	return expression{o.build(x.node, y.node), func(a, b, c uint32) uint32 { return o.eval(x.value(a, b, c), y.value(a, b, c)) },
		"(" + x.text + " " + o.name + " " + y.text + ")"}
}

// Random expressions compiled and run agree with Go's arithmetic.
func TestRandomExpressions(t *testing.T) {
	r := rand.New(rand.NewSource(1))
	for round := 0; round < 300; round++ {
		p := NewProgram(testOrigin)
		helper := p.Func("helper", 2)
		helper.Define(func(b *Block) {
			b.Return(helper.Param(0).Mul(3).Sub(helper.Param(1)))
		})
		f := p.Func("f", 3)
		e := randomExpression(r, f, helper, 4)
		// Use the expression as a value and as a condition.
		f.Define(func(b *Block) {
			v := b.Tmp("v", e.node)
			b.If(e.node, func(b *Block) { b.Return(v.Add(0x100)) })
			b.Return(v)
		})
		cpu := build(t, p)
		for trial := 0; trial < 20; trial++ {
			args := [3]uint32{r.Uint32(), uint32(r.Intn(64)) - 32, r.Uint32() >> uint(r.Intn(32))}
			want := e.value(args[0], args[1], args[2])
			if want != 0 {
				want += 0x100
			}
			if got := call(t, cpu, p, f, args[:]...); got != want {
				t.Fatalf("%s with %#x: got %#x, want %#x", e.text, args, got, want)
			}
		}
	}
}

// Loops, recursion, memory and frame arrays.
func TestStatements(t *testing.T) {
	p := NewProgram(testOrigin)
	fib := p.Func("fib", 1)
	fib.Define(func(b *Block) {
		n := fib.Param(0)
		b.If(n.LtU(2), func(b *Block) { b.Return(n) })
		b.Return(fib.Call(n.Sub(1)).Add(fib.Call(n.Sub(2))))
	})
	// sum fills data[i] = i*i for i < n into a frame array, copies it to
	// memory at testData and returns the sum, skipping odd squares with
	// Continue and stopping at 1000 with Break.
	sum := p.Func("sum", 1)
	sum.Define(func(b *Block) {
		n := sum.Param(0)
		i, total := sum.RegLocal("i"), sum.Local("total")
		buf := sum.Array("buf", 64*4)
		b.Set(total, 0)
		b.Range(i, 0, n, func(b *Block) {
			b.Store32(buf.Add(i.Shl(2)), i.Mul(i))
		})
		b.Range(i, 0, n, func(b *Block) {
			sq := Mem32(buf.Add(i.Shl(2)))
			b.Set(Mem32(K(testData).Add(i.Shl(2))), sq)
			b.If(sq.And(1), func(b *Block) { b.Continue() })
			b.If(total.GtU(1000), func(b *Block) { b.Break() })
			b.Inc(total, sq)
		})
		b.Store16(K(testData+0x400), 0xbeef)
		b.Store8(K(testData+0x403), 0x7f)
		b.Return(total.Add(Mem16(K(testData + 0x402)).Add(MemS8(K(testData + 0x403)))))
	})
	cpu := build(t, p)
	if got := call(t, cpu, p, fib, 15); got != 610 {
		t.Errorf("fib(15) = %d", got)
	}
	// even squares below the break: 0 4 16 36 64 100 144 196 256 324 = 1140
	// passes 1000 after 324 is added; 0x7f from the halfword's low byte
	// plus 0x7f read as a signed byte.
	if got := call(t, cpu, p, sum, 30); got != 1140+0x7f+0x7f {
		t.Errorf("sum(30) = %d", got)
	}
	if got := cpu.Read(testData+4*7, 4); got != 49 {
		t.Errorf("data[7] = %d", got)
	}
}

// DivModU gives the quotient and the remainder of one division.
func TestDivModU(t *testing.T) {
	p := NewProgram(testOrigin)
	f := p.Func("f", 3) // x, d, which: 0 the quotient, 1 the remainder, 2 both summed
	f.Define(func(b *Block) {
		q, r := b.Tmp("q", 0), b.Tmp("r", 0)
		b.DivModU(q, r, f.Param(0), f.Param(1))
		b.If(f.Param(2).Eq(0), func(b *Block) { b.Return(q) })
		b.If(f.Param(2).Eq(1), func(b *Block) { b.Return(r) })
		b.Return(q.Add(r))
	})
	cpu := build(t, p)
	rnd := rand.New(rand.NewSource(3))
	for i := 0; i < 2000; i++ {
		x, d := rnd.Uint32(), rnd.Uint32()>>uint(rnd.Intn(32))
		if d == 0 {
			d = 1
		}
		for which, want := range []uint32{x / d, x % d, x/d + x%d} {
			if got := call(t, cpu, p, f, x, d, uint32(which)); got != want {
				t.Fatalf("%#x / %#x (%d): got %#x, want %#x", x, d, which, got, want)
			}
		}
	}
}

// TestTailCall has stubs at a fixed stride jump on to a shared function
// that notes which stub was entered and the return address it came with.
func TestTailCall(t *testing.T) {
	const stubs, stride = 16, 8
	common := NewProgram(0x30000)
	f := common.Func("common", 1)
	f.Define(func(b *Block) {
		b.Store32(K(testData), f.Param(0))
		b.Store32(K(testData+4), EntryRP())
		b.Return(f.Param(0).Add(7))
	})
	commonCode, err := common.Assemble()
	if err != nil {
		t.Fatal(err)
	}
	p := NewProgram(testOrigin)
	at := p.At("common", 1, 0x30000)
	var funcs []*Func
	for i := 0; i < stubs; i++ {
		p.Align(stride)
		s := p.Func(fmt.Sprintf("stub%d", i), 0)
		s.Define(func(b *Block) { b.TailCall(at, i*3) })
		funcs = append(funcs, s)
	}
	// Through a function with a frame and kept registers as well.
	g := p.Func("g", 2)
	g.Define(func(b *Block) {
		v := b.Tmp("v", g.Param(0).Mul(g.Param(1)))
		b.Do(at.Call(v))
		b.TailCall(at, v.Add(1))
	})
	cpu := build(t, p)
	cpu.Load(0x30000, commonCode)
	for i, s := range funcs {
		if address := p.Address(s.Label); address != testOrigin+uint32(stride*i) {
			t.Fatalf("stub %d at %#x", i, address)
		}
		if got := call(t, cpu, p, s); got != uint32(i*3+7) {
			t.Fatalf("stub %d returned %d", i, got)
		}
		if cpu.Read(testData, 4) != uint32(i*3) {
			t.Fatalf("stub %d passed %d", i, cpu.Read(testData, 4))
		}
	}
	if got := call(t, cpu, p, g, 6, 7); got != 50 {
		t.Fatalf("g returned %d", got)
	}
}
