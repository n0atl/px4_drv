// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

// Package sim runs FR30 code in Go, so that the programs the driver places on
// the MB86H47A can be tested without the chip. It covers the instruction set
// the fr30 package assembles, except interrupts and coprocessor instructions;
// the chip's hardware is modelled by the test through memory regions.
package sim

import (
	"encoding/binary"
	"fmt"
)

// Condition-code bits of PS.
const (
	flagC = 1 << 0
	flagV = 1 << 1
	flagZ = 1 << 2
	flagN = 1 << 3
	flagI = 1 << 4
)

// Region serves the accesses to an address range, for hardware registers:
// Read returns the value of a size-byte access at address, Write takes one.
type Region struct {
	Start, End uint32 // [Start, End)
	Read       func(address uint32, size int) uint32
	Write      func(address uint32, size int, value uint32)
}

// CPU is an FR30 core with its memory.
type CPU struct {
	R       [16]uint32
	PC      uint32
	PS      uint32
	TBR     uint32
	RP      uint32
	MDH     uint32
	MDL     uint32
	Steps   uint64
	pages   map[uint32]*[1 << 16]byte
	regions []Region
	// private is the range of addresses (in 64 KiB pages) this CPU keeps to
	// itself when it shares the rest of memory with others (ShareMemory).
	private            map[uint32]*[1 << 16]byte
	privFrom, privUpTo uint32
	d0, d1             bool
	stopped            bool
	// StopAt ends Run when the PC reaches it.
	StopAt uint32
}

// New makes a CPU with empty memory.
func New() *CPU { return &CPU{pages: map[uint32]*[1 << 16]byte{}, StopAt: 0xffffffff} }

// AddRegion routes an address range to a region's functions.
func (cpu *CPU) AddRegion(region Region) { cpu.regions = append(cpu.regions, region) }

func (cpu *CPU) region(address uint32) *Region {
	for index := range cpu.regions {
		if address >= cpu.regions[index].Start && address < cpu.regions[index].End {
			return &cpu.regions[index]
		}
	}
	return nil
}

func (cpu *CPU) page(address uint32) *[1 << 16]byte {
	pages := cpu.pages
	if address >= cpu.privFrom && address < cpu.privUpTo {
		pages = cpu.private
	}
	page := pages[address>>16]
	if page == nil {
		page = new([1 << 16]byte)
		pages[address>>16] = page
	}
	return page
}

// ShareMemory makes the CPU see other's memory, except from..upTo (a
// multiple of 64 KiB), which stays its own: two cores on one bus, each with
// its local RAM.
func (cpu *CPU) ShareMemory(other *CPU, from, upTo uint32) {
	cpu.pages = other.pages
	cpu.private = map[uint32]*[1 << 16]byte{}
	cpu.privFrom, cpu.privUpTo = from, upTo
}

// Load places bytes in memory.
func (cpu *CPU) Load(address uint32, data []byte) {
	for index, b := range data {
		cpu.page(address + uint32(index))[(address+uint32(index))&0xffff] = b
	}
}

// Bytes reads memory.
func (cpu *CPU) Bytes(address uint32, n int) []byte {
	out := make([]byte, n)
	for index := range out {
		out[index] = cpu.page(address + uint32(index))[(address+uint32(index))&0xffff]
	}
	return out
}

// Read reads a big-endian value of size 1, 2 or 4 bytes, through the regions.
func (cpu *CPU) Read(address uint32, size int) uint32 {
	address &^= uint32(size - 1)
	if region := cpu.region(address); region != nil && region.Read != nil {
		return region.Read(address, size)
	}
	var value uint32
	for index := 0; index < size; index++ {
		value = value<<8 | uint32(cpu.page(address + uint32(index))[(address+uint32(index))&0xffff])
	}
	return value
}

// Write writes a big-endian value of size 1, 2 or 4 bytes, through the regions.
func (cpu *CPU) Write(address uint32, size int, value uint32) {
	address &^= uint32(size - 1)
	if region := cpu.region(address); region != nil && region.Write != nil {
		region.Write(address, size, value)
		return
	}
	for index := size - 1; index >= 0; index-- {
		cpu.page(address + uint32(index))[(address+uint32(index))&0xffff] = byte(value)
		value >>= 8
	}
}

