// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package mb86h47

import (
	. "github.com/Till0196/coax/internal/coaxd/usb/bridge/mb86h47/fr30/fc"
)

// core0Stack is where core 0's stack starts.
const core0Stack = 0xa37fc

// buildCore0 is core 0's program: it moves a ring in SDRAM to endpoint 0x81,
// 192 bytes at a time, through its DMA channel 2, its local FIFO and its
// packetizer. Addresses below 0x20000 are the core's own registers.
func buildCore0(ring core0Ring) func() ([]uint32, error) {
	return func() ([]uint32, error) {
		p := NewProgram(core0Origin)
		main := p.Func("main", 0)
		p.Entry(core0Stack, main)

		reg := func(address uint32) *Node { return Mem32(K(address)) }
		counter := func(offset uint32) *Node { return Mem32(K(core0Counters + offset)) }
		count := func(b *Block, offset uint32) { b.Set(counter(offset), counter(offset).Add(1)) }
		// refill has DMA channel 2 bring the next 0x460 bytes into the
		// FIFO.
		refill := func(b *Block) {
			b.Store32(K(0x1078), 0x460)
			b.Store32(K(0x1068), 0x01000000)
		}

		main.Define(func(b *Block) {
			// Release the core's blocks and stop its DMA channels.
			for _, w := range [][2]uint32{
				{0x0, 0x77}, {0x0, 0}, {0x4, 0xffffffff},
				{0x30, 0x004041ff}, {0x50, 0x004041ff},
				{0x90, 0xffffffff}, {0xb8, 0xffffffff}, {0xc8, 0xffffffff},
				{0x10, 0xffffffff}, {0x20, 0xffffffff}, {0x40, 0},
				{0x1060, 0x300}, {0x1064, 0x300}, {0x1068, 0x300},
				{0x106c, 0x300}, {0x10b0, 0x300}, {0x10b4, 0x300},
			} {
				b.Store32(K(w[0]), w[1])
			}
			b.While(reg(0x1068).And(0x300).Ne(0), func(b *Block) {})

			// DMA channel 2 reads the ring from the engine's current
			// write offset.
			b.Store32(K(0x1018), ring.base)
			b.Store32(K(0x1028), ring.size)
			at := b.Tmp("at", reg(ring.writeOffset))
			b.Store32(K(0x1048), at)
			b.Store32(K(0x1038), at)
			b.Store32(K(0x1000), uint32(0xfff00000))
			// The local FIFO layout.
			for _, w := range [][2]uint32{
				{0x10010, 0}, {0x10030, 0}, {0x10034, 0x800}, {0x10038, 0x1000},
				{0x1003c, 0x1840}, {0x10040, 0x1840}, {0x10080, 0xb8}, {0x10084, 0xb8},
				{0x10088, 0xc0}, {0x1008c, 0xb8}, {0x100b0, 0x1880}, {0x100b4, 0x1900},
			} {
				b.Store16(K(w[0]), w[1])
			}
			// The packetizer, and the output port for 192-byte units.
			for _, w := range [][2]uint32{
				{0x2020, 0}, {0x2024, 0}, {0x202c, 0x03000400}, {0x2030, 0x05010101},
				{0x3008, 0},
				{0x5000, 0x411080c0}, {0x5008, 0x10000000}, {0x5004, 0x01000000},
				{0x200c, 0x01000000},
			} {
				b.Store32(K(w[0]), w[1])
			}

			for _, o := range []uint32{0, 4, 8} {
				b.Set(counter(o), 0)
			}
			refill(b)
			refilling := b.Tmp("refilling", 1) // a refill runs
			units := b.Tmp("units", 0)         // units since the engine was last told how far the ring is read
			b.Loop(func(b *Block) {
				count(b, 0)
				event := b.Tmp("event", reg(0x20))
				b.Set(counter(0xc), event)
				// Event bit 29: the refill is done; acknowledge it.
				b.If(event.And(0x20000000), func(b *Block) {
					b.Store32(K(0x20), 0x20000000)
					b.Set(refilling, 0)
				})

				// Channel 2 reads up to what the engine has written.
				b.Store32(K(0x1038), reg(ring.writeOffset))

				// Refill when more than 0x470 bytes of the 0x840-byte
				// FIFO are free.
				b.If(refilling.Eq(0), func(b *Block) {
					free := b.Tmp("free", Mem16(K(0x10068)).Sub(Mem16(K(0x10058))))
					b.If(free.Eq(0), func(b *Block) {
						b.Set(free, 0x840)
					}).ElseIf(free.Lt(0), func(b *Block) {
						b.Inc(free, 0x840)
					})
					b.If(free.GtU(0x470), func(b *Block) {
						refill(b)
						b.Set(refilling, 1)
						count(b, 8)
					})
				})

				// Event bit 5 with the packetizer idle: send the unit
				// ready in the FIFO as it is.
				b.If(event.And(0x20).Eq(0).OrElse(reg(0x2000).And(7).Ne(0)), func(b *Block) { b.Continue() })
				b.Store32(K(0x20), 0x8)
				b.Store32(K(0x200c), 0x01000000)
				b.Store32(K(0x2004), 0xbf)
				b.Store32(K(0x2000), 0x0d000041)
				count(b, 4)

				// Every ten units, hand channel 2's read offset less the
				// margin back to the engine as the writer's limit.
				b.Inc(units, 1)
				b.If(units.Ne(10), func(b *Block) { b.Continue() })
				b.Set(units, 0)
				read := b.Tmp("read", reg(0x1048))
				b.If(read.LtU(ring.margin), func(b *Block) {
					b.Inc(read, ring.size-ring.margin)
				}).Else(func(b *Block) {
					b.Inc(read, KI(-int32(ring.margin)))
				})
				b.Store32(K(ring.readLimit), read)
			})
		})
		return p.Words()
	}
}
