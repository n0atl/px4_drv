// Copyright 2026 Till0196
// Copyright (C) 2009 DiBcom (http://www.dibcom.fr/)
// SPDX-License-Identifier: GPL-2.0-only
//
// Ported from the Linux media subsystem's dib8000.c: register access, reset,
// clocks, GPIO, AGC and output configuration. The ISDB-T channel setup and the
// tune state machine are in dib8000_tune.go.
//
// Only the DiB8000 proper is ported. The DiB8096P (revision 0x8090) has its own
// register map for much of this and those branches are left out, so the
// revision checks that remain only compare against the older revisions.

package dib8096

import (
	"fmt"
	"time"

	"github.com/Till0196/coax/pkg/i2c"
)

// Layers as LAYER_* in dib8000.c.
const (
	layerAll = -1
	layerA   = 1
	layerB   = 2
	layerC   = 3
)

// Autosearch steps, enum dib8000_autosearch_step.
const (
	autosearchStart          = 0
	autosearchSearchingFFT   = 1
	autosearchSearchingGuard = 2
	autosearchDone           = 100
)

// Power modes, enum dib8000_power_mode.
const (
	powerAll = iota
	powerInterfaceOnly
)

// failure is the first bus error of an operation. Linux carries on after a
// failed register access and reads it as zero; here the first failure is kept,
// later accesses are skipped, and the public call returns it.
type failure struct {
	err error
}

func (failure *failure) record(err error) {
	if failure.err == nil {
		failure.err = err
	}
}

// dib8000Config is struct dib8000_config, reduced to the fields a DiB8096
// board sets. update_lna is always NULL on these boards and is left out, along
// with dib8000_update_lna which only consults it.
type dib8000Config struct {
	outputMPEG2In188Bytes uint8
	hostbusDiversity      uint8
	tunerIsBaseband       uint8

	agc []agcConfig
	pll *bandwidthConfig

	gpioDir    uint16
	gpioVal    uint16
	gpioPWMPos uint16
	pwmFreqDiv uint16

	drives         uint16
	diversityDelay uint16
	divCfg         uint8
	outputMode     int
	refclksel      uint8
}

// Default GPIO settings, DIB8000_GPIO_DEFAULT_*.
const (
	gpioDefaultDirections = 0xffff
	gpioDefaultValues     = 0x0000
	gpioDefaultPWMPos     = 0xffff
)

// dib8000 is struct dib8000_state for the single frontend of a DiB8096. The
// diversity slaves Linux keeps in fe[1..] do not exist on this board.
type dib8000 struct {
	bus     i2c.Bus
	address byte
	failure *failure

	cfg     dib8000Config
	master  i2cMaster
	channel *channel

	wbdRef uint16

	currentBand uint8
	currentAGC  *agcConfig
	timf        uint32
	timfDefault uint32

	differentialConstellation uint8
	diversityOnOff            int

	revision       uint16
	isdbtCfgLoaded uint8
	tuneState      int
	status         int32

	currentDemodBW uint16

	segMask     uint16
	segDiffMask uint16
	mode        uint16
	layerBNbSeg uint8
	layerCNbSeg uint8

	channelParametersSet int
	autosearchState      uint16
	foundNFFT            int
	foundGuard           int
	symbolDuration       uint8
	timeout              time.Time
	longestIntlvLayer    uint8
	outputMode           int

	// The frontend callbacks Linux reaches through fe->ops: the tuner's
	// set_params (the board override on a DiB8096), the tuner's
	// get_frequency, and the board's agc_control.
	tunerSetParams func() error
	tunerFrequency func() uint32
	agcControl     func(before uint8)

	sleep       func(time.Duration)
	now         func() time.Time
	tuneTimeout time.Duration
}

// readWord is dib8000_read_word.
func (state *dib8000) readWord(register uint16) uint16 {
	if state.failure.err != nil {
		return 0
	}
	data, err := i2c.WriteRead(state.bus, state.address, []byte{byte(register >> 8), byte(register)}, 2)
	if err != nil {
		state.failure.record(fmt.Errorf("read DiB8000 register %d: %w", register, err))
		return 0
	}
	if len(data) != 2 {
		state.failure.record(fmt.Errorf("read DiB8000 register %d: %d bytes, want 2", register, len(data)))
		return 0
	}
	return uint16(data[0])<<8 | uint16(data[1])
}

