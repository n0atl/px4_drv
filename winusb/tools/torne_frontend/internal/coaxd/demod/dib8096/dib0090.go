// Copyright 2026 Till0196
// Copyright (C) 2005-9 DiBcom (http://www.dibcom.fr/)
// SPDX-License-Identifier: GPL-2.0-only
//
// Ported from the Linux media subsystem's dib0090.c (GPL-2.0-or-later there,
// taken here under version 2 to combine with dib8000.c).
//
// Left out: the dib0090_fw_* firmware-driven variant and the SoC (in_soc)
// integrations found in the DiB7090 and DiB8096P. The software AGC
// (dib0090_gain_control) is here because the board runs it to start a CBAND
// tune, see board.go.

package dib8096

import (
	"fmt"
	"time"

	"github.com/Till0196/coax/pkg/i2c"
)

// tunerAddress is DEFAULT_DIB0090_I2C_ADDRESS, 0x60, in 8-bit form.
const tunerAddress byte = 0x60 << 1

// Enable bits of registers 0x1b and 0x24.
const (
	enLNA0    = 0x8000
	enLNA1    = 0x4000
	enLNA2    = 0x2000
	enLNA3    = 0x1000
	enMix0    = 0x0800
	enMix1    = 0x0400
	enMix2    = 0x0200
	enMix3    = 0x0100
	enIQADC   = 0x0040
	enPLL     = 0x0020
	enTX      = 0x0010
	enBB      = 0x0008
	enLO      = 0x0004
	enBias    = 0x0001
	enIQANA   = 0x0002
	enDigClk  = 0x0080 // not in the 0x24 reg, only in 0x1b
	enCrystal = 0x0002

	enUHF = 0x22E9
	enVHF = 0x44E9
	enLBD = 0x11E9
	enSBD = 0x44E9
	enCAB = 0x88E9
)

// Calibrations still to run, the bits of tuner.calibrate.
const (
	dcCal      = 0x1
	wbdCal     = 0x2
	tempCal    = 0x4
	captrimCal = 0x8
)

const (
	krosusPLLLocked = 0x800
	krosus          = 0x2
)

// SoC versions, identified to be refused.
const (
	soc            = 0x02
	soc7090P1G11R1 = 0x82
	soc7090P1G21R1 = 0x8a
	soc8090P1G11R1 = 0x86
	soc8090P1G21R1 = 0x8e
)

// Krosus silicon revisions, the low five bits of the version.
const (
	p1AB    = 0x0
	p1C     = 0x1
	p1DEF   = 0x3
	p1G     = 0x7
	p1G21R2 = 0xf
)

// Krosus package variants, bits 5-7 of the version.
const (
	mp001 = 0x1 // Single 9090/8096
	mp005 = 0x4 // Single Sband
	mp008 = 0x6 // Dual diversity VHF-UHF-LBAND
	mp009 = 0x7 // Dual diversity 29098 CBAND-UHF-LBAND-SBAND
)

// tuningEntry is struct dib0090_tuning.
type tuningEntry struct {
	maxFreq     uint32 // for every frequency less than or equal to that field: this information is correct
	switchTrim  uint8
	lnaTune     uint8
	lnaBias     uint16
	v2i         uint16
	mix         uint16
	load        uint16
	tunerEnable uint16
}

// pllEntry is struct dib0090_pll.
type pllEntry struct {
	maxFreq   uint32 // for every frequency less than or equal to that field: this information is correct
	vcoBand   uint8
	hfdivCode uint8
	hfdiv     uint8
	topresc   uint8
}

// wbdSlope is struct dib0090_wbd_slope.
type wbdSlope struct {
	maxFreq    uint16 // for every frequency less than or equal to that field: this information is correct
	slopeCold  uint16
	offsetCold uint16
	slopeHot   uint16
	offsetHot  uint16
	wbdGain    uint8
}

// dcCalibration is struct dc_calibration. A zero addr ends a table.
type dcCalibration struct {
	addr   uint8
	offset uint8
	pga    uint8
	bb1    uint16
	i      uint8
}

// tunerIdentity is struct dib0090_identity.
type tunerIdentity struct {
	version uint8
	product uint8
	p1g     uint8
	inSoC   uint8
}

// dib0090Config is struct dib0090_config, with the fields a DiB8096 board sets.
// The reset, sleep and get_adc_power callbacks are tuner fields.
type dib0090Config struct {
	io struct {
		clockKHz       uint32
		pllBypass      uint8
		pllRange       uint8
		pllPrediv      uint8
		pllLoopdiv     uint8
		adcClockRatio  uint8 // valid is 8, 7 ,6
		pllIntLoopFilt uint16
	}

	// offset in kHz
	freqOffsetKHzUHF int32
	freqOffsetKHzVHF int32

	clkouttobamse uint8 // activate or deactivate clock output
	analogOutput  uint8

	usePWMAGC   uint8
	clkoutdrive uint8

	// The WBD targets of the software AGC. dib809x_dib0090_config leaves both
	// at 0.
	wbdVHFOffset   uint16
	wbdCBandOffset uint16

	fRefClockRatio   uint8
	wbd              []wbdSlope
	forceCrystalMode uint8
}

// tuner is struct dib0090_state: the DiB0090 die, reached through the
// DiB8000's I2C master.
type tuner struct {
	bus     i2c.Bus
	address byte
	failure *failure
	config  *dib0090Config
	channel *channel

	// The board's reset and sleep lines, and the sleep function (time).
	resetLine func(onoff int)
	sleepLine func(onoff int)
	sleep     func(time.Duration)

	currentBand uint8
	tuneState   int
	currentRF   uint32 // kHz

	wbdOffset uint16
	wbdTarget int16 // in dB

	// The software AGC (dib0090_gain_control).
	rfGainLimit int16 // take-over-point: where to split between bb and rf gain
	currentGain int16 // keeps the currently programmed gain
	agcStep     uint8 // new binary search
	agcFreeze   uint8
	gain        [2]uint16 // for channel monitoring
	gainReg     [4]uint16

	// getADCPower is the board's get_adc_power: the demodulator's ADC power
	// in 1/256 dB.
	getADCPower func() int32

	rfRamp []uint16
	bbRamp []uint16

	bb1Def  uint16
	rfLTDef uint16

	// for the captrim/dc-offset search
	step       int8
	adcDiff    int16
	minADCDiff int16

	captrim  int8
	fcaptrim int8

	dc     []dcCalibration
	dcNext int
	bb6    uint16
	bb7    uint16

	currentTuneTable *tuningEntry
	currentPLLTable  *pllEntry

	tunerIsTuned uint8

	identity tunerIdentity

	rfRequest       uint32
	currentStandard uint8

	calibrate   uint8
	rest        uint32
	bias        uint16
	temperature int16

	wbdCalibrationGain uint8
	currentWBDTable    []wbdSlope
	wbdmux             uint16
}