// Error is a fault in the simulated program.
type Error struct {
	PC      uint32
	Message string
}

func (e *Error) Error() string { return fmt.Sprintf("fr30 sim: at %#x: %s", e.PC, e.Message) }

// Run executes from the current PC until the PC reaches StopAt, a step limit
// passes (0 means no limit) or the program faults.
func (cpu *CPU) Run(limit uint64) error {
	cpu.stopped = false
	start := cpu.Steps
	for cpu.PC != cpu.StopAt {
		if limit != 0 && cpu.Steps-start >= limit {
			return &Error{cpu.PC, fmt.Sprintf("still running after %d steps", limit)}
		}
		if err := cpu.Step(); err != nil {
			return err
		}
	}
	return nil
}

// Interrupt takes interrupt vector n between instructions, if the I flag
// allows: PS and PC go on the stack (one stack: R15), I is cleared until
// RETI restores PS, and the CPU goes on at the vector (TBR + 0x3FC - 4n).
// It reports whether the interrupt was taken.
func (cpu *CPU) Interrupt(n int) bool {
	if cpu.PS&flagI == 0 {
		return false
	}
	cpu.R[15] -= 8
	cpu.Write(cpu.R[15]+4, 4, cpu.PS)
	cpu.Write(cpu.R[15], 4, cpu.PC)
	cpu.PS &^= flagI
	cpu.PC = cpu.Read(cpu.TBR+0x3fc-4*uint32(n), 4)
	return true
}

// Call runs the function at address with up to four arguments in R4-R7, the
// stack at sp, until it returns; it gives R4.
func (cpu *CPU) Call(address, sp uint32, args ...uint32) (uint32, error) {
	const returned = 0xfffffff0
	for index, arg := range args {
		cpu.R[4+index] = arg
	}
	cpu.R[15] = sp
	cpu.RP = returned
	cpu.PC = address
	saved := cpu.StopAt
	cpu.StopAt = returned
	err := cpu.Run(1 << 32)
	cpu.StopAt = saved
	return cpu.R[4], err
}

func (cpu *CPU) fetch(address uint32) uint16 { return uint16(cpu.Read(address, 2)) }

func (cpu *CPU) setNZ(value uint32) {
	cpu.PS &^= flagN | flagZ
	if value&0x80000000 != 0 {
		cpu.PS |= flagN
	}
	if value == 0 {
		cpu.PS |= flagZ
	}
}

func (cpu *CPU) flag(bit uint32) bool { return cpu.PS&bit != 0 }

func (cpu *CPU) setFlag(bit uint32, on bool) {
	if on {
		cpu.PS |= bit
	} else {
		cpu.PS &^= bit
	}
}

func (cpu *CPU) add(a, b, carry uint32) uint32 {
	sum := uint64(a) + uint64(b) + uint64(carry)
	result := uint32(sum)
	cpu.setNZ(result)
	cpu.setFlag(flagC, sum>>32 != 0)
	cpu.setFlag(flagV, (^(a^b)&(a^result))&0x80000000 != 0)
	return result
}

// sub is a - b - borrow with FR flags (C = borrow).
func (cpu *CPU) sub(a, b, borrow uint32) uint32 {
	result := a - b - borrow
	cpu.setNZ(result)
	cpu.setFlag(flagC, uint64(a) < uint64(b)+uint64(borrow))
	cpu.setFlag(flagV, ((a^b)&(a^result))&0x80000000 != 0)
	return result
}

func (cpu *CPU) condition(cond uint16) bool {
	n, z, v, c := cpu.flag(flagN), cpu.flag(flagZ), cpu.flag(flagV), cpu.flag(flagC)
	switch cond {
	case 0x0:
		return true
	case 0x1:
		return false
	case 0x2:
		return z
	case 0x3:
		return !z
	case 0x4:
		return c
	case 0x5:
		return !c
	case 0x6:
		return n
	case 0x7:
		return !n
	case 0x8:
		return v
	case 0x9:
		return !v
	case 0xa:
		return n != v
	case 0xb:
		return n == v
	case 0xc:
		return (n != v) || z
	case 0xd:
		return !((n != v) || z)
	case 0xe:
		return c || z
	default:
		return !(c || z)
	}
}