// modifyWord writes register back with the bits outside keep cleared and set
// or'ed in, Linux's read-modify-write of a register. A bus that can modify a
// register itself does, without the read coming back here first.
func (state *dib8000) modifyWord(register, keep, set uint16) {
	if state.failure.err != nil {
		return
	}
	if modifier, ok := state.bus.(i2c.Modifier); ok {
		if err := modifier.Modify16(state.address, register, keep, set); err != nil {
			state.failure.record(fmt.Errorf("modify DiB8000 register %d: %w", register, err))
		}
		return
	}
	state.writeWord(register, state.readWord(register)&keep|set)
}

// read32 is dib8000_read32: two consecutive registers, high word first.
func (state *dib8000) read32(register uint16) uint32 {
	high := state.readWord(register + 0)
	low := state.readWord(register + 1)
	return uint32(high)<<16 | uint32(low)
}

// writeWord is dib8000_write_word.
func (state *dib8000) writeWord(register, value uint16) {
	if state.failure.err != nil {
		return
	}
	err := state.bus.Write(state.address, []byte{byte(register >> 8), byte(register), byte(value >> 8), byte(value)})
	if err != nil {
		state.failure.record(fmt.Errorf("write DiB8000 register %d: %w", register, err))
	}
}

func (state *dib8000) msleep(milliseconds int) {
	state.sleep(time.Duration(milliseconds) * time.Millisecond)
}

// fftToMode is fft_to_mode: the chip's mode number for the FFT size.
func (state *dib8000) fftToMode() uint16 {
	switch state.channel.transmissionMode {
	case transmission2K:
		return 1
	case transmission4K:
		return 2
	default:
		return 3
	}
}

// setAcquisitionMode is dib8000_set_acquisition_mode.
func (state *dib8000) setAcquisitionMode() {
	nud := state.readWord(298)
	nud |= 1<<3 | 1<<0
	state.writeWord(298, nud)
}

// setOutputMode is dib8000_set_output_mode.
func (state *dib8000) setOutputMode(mode int) error {
	// By default SDRAM deinterleaving is enabled.
	var outreg uint16
	fifoThreshold := uint16(1792)
	sram := uint16(0x0205)

	state.outputMode = mode
	smoMode := state.readWord(299)&0x0050 | 1<<1

	switch mode {
	case outmodeParallelGated: // STBs with parallel gated clock
		outreg = 1 << 10 // 0x0400
	case outmodeParallelCont: // STBs with parallel continuous clock
		outreg = 1<<10 | 1<<6 // 0x0440
	case outmodeSerial: // STBs with serial input
		outreg = 1<<10 | 2<<6 | 0<<1 // 0x0482
	case outmodeDiversity:
		if state.cfg.hostbusDiversity != 0 {
			outreg = 1<<10 | 4<<6 // 0x0500
			sram &= 0xfdff
		} else {
			sram |= 0x0c00
		}
	case outmodeFIFO: // e.g. USB feeding
		smoMode |= 3 << 1
		fifoThreshold = 512
		outreg = 1<<10 | 5<<6
	case outmodeHighZ: // disable
		outreg = 0
	case outmodeAnalogADC:
		outreg = 1<<10 | 3<<6
		state.setAcquisitionMode()
	default:
		return fmt.Errorf("DiB8000 output mode %d not handled", mode)
	}

	if state.cfg.outputMPEG2In188Bytes != 0 {
		smoMode |= 1 << 5
	}

	state.writeWord(299, smoMode)
	state.writeWord(300, fifoThreshold) // synchronous fread
	state.writeWord(1286, outreg)
	state.writeWord(1291, sram)
	return state.failure.err
}