// readReg is dib0090_read_reg: a one-byte register address, then a two-byte
// read, as two I2C messages.
func (state *tuner) readReg(register uint8) uint16 {
	if state.failure.err != nil {
		return 0
	}
	data, err := i2c.WriteRead(state.bus, state.address, []byte{register}, 2)
	if err != nil {
		state.failure.record(fmt.Errorf("read DiB0090 register %#02x: %w", register, err))
		return 0
	}
	if len(data) != 2 {
		state.failure.record(fmt.Errorf("read DiB0090 register %#02x: %d bytes, want 2", register, len(data)))
		return 0
	}
	return uint16(data[0])<<8 | uint16(data[1])
}

// writeReg is dib0090_write_reg.
func (state *tuner) writeReg(register uint8, value uint16) {
	if state.failure.err != nil {
		return
	}
	if err := state.bus.Write(state.address, []byte{register, byte(value >> 8), byte(value)}); err != nil {
		state.failure.record(fmt.Errorf("write DiB0090 register %#02x: %w", register, err))
	}
}

func (state *tuner) msleep(milliseconds int) {
	state.sleep(time.Duration(milliseconds) * time.Millisecond)
}

// writeRegs is dib0090_write_regs: consecutive registers from values.
func (state *tuner) writeRegs(register uint8, values []uint16) {
	for _, value := range values {
		state.writeReg(register, value)
		register++
	}
}

// identify is dib0090_identify.
func (state *tuner) identify() error {
	identity := &state.identity

	v := state.readReg(0x1a)
	if state.failure.err != nil {
		return state.failure.err
	}

	identity.p1g = 0
	identity.inSoC = 0

	// without PLL lock info
	v &^= krosusPLLLocked

	identity.version = uint8(v & 0xff)
	identity.product = uint8((v >> 8) & 0xf)

	if identity.product != krosus {
		return fmt.Errorf("DiB0090: unknown product (version register %#04x)", v)
	}

	if identity.version&0x3 == soc {
		// SOC 7090/8090 P1G: the DiB7090 and DiB8096P integrations.
		return fmt.Errorf("DiB0090: SoC version %#02x belongs to a DiB7090 or DiB8096P, which this driver does not support", identity.version)
	}

	switch (identity.version >> 5) & 0x7 {
	case mp001, mp005, mp008, mp009:
		// MP001: 9090/8096, MP005: single Sband, MP008: diversity
		// VHF-UHF-LBAND, MP009: diversity 29098 CBAND-UHF-LBAND-SBAND
	default:
		return fmt.Errorf("DiB0090: unknown package variant (version %#02x)", identity.version)
	}

	switch identity.version & 0x1f {
	case p1G21R2, p1G:
		identity.p1g = 1
	case p1DEF, p1C:
	case p1AB:
		return fmt.Errorf("DiB0090: P1-A/B detected: driver is deactivated - not available")
	default:
		return fmt.Errorf("DiB0090: unknown revision (version %#02x)", identity.version)
	}

	return nil
}

// resetDigital is dib0090_reset_digital. The PLL reconfiguration Linux does
// when the tuner's PLL is not bypassed is left out: this board bypasses it.
func (state *tuner) resetDigital() {
	cfg := state.config

	// HARD_RESET
	if state.resetLine != nil {
		if state.sleepLine != nil {
			state.sleepLine(0)
		}
		state.msleep(10)
		state.resetLine(1)
		state.msleep(10)
		state.resetLine(0)
		state.msleep(10)
	}

	state.writeReg(0x24, enPLL|enCrystal)

	state.writeReg(0x1b, enDigClk|enPLL|enCrystal) // PLL, DIG_CLK and CRYSTAL remain
	// adcClkOutRatio=8->7, release reset
	state.writeReg(0x20, uint16(cfg.io.adcClockRatio-1)<<11|0<<10|1<<9|1<<8|0<<4|0)
	notAnalog := uint16(0)
	if cfg.analogOutput == 0 {
		notAnalog = 1
	}
	if cfg.clkoutdrive != 0 {
		state.writeReg(0x23, 0<<15|notAnalog<<14|2<<10|1<<9|0<<8|
			uint16(cfg.clkoutdrive)<<5|uint16(cfg.clkouttobamse)<<4|0<<2|0)
	} else {
		state.writeReg(0x23, 0<<15|notAnalog<<14|2<<10|1<<9|0<<8|
			7<<5|uint16(cfg.clkouttobamse)<<4|0<<2|0)
	}

	// Read Pll current config
	pllCfg := state.readReg(0x21)

	if cfg.io.pllBypass != 0 {
		pllCfg |= uint16(cfg.io.pllBypass) << 15
		state.writeReg(0x21, pllCfg)
	}
}

// wakeup is dib0090_wakeup.
func (state *tuner) wakeup() {
	if state.sleepLine != nil {
		state.sleepLine(0)
	}

	// enable dataTX in case we have been restarted in the wrong moment
	state.writeReg(0x23, state.readReg(0x23)|1<<14)
}

// sleepTuner is dib0090_sleep.
func (state *tuner) sleepTuner() {
	if state.sleepLine != nil {
		state.sleepLine(1)
	}
}

// dccFreq is dib0090_dcc_freq.
func (state *tuner) dccFreq(fast uint8) {
	if fast != 0 {
		state.writeReg(0x04, 0)
	} else {
		state.writeReg(0x04, 1)
	}
}

// setBoost is dib0090_set_boost.
func (state *tuner) setBoost(onoff uint16) {
	state.bb1Def &= 0xdfff
	state.bb1Def |= onoff << 13
}

