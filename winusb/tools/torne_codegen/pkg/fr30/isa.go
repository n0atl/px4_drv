// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fr30

// The FR30 instruction set. Methods take their operands in the order the
// assembler writes them: LD(j, i) is "LD @Rj,Ri", ST(i, j) is "ST Ri,@Rj".

// SReg is a dedicated register reached by MOV, PUSHS and POPS.
type SReg uint8

// Dedicated registers, by their number in the encodings.
const (
	TBR SReg = 0 // table base
	RP  SReg = 1 // return pointer
	SSP SReg = 2 // system stack pointer
	USP SReg = 3 // user stack pointer
	MDH SReg = 4 // multiply/divide high
	MDL SReg = 5 // multiply/divide low
)

// Immediates.

// LDI8 loads an 8-bit unsigned immediate: LDI:8 #value,Ri.
func (a *Assembler) LDI8(value uint32, i Reg) {
	if value > 0xff {
		a.fail("LDI:8 immediate %#x too wide", value)
	}
	a.half(0xc000 | uint16(value&0xff)<<4 | uint16(i&15))
}

// LDI20 loads a 20-bit unsigned immediate: LDI:20 #value,Ri.
func (a *Assembler) LDI20(value uint32, i Reg) {
	if value > 0xfffff {
		a.fail("LDI:20 immediate %#x too wide", value)
	}
	a.half(0x9b00 | uint16(value>>16&15)<<4 | uint16(i&15))
	a.half(uint16(value))
}

// LDI32 loads a 32-bit immediate: LDI:32 #value,Ri.
func (a *Assembler) LDI32(value uint32, i Reg) {
	a.half(0x9f80 | uint16(i&15))
	a.half(uint16(value >> 16))
	a.half(uint16(value))
}

// LDI loads any 32-bit value with the shortest of LDI:8, LDI:20 and LDI:32.
func (a *Assembler) LDI(value uint32, i Reg) {
	switch {
	case value <= 0xff:
		a.LDI8(value, i)
	case value <= 0xfffff:
		a.LDI20(value, i)
	default:
		a.LDI32(value, i)
	}
}

// LDIAddr loads the address of a label (LDI:32), resolved at the end.
func (a *Assembler) LDIAddr(label *Label, i Reg) {
	a.absolute([]byte{0x9f, 0x80 | byte(i&15), 0, 0, 0, 0}, 2, label)
}

// Data.

// Word emits a 32-bit data word.
func (a *Assembler) Word(value uint32) {
	a.bytes(byte(value>>24), byte(value>>16), byte(value>>8), byte(value))
	a.markData()
}

// Half emits a 16-bit data halfword.
func (a *Assembler) Half(value uint16) {
	a.half(value)
	a.markData()
}

// Byte emits data bytes.
func (a *Assembler) Byte(values ...byte) {
	a.bytes(values...)
	a.markData()
}

// markData notes that the last piece holds data.
func (a *Assembler) markData() {
	if len(a.pieces) > 0 {
		a.pieces[len(a.pieces)-1].data = true
	}
}

// WordAddr emits the address of a label as a data word.
func (a *Assembler) WordAddr(label *Label) { a.absolute(make([]byte, 4), 0, label) }

// Loads and stores through registers.

// LD loads a word: LD @Rj,Ri.
func (a *Assembler) LD(j, i Reg) { a.op(0x04, j, i) }

// LDUH loads an unsigned halfword: LDUH @Rj,Ri.
func (a *Assembler) LDUH(j, i Reg) { a.op(0x05, j, i) }

// LDUB loads an unsigned byte: LDUB @Rj,Ri.
func (a *Assembler) LDUB(j, i Reg) { a.op(0x06, j, i) }

// LDR13 loads a word: LD @(R13,Rj),Ri.
func (a *Assembler) LDR13(j, i Reg) { a.op(0x00, j, i) }

// LDUHR13 loads an unsigned halfword: LDUH @(R13,Rj),Ri.
func (a *Assembler) LDUHR13(j, i Reg) { a.op(0x01, j, i) }

// LDUBR13 loads an unsigned byte: LDUB @(R13,Rj),Ri.
func (a *Assembler) LDUBR13(j, i Reg) { a.op(0x02, j, i) }

// ST stores a word: ST Ri,@Rj.
func (a *Assembler) ST(i, j Reg) { a.op(0x14, j, i) }