// setDiversityIn is dib8000_set_diversity_in.
func (state *dib8000) setDiversityIn(onoff int) {
	syncWait := state.readWord(273) & 0xfff0

	if state.differentialConstellation == 0 {
		state.writeWord(272, 1<<9)            // dvsy_off_lmod4 = 1
		state.writeWord(273, syncWait|1<<2|2) // sync_enable = 1; comb_mode = 2
	} else {
		state.writeWord(272, 0)        // dvsy_off_lmod4 = 0
		state.writeWord(273, syncWait) // sync_enable = 0; comb_mode = 0
	}
	state.diversityOnOff = onoff

	switch onoff {
	case 0: // only use the internal way - not the diversity input
		state.writeWord(270, 1)
		state.writeWord(271, 0)
	case 1: // both ways
		state.writeWord(270, 6)
		state.writeWord(271, 6)
	case 2: // only the diversity input
		state.writeWord(270, 0)
		state.writeWord(271, 1)
	}

	if state.revision == 0x8002 {
		tmp := state.readWord(903)
		state.writeWord(903, tmp&^(1<<3))
		state.msleep(30)
		state.writeWord(903, tmp|1<<3)
	}
}

// setPowerMode is dib8000_set_power_mode.
func (state *dib8000) setPowerMode(mode int) {
	// By default everything is going to be powered off.
	reg774 := uint16(0x3fff)
	reg775 := uint16(0xffff)
	reg776 := uint16(0xffff)
	reg900 := state.readWord(900)&0xfffc | 0x3
	reg1280 := state.readWord(1280)&0x00ff | 0xff00

	// Now, depending on the requested mode, we power on.
	switch mode {
	case powerAll: // power up everything in the demod
		reg774 = 0x0000
		reg775 = 0x0000
		reg776 = 0x0000
		reg900 &= 0xfffc
		reg1280 &= 0x00ff
	case powerInterfaceOnly:
		reg1280 &= 0x00ff
	}

	state.writeWord(774, reg774)
	state.writeWord(775, reg775)
	state.writeWord(776, reg776)
	state.writeWord(900, reg900)
	state.writeWord(1280, reg1280)
}

// setADCState is dib8000_set_adc_state.
func (state *dib8000) setADCState(no int) {
	reg907 := state.readWord(907)
	reg908 := state.readWord(908)

	switch no {
	case adcSlowOn:
		reg908 |= 1<<1 | 1<<0
		state.writeWord(908, reg908)
		reg908 &^= 1 << 1
	case adcSlowOff:
		reg908 |= 1<<1 | 1<<0
	case adcOn:
		reg907 &= 0x0fff
		reg908 &= 0x0003
	case adcOff: // leave the VBG voltage on
		reg907 = 1<<13 | 1<<12
		reg908 = 1<<6 | 1<<5 | 1<<4 | 1<<3 | 1<<1
	case adcVBGEnable:
		reg907 &^= 1 << 15
	case adcVBGDisable:
		reg907 |= 1 << 15
	}

	state.writeWord(907, reg907)
	state.writeWord(908, reg908)
}

// getADCPower is dib8000_get_adc_power: the power the ADC sees, raw or, with
// mode set, in 1/256 dB.
func (state *dib8000) getADCPower(mode uint8) int32 {
	val := int32(state.read32(384))
	if mode != 0 {
		tmp := uint32(val)
		exp := uint32(0)
		for tmp >>= 1; tmp != 0; tmp >>= 1 {
			exp++
		}
		mant := uint32(int64(val) * 1000 / int64(uint32(1)<<exp))
		// index of the LUT. Linux reads past the table for a power of 0; the
		// index is held to it here.
		ix := int(uint8((mant - 1000) / 100))
		if ix >= len(lut1000lnMant) {
			ix = len(lut1000lnMant) - 1
		}
		val = lut1000lnMant[ix] + 693*(int32(exp)-20) - 6908
		val = val * 256 / 1000
	}
	return val
}

// pwmAGCReset is dib8000_pwm_agc_reset: the demodulator's ADC back on and the
// AGC set up for the band, after the tuner's software AGC has run.
func (state *dib8000) pwmAGCReset() {
	state.setADCState(adcOn)
	state.failure.record(state.setAGCConfig(bandOfFrequency(state.channel.frequency / 1000)))
}