// setRFRampPWM is dib0090_set_rframp_pwm.
func (state *tuner) setRFRampPWM(cfg []uint16) {
	state.rfRamp = cfg

	state.writeReg(0x2a, 0xffff)

	state.writeRegs(0x2c, cfg[3:3+6])
	state.writeRegs(0x3e, cfg[9:9+2])
}

// setBBRampPWM is dib0090_set_bbramp_pwm.
func (state *tuner) setBBRampPWM(cfg []uint16) {
	state.bbRamp = cfg

	state.setBoost(boolWord(cfg[0] > 500)) // we want the boost if the gain is higher that 50dB

	state.writeReg(0x33, 0xffff)
	state.writeRegs(0x35, cfg[3:3+4])
}

// pwmGainReset is dib0090_pwm_gain_reset: it hands the gain to the
// demodulator's PWM AGC, with the ramp for the band.
func (state *tuner) pwmGainReset() {
	bbRamp := bbRampPWMNormal // default baseband config
	var rfRamp []uint16
	enPWMRFMux := uint16(1)

	// reset the AGC
	if state.config.usePWMAGC != 0 {
		switch state.currentBand {
		case bandCBand:
			rfRamp = rfRampPWMCBand
		case bandVHF:
			rfRamp = rfRampPWMVHF
		case bandUHF:
			rfRamp = rfRampPWMUHF
		}
		if rfRamp != nil {
			state.setRFRampPWM(rfRamp)
		}
		state.setBBRampPWM(bbRamp)

		// activate the ramp generator using PWM control
		if rfRamp != nil && ((state.rfRamp != nil && state.rfRamp[0] == 0) ||
			(state.currentBand == bandCBand &&
				(state.identity.version&0x1f) <= p1DEF)) {
			// DE-Engage mux for direct gain reg control
			enPWMRFMux = 0
		}

		state.writeReg(0x32, enPWMRFMux<<12|enPWMRFMux<<11)

		// Set fast servo cutoff to start AGC; 0 = 1KHz ; 1 = 50Hz ; 2 = 150Hz ; 3 = 50KHz ; 4 = servo fast
		// (Linux writes 3 for the DiB7090 SoC, which is not supported here.)
		state.writeReg(0x04, 1)
		state.writeReg(0x39, 1<<10) // 0 gain by default
	}
}

// The software AGC's filters: the gain moves by 1/2^gainAlpha of the error and
// the take-over point by 1/2^wbdAlpha.
const (
	adcTarget = -220
	gainAlpha = 5
	wbdAlpha  = 6
)

// wbdTargetFor is dib0090_wbd_target.
func (state *tuner) wbdTargetFor() {
	offset := uint16(250)
	if state.currentBand == bandVHF {
		offset = state.config.wbdVHFOffset
	}
	if state.currentBand == bandCBand {
		offset = state.config.wbdCBandOffset
	}
	state.wbdTarget = state.wbdToDB(state.wbdOffset + offset)
}

// rampAt is a ramp entry, or 0 past the end. dib0090_gain_apply walks five RF
// amplifiers of three words each from the second word, which is 16 words, and
// the PWM ramps it is given hold 11: Linux reads whatever follows the table.
// Here that is 0, an amplifier with no gain.
func rampAt(ramp []uint16, index int) uint16 {
	if index < len(ramp) {
		return ramp[index]
	}
	return 0
}

// gainApply is dib0090_gain_apply: moves the gain and the take-over point by
// the errors given and writes the amplifiers.
func (state *tuner) gainApply(gainDelta, topDelta int16, force bool) {
	var gainReg [4]uint16

	if topDelta < -511 {
		topDelta = -511
	}
	if topDelta > 511 {
		topDelta = 511
	}

	if force {
		topDelta = int16(int(topDelta) * (1 << wbdAlpha))
		gainDelta = int16(int(gainDelta) * (1 << gainAlpha))
	}

	if int(topDelta) >= int(int16(state.rfRamp[0]<<wbdAlpha))-int(state.rfGainLimit) { // overflow
		state.rfGainLimit = int16(state.rfRamp[0] << wbdAlpha)
	} else {
		state.rfGainLimit += topDelta
	}

	if state.rfGainLimit < 0 { // underflow
		state.rfGainLimit = 0
	}

	// use gain as a temporary variable and correct current_gain
	gain := (uint16(state.rfGainLimit>>wbdAlpha) + state.bbRamp[0]) << gainAlpha
	if int(gainDelta) >= int(int16(gain))-int(state.currentGain) { // overflow
		state.currentGain = int16(gain)
	} else {
		state.currentGain += gainDelta
	}
	// cannot be less than 0 (only if gain_delta is less than 0 we can have current_gain < 0)
	if state.currentGain < 0 {
		state.currentGain = 0
	}

	// now split total gain to rf and bb gain
	gain = uint16(state.currentGain >> gainAlpha)

	var rf, bb uint16
	// requested gain is bigger than rf gain limit - ACI/WBD adjustment
	if gain > uint16(state.rfGainLimit>>wbdAlpha) {
		rf = uint16(state.rfGainLimit >> wbdAlpha)
		bb = gain - rf
		if bb > state.bbRamp[0] {
			bb = state.bbRamp[0]
		}
	} else { // high signal level -> all gains put on RF
		rf = gain
		bb = 0
	}

	state.gain[0] = rf
	state.gain[1] = bb

	// software ramp
	// Start with RF gains
	ramp, at := state.rfRamp, 1 // point on RF LNA1 max gain
	ref := rf
	for i := 0; i < 7; i++ { // Go over all amplifiers => 5RF amps + 2 BB amps = 7 amps
		g0, g1, g2 := int(rampAt(ramp, at)), int(rampAt(ramp, at+1)), int(rampAt(ramp, at+2))
		var v uint16
		switch {
		case g0 == 0 || int(ref) < g1-g0: // the amp is off, or starts to work from a higher gain
			v = 0
		case int(ref) >= g1: // higher than the high working point of this amp
			v = uint16(g2)
		default: // somewhere in its range
			v = uint16((int(ref) - (g1 - g0)) * g2 / g0)
		}

		switch i {
		case 0: // LNA 1 reg mapping
			gainReg[0] = v
		case 1: // LNA 2 reg mapping
			gainReg[0] |= v << 7
		case 2: // LNA 3 reg mapping
			gainReg[1] = v
		case 3: // LNA 4 reg mapping
			gainReg[1] |= v << 7
		case 4: // CBAND LNA reg mapping
			gainReg[2] = v | state.rfLTDef
		case 5: // BB gain 1 reg mapping
			gainReg[3] = v << 3
		case 6: // BB gain 2 reg mapping
			gainReg[3] |= v << 8
		}

		at += 3 // go to next gain bloc

		// When RF is finished, start with BB
		if i == 4 {
			ramp, at = state.bbRamp, 1 // point on BB gain 1 max gain
			ref = bb
		}
	}
	gainReg[3] |= state.bb1Def
	gainReg[3] |= (bb % 10) * 100 / 125

	// Write the amplifier regs
	for i, register := range [4]uint8{0x08, 0x0a, 0x0f, 0x01} {
		v := gainReg[i]
		if force || state.gainReg[i] != v {
			state.gainReg[i] = v
			state.writeReg(register, v)
		}
	}
}