// STH stores a halfword: STH Ri,@Rj.
func (a *Assembler) STH(i, j Reg) { a.op(0x15, j, i) }

// STB stores a byte: STB Ri,@Rj.
func (a *Assembler) STB(i, j Reg) { a.op(0x16, j, i) }

// STR13 stores a word: ST Ri,@(R13,Rj).
func (a *Assembler) STR13(i, j Reg) { a.op(0x10, j, i) }

// STHR13 stores a halfword: STH Ri,@(R13,Rj).
func (a *Assembler) STHR13(i, j Reg) { a.op(0x11, j, i) }

// STBR13 stores a byte: STB Ri,@(R13,Rj).
func (a *Assembler) STBR13(i, j Reg) { a.op(0x12, j, i) }

// disp8 is the 8-bit field of the @(R14,disp) forms: disp in units of scale.
func (a *Assembler) disp8(opcode uint16, disp, scale int, i Reg) {
	if disp%scale != 0 || disp < -128*scale || disp > 127*scale {
		a.fail("R14 displacement %d out of range", disp)
	}
	a.half(opcode | uint16(uint8(int8(disp/scale)))<<4 | uint16(i&15))
}

// LDR14 loads a word: LD @(R14,disp),Ri, disp a multiple of 4 in -512..508.
func (a *Assembler) LDR14(disp int, i Reg) { a.disp8(0x2000, disp, 4, i) }

// LDUHR14 loads a halfword: LDUH @(R14,disp),Ri, disp even in -256..254.
func (a *Assembler) LDUHR14(disp int, i Reg) { a.disp8(0x4000, disp, 2, i) }

// LDUBR14 loads a byte: LDUB @(R14,disp),Ri, disp in -128..127.
func (a *Assembler) LDUBR14(disp int, i Reg) { a.disp8(0x6000, disp, 1, i) }

// STR14 stores a word: ST Ri,@(R14,disp).
func (a *Assembler) STR14(i Reg, disp int) { a.disp8(0x3000, disp, 4, i) }

// STHR14 stores a halfword: STH Ri,@(R14,disp).
func (a *Assembler) STHR14(i Reg, disp int) { a.disp8(0x5000, disp, 2, i) }

// STBR14 stores a byte: STB Ri,@(R14,disp).
func (a *Assembler) STBR14(i Reg, disp int) { a.disp8(0x7000, disp, 1, i) }

// LDR15 loads a word from the stack: LD @(R15,disp),Ri, disp a multiple of
// 4 in 0..60.
func (a *Assembler) LDR15(disp int, i Reg) {
	if disp%4 != 0 || disp < 0 || disp > 60 {
		a.fail("R15 displacement %d out of range", disp)
	}
	a.op(0x03, Reg(disp/4), i)
}

// STR15 stores a word on the stack: ST Ri,@(R15,disp).
func (a *Assembler) STR15(i Reg, disp int) {
	if disp%4 != 0 || disp < 0 || disp > 60 {
		a.fail("R15 displacement %d out of range", disp)
	}
	a.op(0x13, Reg(disp/4), i)
}

// PUSH stores a register below the stack pointer: ST Ri,@-R15.
func (a *Assembler) PUSH(i Reg) { a.op(0x17, 0, i) }

// POP loads a register from the stack: LD @R15+,Ri.
func (a *Assembler) POP(i Reg) { a.op(0x07, 0, i) }

// PUSHS pushes a dedicated register: ST Rs,@-R15.
func (a *Assembler) PUSHS(s SReg) { a.op(0x17, 8, Reg(s)) }

// POPS pops a dedicated register: LD @R15+,Rs.
func (a *Assembler) POPS(s SReg) { a.op(0x07, 8, Reg(s)) }

// PUSHPS pushes the program status: ST PS,@-R15.
func (a *Assembler) PUSHPS() { a.half(0x1790) }

// POPPS pops the program status: LD @R15+,PS.
func (a *Assembler) POPPS() { a.half(0x0790) }

// Moves.

// MOV copies a register: MOV Rj,Ri.
func (a *Assembler) MOV(j, i Reg) { a.op(0x8b, j, i) }

// MOVFromS reads a dedicated register: MOV Rs,Ri.
func (a *Assembler) MOVFromS(s SReg, i Reg) { a.op(0xb7, Reg(s), i) }