// setBandwidth is dib8000_set_bandwidth. The bandwidth itself is not used: a
// DiB8000 channel is always 6MHz, and only the timing frequency is written.
func (state *dib8000) setBandwidth(bandwidth uint32) {
	timf := state.timf
	if state.timf == 0 {
		timf = state.timfDefault
	}
	state.writeWord(29, uint16((timf>>16)&0xffff))
	state.writeWord(30, uint16(timf&0xffff))
}

// sadCalibrate is dib8000_sad_calib.
func (state *dib8000) sadCalibrate() {
	// internal
	state.writeWord(923, 0<<1|0<<0)
	state.writeWord(924, 776)

	// do the calibration
	state.writeWord(923, 1<<0)
	state.writeWord(923, 0<<0)

	state.msleep(1)
}

// setWBDRef is dib8000_set_wbd_ref.
func (state *dib8000) setWBDRef(value uint16) {
	if value > 4095 {
		value = 4095
	}
	state.wbdRef = value
	state.writeWord(106, value)
}

// resetPLLCommon is dib8000_reset_pll_common.
func (state *dib8000) resetPLLCommon(bandwidth *bandwidthConfig) {
	state.writeWord(23, uint16(((bandwidth.internal*1000)>>16)&0xffff))
	state.writeWord(24, uint16((bandwidth.internal*1000)&0xffff))
	state.writeWord(27, uint16((bandwidth.ifreq>>16)&0x01ff))
	state.writeWord(28, uint16(bandwidth.ifreq&0xffff))
	state.writeWord(26, uint16((bandwidth.ifreq>>25)&0x0003))
	state.writeWord(922, bandwidth.sadCfg)
}

// resetPLL is dib8000_reset_pll.
func (state *dib8000) resetPLL() {
	pll := state.cfg.pll

	state.writeWord(901, uint16(pll.pllPrediv)<<8|uint16(pll.pllRatio)<<0)

	clkCfg1 := uint16(1<<10 | 0<<9 | uint16(pll.ioClkEnCore)<<8 |
		uint16(pll.bypclkDiv)<<5 | uint16(pll.enableRefdiv)<<4 |
		1<<3 | uint16(pll.pllRange)<<1 |
		uint16(pll.pllReset)<<0)

	state.writeWord(902, clkCfg1)
	clkCfg1 = clkCfg1&0xfff7 | uint16(pll.pllBypass)<<3
	state.writeWord(902, clkCfg1)

	// smpl_cfg: P_refclksel=2, P_ensmplsel=1 nodivsmpl=1
	switch {
	case state.cfg.pll.adcClkSrc == 0:
		state.writeWord(904, 0<<15|0<<12|0<<10|
			uint16(pll.modulo)<<8|
			uint16(pll.adcClkSrc)<<7|0<<1)
	case state.cfg.refclksel != 0:
		state.writeWord(904, 0<<15|1<<12|
			uint16(state.cfg.refclksel&0x3)<<10|
			uint16(pll.modulo)<<8|
			uint16(pll.adcClkSrc)<<7|0<<1)
	default:
		state.writeWord(904, 0<<15|1<<12|
			3<<10|uint16(pll.modulo)<<8|
			uint16(pll.adcClkSrc)<<7|0<<1)
	}

	state.resetPLLCommon(pll)
}

// updatePLL is dib8000_update_pll for the DiB8000. The branch that rebuilds
// the whole PLL on a bandwidth change is left out: the demodulator's
// bandwidth is 6MHz from initialisation on and the board refuses any other,
// so only the ratio update is reachable.
func (state *dib8000) updatePLL(pll *bandwidthConfig, bandwidth uint32, ratio uint8) {
	// Get back the old values. Register 1856 is the DiB8096P's PLL
	// configuration; Linux reads it on every revision and so does this.
	reg1856 := state.readWord(1856)
	prediv := uint8(reg1856 & 0x3f)
	loopdiv := uint8((reg1856 >> 6) & 0x3f)

	if pll == nil || (pll.pllPrediv == prediv && pll.pllRatio == loopdiv) {
		return
	}

	if ratio != 0 {
		// Ratio update: only the PLL ratio is changed.
		state.writeWord(901, uint16(state.cfg.pll.pllPrediv)<<8|uint16(ratio)<<0)
	}
}