// gainControl is dib0090_gain_control: one step of the software AGC, run from
// agcStart until the tune state reaches agcStop. It returns how long to wait
// before the next step, in ms.
func (state *tuner) gainControl() int {
	ret := 10

	applyGainImmediately := true
	var wbdError, adcError int16

	if state.tuneState == agcStart {
		state.agcFreeze = 0
		state.writeReg(0x04, 0x0)

		switch {
		case state.currentBand == bandVHF && state.identity.p1g == 0:
			state.setRFRamp(rfRampPWMVHF)
			state.setBBRamp(bbRampPWMNormal)
		case state.currentBand == bandCBand && state.identity.p1g == 0:
			state.setRFRamp(rfRampPWMCBand)
			state.setBBRamp(bbRampPWMNormal)
		case (state.currentBand == bandCBand || state.currentBand == bandVHF) && state.identity.p1g != 0:
			state.setRFRamp(rfRampPWMCBand7090P)
			state.setBBRamp(bbRampPWMNormalSoCs)
		default:
			state.setRFRamp(rfRampPWMUHF)
			state.setBBRamp(bbRampPWMNormal)
		}

		state.writeReg(0x32, 0)
		state.writeReg(0x39, 0)

		state.wbdTargetFor()

		state.rfGainLimit = int16(state.rfRamp[0] << wbdAlpha)
		state.currentGain = int16((state.rfRamp[0] + state.bbRamp[0]) / 2 << gainAlpha)

		state.tuneState = agcStep0
	} else if state.agcFreeze == 0 {
		cnt := 1
		if state.tuneState == agcStep0 {
			cnt = 5
		}
		var wbd int16
		for i := 0; i < cnt; i++ {
			wbd += state.wbdToDB(uint16(state.getSlowADCVal()))
		}
		wbd /= int16(cnt)
		wbdError = state.wbdTarget - wbd

		if state.tuneState == agcStep0 {
			if wbdError < 0 && state.rfGainLimit > 0 && state.identity.p1g == 0 {
				// in case of CBAND tune reduce first the lt_gain2 before adjusting the RF gain
				ltg2 := (state.rfLTDef >> 10) & 0x7
				if state.currentBand == bandCBand && ltg2 != 0 {
					ltg2 >>= 1
					state.rfLTDef &= ltg2 << 10 // reduce in 3 steps from 7 to 0
				}
			} else {
				state.agcStep = 0
				state.tuneState = agcStep1
			}
		} else {
			// calc the adc power
			adc := state.getADCPower()
			adc = int32((int64(adc)*355774 + 1<<20) >> 21) // included in [0:-700]

			adcError = int16(adcTarget - adc)
			// Linux adds 60 here for an ISDB-T layer in 16QAM or 64QAM. A tune
			// starts from a cleared channel, which has no layers.

			if state.tuneState == agcStep1 { // quickly go to the correct range of the ADC power
				step := state.agcStep
				state.agcStep++
				if abs(int(adcError)) < 50 || step > 5 {
					state.writeReg(0x02, 1<<15|3<<11|6<<6|32)
					state.writeReg(0x04, 0x01) // 0 = 1KHz ; 1 = 150Hz ; 2 = 50Hz ; 3 = 50KHz ; 4 = servo fast
					state.tuneState = agcStop
				}
			} else {
				// everything higher than or equal to CT_AGC_STOP means tracking
				ret = 100 // 10ms interval
				applyGainImmediately = false
			}
		}
	}

	// apply gain
	if state.agcFreeze == 0 {
		state.gainApply(adcError, wbdError, applyGainImmediately)
	}
	return ret
}

// setRFRamp is dib0090_set_rframp.
func (state *tuner) setRFRamp(cfg []uint16) {
	state.rfRamp = cfg
}

// setBBRamp is dib0090_set_bbramp.
func (state *tuner) setBBRamp(cfg []uint16) {
	state.bbRamp = cfg
	state.setBoost(boolWord(cfg[0] > 500)) // we want the boost if the gain is higher that 50dB
}

// setDCServo is dib0090_set_dc_servo.
func (state *tuner) setDCServo(dcServoCutoff uint8) {
	if dcServoCutoff < 4 {
		state.writeReg(0x04, uint16(dcServoCutoff))
	}
}

// getSlowADCVal is dib0090_get_slow_adc_val.
func (state *tuner) getSlowADCVal() uint32 {
	return uint32(state.readReg(0x1d))
}

// slope is struct slope.
type slope struct {
	rangeValue int16
	slope      int16
}

// slopesToScale is slopes_to_scale.
func slopesToScale(slopes []slope, val int16) uint16 {
	var rest, ret uint16
	for i := range slopes {
		if val > slopes[i].rangeValue {
			rest = uint16(slopes[i].rangeValue)
		} else {
			rest = uint16(val)
		}
		ret += uint16((int(rest) * int(slopes[i].slope)) / int(slopes[i].rangeValue))
		val -= int16(rest)
	}
	return ret
}

// dib0090WBDSlopes is dib0090_wbd_slopes.
var dib0090WBDSlopes = []slope{
	{66, 120},  // -64,-52: offset -   65
	{600, 170}, // -52,-35: 65     -  665
	{170, 250}, // -45,-10: 665    - 835
}

// wbdToDB is dib0090_wbd_to_db.
func (state *tuner) wbdToDB(wbd uint16) int16 {
	wbd &= 0x3ff
	if wbd < state.wbdOffset {
		wbd = 0
	} else {
		wbd -= state.wbdOffset
	}
	// -64dB is the floor
	return -640 + int16(slopesToScale(dib0090WBDSlopes, int16(wbd)))
}

