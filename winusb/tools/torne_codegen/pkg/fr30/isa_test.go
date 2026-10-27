// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package fr30

import (
	"encoding/hex"
	"testing"
)

// Every instruction form, with the bytes fr30-elf-as (binutils 2.43)
// assembles the same line to.
func TestInstructionSet(t *testing.T) {
	cases := []struct {
		asm  string
		want string
		emit func(a *Assembler)
	}{
		{"ldi:8 0x40,r13", "c40d", func(a *Assembler) { a.LDI8(0x40, R13) }},
		{"ldi:20 0xa3f00,r0", "9ba03f00", func(a *Assembler) { a.LDI20(0xa3f00, R0) }},
		{"ldi:32 0x600df00d,r1", "9f81600df00d", func(a *Assembler) { a.LDI32(0x600df00d, R1) }},
		{"ld @r12,r0", "04c0", func(a *Assembler) { a.LD(R12, R0) }},
		{"lduh @r12,r0", "05c0", func(a *Assembler) { a.LDUH(R12, R0) }},
		{"ldub @r8,r0", "0680", func(a *Assembler) { a.LDUB(R8, R0) }},
		{"ld @(r13,r7),r3", "0073", func(a *Assembler) { a.LDR13(R7, R3) }},
		{"lduh @(r13,r7),r3", "0173", func(a *Assembler) { a.LDUHR13(R7, R3) }},
		{"ldub @(r13,r8),r0", "0280", func(a *Assembler) { a.LDUBR13(R8, R0) }},
		{"st r1,@r0", "1401", func(a *Assembler) { a.ST(R1, R0) }},
		{"sth r1,@r0", "1501", func(a *Assembler) { a.STH(R1, R0) }},
		{"stb r0,@r8", "1680", func(a *Assembler) { a.STB(R0, R8) }},
		{"st r4,@(r13,r8)", "1084", func(a *Assembler) { a.STR13(R4, R8) }},
		{"sth r4,@(r13,r8)", "1184", func(a *Assembler) { a.STHR13(R4, R8) }},
		{"stb r0,@(r13,r10)", "12a0", func(a *Assembler) { a.STBR13(R0, R10) }},
		{"ld @(r14,-8),r3", "2fe3", func(a *Assembler) { a.LDR14(-8, R3) }},
		{"ld @(r14,508),r3", "27f3", func(a *Assembler) { a.LDR14(508, R3) }},
		{"lduh @(r14,-256),r2", "4802", func(a *Assembler) { a.LDUHR14(-256, R2) }},
		{"ldub @(r14,127),r2", "67f2", func(a *Assembler) { a.LDUBR14(127, R2) }},
		{"st r5,@(r14,-512)", "3805", func(a *Assembler) { a.STR14(R5, -512) }},
		{"sth r5,@(r14,254)", "57f5", func(a *Assembler) { a.STHR14(R5, 254) }},
		{"stb r5,@(r14,-128)", "7805", func(a *Assembler) { a.STBR14(R5, -128) }},
		{"ld @(r15,60),r4", "03f4", func(a *Assembler) { a.LDR15(60, R4) }},
		{"st r4,@(r15,8)", "1324", func(a *Assembler) { a.STR15(R4, 8) }},
		{"st r9,@-r15", "1709", func(a *Assembler) { a.PUSH(R9) }},
		{"ld @r15+,r9", "0709", func(a *Assembler) { a.POP(R9) }},
		{"st rp,@-r15", "1781", func(a *Assembler) { a.PUSHS(RP) }},
		{"ld @r15+,mdl", "0785", func(a *Assembler) { a.POPS(MDL) }},
		{"st ps,@-r15", "1790", func(a *Assembler) { a.PUSHPS() }},
		{"ld @r15+,ps", "0790", func(a *Assembler) { a.POPPS() }},
		{"mov r4,r2", "8b42", func(a *Assembler) { a.MOV(R4, R2) }},
		{"mov mdl,r3", "b753", func(a *Assembler) { a.MOVFromS(MDL, R3) }},
		{"mov r3,tbr", "b303", func(a *Assembler) { a.MOVToS(R3, TBR) }},
		{"mov ps,r6", "1716", func(a *Assembler) { a.MOVFromPS(R6) }},
		{"mov r6,ps", "0716", func(a *Assembler) { a.MOVToPS(R6) }},
		{"add r0,r4", "a604", func(a *Assembler) { a.ADD(R0, R4) }},
		{"add 4,r1", "a441", func(a *Assembler) { a.ADDI(4, R1) }},
		{"add2 -4,r3", "a5c3", func(a *Assembler) { a.ADD2(-4, R3) }},
		{"addc r1,r2", "a712", func(a *Assembler) { a.ADDC(R1, R2) }},
		{"addn r1,r2", "a212", func(a *Assembler) { a.ADDN(R1, R2) }},
		{"addn 7,r2", "a072", func(a *Assembler) { a.ADDNI(7, R2) }},
		{"addn2 -7,r2", "a192", func(a *Assembler) { a.ADDN2(-7, R2) }},
		{"sub r2,r4", "ac24", func(a *Assembler) { a.SUB(R2, R4) }},
		{"subc r2,r4", "ad24", func(a *Assembler) { a.SUBC(R2, R4) }},
		{"subn r2,r4", "ae24", func(a *Assembler) { a.SUBN(R2, R4) }},
		{"cmp r0,r4", "aa04", func(a *Assembler) { a.CMP(R0, R4) }},
		{"cmp 1,r5", "a815", func(a *Assembler) { a.CMPI(1, R5) }},
		{"cmp2 -3,r5", "a9d5", func(a *Assembler) { a.CMP2(-3, R5) }},
		{"and r0,r4", "8204", func(a *Assembler) { a.AND(R0, R4) }},
		{"and r0,@r4", "8404", func(a *Assembler) { a.ANDM(R0, R4) }},
		{"andh r0,@r4", "8504", func(a *Assembler) { a.ANDH(R0, R4) }},
		{"andb r0,@r4", "8604", func(a *Assembler) { a.ANDB(R0, R4) }},
		{"or r0,r4", "9204", func(a *Assembler) { a.OR(R0, R4) }},
		{"or r0,@r4", "9404", func(a *Assembler) { a.ORM(R0, R4) }},
		{"orh r0,@r4", "9504", func(a *Assembler) { a.ORH(R0, R4) }},
		{"orb r0,@r4", "9604", func(a *Assembler) { a.ORB(R0, R4) }},
		{"eor r0,r4", "9a04", func(a *Assembler) { a.EOR(R0, R4) }},
		{"eor r0,@r4", "9c04", func(a *Assembler) { a.EORM(R0, R4) }},
		{"eorh r0,@r4", "9d04", func(a *Assembler) { a.EORH(R0, R4) }},
		{"eorb r0,@r4", "9e04", func(a *Assembler) { a.EORB(R0, R4) }},
		{"bandl 5,@r3", "8053", func(a *Assembler) { a.BANDL(5, R3) }},
		{"bandh 5,@r3", "8153", func(a *Assembler) { a.BANDH(5, R3) }},
		{"borl 5,@r3", "9053", func(a *Assembler) { a.BORL(5, R3) }},
		{"borh 5,@r3", "9153", func(a *Assembler) { a.BORH(5, R3) }},
		{"beorl 5,@r3", "9853", func(a *Assembler) { a.BEORL(5, R3) }},
		{"beorh 5,@r3", "9953", func(a *Assembler) { a.BEORH(5, R3) }},
		{"btstl 5,@r3", "8853", func(a *Assembler) { a.BTSTL(5, R3) }},
		{"btsth 5,@r3", "8953", func(a *Assembler) { a.BTSTH(5, R3) }},
		{"mul r1,r2", "af12", func(a *Assembler) { a.MUL(R1, R2) }},
		{"mulu r1,r2", "ab12", func(a *Assembler) { a.MULU(R1, R2) }},
		{"mulh r1,r2", "bf12", func(a *Assembler) { a.MULH(R1, R2) }},
		{"muluh r1,r2", "bb12", func(a *Assembler) { a.MULUH(R1, R2) }},
		{"div0s r3", "9743", func(a *Assembler) { a.DIV0S(R3) }},
		{"div0u r3", "9753", func(a *Assembler) { a.DIV0U(R3) }},
		{"div1 r3", "9763", func(a *Assembler) { a.DIV1(R3) }},
		{"div2 r3", "9773", func(a *Assembler) { a.DIV2(R3) }},
		{"div3", "9f60", func(a *Assembler) { a.DIV3() }},
		{"div4s", "9f70", func(a *Assembler) { a.DIV4S() }},
		{"lsl 2,r0", "b420", func(a *Assembler) { a.LSL(2, R0) }},
		{"lsl2 8,r0", "b580", func(a *Assembler) { a.LSL2(8, R0) }},
		{"lsl r1,r0", "b610", func(a *Assembler) { a.LSLR(R1, R0) }},
		{"lsr 4,r1", "b041", func(a *Assembler) { a.LSR(4, R1) }},
		{"lsr2 12,r1", "b1c1", func(a *Assembler) { a.LSR2(12, R1) }},
		{"lsr r2,r1", "b221", func(a *Assembler) { a.LSRR(R2, R1) }},
		{"asr 4,r1", "b841", func(a *Assembler) { a.ASR(4, R1) }},
		{"asr2 12,r1", "b9c1", func(a *Assembler) { a.ASR2(12, R1) }},
		{"asr r2,r1", "ba21", func(a *Assembler) { a.ASRR(R2, R1) }},
		{"extsb r7", "9787", func(a *Assembler) { a.EXTSB(R7) }},
		{"extub r7", "9797", func(a *Assembler) { a.EXTUB(R7) }},
		{"extsh r7", "97a7", func(a *Assembler) { a.EXTSH(R7) }},
		{"extuh r7", "97b7", func(a *Assembler) { a.EXTUH(R7) }},
		{"jmp @r12", "970c", func(a *Assembler) { a.JMP(R12) }},
		{"jmp:d @r12", "9f0c", func(a *Assembler) { a.JMPD(R12) }},
		{"call @r12", "971c", func(a *Assembler) { a.CALL(R12) }},
		{"call:d @r12", "9f1c", func(a *Assembler) { a.CALLD(R12) }},
		{"ret", "9720", func(a *Assembler) { a.RET() }},
		{"ret:d", "9f20", func(a *Assembler) { a.RETD() }},
		{"reti", "9730", func(a *Assembler) { a.RETI() }},
		{"int 0x40", "1f40", func(a *Assembler) { a.INT(0x40) }},
		{"inte", "9f30", func(a *Assembler) { a.INTE() }},
		{"nop", "9fa0", func(a *Assembler) { a.NOP() }},
		{"andccr 0xef", "83ef", func(a *Assembler) { a.ANDCCR(0xef) }},
		{"orccr 0x10", "9310", func(a *Assembler) { a.ORCCR(0x10) }},
		{"stilm 0x1f", "871f", func(a *Assembler) { a.STILM(0x1f) }},
		{"addsp -16", "a3fc", func(a *Assembler) { a.ADDSP(-16) }},
		{"addsp 508", "a37f", func(a *Assembler) { a.ADDSP(508) }},
		{"enter 12", "0f03", func(a *Assembler) { a.ENTER(12) }},
		{"leave", "9f90", func(a *Assembler) { a.LEAVE() }},
		{"xchb @r1,r2", "8a12", func(a *Assembler) { a.XCHB(R1, R2) }},
		{"stm0 (r0,r2,r7)", "8ea1", func(a *Assembler) { a.STM(R0, R2, R7) }},
		{"stm1 (r8,r9,r14)", "8fc2", func(a *Assembler) { a.STM(R8, R9, R14) }},
		{"ldm0 (r0,r2,r7)", "8c85", func(a *Assembler) { a.LDM(R0, R2, R7) }},
		{"ldm1 (r8,r9,r14)", "8d43", func(a *Assembler) { a.LDM(R8, R9, R14) }},
		{"dmov @0x88,r13", "0822", func(a *Assembler) { a.DMOVToR13(0x88) }},
		{"dmov r13,@0x88", "1822", func(a *Assembler) { a.DMOVFromR13(0x88) }},
		{"dmov @0x88,@r13+", "0c22", func(a *Assembler) { a.DMOVToR13Inc(0x88) }},
		{"dmov @r13+,@0x88", "1c22", func(a *Assembler) { a.DMOVFromR13Inc(0x88) }},
		{"dmov @0x88,@-r15", "0b22", func(a *Assembler) { a.DMOVPush(0x88) }},
		{"dmov @r15+,@0x88", "1b22", func(a *Assembler) { a.DMOVPop(0x88) }},
		{"dmovh @0x88,r13", "0944", func(a *Assembler) { a.DMOVHToR13(0x88) }},
		{"dmovh r13,@0x88", "1944", func(a *Assembler) { a.DMOVHFromR13(0x88) }},
		{"dmovb @0x88,r13", "0a88", func(a *Assembler) { a.DMOVBToR13(0x88) }},
		{"dmovb r13,@0x88", "1a88", func(a *Assembler) { a.DMOVBFromR13(0x88) }},
	}
	for _, test := range cases {
		a := New(0)
		test.emit(a)
		code, err := a.Assemble()
		if err != nil {
			t.Errorf("%s: %v", test.asm, err)
			continue
		}
		if got := hex.EncodeToString(code); got != test.want {
			t.Errorf("%s: %s, want %s", test.asm, got, test.want)
		}
	}
}