// MOVToS writes a dedicated register: MOV Ri,Rs.
func (a *Assembler) MOVToS(i Reg, s SReg) { a.op(0xb3, Reg(s), i) }

// MOVFromPS reads the program status: MOV PS,Ri.
func (a *Assembler) MOVFromPS(i Reg) { a.op(0x17, 1, i) }

// MOVToPS writes the program status: MOV Ri,PS.
func (a *Assembler) MOVToPS(i Reg) { a.op(0x07, 1, i) }

// Arithmetic. The N forms leave the flags alone.

// ADD adds a register: ADD Rj,Ri.
func (a *Assembler) ADD(j, i Reg) { a.op(0xa6, j, i) }

// ADDI adds an immediate 0..15: ADD #value,Ri.
func (a *Assembler) ADDI(value int, i Reg) { a.imm4(0xa4, value, 0, 15, i) }

// ADD2 adds a negative immediate -16..-1: ADD2 #value,Ri.
func (a *Assembler) ADD2(value int, i Reg) { a.imm4(0xa5, value, -16, -1, i) }

// ADDC adds a register and the carry: ADDC Rj,Ri.
func (a *Assembler) ADDC(j, i Reg) { a.op(0xa7, j, i) }

// ADDN adds a register without touching the flags: ADDN Rj,Ri.
func (a *Assembler) ADDN(j, i Reg) { a.op(0xa2, j, i) }

// ADDNI adds 0..15 without touching the flags: ADDN #value,Ri.
func (a *Assembler) ADDNI(value int, i Reg) { a.imm4(0xa0, value, 0, 15, i) }

// ADDN2 adds -16..-1 without touching the flags: ADDN2 #value,Ri.
func (a *Assembler) ADDN2(value int, i Reg) { a.imm4(0xa1, value, -16, -1, i) }

// SUB subtracts a register: SUB Rj,Ri (Ri -= Rj).
func (a *Assembler) SUB(j, i Reg) { a.op(0xac, j, i) }

// SUBC subtracts a register and the carry: SUBC Rj,Ri.
func (a *Assembler) SUBC(j, i Reg) { a.op(0xad, j, i) }

// SUBN subtracts without touching the flags: SUBN Rj,Ri.
func (a *Assembler) SUBN(j, i Reg) { a.op(0xae, j, i) }

// CMP compares with a register: CMP Rj,Ri (flags of Ri - Rj).
func (a *Assembler) CMP(j, i Reg) { a.op(0xaa, j, i) }

// CMPI compares with an immediate 0..15: CMP #value,Ri.
func (a *Assembler) CMPI(value int, i Reg) { a.imm4(0xa8, value, 0, 15, i) }

// CMP2 compares with an immediate -16..-1: CMP2 #value,Ri.
func (a *Assembler) CMP2(value int, i Reg) { a.imm4(0xa9, value, -16, -1, i) }

// Logic.

// AND masks with a register: AND Rj,Ri.
func (a *Assembler) AND(j, i Reg) { a.op(0x82, j, i) }

// ANDM masks a word in memory: AND Rj,@Ri.
func (a *Assembler) ANDM(j, i Reg) { a.op(0x84, j, i) }

// ANDH masks a halfword in memory: ANDH Rj,@Ri.
func (a *Assembler) ANDH(j, i Reg) { a.op(0x85, j, i) }

// ANDB masks a byte in memory: ANDB Rj,@Ri.
func (a *Assembler) ANDB(j, i Reg) { a.op(0x86, j, i) }

// OR merges a register: OR Rj,Ri.
func (a *Assembler) OR(j, i Reg) { a.op(0x92, j, i) }

// ORM merges into a word in memory: OR Rj,@Ri.
func (a *Assembler) ORM(j, i Reg) { a.op(0x94, j, i) }

// ORH merges into a halfword in memory: ORH Rj,@Ri.
func (a *Assembler) ORH(j, i Reg) { a.op(0x95, j, i) }

// ORB merges into a byte in memory: ORB Rj,@Ri.
func (a *Assembler) ORB(j, i Reg) { a.op(0x96, j, i) }

// EOR exclusive-ors a register: EOR Rj,Ri.
func (a *Assembler) EOR(j, i Reg) { a.op(0x9a, j, i) }