// getWBDTarget is dib0090_get_wbd_target: the wideband-detector target for
// the channel, compensated for the temperature measured at the last tune.
func (state *tuner) getWBDTarget() uint16 {
	fMHz := state.channel.frequency / 1000000
	currentTemp := int32(state.temperature)
	index := 0

	for fMHz > uint32(state.currentWBDTable[index].maxFreq) {
		index++
	}
	wbd := &state.currentWBDTable[index]

	if currentTemp < 0 {
		currentTemp = 0
	}
	if currentTemp > 128 {
		currentTemp = 128
	}

	state.wbdmux &^= 7 << 13
	if wbd.wbdGain != 0 {
		state.wbdmux |= uint16(wbd.wbdGain) << 13
	} else {
		state.wbdmux |= 4 << 13
	}

	state.writeReg(0x10, state.wbdmux)

	// Unsigned arithmetic in C, stored signed.
	wbdThot := int32(uint32(wbd.offsetHot) - (uint32(wbd.slopeHot)*fMHz)>>6)
	wbdTcold := int32(uint32(wbd.offsetCold) - (uint32(wbd.slopeCold)*fMHz)>>6)

	wbdTcold += ((wbdThot - wbdTcold) * currentTemp) >> 7

	state.wbdTarget = state.wbdToDB(uint16(int32(state.wbdOffset) + wbdTcold))

	return uint16(int32(state.wbdOffset) + wbdTcold)
}

// setDefaultConfig is dib0090_set_default_config: runs of a count, the first
// register, then the values, until a zero count.
func (state *tuner) setDefaultConfig(n []uint16) {
	for {
		count := n[0]
		if count == 0 {
			return
		}
		register := uint8(n[1])
		n = n[2:]
		for ; count > 0; count-- {
			state.writeReg(register, n[0])
			n = n[1:]
			register++
		}
	}
}

// eFuse bounds.
const (
	capValueMin = 9
	capValueMax = 40
	hrMin       = 25
	hrMax       = 40
	polyMin     = 0
	polyMax     = 8
)

// setEFuse is dib0090_set_EFUSE: the baseband trims burnt in at the factory,
// or computed from a measurement when the fuses are blank.
func (state *tuner) setEFuse() {
	var c, h, n uint8

	e2 := state.readReg(0x26)
	e4 := state.readReg(0x28)

	// Linux compares the whole version byte with the revision codes here,
	// so only a blank fuse set normally takes this branch; kept.
	if state.identity.version == p1DEF || state.identity.version == p1G || e2 == 0xffff {
		state.writeReg(0x22, 0x10)
		cal := (state.readReg(0x22) >> 6) & 0x3ff

		if cal < 670 || cal == 1023 {
			cal = 850
		}
		n = uint8(165 - ((int(cal) * 10) >> 6))
		e2 = 3<<12 | 34<<6 | uint16(n)
		e4 = e2
	}

	if e2 != e4 {
		e2 &= e4 // Remove the redundancy
	}

	if e2 != 0xffff {
		c = uint8(e2 & 0x3f)
		n = uint8((e2 >> 12) & 0xf)
		h = uint8((e2 >> 6) & 0x3f)

		if c >= capValueMax || c <= capValueMin {
			c = 32
		} else {
			c += 14
		}
		if h >= hrMax || h <= hrMin {
			h = 34
		}
		if n >= polyMax || n <= polyMin {
			n = 3
		}

		state.writeReg(0x13, uint16(h)<<10)
		e2 = uint16(n)<<11 | uint16(h>>2)<<6 | uint16(c)
		state.writeReg(0x2, e2) // Load the BB_2
	}
}

// reset is dib0090_reset.
func (state *tuner) reset() error {
	state.resetDigital()
	if err := state.identify(); err != nil {
		return err
	}

	if (state.readReg(0x1a)>>5)&0x2 != 0 {
		state.writeReg(0x1b, enIQADC|enBB|enBias|enDigClk|enPLL|enCrystal)
	} else {
		state.writeReg(0x1b, enDigClk|enPLL|enCrystal)
	}

	state.setDefaultConfig(dib0090Defaults)

	if state.identity.p1g != 0 {
		state.setDefaultConfig(dib0090P1GAdditionalDefaults)
	}

	// Update the efuse: only available for Krosus > P1C
	if state.identity.version&0x1f >= p1DEF {
		state.setEFuse()
	}

	// Configure in function of the crystal
	if state.config.forceCrystalMode != 0 {
		state.writeReg(0x14, uint16(state.config.forceCrystalMode&3))
	} else if state.config.io.clockKHz >= 24000 {
		state.writeReg(0x14, 1)
	} else {
		state.writeReg(0x14, 2)
	}

	// enable iq-offset-calibration and wbd-calibration when tuning next time
	state.calibrate = dcCal | wbdCal | tempCal

	return state.failure.err
}

// steps is the steps() macro of the DC calibration.
func steps(u int8) int8 {
	if u > 15 {
		return u - 16
	}
	return u
}

// internWait is INTERN_WAIT.
const internWait = 10

// getOffset is dib0090_get_offset.
func (state *tuner) getOffset() int {
	ret := internWait * 10

	switch state.tuneState {
	case tunerStep2:
		// Turns to positive
		state.writeReg(0x1f, 0x7)
		state.tuneState = tunerStep3

	case tunerStep3:
		state.adcDiff = int16(state.readReg(0x1d))

		// Turns to negative
		state.writeReg(0x1f, 0x4)
		state.tuneState = tunerStep4

	case tunerStep4:
		state.adcDiff = int16(int(state.adcDiff) - int(state.readReg(0x1d)))
		state.tuneState = tunerStep5
		ret = 0
	}

	return ret
}

// setTrim is dib0090_set_trim.
func (state *tuner) setTrim() {
	dc := &state.dc[state.dcNext]
	val := &state.bb6
	if dc.addr == 0x07 {
		val = &state.bb7
	}

	*val &^= 0x1f << dc.offset
	*val |= uint16(int32(state.step) << dc.offset)

	state.writeReg(dc.addr, *val)
}

func abs(value int) int {
	if value < 0 {
		return -value
	}
	return value
}