// resetGPIO is dib8000_reset_gpio.
func (state *dib8000) resetGPIO() {
	state.writeWord(1029, state.cfg.gpioDir)
	state.writeWord(1030, state.cfg.gpioVal)

	// 782 (P_gpio_od, the open-drain selection) is left as the chip starts.

	state.writeWord(1032, state.cfg.gpioPWMPos)

	state.writeWord(1037, state.cfg.pwmFreqDiv)
}

// setGPIO is dib8000_set_gpio (dib8000_cfg_gpio). Direction 0 is output.
func (state *dib8000) setGPIO(num, dir, value uint8) {
	state.cfg.gpioDir = state.readWord(1029)
	state.cfg.gpioDir &^= 1 << num              // reset the direction bit
	state.cfg.gpioDir |= uint16(dir&0x1) << num // set the new direction
	state.writeWord(1029, state.cfg.gpioDir)

	state.cfg.gpioVal = state.readWord(1030)
	state.cfg.gpioVal &^= 1 << num                 // reset the value bit
	state.cfg.gpioVal |= uint16(value&0x01) << num // set the new value
	state.writeWord(1030, state.cfg.gpioVal)
}

// identify is dib8000_identify. The vendor register is read twice because the
// first read after power-up sometimes glitches.
func (state *dib8000) identify() (uint16, error) {
	state.readWord(896)
	if value := state.readWord(896); value != 0x01b3 {
		if state.failure.err != nil {
			return 0, state.failure.err
		}
		return 0, fmt.Errorf("DiB8000: wrong vendor ID %#04x", value)
	}

	value := state.readWord(897)
	if state.failure.err != nil {
		return 0, state.failure.err
	}
	switch value {
	case 0x8000, 0x8001, 0x8002, 0x8090:
		// DiB8000A, DiB8000B, DiB8000C and DiB8096P.
		return value, nil
	}
	return 0, fmt.Errorf("DiB8000: wrong device ID %#04x", value)
}

// resetStats is the part of dib8000_reset_stats that touches the chip: it
// reads the uncorrected-block counter to take its starting value. The DVBv5
// statistics it initialises have no counterpart here.
func (state *dib8000) resetStats() {
	state.readUncBlocks()
}