// EORM exclusive-ors a word in memory: EOR Rj,@Ri.
func (a *Assembler) EORM(j, i Reg) { a.op(0x9c, j, i) }

// EORH exclusive-ors a halfword in memory: EORH Rj,@Ri.
func (a *Assembler) EORH(j, i Reg) { a.op(0x9d, j, i) }

// EORB exclusive-ors a byte in memory: EORB Rj,@Ri.
func (a *Assembler) EORB(j, i Reg) { a.op(0x9e, j, i) }

// Bit operations on the byte at @Ri; L works on its low nibble, H on its
// high one, with a 4-bit mask.

// BANDL: BANDL #mask,@Ri.
func (a *Assembler) BANDL(mask int, i Reg) { a.imm4(0x80, mask, 0, 15, i) }

// BANDH: BANDH #mask,@Ri.
func (a *Assembler) BANDH(mask int, i Reg) { a.imm4(0x81, mask, 0, 15, i) }

// BORL: BORL #mask,@Ri.
func (a *Assembler) BORL(mask int, i Reg) { a.imm4(0x90, mask, 0, 15, i) }

// BORH: BORH #mask,@Ri.
func (a *Assembler) BORH(mask int, i Reg) { a.imm4(0x91, mask, 0, 15, i) }

// BEORL: BEORL #mask,@Ri.
func (a *Assembler) BEORL(mask int, i Reg) { a.imm4(0x98, mask, 0, 15, i) }

// BEORH: BEORH #mask,@Ri.
func (a *Assembler) BEORH(mask int, i Reg) { a.imm4(0x99, mask, 0, 15, i) }

// BTSTL: BTSTL #mask,@Ri.
func (a *Assembler) BTSTL(mask int, i Reg) { a.imm4(0x88, mask, 0, 15, i) }

// BTSTH: BTSTH #mask,@Ri.
func (a *Assembler) BTSTH(mask int, i Reg) { a.imm4(0x89, mask, 0, 15, i) }

// Multiply and divide.

// MUL multiplies signed 32x32 into MDH:MDL: MUL Rj,Ri.
func (a *Assembler) MUL(j, i Reg) { a.op(0xaf, j, i) }

// MULU multiplies unsigned 32x32 into MDH:MDL: MULU Rj,Ri.
func (a *Assembler) MULU(j, i Reg) { a.op(0xab, j, i) }

// MULH multiplies signed 16x16 into MDL: MULH Rj,Ri.
func (a *Assembler) MULH(j, i Reg) { a.op(0xbf, j, i) }

// MULUH multiplies unsigned 16x16 into MDL: MULUH Rj,Ri.
func (a *Assembler) MULUH(j, i Reg) { a.op(0xbb, j, i) }

// DIV0S starts a signed division of MDL by Ri: DIV0S Ri.
func (a *Assembler) DIV0S(i Reg) { a.op(0x97, 4, i) }

// DIV0U starts an unsigned division of MDL by Ri: DIV0U Ri.
func (a *Assembler) DIV0U(i Reg) { a.op(0x97, 5, i) }

// DIV1 is one division step: DIV1 Ri.
func (a *Assembler) DIV1(i Reg) { a.op(0x97, 6, i) }

// DIV2 corrects a signed division: DIV2 Ri.
func (a *Assembler) DIV2(i Reg) { a.op(0x97, 7, i) }

// DIV3 corrects a signed division: DIV3.
func (a *Assembler) DIV3() { a.half(0x9f60) }

// DIV4S corrects a signed division: DIV4S.
func (a *Assembler) DIV4S() { a.half(0x9f70) }

// Shifts.

// LSL shifts left by amount 0..15: LSL #amount,Ri.
func (a *Assembler) LSL(amount int, i Reg) { a.imm4(0xb4, amount, 0, 15, i) }

// LSL2 shifts left by 16+amount (amount 0..15): LSL2 #amount,Ri.
func (a *Assembler) LSL2(amount int, i Reg) { a.imm4(0xb5, amount, 0, 15, i) }

// LSLR shifts left by Rj: LSL Rj,Ri.
func (a *Assembler) LSLR(j, i Reg) { a.op(0xb6, j, i) }

// LSR shifts right by amount 0..15: LSR #amount,Ri.
func (a *Assembler) LSR(amount int, i Reg) { a.imm4(0xb0, amount, 0, 15, i) }

