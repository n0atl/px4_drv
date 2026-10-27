// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
//
// dibx000_common.c carries no copyright line of its own; its MODULE_AUTHOR is
// Patrick Boettcher.
//
// Ported from the Linux media subsystem's dibx000_common.c and
// dibx000_common.h: the pieces the DiB8000 needs.

package dib8096

import "github.com/Till0196/coax/pkg/i2c"

// Frequency bands, as the BAND_* bits of dibx000_common.h. An AGC setting
// declares the bands it covers by OR-ing them.
const (
	bandLBand uint8 = 0x01
	bandUHF   uint8 = 0x02
	bandVHF   uint8 = 0x04
	bandSBand uint8 = 0x08
	bandFM    uint8 = 0x10
	bandCBand uint8 = 0x20
)

// bandOfFrequency is BAND_OF_FREQUENCY. The FM test sits after CBAND's wider
// bound and so can never match; the macro is kept as written.
func bandOfFrequency(kHz uint32) uint8 {
	switch {
	case kHz <= 170000:
		return bandCBand
	case kHz <= 115000:
		return bandFM
	case kHz <= 250000:
		return bandVHF
	case kHz <= 863000:
		return bandUHF
	case kHz <= 2000000:
		return bandLBand
	default:
		return bandSBand
	}
}

// Chip output modes, the OUTMODE_* values.
const (
	outmodeHighZ         = 0
	outmodeParallelGated = 1
	outmodeParallelCont  = 2
	outmodeSerial        = 7
	outmodeDiversity     = 4
	outmodeFIFO          = 5
	outmodeAnalogADC     = 6
)

// ADC states, enum dibx000_adc_states.
const (
	adcSlowOn = iota
	adcSlowOff
	adcOn
	adcOff
	adcVBGEnable
	adcVBGDisable
)

// Tune states shared by the demodulator's and the tuner's state machines,
// enum frontend_tune_state. The numbers matter: both machines compare them.
const (
	tunerStart = 10 + iota
	tunerStep0
	tunerStep1
	tunerStep2
	tunerStep3
	tunerStep4
	tunerStep5
	tunerStep6
	tunerStep7
	tunerStop
)

const (
	agcStart = 20 + iota
	agcStep0
	agcStep1
	agcStep2
	agcStep3
	agcStep4
	agcStop
)

const (
	demodStart = 30 + iota
	demodStep1
	demodStep2
	demodStep3
	demodStep4
	demodStep5
	demodStep6
	demodStep7
	demodStep8
	demodStep9
	demodStep10
	demodStep11
)

const (
	demodSearchNext = 51 + iota
	demodStepLocked
	demodStop
)

// Tune outcomes, the FE_STATUS_* values. Negative numbers are successes of
// increasing depth, which is why the set_frontend loop compares them.
const (
	statusTuneFailed       int32 = 0
	statusTuneTimedOut     int32 = -1
	statusTuneTimeTooShort int32 = -2
	statusTunePending      int32 = -3
	statusStdSuccess       int32 = -4
	statusFFTSuccess       int32 = -5
	statusDemodSuccess     int32 = -6
	statusLocked           int32 = -7
	statusDataLocked       int32 = -8
)

// callbackTimeNever is FE_CALLBACK_TIME_NEVER, what a tune step returns when
// it has nothing left to do.
const callbackTimeNever uint32 = 0xffffffff

// ctrl_timf operations.
const (
	timfSet    = 0x00
	timfGet    = 0x01
	timfUpdate = 0x02
)

// agcConfig is struct dibx000_agc_config.
type agcConfig struct {
	bandCaps uint8

	setup uint16

	invGain      uint16
	timeStabiliz uint16

	alphaLevel uint8
	thlock     uint16

	wbdInv   uint8
	wbdRef   uint16
	wbdSel   uint8
	wbdAlpha uint8

	agc1Max uint16
	agc1Min uint16
	agc2Max uint16
	agc2Min uint16

	agc1Pt1 uint8
	agc1Pt2 uint8
	agc1Pt3 uint8

	agc1Slope1 uint8
	agc1Slope2 uint8

	agc2Pt1 uint8
	agc2Pt2 uint8

	agc2Slope1 uint8
	agc2Slope2 uint8

	alphaMant uint8
	alphaExp  uint8

	betaMant uint8
	betaExp  uint8

	performAGCSoftsplit uint8

	split struct {
		min      uint16
		max      uint16
		minThres uint16
		maxThres uint16
	}
}