// reset is dib8000_reset.
func (state *dib8000) reset() error {
	revision, err := state.identify()
	if err != nil {
		return err
	}
	state.revision = revision
	if state.revision == 0x8090 {
		return fmt.Errorf("DiB8096P (device ID 0x8090) is not supported by this driver")
	}

	// sram lead in, rdy
	state.writeWord(1287, 0x0003)

	// Revision 0x8000 is the DiB8000 MA, which Linux reports as unsupported and
	// then initialises anyway; so does this.

	state.master.reset()

	state.setPowerMode(powerAll)

	// Always leave the VBG voltage on: it consumes almost nothing but takes a
	// long time to start.
	state.setADCState(adcOff)

	// restart all parts
	state.writeWord(770, 0xffff)
	state.writeWord(771, 0xffff)
	state.writeWord(772, 0xfffc)
	state.writeWord(898, 0x000c) // restart sad
	state.writeWord(1280, 0x004d)
	state.writeWord(1281, 0x000c)

	state.writeWord(770, 0x0000)
	state.writeWord(771, 0x0000)
	state.writeWord(772, 0x0000)
	state.writeWord(898, 0x0004) // sad
	state.writeWord(1280, 0x0000)
	state.writeWord(1281, 0x0000)

	// drives
	if state.cfg.drives != 0 {
		state.writeWord(906, state.cfg.drives)
	} else {
		// min drive SDRAM - not optimal - adjust
		state.writeWord(906, 0x2d98)
	}

	state.resetPLL()
	state.writeWord(898, 0x0004)

	state.resetGPIO()

	if err := state.setOutputMode(outmodeHighZ); err != nil {
		return err
	}

	state.currentAGC = nil

	// P_iqc_alpha_pha, P_iqc_alpha_amp, P_iqc_dcc_alpha, ...
	// P_iqc_ca2 = 0; P_iqc_impnc_on = 0; P_iqc_mode = 0;
	if state.cfg.pll.ifreq == 0 {
		state.writeWord(40, 0x0755) // P_iqc_corr_inh = 0 enable IQcorr block
	} else {
		state.writeWord(40, 0x1f55) // P_iqc_corr_inh = 1 disable IQcorr block
	}

	// The defaults are runs of consecutive registers: a count, the first
	// register, then the values, until a zero count.
	defaults := dib8000Defaults
	for {
		count := defaults[0]
		if count == 0 {
			break
		}
		register := defaults[1]
		defaults = defaults[2:]
		for ; count > 0; count-- {
			state.writeWord(register, defaults[0])
			defaults = defaults[1:]
			register++
		}
	}

	state.isdbtCfgLoaded = 0

	// div_cfg override for special configs. Linux tests the revision against
	// decimal 8090 here, which is always true; kept.
	if state.revision != 8090 && state.cfg.divCfg != 0 {
		state.writeWord(903, uint16(state.cfg.divCfg))
	}

	// unforce divstr regardless whether i2c enumeration was done or not
	state.modifyWord(1285, ^uint16(1<<1), 0)

	state.setBandwidth(6000)

	state.setADCState(adcSlowOn)
	state.sadCalibrate()
	state.setADCState(adcSlowOff)

	// ber_rs_len = 3
	state.modifyWord(285, ^uint16(0x60), 3<<5)

	state.setPowerMode(powerInterfaceOnly)

	state.resetStats()

	return state.failure.err
}

// restartAGC is dib8000_restart_agc: P_restart_iqc and P_restart_agc.
func (state *dib8000) restartAGC() {
	state.writeWord(770, 0x0a00)
	state.writeWord(770, 0x0000)
}

// setAGCConfig is dib8000_set_agc_config.
func (state *dib8000) setAGCConfig(band uint8) error {
	if state.currentBand == band && state.currentAGC != nil {
		return nil
	}
	state.currentBand = band

	var agc *agcConfig
	for i := range state.cfg.agc {
		if state.cfg.agc[i].bandCaps&band != 0 {
			agc = &state.cfg.agc[i]
			break
		}
	}
	if agc == nil {
		return fmt.Errorf("DiB8000: no valid AGC configuration for band %#02x", band)
	}

	state.currentAGC = agc

	// AGC
	state.writeWord(76, agc.setup)
	state.writeWord(77, agc.invGain)
	state.writeWord(78, agc.timeStabiliz)
	state.writeWord(101, uint16(agc.alphaLevel)<<12|agc.thlock)

	// Demod AGC loop configuration
	state.writeWord(102, uint16(agc.alphaMant)<<5|uint16(agc.alphaExp))
	state.writeWord(103, uint16(agc.betaMant)<<6|uint16(agc.betaExp))

	// AGC continued
	if state.wbdRef != 0 {
		state.writeWord(106, state.wbdRef)
	} else { // use default
		state.writeWord(106, agc.wbdRef)
	}

	state.writeWord(107, uint16(agc.wbdAlpha)<<9|uint16(agc.performAGCSoftsplit)<<8)
	state.writeWord(108, agc.agc1Max)
	state.writeWord(109, agc.agc1Min)
	state.writeWord(110, agc.agc2Max)
	state.writeWord(111, agc.agc2Min)
	state.writeWord(112, uint16(agc.agc1Pt1)<<8|uint16(agc.agc1Pt2))
	state.writeWord(113, uint16(agc.agc1Slope1)<<8|uint16(agc.agc1Slope2))
	state.writeWord(114, uint16(agc.agc2Pt1)<<8|uint16(agc.agc2Pt2))
	state.writeWord(115, uint16(agc.agc2Slope1)<<8|uint16(agc.agc2Slope2))

	state.writeWord(75, uint16(agc.agc1Pt3))
	state.modifyWord(923, 0xffe3, uint16(agc.wbdInv)<<4|uint16(agc.wbdSel)<<2)

	return nil
}