// Step executes one instruction, with its delay slot if it has one.
func (cpu *CPU) Step() error {
	pc := cpu.PC
	op := cpu.fetch(pc)
	cpu.PC = pc + 2
	cpu.Steps++
	next, jump, err := cpu.execute(pc, op)
	if err != nil {
		return err
	}
	if jump == jumpDelayed {
		slot := cpu.fetch(cpu.PC)
		cpu.PC += 2
		cpu.Steps++
		if _, j, err := cpu.execute(cpu.PC-2, slot); err != nil {
			return err
		} else if j != jumpNone {
			return &Error{pc, "branch in a delay slot"}
		}
		cpu.PC = next
	} else if jump == jumpNow {
		cpu.PC = next
	}
	return nil
}

type jumpKind uint8

const (
	jumpNone jumpKind = iota
	jumpNow
	jumpDelayed
)

func (cpu *CPU) execute(pc uint32, op uint16) (uint32, jumpKind, error) {
	hi := uint8(op >> 8)
	j := (op >> 4) & 15
	i := op & 15
	r := &cpu.R
	bad := func() (uint32, jumpKind, error) {
		return 0, jumpNone, &Error{pc, fmt.Sprintf("unsupported instruction %04x", op)}
	}
	// Formats with a field wider than one byte.
	switch op >> 12 {
	case 0x2: // LD @(R14,disp10),Ri
		r[i] = cpu.Read(r[14]+uint32(int32(int8(op>>4))*4), 4)
		return 0, jumpNone, nil
	case 0x3:
		cpu.Write(r[14]+uint32(int32(int8(op>>4))*4), 4, r[i])
		return 0, jumpNone, nil
	case 0x4:
		r[i] = cpu.Read(r[14]+uint32(int32(int8(op>>4))*2), 2)
		return 0, jumpNone, nil
	case 0x5:
		cpu.Write(r[14]+uint32(int32(int8(op>>4))*2), 2, r[i])
		return 0, jumpNone, nil
	case 0x6:
		r[i] = cpu.Read(r[14]+uint32(int32(int8(op>>4))), 1)
		return 0, jumpNone, nil
	case 0x7:
		cpu.Write(r[14]+uint32(int32(int8(op>>4))), 1, r[i])
		return 0, jumpNone, nil
	case 0xc: // LDI:8
		r[i] = uint32(op>>4) & 0xff
		return 0, jumpNone, nil
	case 0xd: // CALL label12 / CALL:D
		displacement := int32(op&0x7ff) << 21 >> 20
		target := uint32(int32(pc) + 2 + displacement)
		if op&0x800 != 0 {
			cpu.RP = pc + 4
			return target, jumpDelayed, nil
		}
		cpu.RP = pc + 2
		return target, jumpNow, nil
	case 0xe, 0xf: // Bcc / Bcc:D
		target := uint32(int32(pc) + 2 + int32(int8(op))*2)
		taken := cpu.condition((op >> 8) & 15)
		if op>>12 == 0xf {
			if taken {
				return target, jumpDelayed, nil
			}
			return pc + 4, jumpDelayed, nil
		}
		if taken {
			return target, jumpNow, nil
		}
		return 0, jumpNone, nil
	}
	switch hi {
	case 0x00:
		r[i] = cpu.Read(r[13]+r[j], 4)
	case 0x01:
		r[i] = cpu.Read(r[13]+r[j], 2)
	case 0x02:
		r[i] = cpu.Read(r[13]+r[j], 1)
	case 0x03:
		r[i] = cpu.Read(r[15]+uint32(j)*4, 4)
	case 0x04:
		r[i] = cpu.Read(r[j], 4)
	case 0x05:
		r[i] = cpu.Read(r[j], 2)
	case 0x06:
		r[i] = cpu.Read(r[j], 1)
	case 0x07:
		switch j {
		case 0: // LD @R15+,Ri
			value := cpu.Read(r[15], 4)
			r[15] += 4
			r[i] = value
		case 1: // MOV Ri,PS
			cpu.PS = cpu.PS&^0xff | r[i]&0xff
		case 8: // LD @R15+,Rs
			value := cpu.Read(r[15], 4)
			r[15] += 4
			if err := cpu.setS(pc, i, value); err != nil {
				return 0, jumpNone, err
			}
		case 9:
			cpu.PS = cpu.Read(r[15], 4)
			r[15] += 4
		default:
			return bad()
		}
	case 0x08, 0x09, 0x0a, 0x18, 0x19, 0x1a, 0x0b, 0x1b, 0x0c, 0x1c:
		return cpu.dmov(pc, op)
	case 0x0f: // ENTER
		cpu.Write(r[15]-4, 4, r[14])
		r[14] = r[15] - 4
		r[15] -= uint32(op&0xff) * 4
	case 0x10:
		cpu.Write(r[13]+r[j], 4, r[i])
	case 0x11:
		cpu.Write(r[13]+r[j], 2, r[i])
	case 0x12:
		cpu.Write(r[13]+r[j], 1, r[i])
	case 0x13:
		cpu.Write(r[15]+uint32(j)*4, 4, r[i])
	case 0x14:
		cpu.Write(r[j], 4, r[i])
	case 0x15:
		cpu.Write(r[j], 2, r[i])
	case 0x16:
		cpu.Write(r[j], 1, r[i])
	case 0x17:
		switch j {
		case 0: // ST Ri,@-R15
			value := r[i]
			r[15] -= 4
			cpu.Write(r[15], 4, value)
		case 1: // MOV PS,Ri
			r[i] = cpu.PS
		case 8:
			value, err := cpu.getS(pc, i)
			if err != nil {
				return 0, jumpNone, err
			}
			r[15] -= 4
			cpu.Write(r[15], 4, value)
		case 9:
			r[15] -= 4
			cpu.Write(r[15], 4, cpu.PS)
		default:
			return bad()
		}
	case 0x1f: // INT
		return 0, jumpNone, &Error{pc, fmt.Sprintf("INT %#x", op&0xff)}
	case 0x80, 0x81, 0x90, 0x91, 0x98, 0x99, 0x88, 0x89:
		cpu.bitOp(hi, uint8(j), r[i])
	case 0x82:
		r[i] &= r[j]
		cpu.setNZ(r[i])
	case 0x92:
		r[i] |= r[j]
		cpu.setNZ(r[i])
	case 0x9a:
		r[i] ^= r[j]
		cpu.setNZ(r[i])
	case 0x84, 0x94, 0x9c:
		cpu.memLogic(hi, 4, r[j], r[i])
	case 0x85, 0x95, 0x9d:
		cpu.memLogic(hi, 2, r[j], r[i])
	case 0x86, 0x96, 0x9e:
		cpu.memLogic(hi, 1, r[j], r[i])
	case 0x83: // ANDCCR
		cpu.PS &= 0xffffff00 | uint32(op&0xff)
	case 0x93: // ORCCR
		cpu.PS |= uint32(op & 0xff)
	case 0x87: // STILM
		cpu.PS = cpu.PS&^(0x1f<<16) | uint32(op&0x1f)<<16
	case 0x8a: // XCHB @Rj,Ri
		value := cpu.Read(r[j], 1)
		cpu.Write(r[j], 1, r[i])
		r[i] = value
	case 0x8b:
		r[i] = r[j]
	case 0x8c, 0x8d: // LDM0 / LDM1
		base := 0
		if hi == 0x8d {
			base = 8
		}
		for bit := 0; bit < 8; bit++ {
			if op&(1<<bit) != 0 {
				value := cpu.Read(r[15], 4)
				r[15] += 4
				r[base+bit] = value
			}
		}
	case 0x8e, 0x8f: // STM0 / STM1: bit 7-n is Rn, pushed from the highest
		base := 0
		if hi == 0x8f {
			base = 8
		}
		for bit := 0; bit < 8; bit++ {
			if op&(1<<bit) != 0 {
				value := r[base+7-bit]
				r[15] -= 4
				cpu.Write(r[15], 4, value)
			}
		}
	case 0x97:
		return cpu.op97(pc, op)
	case 0x9b: // LDI:20
		r[i] = uint32(j)<<16 | uint32(cpu.fetch(pc+2))
		cpu.PC = pc + 4
	case 0x9f:
		return cpu.op9f(pc, op)
	case 0xa0:
		r[i] += uint32(j)
	case 0xa1:
		r[i] += uint32(int32(j) - 16)
	case 0xa2:
		r[i] += r[j]
	case 0xa3: // ADDSP
		r[15] += uint32(int32(int8(op)) * 4)
	case 0xa4:
		r[i] = cpu.add(r[i], uint32(j), 0)
	case 0xa5:
		r[i] = cpu.add(r[i], uint32(int32(j)-16), 0)
	case 0xa6:
		r[i] = cpu.add(r[i], r[j], 0)
	case 0xa7:
		carry := uint32(0)
		if cpu.flag(flagC) {
			carry = 1
		}
		r[i] = cpu.add(r[i], r[j], carry)
	case 0xa8:
		cpu.sub(r[i], uint32(j), 0)
	case 0xa9:
		cpu.sub(r[i], uint32(int32(j)-16), 0)
	case 0xaa:
		cpu.sub(r[i], r[j], 0)
	case 0xab: // MULU
		product := uint64(r[j]) * uint64(r[i])
		cpu.MDH, cpu.MDL = uint32(product>>32), uint32(product)
		cpu.setNZ(cpu.MDL)
		cpu.setFlag(flagV, cpu.MDH != 0)
	case 0xac:
		r[i] = cpu.sub(r[i], r[j], 0)
	case 0xad:
		borrow := uint32(0)
		if cpu.flag(flagC) {
			borrow = 1
		}
		r[i] = cpu.sub(r[i], r[j], borrow)
	case 0xae:
		r[i] -= r[j]
	case 0xaf: // MUL
		product := int64(int32(r[j])) * int64(int32(r[i]))
		cpu.MDH, cpu.MDL = uint32(uint64(product)>>32), uint32(product)
		cpu.setNZ(cpu.MDL)
		cpu.setFlag(flagV, product != int64(int32(product)))
	case 0xb0, 0xb1, 0xb2, 0xb4, 0xb5, 0xb6, 0xb8, 0xb9, 0xba:
		cpu.shift(hi, uint32(j), &r[i])
	case 0xb3: // MOV Ri,Rs
		if err := cpu.setS(pc, j, r[i]); err != nil {
			return 0, jumpNone, err
		}
	case 0xb7: // MOV Rs,Ri
		value, err := cpu.getS(pc, j)
		if err != nil {
			return 0, jumpNone, err
		}
		r[i] = value
	case 0xbb: // MULUH
		cpu.MDL = (r[j] & 0xffff) * (r[i] & 0xffff)
		cpu.setNZ(cpu.MDL)
	case 0xbf: // MULH
		cpu.MDL = uint32(int32(int16(r[j])) * int32(int16(r[i])))
		cpu.setNZ(cpu.MDL)
	default:
		return bad()
	}
	return 0, jumpNone, nil
}

