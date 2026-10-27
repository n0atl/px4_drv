// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package mb86h47

import (
	"fmt"

	. "github.com/Till0196/coax/internal/coaxd/usb/bridge/mb86h47/fr30/fc"
)

// The trap record: what trapRecorder notes at TrapRecord.
const (
	trapMagic       = 0x7ea9
	trapMagicAt     = 0x00
	trapVectorAt    = 0x04
	trapPCAt        = 0x08 // where the CPU was interrupted
	trapPSAt        = 0x0c
	trapRPAt        = 0x10
	trapStackAt     = 0x14 // R15, the stack the CPU pushed PC and PS on
	trapCountAt     = 0x18
	trapStubStride  = 8
	trapStubVectors = 256
)

// buildTrapRecorder is the recorder at trapCommon. A stub enters it with the
// vector as its argument and the stack as the CPU left it, PC then PS; it
// notes them, RP and R15 and then waits with interrupts on.
func buildTrapRecorder() ([]uint32, error) {
	p := NewProgram(trapCommon)
	record := p.Func("record", 1)
	record.Define(func(b *Block) {
		at := func(offset uint32) *Node { return K(TrapRecord + offset) }
		b.Store32(at(trapMagicAt), trapMagic)
		b.Store32(at(trapVectorAt), record.Param(0))
		b.Store32(at(trapPCAt), Mem32(EntrySP()))
		b.Store32(at(trapPSAt), Mem32(EntrySP().Add(4)))
		b.Store32(at(trapRPAt), EntryRP())
		b.Store32(at(trapStackAt), EntrySP())
		b.Inc(Mem32(at(trapCountAt)), 1)
		b.EnableInterrupts()
		b.Loop(func(b *Block) {})
	})
	return p.Words()
}

// buildTrapStubs is one stub per vector, trapStubStride bytes apart from
// trapStubs, each going on to the recorder with its vector.
func buildTrapStubs() ([]uint32, error) {
	p := NewProgram(trapStubs)
	record := p.At("record", 1, trapCommon)
	for vector := 0; vector < trapStubVectors; vector++ {
		p.Align(trapStubStride)
		stub := p.Func(fmt.Sprintf("stub%d", vector), 0)
		stub.Define(func(b *Block) { b.TailCall(record, vector) })
	}
	words, err := p.Words()
	if err == nil && len(words) != trapStubVectors*trapStubStride/4 {
		err = fmt.Errorf("trap stubs take %d words", len(words))
	}
	return words, err
}