// dcOffsetCalibration is dib0090_dc_offset_calibration.
func (state *tuner) dcOffsetCalibration() int {
	ret := 0

	switch state.tuneState {
	case tunerStart:
		// force vcm2 = 0.8V
		state.bb6 = 0
		state.bb7 = 0x040d

		// the LNA AND LO are off
		reg := state.readReg(0x24) & 0x0ffb // shutdown lna and lo
		state.writeReg(0x24, reg)

		state.wbdmux = state.readReg(0x10)
		state.writeReg(0x10, state.wbdmux&^(0xff<<3)|0x7<<3|0x3)
		state.writeReg(0x23, state.readReg(0x23)&^(1<<14))

		state.dc = dcTable
		if state.identity.p1g != 0 {
			state.dc = dcP1GTable
		}
		state.dcNext = 0
		state.dcStep0()
		ret = 50

	case tunerStep0:
		state.dcStep0()
		ret = 50

	case tunerStep1:
		state.setTrim()
		state.tuneState = tunerStep2

	case tunerStep2, tunerStep3, tunerStep4:
		ret = state.getOffset()

	case tunerStep5: // found an offset
		dc := &state.dc[state.dcNext]
		if state.step == 0 && state.adcDiff < 0 {
			state.minADCDiff = -1023
		}

		// first turn for this frequency
		if state.step == 0 {
			if dc.pga != 0 && state.adcDiff < 0 {
				state.step = 0x10
			}
			if dc.pga == 0 && state.adcDiff > 0 {
				state.step = 0x10
			}
		}

		// Look for a change of sign in adc_diff; min_adc_diff stores step N-1.
		if uint16(state.adcDiff)&0x8000 == uint16(state.minADCDiff)&0x8000 && steps(state.step) < 15 {
			// stop search when the delta the sign is changing and Steps =15 and Step=0 is force for continuance
			state.step++
			state.minADCDiff = state.adcDiff
			state.tuneState = tunerStep1
		} else {
			// the minimum was what we have seen in the step before
			if abs(int(state.adcDiff)) > abs(int(state.minADCDiff)) {
				state.step--
			}

			state.setTrim()

			state.dcNext++
			if state.dc[state.dcNext].addr == 0 { // done
				state.tuneState = tunerStep6
			} else {
				state.tuneState = tunerStep0
			}
		}

	case tunerStep6:
		state.writeReg(0x07, state.bb7&^0x0008)
		state.writeReg(0x1f, 0x7)
		state.tuneState = tunerStart // reset done -> real tuning can now begin
		state.calibrate &^= dcCal
	}
	return ret
}

// dcStep0 is CT_TUNER_STEP_0 of the DC calibration, which CT_TUNER_START
// falls through into.
func (state *tuner) dcStep0() {
	dc := &state.dc[state.dcNext]
	state.writeReg(0x01, dc.bb1)
	state.writeReg(0x07, state.bb7|uint16(dc.i)<<7)

	state.step = 0
	state.minADCDiff = 1023
	state.tuneState = tunerStep1
}

// wbdCalibration is dib0090_wbd_calibration.
func (state *tuner) wbdCalibration() int {
	var wbdGain uint8

	switch state.tuneState {
	case tunerStart:
		index := 0
		for state.currentRF/1000 > uint32(state.currentWBDTable[index].maxFreq) {
			index++
		}
		wbd := &state.currentWBDTable[index]
		if wbd.wbdGain != 0 {
			wbdGain = wbd.wbdGain
		} else {
			// Linux lowers this to 2 for LBAND and SBAND, but only under
			// CONFIG_BAND_LBAND/SBAND, which dib0090.c never defines.
			wbdGain = 4
		}

		if wbdGain == state.wbdCalibrationGain { // the WBD calibration has already been done
			state.tuneState = tunerStart
			state.calibrate &^= wbdCal
			return 0
		}

		state.writeReg(0x10, 0x1b81|1<<10|uint16(wbdGain)<<13|1<<3)

		state.writeReg(0x24, (enUHF&0x0fff)|1<<1)
		state.tuneState = tunerStep0
		state.wbdCalibrationGain = wbdGain
		return 90 // wait for the WBDMUX to switch and for the ADC to sample

	case tunerStep0:
		state.wbdOffset = uint16(state.getSlowADCVal())
		state.tuneState = tunerStart // reset done -> real tuning can now begin
		state.calibrate &^= wbdCal
	}
	return 0
}

// setBandwidth is dib0090_set_bandwidth.
func (state *tuner) setBandwidth() {
	var tmp uint16
	bandwidthKHz := state.channel.bandwidthHz / 1000

	switch {
	case bandwidthKHz <= 5000:
		tmp = 3 << 14
	case bandwidthKHz <= 6000:
		tmp = 2 << 14
	case bandwidthKHz <= 7000:
		tmp = 1 << 14
	default:
		tmp = 0 << 14
	}

	state.bb1Def &= 0x3fff
	state.bb1Def |= tmp

	state.writeReg(0x01, state.bb1Def) // be sure that we have the right bb-filter

	state.writeReg(0x03, 0x6008)             // = 0x6008 : vcm3_trim = 1 ; filter2_gm1_trim = 8 ; filter2_cutoff_freq = 0
	state.writeReg(0x04, 0x1)                // 0 = 1KHz ; 1 = 50Hz ; 2 = 150Hz ; 3 = 50KHz ; 4 = servo fast
	state.writeReg(0x02, 5<<11|8<<6|22&0x3f) // 22 = cap_value
	state.writeReg(0x05, 0xabcd)             // = 0xabcd : attenuator_ibias_tri = 2 ; input_stage_ibias_tr = 2 ; nc = 11 ; ext_gm_trim = 1 ; obuf_ibias_trim = 4 ; filter13_gm2_ibias_t = 13
}