// LSR2 shifts right by 16+amount (amount 0..15): LSR2 #amount,Ri.
func (a *Assembler) LSR2(amount int, i Reg) { a.imm4(0xb1, amount, 0, 15, i) }

// LSRR shifts right by Rj: LSR Rj,Ri.
func (a *Assembler) LSRR(j, i Reg) { a.op(0xb2, j, i) }

// ASR shifts right arithmetically by amount 0..15: ASR #amount,Ri.
func (a *Assembler) ASR(amount int, i Reg) { a.imm4(0xb8, amount, 0, 15, i) }

// ASR2 shifts right arithmetically by 16+amount: ASR2 #amount,Ri.
func (a *Assembler) ASR2(amount int, i Reg) { a.imm4(0xb9, amount, 0, 15, i) }

// ASRR shifts right arithmetically by Rj: ASR Rj,Ri.
func (a *Assembler) ASRR(j, i Reg) { a.op(0xba, j, i) }

// Shl shifts left by 0..31 with LSL or LSL2.
func (a *Assembler) Shl(amount int, i Reg) {
	if amount >= 16 {
		a.LSL2(amount-16, i)
	} else {
		a.LSL(amount, i)
	}
}

// Shr shifts right by 0..31 with LSR or LSR2.
func (a *Assembler) Shr(amount int, i Reg) {
	if amount >= 16 {
		a.LSR2(amount-16, i)
	} else {
		a.LSR(amount, i)
	}
}

// Sar shifts right arithmetically by 0..31 with ASR or ASR2.
func (a *Assembler) Sar(amount int, i Reg) {
	if amount >= 16 {
		a.ASR2(amount-16, i)
	} else {
		a.ASR(amount, i)
	}
}

// Extensions.

// EXTSB sign-extends the low byte: EXTSB Ri.
func (a *Assembler) EXTSB(i Reg) { a.op(0x97, 8, i) }

// EXTUB zero-extends the low byte: EXTUB Ri.
func (a *Assembler) EXTUB(i Reg) { a.op(0x97, 9, i) }

// EXTSH sign-extends the low halfword: EXTSH Ri.
func (a *Assembler) EXTSH(i Reg) { a.op(0x97, 10, i) }

// EXTUH zero-extends the low halfword: EXTUH Ri.
func (a *Assembler) EXTUH(i Reg) { a.op(0x97, 11, i) }

// Control.

// JMP jumps to the address in a register: JMP @Ri.
func (a *Assembler) JMP(i Reg) { a.op(0x97, 0, i) }

// JMPD jumps after the next instruction: JMP:D @Ri.
func (a *Assembler) JMPD(i Reg) { a.op(0x9f, 0, i) }

// CALL calls the address in a register, keeping the return address in RP:
// CALL @Ri.
func (a *Assembler) CALL(i Reg) { a.op(0x97, 1, i) }

// CALLD calls after the next instruction: CALL:D @Ri.
func (a *Assembler) CALLD(i Reg) { a.op(0x9f, 1, i) }

// RET returns to the address in RP.
func (a *Assembler) RET() { a.half(0x9720) }

// RETD returns after the next instruction: RET:D.
func (a *Assembler) RETD() { a.half(0x9f20) }

// RETI returns from an interrupt.
func (a *Assembler) RETI() { a.half(0x9730) }

// INT raises software interrupt vector 0..255: INT #vector.
func (a *Assembler) INT(vector uint8) { a.half(0x1f00 | uint16(vector)) }

// INTE raises the emulator interrupt.
func (a *Assembler) INTE() { a.half(0x9f30) }

// NOP does nothing.
func (a *Assembler) NOP() { a.half(NOPCode) }

// NOPCode is NOP's encoding, for words built by hand.
const NOPCode = 0x9fa0

// ANDCCR clears condition-code bits: ANDCCR #mask (0xEF clears I).
func (a *Assembler) ANDCCR(mask uint8) { a.half(0x8300 | uint16(mask)) }

// ORCCR sets condition-code bits: ORCCR #mask (0x10 sets I).
func (a *Assembler) ORCCR(mask uint8) { a.half(0x9300 | uint16(mask)) }

// STILM sets the interrupt level mask: STILM #level.
func (a *Assembler) STILM(level uint8) { a.half(0x8700 | uint16(level)) }