func (cpu *CPU) getS(pc uint32, s uint16) (uint32, error) {
	switch s {
	case 0:
		return cpu.TBR, nil
	case 1:
		return cpu.RP, nil
	case 2, 3:
		return cpu.R[15], nil
	case 4:
		return cpu.MDH, nil
	case 5:
		return cpu.MDL, nil
	}
	return 0, &Error{pc, fmt.Sprintf("dedicated register %d", s)}
}

func (cpu *CPU) setS(pc uint32, s uint16, value uint32) error {
	switch s {
	case 0:
		cpu.TBR = value
	case 1:
		cpu.RP = value
	case 2, 3:
		cpu.R[15] = value
	case 4:
		cpu.MDH = value
	case 5:
		cpu.MDL = value
	default:
		return &Error{pc, fmt.Sprintf("dedicated register %d", s)}
	}
	return nil
}

func (cpu *CPU) bitOp(hi uint8, mask uint8, address uint32) {
	value := cpu.Read(address, 1)
	var m uint32
	if hi&1 == 0 {
		m = uint32(mask) // low nibble
	} else {
		m = uint32(mask) << 4
	}
	switch hi {
	case 0x80: // BANDL: the high nibble stays
		value &= 0xf0 | m
	case 0x81: // BANDH: the low nibble stays
		value &= 0x0f | m
	case 0x90, 0x91:
		value |= m
	case 0x98, 0x99:
		value ^= m
	case 0x88, 0x89:
		test := value & m
		cpu.setFlag(flagZ, test == 0)
		cpu.setFlag(flagN, hi == 0x89 && test&0x80 != 0)
		return
	}
	cpu.Write(address, 1, value&0xff)
}

