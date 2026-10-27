// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fc

import (
	"fmt"

	"github.com/Till0196/coax/pkg/fr30"
)

type nodeKind uint8

const (
	nConst     nodeKind = iota
	nLabel              // address of a label
	nVar                // local variable
	nFrameAddr          // address of a frame array
	nLoad               // memory read
	nBin                // binary operator
	nNot                // logical not
	nNeg                // arithmetic negation
	nCompl              // bitwise complement
	nCall               // function call
	nMDH                // the multiply/divide high register
	nEntrySP            // the stack pointer as the function was entered
	nEntryRP            // the return address as the function was entered
)

type binOp uint8

const (
	opAdd binOp = iota
	opSub
	opMul
	opDivU
	opDivUShort // opDivU with a quotient known to fit in value bits
	opModU
	opDiv
	opMod
	opAnd
	opOr
	opXor
	opShl
	opShr
	opSar
	opEq
	opNe
	opLtU
	opLeU
	opGtU
	opGeU
	opLt
	opLe
	opGt
	opGe
	opLAnd // logical and, short-circuit
	opLOr
)

func (op binOp) isCompare() bool { return op >= opEq && op <= opGe }

// Node is an expression, or a place (variable or memory) when it can be
// assigned.
type Node struct {
	kind   nodeKind
	value  uint32
	label  *fr30.Label
	f      *Func
	slot   int
	pinned bool
	reg    fr30.Reg
	name   string
	size   int  // nLoad: 1, 2 or 4
	signed bool // nLoad: sign-extend
	op     binOp
	l, r   *Node
	callee *Func
	args   []*Node
}

// K is a constant.
func K(value uint32) *Node { return &Node{kind: nConst, value: value} }

// KI is a signed constant.
func KI(value int32) *Node { return K(uint32(value)) }

// Addr is the address of a label (a function or a Table).
func Addr(label *fr30.Label) *Node { return &Node{kind: nLabel, label: label} }

// MDHigh reads the multiply/divide high register (the remainder after a
// division).
func MDHigh() *Node { return &Node{kind: nMDH} }

// EntrySP is the stack pointer as the function was entered, and EntryRP its
// return address; they are for a function that calls nothing and has no
// locals, such as one entered from an exception, where the stack holds what
// the CPU pushed.
func EntrySP() *Node { return &Node{kind: nEntrySP} }

// EntryRP is the return address the function was entered with.
func EntryRP() *Node { return &Node{kind: nEntryRP} }

func node(x any) *Node {
	switch v := x.(type) {
	case *Node:
		return v
	case int:
		return K(uint32(int32(v)))
	case int32:
		return K(uint32(v))
	case uint32:
		return K(v)
	case uint16:
		return K(uint32(v))
	case uint8:
		return K(uint32(v))
	case *fr30.Label:
		return Addr(v)
	case bool:
		if v {
			return K(1)
		}
		return K(0)
	}
	panic(fmt.Sprintf("fc: %T is not an expression", x))
}

func load(size int, signed bool, address any) *Node {
	return &Node{kind: nLoad, size: size, signed: signed, l: node(address)}
}

// Mem32 is the word at address, readable and assignable.
func Mem32(address any) *Node { return load(4, false, address) }

// Mem16 is the unsigned halfword at address.
func Mem16(address any) *Node { return load(2, false, address) }

// Mem8 is the unsigned byte at address.
func Mem8(address any) *Node { return load(1, false, address) }

// MemS16 is the halfword at address, sign-extended when read.
func MemS16(address any) *Node { return load(2, true, address) }

// MemS8 is the byte at address, sign-extended when read.
func MemS8(address any) *Node { return load(1, true, address) }

func (n *Node) bin(op binOp, x any) *Node { return &Node{kind: nBin, op: op, l: n, r: node(x)} }

// Add is n + x.
func (n *Node) Add(x any) *Node { return n.bin(opAdd, x) }

// Sub is n - x.
func (n *Node) Sub(x any) *Node { return n.bin(opSub, x) }

// Mul is the low 32 bits of n * x.
func (n *Node) Mul(x any) *Node {
	// A power of two is a shift: MUL takes five cycles.
	if c, ok := constant(node(x)); ok && c != 0 && c&(c-1) == 0 {
		shift := 0
		for c > 1 {
			c >>= 1
			shift++
		}
		if shift == 0 {
			return n
		}
		return n.Shl(shift)
	}
	return n.bin(opMul, x)
}

// DivU is n / x unsigned.
func (n *Node) DivU(x any) *Node { return n.bin(opDivU, x) }