// ADDSP adds to the stack pointer, a multiple of 4 in -512..508: ADDSP #n.
func (a *Assembler) ADDSP(n int) {
	if n%4 != 0 || n < -512 || n > 508 {
		a.fail("ADDSP %d out of range", n)
	}
	a.half(0xa300 | uint16(uint8(int8(n/4))))
}

// ENTER makes a frame of size bytes (a multiple of 4 in 4..1020): it pushes
// R14, points R14 at it and lowers R15: ENTER #size.
func (a *Assembler) ENTER(size int) {
	if size%4 != 0 || size < 4 || size > 1020 {
		a.fail("ENTER %d out of range", size)
	}
	a.half(0x0f00 | uint16(size/4))
}

// LEAVE undoes ENTER.
func (a *Assembler) LEAVE() { a.half(0x9f90) }

// XCHB exchanges Ri with the byte at @Rj: XCHB @Rj,Ri.
func (a *Assembler) XCHB(j, i Reg) { a.op(0x8a, j, i) }

// STM pushes a set of registers (any of R0-R15) with STM0/STM1, the higher
// ones first, so that they sit in ascending order from the new R15.
func (a *Assembler) STM(regs ...Reg) {
	low, high := regMasks(regs)
	if high != 0 {
		a.half(0x8f00 | uint16(reverse8(high)))
	}
	if low != 0 {
		a.half(0x8e00 | uint16(reverse8(low)))
	}
}

// LDM pops a set of registers pushed by STM with LDM0/LDM1.
func (a *Assembler) LDM(regs ...Reg) {
	low, high := regMasks(regs)
	if low != 0 {
		a.half(0x8c00 | uint16(low))
	}
	if high != 0 {
		a.half(0x8d00 | uint16(high))
	}
}

func regMasks(regs []Reg) (low, high uint8) {
	for _, r := range regs {
		if r < 8 {
			low |= 1 << r
		} else {
			high |= 1 << (r - 8)
		}
	}
	return low, high
}

func reverse8(b uint8) uint8 {
	var r uint8
	for bit := 0; bit < 8; bit++ {
		if b&(1<<bit) != 0 {
			r |= 0x80 >> bit
		}
	}
	return r
}

// Moves to and from the low 1 KiB (DMOV): dir is a byte address there (words),
// 512 bytes (halfwords) or 256 bytes (bytes).

func (a *Assembler) dir(opcode uint8, dir, scale int) {
	if dir%scale != 0 || dir < 0 || dir >= 256*scale {
		a.fail("DMOV address %#x out of range", dir)
	}
	a.half(uint16(opcode)<<8 | uint16(dir/scale))
}

// DMOVToR13 loads a word: DMOV @dir,R13.
func (a *Assembler) DMOVToR13(dir int) { a.dir(0x08, dir, 4) }

// DMOVFromR13 stores a word: DMOV R13,@dir.
func (a *Assembler) DMOVFromR13(dir int) { a.dir(0x18, dir, 4) }

// DMOVToR13Inc copies a word to @R13 and advances R13: DMOV @dir,@R13+.
func (a *Assembler) DMOVToR13Inc(dir int) { a.dir(0x0c, dir, 4) }

// DMOVFromR13Inc copies a word from @R13 and advances R13: DMOV @R13+,@dir.
func (a *Assembler) DMOVFromR13Inc(dir int) { a.dir(0x1c, dir, 4) }

// DMOVPush pushes a word: DMOV @dir,@-R15.
func (a *Assembler) DMOVPush(dir int) { a.dir(0x0b, dir, 4) }

// DMOVPop pops a word: DMOV @R15+,@dir.
func (a *Assembler) DMOVPop(dir int) { a.dir(0x1b, dir, 4) }

// DMOVHToR13 loads a halfword: DMOVH @dir,R13.
func (a *Assembler) DMOVHToR13(dir int) { a.dir(0x09, dir, 2) }

// DMOVHFromR13 stores a halfword: DMOVH R13,@dir.
func (a *Assembler) DMOVHFromR13(dir int) { a.dir(0x19, dir, 2) }

// DMOVBToR13 loads a byte: DMOVB @dir,R13.
func (a *Assembler) DMOVBToR13(dir int) { a.dir(0x0a, dir, 1) }

// DMOVBFromR13 stores a byte: DMOVB R13,@dir.
func (a *Assembler) DMOVBFromR13(dir int) { a.dir(0x1a, dir, 1) }