func (cpu *CPU) memLogic(hi uint8, size int, j, address uint32) {
	value := cpu.Read(address, size)
	switch hi & 0xf8 {
	case 0x80:
		value &= j
	case 0x90:
		value |= j
	default:
		value ^= j
	}
	shift := uint(32 - size*8)
	value = value << shift >> shift
	cpu.PS &^= flagN | flagZ
	if value>>(uint(size*8)-1)&1 != 0 {
		cpu.PS |= flagN
	}
	if value == 0 {
		cpu.PS |= flagZ
	}
	cpu.Write(address, size, value)
}

func (cpu *CPU) shift(hi uint8, amount uint32, reg *uint32) {
	switch hi {
	case 0xb1, 0xb5, 0xb9:
		amount += 16
	case 0xb2, 0xb6, 0xba:
		amount = cpu.R[amount] & 31
	}
	value := *reg
	carry := false
	if amount != 0 {
		switch hi {
		case 0xb4, 0xb5, 0xb6:
			carry = value>>(32-amount)&1 != 0
			value <<= amount
		case 0xb0, 0xb1, 0xb2:
			carry = value>>(amount-1)&1 != 0
			value >>= amount
		default:
			carry = uint32(int32(value)>>(amount-1))&1 != 0
			value = uint32(int32(value) >> amount)
		}
	}
	*reg = value
	cpu.setNZ(value)
	cpu.setFlag(flagC, carry)
}