// agcSoftSplit is dib8000_agc_soft_split.
func (state *dib8000) agcSoftSplit() int {
	agc := state.currentAGC
	if agc == nil || agc.performAGCSoftsplit == 0 || agc.split.max == 0 {
		return 0
	}

	// n_agc_global
	global := state.readWord(390)

	var splitOffset uint16
	switch {
	case global > agc.split.minThres:
		splitOffset = agc.split.min
	case global < agc.split.maxThres:
		splitOffset = agc.split.max
	default:
		splitOffset = uint16(int(agc.split.max) *
			(int(global) - int(agc.split.minThres)) /
			(int(agc.split.maxThres) - int(agc.split.minThres)))
	}

	// P_agc_force_split and P_agc_split_offset
	state.modifyWord(107, 0xff00, splitOffset)
	return 5000
}

// agcStartup is dib8000_agc_startup. It returns the time to wait before the
// next step, in 0.1ms units.
func (state *dib8000) agcStartup() int {
	ret := 0

	switch state.tuneState {
	case agcStart:
		// set power-up level: interf+analog+AGC
		state.setADCState(adcOn)

		if state.setAGCConfig(bandOfFrequency(state.channel.frequency/1000)) != nil {
			state.tuneState = agcStop
			state.status = statusTuneFailed
			break
		}

		ret = 70
		state.tuneState = agcStep0

	case agcStep0:
		// AGC initialization
		if state.agcControl != nil {
			state.agcControl(1)
		}

		state.restartAGC()

		// wait AGC rough lock time
		ret = 50
		state.tuneState = agcStep1

	case agcStep1:
		// wait AGC accurate lock time. dib8000_update_lna would shorten the
		// wait when the board has an LNA to switch; this one has none.
		ret = 70
		state.tuneState = agcStep2

	case agcStep2:
		state.agcSoftSplit()

		if state.agcControl != nil {
			state.agcControl(0)
		}

		state.tuneState = agcStop

	default:
		ret = state.agcSoftSplit()
	}
	return ret
}

// updateTimf is dib8000_update_timf.
func (state *dib8000) updateTimf() {
	state.timf = state.read32(435)
	timf := state.timf

	state.writeWord(29, uint16(timf>>16))
	state.writeWord(30, uint16(timf&0xffff))
}

// ctrlTimf is dib8000_ctrl_timf.
func (state *dib8000) ctrlTimf(op uint8, timf uint32) uint32 {
	switch op {
	case timfSet:
		state.timf = timf
	case timfUpdate:
		state.updateTimf()
	case timfGet:
	}
	state.setBandwidth(6000)
	return state.timf
}

// wakeup is dib8000_wakeup.
func (state *dib8000) wakeup() {
	state.setPowerMode(powerAll)
	state.setADCState(adcOn)
	state.setADCState(adcSlowOn)
}

// sleepChip is dib8000_sleep.
func (state *dib8000) sleepChip() error {
	if err := state.setOutputMode(outmodeHighZ); err != nil {
		return err
	}
	state.setPowerMode(powerInterfaceOnly)
	state.setADCState(adcSlowOff)
	state.setADCState(adcOff)
	return state.failure.err
}

// initialize is the chip part of dib8000_init: identify, set up the I2C
// master, reset, and restore the BER window the reset leaves.
func (state *dib8000) initialize() error {
	state.timfDefault = state.cfg.pll.timf

	if _, err := state.identify(); err != nil {
		return err
	}

	state.master.initialize(state, deviceDiB8000)

	if err := state.reset(); err != nil {
		return err
	}

	// ber_rs_len = 3
	state.modifyWord(285, ^uint16(0x60), 3<<5)
	state.currentDemodBW = 6000

	return state.failure.err
}