// bandwidthConfig is struct dibx000_bandwidth_config: the demodulator's
// clock tree.
type bandwidthConfig struct {
	internal uint32
	sampling uint32

	pllPrediv uint8
	pllRatio  uint8
	pllRange  uint8
	pllReset  uint8
	pllBypass uint8

	enableRefdiv uint8
	bypclkDiv    uint8
	ioClkEnCore  uint8
	adcClkSrc    uint8
	modulo       uint8

	sadCfg uint16

	ifreq uint32
	timf  uint32

	xtalHz uint32
}

// I2C master interfaces, enum dibx000_i2c_interface: which pins the chip's
// own I2C master drives.
const (
	interfaceTuner  uint16 = 0
	interfaceGPIO12 uint16 = 1
	interfaceGPIO34 uint16 = 2
	interfaceGPIO67 uint16 = 3
)

// Device revisions the common code tells apart. Only DIB8000 is used here, but
// the comparisons below are against the others.
const (
	deviceDiB3000MC = 1
	deviceDiB7000   = 2
	deviceDiB7000P  = 11
	deviceDiB7000MC = 12
	deviceDiB8000   = 13
)

// i2cMaster is struct dibx000_i2c_master: the DiB8000's own I2C master, in
// front of the DiB0090 die sharing its package, driven through the DiB8000's
// registers base..base+4: data, control, status, speed and interface select.
// Only its gate is used, as on Linux's stk809x boards: the DiB8000 opens it to
// the tuner and the host talks to the tuner on its own bus.
type i2cMaster struct {
	demod             *dib8000
	deviceRevision    uint16
	baseRegister      uint16
	selectedInterface uint16
	// gate is the 8-bit address the gate is open to, 0 when closed.
	gate byte
}

// initialize is dibx000_init_i2c_master: it picks the register base and
// closes the gate.
func (master *i2cMaster) initialize(demod *dib8000, deviceRevision uint16) {
	master.demod = demod
	master.deviceRevision = deviceRevision
	if deviceRevision == deviceDiB7000P || deviceRevision == deviceDiB8000 {
		master.baseRegister = 1024
	} else {
		master.baseRegister = 768
	}
	master.demod.writeWord(master.baseRegister+1, master.gateControl(0, false))
	master.gate = 0
}

// reset is dibx000_reset_i2c_master.
func (master *i2cMaster) reset() {
	master.demod.writeWord(master.baseRegister+1, master.gateControl(0, false))
	master.gate = 0
	// Force a select the first time.
	master.selectedInterface = 0xff
	master.selectInterface(interfaceTuner)
}

// gateControl is dibx000_i2c_gate_ctrl's register value. Bit 7 chooses between
// the master and the gate: clear, the gate is open to address. Later chips moved
// the field up one bit.
func (master *i2cMaster) gateControl(address7 uint8, open bool) uint16 {
	var value uint16
	if open {
		value = uint16(address7) << 8
	} else {
		value = 1 << 7
	}
	if master.deviceRevision > deviceDiB7000 {
		value <<= 1
	}
	return value
}

// selectInterface is dibx000_i2c_select_interface.
func (master *i2cMaster) selectInterface(intf uint16) {
	if master.deviceRevision > deviceDiB3000MC && master.selectedInterface != intf {
		master.selectedInterface = intf
		master.demod.writeWord(master.baseRegister+4, intf)
	}
}

// tunerBus is dibx000_i2c_gated_tuner_xfer: messages to the tuner go out on
// the host's bus once the DiB8000 has opened its gate to the tuner's address.
// Linux closes the gate after every message; here it stays open, since the
// DiB8000 still answers at its own address and every open and close costs a
// register write.
type tunerBus struct {
	master *i2cMaster
	parent i2c.Bus
}

// through runs fn with the gate open to address.
func (bus tunerBus) through(address byte, fn func() error) error {
	master := bus.master
	if master.gate != address {
		master.selectInterface(interfaceTuner)
		master.demod.writeWord(master.baseRegister+1, master.gateControl(address>>1, true))
		if err := master.demod.failure.err; err != nil {
			return err
		}
		master.gate = address
	}
	return fn()
}

func (bus tunerBus) Write(address byte, data []byte) error {
	return bus.through(address, func() error { return bus.parent.Write(address, data) })
}

func (bus tunerBus) Read(address byte, length int) (data []byte, err error) {
	err = bus.through(address, func() (err error) {
		data, err = bus.parent.Read(address, length)
		return err
	})
	return data, err
}

// WriteRead is a register read with a repeated start between the register
// number and the value, which is how the tuner has to be read: it answers
// zeros when the write is stopped first.
func (bus tunerBus) WriteRead(address byte, write []byte, readLength int) (data []byte, err error) {
	err = bus.through(address, func() (err error) {
		data, err = i2c.WriteRead(bus.parent, address, write, readLength)
		return err
	})
	return data, err
}