func (cpu *CPU) op97(pc uint32, op uint16) (uint32, jumpKind, error) {
	i := op & 15
	r := &cpu.R
	switch (op >> 4) & 15 {
	case 0: // JMP @Ri
		return r[i], jumpNow, nil
	case 1: // CALL @Ri
		cpu.RP = pc + 2
		return r[i], jumpNow, nil
	case 2: // RET
		if op != 0x9720 {
			break
		}
		return cpu.RP, jumpNow, nil
	case 3: // RETI
		if op != 0x9730 {
			break
		}
		next := cpu.Read(r[15], 4)
		cpu.PS = cpu.Read(r[15]+4, 4)
		r[15] += 8
		return next, jumpNow, nil
	case 4: // DIV0S
		cpu.d0 = cpu.MDL&0x80000000 != 0
		cpu.d1 = cpu.d0 != (r[i]&0x80000000 != 0)
		if cpu.d0 {
			cpu.MDH = 0xffffffff
		} else {
			cpu.MDH = 0
		}
		return 0, jumpNone, nil
	case 5: // DIV0U
		cpu.d0, cpu.d1 = false, false
		cpu.MDH = 0
		return 0, jumpNone, nil
	case 6: // DIV1: one step, adding the divisor when the signs differ
		cpu.MDH = cpu.MDH<<1 | cpu.MDL>>31
		cpu.MDL <<= 1
		var tmp uint32
		var carry bool
		if cpu.d1 {
			sum := uint64(cpu.MDH) + uint64(r[i])
			tmp, carry = uint32(sum), sum>>32 != 0
		} else {
			tmp, carry = cpu.MDH-r[i], cpu.MDH < r[i]
		}
		if cpu.d0 != cpu.d1 != carry == false {
			cpu.MDH = tmp
			cpu.MDL |= 1
		}
		cpu.setFlag(flagC, carry)
		cpu.setFlag(flagZ, tmp == 0)
		return 0, jumpNone, nil
	case 7: // DIV2: a remainder of exactly the divisor becomes zero
		var tmp uint32
		if cpu.d1 {
			tmp = cpu.MDH + r[i]
		} else {
			tmp = cpu.MDH - r[i]
		}
		if tmp == 0 {
			cpu.setFlag(flagZ, true)
			cpu.MDH = 0
		} else {
			cpu.setFlag(flagZ, false)
		}
		return 0, jumpNone, nil
	case 8:
		r[i] = uint32(int32(int8(r[i])))
		return 0, jumpNone, nil
	case 9:
		r[i] &= 0xff
		return 0, jumpNone, nil
	case 10:
		r[i] = uint32(int32(int16(r[i])))
		return 0, jumpNone, nil
	case 11:
		r[i] &= 0xffff
		return 0, jumpNone, nil
	}
	return 0, jumpNone, &Error{pc, fmt.Sprintf("unsupported instruction %04x", op)}
}