// ModU is n % x unsigned.
func (n *Node) ModU(x any) *Node { return n.bin(opModU, x) }

// DivUShort is DivU for a quotient known to be below 1<<bits (1..31): the
// division takes that many steps instead of 32. A larger quotient comes
// out wrong.
func (n *Node) DivUShort(x any, bits int) *Node {
	d := n.bin(opDivUShort, x)
	d.value = uint32(bits)
	return d
}

// Div is n / x signed, truncated toward zero.
func (n *Node) Div(x any) *Node { return n.bin(opDiv, x) }

// Mod is n % x signed, with the sign of n.
func (n *Node) Mod(x any) *Node { return n.bin(opMod, x) }

// And is n & x.
func (n *Node) And(x any) *Node { return n.bin(opAnd, x) }

// Or is n | x.
func (n *Node) Or(x any) *Node { return n.bin(opOr, x) }

// Xor is n ^ x.
func (n *Node) Xor(x any) *Node { return n.bin(opXor, x) }

// Shl is n << x (x taken mod 32).
func (n *Node) Shl(x any) *Node { return n.bin(opShl, x) }

// Shr is n >> x, logical.
func (n *Node) Shr(x any) *Node { return n.bin(opShr, x) }

// Sar is n >> x, arithmetic.
func (n *Node) Sar(x any) *Node { return n.bin(opSar, x) }

// Eq is n == x (1 or 0).
func (n *Node) Eq(x any) *Node { return n.bin(opEq, x) }

// Ne is n != x.
func (n *Node) Ne(x any) *Node { return n.bin(opNe, x) }

// LtU is n < x unsigned.
func (n *Node) LtU(x any) *Node { return n.bin(opLtU, x) }

// LeU is n <= x unsigned.
func (n *Node) LeU(x any) *Node { return n.bin(opLeU, x) }

// GtU is n > x unsigned.
func (n *Node) GtU(x any) *Node { return n.bin(opGtU, x) }

// GeU is n >= x unsigned.
func (n *Node) GeU(x any) *Node { return n.bin(opGeU, x) }

// Lt is n < x signed.
func (n *Node) Lt(x any) *Node { return n.bin(opLt, x) }

// Le is n <= x signed.
func (n *Node) Le(x any) *Node { return n.bin(opLe, x) }

// Gt is n > x signed.
func (n *Node) Gt(x any) *Node { return n.bin(opGt, x) }

// Ge is n >= x signed.
func (n *Node) Ge(x any) *Node { return n.bin(opGe, x) }

// AndAlso is n && x: x is evaluated only when n holds.
func (n *Node) AndAlso(x any) *Node { return n.bin(opLAnd, x) }

// OrElse is n || x: x is evaluated only when n does not hold.
func (n *Node) OrElse(x any) *Node { return n.bin(opLOr, x) }

// Not is !n (1 when n is 0).
func (n *Node) Not() *Node { return &Node{kind: nNot, l: n} }

// Neg is -n.
func (n *Node) Neg() *Node { return &Node{kind: nNeg, l: n} }

// Compl is ^n.
func (n *Node) Compl() *Node { return &Node{kind: nCompl, l: n} }

// Field is (n >> shift) & (1<<width - 1).
func (n *Node) Field(shift, width int) *Node {
	if shift+width == 32 {
		return n.Shr(shift)
	}
	return n.Shr(shift).And(uint32(1)<<width - 1)
}

// Mem32 is the word at n + offset.
func (n *Node) Mem32(offset int) *Node { return Mem32(n.Add(offset)) }

// Mem16 is the unsigned halfword at n + offset.
func (n *Node) Mem16(offset int) *Node { return Mem16(n.Add(offset)) }

// Mem8 is the unsigned byte at n + offset.
func (n *Node) Mem8(offset int) *Node { return Mem8(n.Add(offset)) }

// Call calls the function with args.
func (f *Func) Call(args ...any) *Node {
	if len(args) != f.nparams {
		f.p.fail("%s called with %d arguments, takes %d", f.Name, len(args), f.nparams)
	}
	n := &Node{kind: nCall, callee: f}
	for _, a := range args {
		n.args = append(n.args, node(a))
	}
	return n
}

// Min is the signed minimum of a and b.
func Min(b *Block, a, x any) *Node { return b.sel(node(a).Lt(x), a, x) }

// Max is the signed maximum of a and b.
func Max(b *Block, a, x any) *Node { return b.sel(node(a).Gt(x), a, x) }