// captrimSearch is dib0090_captrim_search. P1G silicon searches in hardware;
// earlier revisions binary-search the capacitor trim against the slow ADC.
func (state *tuner) captrimSearch() int {
	ret := 0
	lo4 := uint16(0xe900)

	switch state.tuneState {
	case tunerStart:
		state.writeReg(0x10, 0x2B1)
		state.writeReg(0x1e, 0x0032)

		if state.tunerIsTuned == 0 {
			// prepare a complete captrim
			if state.identity.p1g == 0 {
				state.step = 64
				state.captrim = 64
				state.fcaptrim = 64
			}

			state.currentRF = state.rfRequest
		} else { // we are already tuned to this frequency - the configuration is correct
			if state.identity.p1g == 0 {
				// do a minimal captrim even if the frequency has not changed
				state.step = 4
				state.captrim = int8(state.readReg(0x18) & 0x7f)
				state.fcaptrim = state.captrim
			}
		}
		state.adcDiff = 3000
		state.tuneState = tunerStep0

	case tunerStep0:
		if state.identity.p1g != 0 {
			ratio := uint16(31)

			state.writeReg(0x40, 3<<7|ratio<<2|1<<1|1)
			state.readReg(0x40)
			ret = 50
		} else {
			state.step /= 2
			state.writeReg(0x18, lo4|uint16(state.captrim))
		}
		state.tuneState = tunerStep1

	case tunerStep1:
		if state.identity.p1g != 0 {
			state.writeReg(0x40, 0x18c|0<<1|0)
			state.readReg(0x40)

			state.fcaptrim = int8(state.readReg(0x18) & 0x7F)
			state.tuneState = tunerStep3
		} else {
			// MERGE for all krosus before P1G
			adc := uint16(state.getSlowADCVal())
			var adcTarget int16
			var stepSign int8

			if state.rest == 0 {
				adcTarget = 200
			} else {
				adcTarget = 400
			}

			if int(adc) >= int(adcTarget) {
				adc -= uint16(adcTarget)
				stepSign = -1
			} else {
				adc = uint16(adcTarget) - adc
				stepSign = 1
			}

			if int(adc) < int(state.adcDiff) {
				state.adcDiff = int16(adc)
				state.fcaptrim = state.captrim
			}

			state.captrim += stepSign * state.step
			if state.step >= 1 {
				state.tuneState = tunerStep0
			} else {
				state.tuneState = tunerStep2
			}

			ret = 25
		}

	case tunerStep2: // this step is only used by krosus < P1G
		// write the final captrim config
		state.writeReg(0x18, lo4|uint16(state.fcaptrim))

		state.tuneState = tunerStep3

	case tunerStep3:
		state.calibrate &^= captrimCal
		state.tuneState = tunerStep0
	}

	return ret
}

// getTemperature is dib0090_get_temperature.
func (state *tuner) getTemperature() int {
	ret := 15

	switch state.tuneState {
	case tunerStart:
		state.wbdmux = state.readReg(0x10)
		state.writeReg(0x10, state.wbdmux&^(0xff<<3)|0x8<<3)

		state.bias = state.readReg(0x13)
		state.writeReg(0x13, state.bias|0x3<<8)

		state.tuneState = tunerStep0
		// wait for the WBDMUX to switch and for the ADC to sample

	case tunerStep0:
		state.adcDiff = int16(state.getSlowADCVal())
		state.writeReg(0x13, state.bias&^(0x3<<8)|0x2<<8)
		state.tuneState = tunerStep1

	case tunerStep1:
		val := int16(state.getSlowADCVal())
		state.temperature = int16(int(int16((int(val)-int(state.adcDiff))*180))>>8) + 55

		state.tuneState = tunerStep2

	case tunerStep2:
		state.writeReg(0x13, state.bias)
		state.writeReg(0x10, state.wbdmux) // write back original WBDMUX

		state.tuneState = tunerStart
		state.calibrate &^= tempCal
		if state.config.analogOutput == 0 {
			state.writeReg(0x23, state.readReg(0x23)|1<<14)
		}

	default:
		ret = 0
	}
	return ret
}

// wbdMux is WBD, the wideband detector mux setting: 1 1 1 1 0000 0 0 1.
const wbdMux = 0x781

// lpf is LPF, the loop filter margin in kHz.
const lpf = 100

