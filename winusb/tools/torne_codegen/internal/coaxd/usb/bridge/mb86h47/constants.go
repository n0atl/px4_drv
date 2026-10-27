// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
// Extracted from pinned runtime files; only build-time constants remain.
package mb86h47
import "time"
const (
    i2cStatus = 0xfd8fe0
    i2cControl = 0xfd8fe4
    i2cData = 0xfd8ff0
)
const (
	endpointStream = 0x81
	requestMemory  = 0xbc

	vendorOut = 0x40
	vendorIn  = 0xc0

	controlTimeoutMS = 1000
)
const (
	registerGPIODirection = 0x90014
	registerGPIOValue     = 0x90018

	// addressState reads 0 on a chip that has run nothing since power-up.
	addressState = 0x82008
)
const (
	vectorTable     = 0xffc00
	firstFreeVector = 20
	trapStubs       = 0xe3000 // SRAM the power-up code leaves unused
	trapCommon      = 0xe3800
	interruptLevels = 0x440
	// TrapRecord is where a trapped exception or interrupt is noted:
	// magic 0x7ea9, vector, PC, PS, RP, R15, count.
	TrapRecord = 0x41ff0000
)
const readLimit = 64
const writeLimit = 64
const (
	core1Base   = 0x1700000
	core1Origin = 0xa0000

	mailbox        = 0xa3f00
	mailboxWrite   = 0x04
	mailboxAlive   = 0x7c
	mailboxAnswer  = 0x80
	mailboxRead    = 0x84
	i2cMaxTransfer = 32

	// ringScratch is the mailbox layout core 1 fills in for a ring access.
	ringScratch          = 0xa3d00
	ringAddresses        = 0xa3d40
	ring                 = 0xa3e00
	ringSlots            = 16
	ringAnswer           = 0xa3e40
	ringFailure          = 0xa3e44
	ringLap       uint32 = 1 << 31
	ringRead             = 1 << 30
	ringShort            = 1 << 29
	ringModify           = 1 << 28
	ringMask             = 1 << 27
	ringRegister         = 0x7ff

	// Where a modify keeps its value and mask while it reads, in the scratch
	// mailbox.
	modifyValue = 0x60
	modifyMask  = 0x62

	// ringExecute in wValue's low byte makes a read a ring read of its own:
	// the request carries the access and returns the answer (addressing.go).
	ringExecute = 0xfe
)
const (
	i2cOK = iota
	i2cBusError
	i2cNoAddressAck
	i2cNoDataAck
	i2cStuck
)
const i2cTimeout = 500 * time.Millisecond
const (
	commandBase   uint32 = 0x82100000
	commandSlots         = 16
	commandSize          = 512
	commandWords         = commandSize/4 - 3
	commandAnswer uint32 = 1 << 31
	commandReads         = 14

	memoryAccess = ringMask
	memoryRead   = ringRead
	memoryFIFO   = 1 << 29

	endpointCommand = 0x02
	endpointAnswer  = 0x83

	// Core 1's variables for the channel, in its data RAM.
	commandVars    = 0xa3c00
	commandExpect  = 0x00
	commandSlot    = 0x04
	commandPointer = 0x08
	commandLeft    = 0x0c
	commandCount   = 0x10
	commandError   = 0x14
	commandFlags   = 0x18
	commandMask    = 0x1c
	commandOp      = 0x20
	commandAnswers = 0xa3c40

	// Endpoint 0x83's FIFO and the register that sends what it holds.
	answerFIFO = 0xfdbd8c
	answerSend = 0xfdbd1c

	commandTimeoutMS = 500
)
const (
	sourcePacketSize = 192
	transportSize    = 188
	timestampSize    = sourcePacketSize - transportSize
	syncByte         = 0x47
)
const (
	streamReadUnit = 1536
	maxStreamRead  = 64 * 1024
)
const (
	ringBase = 0x00ae4000
	ringSize = 0x00600000 // 32768 units of 192 bytes

	engineBase = 0xd00000  // TS engine; its own core stays in reset
	core0Base  = 0x1600000 // codec core 0
)
const (
	core0Origin = 0xa0000
	// Counters for debugging: +0 loops, +4 units sent, +8 FIFO refills, +0xC
	// last event word.
	core0Counters = 0xa3f00
)
const idleMargin = 5 * time.Millisecond
type core0Ring struct {
	base, size             uint32
	writeOffset, readLimit uint32
	margin                 uint32
}