func (cpu *CPU) op9f(pc uint32, op uint16) (uint32, jumpKind, error) {
	i := op & 15
	r := &cpu.R
	switch (op >> 4) & 15 {
	case 0: // JMP:D
		return r[i], jumpDelayed, nil
	case 1: // CALL:D
		cpu.RP = pc + 4
		return r[i], jumpDelayed, nil
	case 2:
		if op == 0x9f20 { // RET:D
			return cpu.RP, jumpDelayed, nil
		}
	case 8: // LDI:32
		r[i] = uint32(cpu.fetch(pc+2))<<16 | uint32(cpu.fetch(pc+4))
		cpu.PC = pc + 6
		return 0, jumpNone, nil
	case 6:
		if op == 0x9f60 { // DIV3
			if cpu.flag(flagZ) {
				cpu.MDL++
			}
			return 0, jumpNone, nil
		}
	case 7:
		if op == 0x9f70 { // DIV4S
			if cpu.d1 {
				cpu.MDL = -cpu.MDL
			}
			return 0, jumpNone, nil
		}
	case 9:
		if op == 0x9f90 { // LEAVE
			r[15] = r[14] + 4
			r[14] = cpu.Read(r[15]-4, 4)
			return 0, jumpNone, nil
		}
	case 10:
		if op == 0x9fa0 { // NOP
			return 0, jumpNone, nil
		}
	}
	return 0, jumpNone, &Error{pc, fmt.Sprintf("unsupported instruction %04x", op)}
}

func (cpu *CPU) dmov(pc uint32, op uint16) (uint32, jumpKind, error) {
	hi := uint8(op >> 8)
	d := uint32(op & 0xff)
	r := &cpu.R
	switch hi {
	case 0x08:
		r[13] = cpu.Read(d*4, 4)
	case 0x18:
		cpu.Write(d*4, 4, r[13])
	case 0x0c:
		cpu.Write(r[13], 4, cpu.Read(d*4, 4))
		r[13] += 4
	case 0x1c:
		cpu.Write(d*4, 4, cpu.Read(r[13], 4))
		r[13] += 4
	case 0x0b:
		value := cpu.Read(d*4, 4)
		r[15] -= 4
		cpu.Write(r[15], 4, value)
	case 0x1b:
		cpu.Write(d*4, 4, cpu.Read(r[15], 4))
		r[15] += 4
	case 0x09:
		r[13] = cpu.Read(d*2, 2)
	case 0x19:
		cpu.Write(d*2, 2, r[13])
	case 0x0a:
		r[13] = cpu.Read(d, 1)
	case 0x1a:
		cpu.Write(d, 1, r[13])
	default:
		return 0, jumpNone, &Error{pc, fmt.Sprintf("unsupported instruction %04x", op)}
	}
	return 0, jumpNone, nil
}

// LoadWords places big-endian words in memory.
func (cpu *CPU) LoadWords(address uint32, words []uint32) {
	data := make([]byte, 0, len(words)*4)
	for _, word := range words {
		data = binary.BigEndian.AppendUint32(data, word)
	}
	cpu.Load(address, data)
}

// Clone makes a CPU of its own with a copy of this one's registers and
// memory (its private range and what it shares alike), without regions.
func (cpu *CPU) Clone() *CPU {
	c := New()
	c.R, c.PC, c.PS, c.TBR, c.RP, c.MDH, c.MDL = cpu.R, cpu.PC, cpu.PS, cpu.TBR, cpu.RP, cpu.MDH, cpu.MDL
	for k, p := range cpu.pages {
		q := *p
		c.pages[k] = &q
	}
	for k, p := range cpu.private {
		q := *p
		c.pages[k] = &q
	}
	return c
}

// Pages gives the numbers of the 64 KiB pages that hold memory.
func (cpu *CPU) Pages() []uint32 {
	var out []uint32
	for k := range cpu.pages {
		out = append(out, k)
	}
	for k := range cpu.private {
		out = append(out, k)
	}
	return out
}