// tune is dib0090_tune, one step of the tuner's state machine. It returns
// the time to wait before the next step in 0.1ms units.
func (state *tuner) tune() uint32 {
	tune := state.currentTuneTable
	pll := state.currentPLLTable
	c := state.channel

	var lo5, lo6, den, tmp uint16
	var fbDiv, rest, fRef, vcofKHz uint32
	ret := uint32(10) // 1ms is the default delay most of the time

	// in any case we first need to do a calibration if needed
	if state.tuneState == tunerStart {
		if state.calibrate&(dcCal|tempCal|wbdCal) != 0 {
			// deactivate DataTX before some calibrations
			state.writeReg(0x23, state.readReg(0x23)&^(1<<14))
		} else if state.config.analogOutput == 0 {
			// Activate DataTX in case a calibration has been done before
			state.writeReg(0x23, state.readReg(0x23)|1<<14)
		}
	}

	switch {
	case state.calibrate&dcCal != 0:
		return uint32(state.dcOffsetCalibration())
	case state.calibrate&wbdCal != 0:
		if state.currentRF == 0 {
			state.currentRF = c.frequency / 1000
		}
		return uint32(state.wbdCalibration())
	case state.calibrate&tempCal != 0:
		return uint32(state.getTemperature())
	case state.calibrate&captrimCal != 0:
		return uint32(state.captrimSearch())
	}

	switch state.tuneState {
	case tunerStart:
		state.currentBand = bandOfFrequency(c.frequency / 1000)
		offset := state.config.freqOffsetKHzVHF
		if state.currentBand == bandUHF {
			offset = state.config.freqOffsetKHzUHF
		}
		state.rfRequest = c.frequency/1000 + uint32(offset)

		// ISDB-T 1-seg would shift the tuning frequency here; not ported.

		if state.currentRF != state.rfRequest || int(state.currentStandard) != c.deliverySystem {
			state.tunerIsTuned = 0
			state.currentRF = 0
			state.currentStandard = 0

			tuneTable := dib0090TuningTable
			if state.identity.p1g != 0 {
				tuneTable = dib0090P1GTuningTable
			}

			tmp = uint16((state.identity.version >> 5) & 0x7)

			if tmp == mp005 || tmp == mp009 {
				// CBAND tuner version for VHF
				if state.currentBand == bandFM || state.currentBand == bandCBand || state.currentBand == bandVHF {
					state.currentBand = bandCBand // Force CBAND

					tuneTable = dib0090TuningTableFMVHFOnCBand
					if state.identity.p1g != 0 {
						tuneTable = dib0090P1GTuningTableFMVHFOnCBand
					}
				}
			}

			pllTable := dib0090PLLTable
			if state.identity.p1g != 0 {
				pllTable = dib0090P1GPLLTable
			}

			// Look for the interval. Linux walks off the end of the table
			// above its last entry; the board's frequency limits keep it
			// inside.
			tuneIndex := 0
			for state.rfRequest > tuneTable[tuneIndex].maxFreq {
				tuneIndex++
			}
			pllIndex := 0
			for state.rfRequest > pllTable[pllIndex].maxFreq {
				pllIndex++
			}
			tune = &tuneTable[tuneIndex]
			pll = &pllTable[pllIndex]

			state.currentTuneTable = tune
			state.currentPLLTable = pll

			state.writeReg(0x0b, 0xb800|uint16(tune.switchTrim))

			vcofKHz = (uint32(pll.hfdiv) * state.rfRequest) * 2

			fRef = state.config.io.clockKHz
			if state.config.fRefClockRatio != 0 {
				fRef /= uint32(state.config.fRefClockRatio)
			}

			fbDiv = vcofKHz / uint32(pll.topresc) / fRef
			rest = vcofKHz/uint32(pll.topresc) - fbDiv*fRef

			switch {
			case rest < lpf:
				rest = 0
			case rest < 2*lpf:
				rest = 2 * lpf
			case rest > fRef-lpf:
				rest = 0
				fbDiv++
			case rest > fRef-2*lpf:
				rest = fRef - 2*lpf
			}
			rest = (rest * 6528) / (fRef / 10)
			state.rest = rest

			// external loop filter, otherwise:
			// lo5 = (0 << 15) | (0 << 12) | (0 << 11) | (3 << 9) | (4 << 6) | (3 << 4) | 4;
			// lo6 = 0x0e34

			if rest == 0 {
				if pll.vcoBand != 0 {
					lo5 = 0x049f
				} else {
					lo5 = 0x041f
				}
			} else {
				switch {
				case pll.vcoBand != 0:
					lo5 = 0x049e
				case state.config.analogOutput != 0:
					lo5 = 0x041d
				default:
					lo5 = 0x041c
				}
			}

			if state.identity.p1g != 0 { // Bias is done automatically in P1G
				lo5 = 0x42c
			}

			lo5 |= uint16(pll.hfdivCode)<<11 | uint16(pll.vcoBand)<<7 // bit 15 is the split to the slave, we do not do it here

			if state.config.io.pllIntLoopFilt == 0 {
				if state.identity.p1g != 0 || rest == 0 {
					lo6 = 0xfff8
				} else {
					lo6 = 0xff28
				}
			} else {
				lo6 = state.config.io.pllIntLoopFilt << 3
			}

			den = 1

			if rest > 0 {
				lo6 |= 1<<2 | 2
				den = 255
			}
			state.writeReg(0x15, uint16(fbDiv))
			if state.config.fRefClockRatio != 0 {
				state.writeReg(0x16, den<<8|uint16(state.config.fRefClockRatio))
			} else {
				state.writeReg(0x16, den<<8|1)
			}
			state.writeReg(0x17, uint16(rest))
			state.writeReg(0x19, lo5)
			state.writeReg(0x1c, lo6)

			lo6 = tune.tunerEnable
			if state.config.analogOutput != 0 {
				lo6 = lo6&0xff9f | 0x2
			}

			state.writeReg(0x24, lo6|enLO|uint16(state.config.usePWMAGC)*enCrystal)
		}

		state.currentRF = state.rfRequest
		state.currentStandard = uint8(c.deliverySystem)

		ret = 20
		state.calibrate = captrimCal // captrim search now

	case tunerStep0:
		// Because of the captrim calibration, this step must stay the one
		// that follows the captrim state machine.
		index := 0
		for state.currentRF/1000 > uint32(state.currentWBDTable[index].maxFreq) {
			index++
		}
		wbd := &state.currentWBDTable[index]

		state.writeReg(0x1e, 0x07ff)

		gain := uint16(4)
		i := uint16(3)

		if wbd.wbdGain != 0 {
			gain = uint16(wbd.wbdGain)
		}

		state.wbdmux = gain<<13 | i<<11 | (wbdMux | uint16(state.config.usePWMAGC)<<1)
		state.writeReg(0x10, state.wbdmux)

		if tune.tunerEnable == enCAB && state.identity.p1g != 0 {
			// P1G: the cable band is selected
			state.writeReg(0x09, tune.lnaBias)
			state.writeReg(0x0b, 0xb800|uint16(tune.lnaTune)<<6|uint16(tune.switchTrim))
		} else {
			state.writeReg(0x09, uint16(tune.lnaTune)<<5|tune.lnaBias)
		}

		state.writeReg(0x0c, tune.v2i)
		state.writeReg(0x0d, tune.mix)
		state.writeReg(0x0e, tune.load)
		state.tuneState = tunerStep1

	case tunerStep1:
		// initialize the lt gain register
		state.rfLTDef = 0x7c00

		state.setBandwidth()
		state.tunerIsTuned = 1

		state.calibrate |= wbdCal
		state.calibrate |= tempCal
		state.tuneState = tunerStop

	default:
		ret = callbackTimeNever
	}
	return ret
}

// getFrequency is dib0090_get_frequency, in Hz.
func (state *tuner) getFrequency() uint32 {
	return 1000 * state.currentRF
}

// setParams is dib0090_set_params: the tune state machine run to its end.
func (state *tuner) setParams() error {
	state.tuneState = tunerStart

	for {
		ret := state.tune()
		if ret == callbackTimeNever || state.failure.err != nil {
			break
		}

		// dib0090_tune counts in 0.1ms, but Linux sleeps in whole jiffies and
		// found that anything below 10ms can keep the tuner from locking; so
		// it rounds up to 10ms steps.
		ret = 10 * (ret + 99) / 100
		state.msleep(int(ret))
		if state.tuneState == tunerStop {
			break
		}
	}
	return state.failure.err
}

// register is dib0090_register.
func (state *tuner) register() error {
	if state.config.wbd == nil {
		state.currentWBDTable = dib0090WBDTableDefault
	} else {
		state.currentWBDTable = state.config.wbd
	}

	return state.reset()
}
