// Copyright 2026 Till0196
// Copyright (C) 2005-9 DiBcom, SA et al
// SPDX-License-Identifier: GPL-2.0-only
//
// The board configuration and tune override are ported from the Linux media
// subsystem's dib0700_devices.c (its STK8096GP entries: dib809x_dib8000_config,
// dib809x_dib0090_config, dib8096_set_param_override). The CECH-ZD1 J takes
// the same values, except that its transport stream goes out as parallel data
// with a gated clock.

// Package dib8096 drives the DiBcom DiB8096, a DiB8000 ISDB-T demodulator and
// a DiB0090 tuner in one package, as the Sony CECH-ZD1 J carries it. The
// tuner is reached only through the demodulator's internal I2C master, so the
// whole part answers at one address.
//
// It is a port of the Linux dib8000, dib0090 and dibx000_common drivers,
// restricted to 13-segment ISDB-T at 6MHz with all transmission parameters
// found by the demodulator's own search. Register numbers are kept in decimal
// as Linux writes them, so the two can be read side by side.
package dib8096

import (
	"fmt"
	"math"
	"time"

	"github.com/Till0196/coax/pkg/i2c"
)

// Address is the DiB8000's I2C address in 8-bit form, Linux's
// DEFAULT_DIB8000_I2C_ADDRESS (18).
const Address byte = 0x12

// OutputMode is how the transport stream leaves the chip.
type OutputMode int

const (
	// OutputHighZ disables the transport stream pins.
	OutputHighZ OutputMode = iota
	// OutputParallelGatedClock is 8-bit parallel data with a clock that only
	// runs while data is valid; the CECH-ZD1 J's bridge takes it this way.
	OutputParallelGatedClock
	// OutputParallelContinuousClock is 8-bit parallel with a free-running clock.
	OutputParallelContinuousClock
	// OutputSerial is a one-bit serial stream.
	OutputSerial
	// OutputFIFO is the chip's FIFO mode, which Linux's USB sticks use.
	OutputFIFO
)

// outmode maps an OutputMode to the chip's OUTMODE_* value.
func (mode OutputMode) outmode() (int, error) {
	switch mode {
	case OutputHighZ:
		return outmodeHighZ, nil
	case OutputParallelGatedClock:
		return outmodeParallelGated, nil
	case OutputParallelContinuousClock:
		return outmodeParallelCont, nil
	case OutputSerial:
		return outmodeSerial, nil
	case OutputFIFO:
		return outmodeFIFO, nil
	}
	return 0, fmt.Errorf("DiB8096: unknown output mode %d", int(mode))
}

// dib8090AGCConfig is dib8090_agc_config: one setting for UHF and VHF (and
// the L and S bands) and one for CBAND.
var dib8090AGCConfig = []agcConfig{
	{
		bandCaps: bandUHF | bandVHF | bandLBand | bandSBand,
		// P_agc_use_sd_mod1=0, P_agc_use_sd_mod2=0, P_agc_freq_pwm_div=1,
		// P_agc_inv_pwm1=0, P_agc_inv_pwm2=0, P_agc_inh_dc_rv_est=0,
		// P_agc_time_est=3, P_agc_freeze=0, P_agc_nb_est=5, P_agc_write=0
		setup: 0<<15 | 0<<14 | 5<<11 | 0<<10 | 0<<9 | 0<<8 |
			3<<5 | 0<<4 | 5<<1 | 0<<0,

		invGain:      787,
		timeStabiliz: 10,

		alphaLevel: 0,
		thlock:     118,

		wbdInv:   0,
		wbdRef:   3530,
		wbdSel:   1,
		wbdAlpha: 5,

		agc1Max: 65535,
		agc1Min: 0,

		agc2Max: 65535,
		agc2Min: 0,

		agc1Pt1:    0,
		agc1Pt2:    32,
		agc1Pt3:    114,
		agc1Slope1: 143,
		agc1Slope2: 144,
		agc2Pt1:    114,
		agc2Pt2:    227,
		agc2Slope1: 116,
		agc2Slope2: 117,

		alphaMant: 28,
		alphaExp:  26,
		betaMant:  31,
		betaExp:   51,

		performAGCSoftsplit: 0,
	},
	{
		bandCaps: bandCBand,
		setup: 0<<15 | 0<<14 | 5<<11 | 0<<10 | 0<<9 | 0<<8 |
			3<<5 | 0<<4 | 5<<1 | 0<<0,

		invGain:      787,
		timeStabiliz: 10,

		alphaLevel: 0,
		thlock:     118,

		wbdInv:   0,
		wbdRef:   3530,
		wbdSel:   1,
		wbdAlpha: 5,

		agc1Max: 0,
		agc1Min: 0,

		agc2Max: 65535,
		agc2Min: 0,

		agc1Pt1:    0,
		agc1Pt2:    32,
		agc1Pt3:    114,
		agc1Slope1: 143,
		agc1Slope2: 144,
		agc2Pt1:    114,
		agc2Pt2:    227,
		agc2Slope1: 116,
		agc2Slope2: 117,

		alphaMant: 28,
		alphaExp:  26,
		betaMant:  31,
		betaExp:   51,

		performAGCSoftsplit: 0,
	},
}

// dib8090PLLConfig12MHz is dib8090_pll_config_12mhz: a 12MHz crystal
// multiplied by 18 to a 216MHz PLL, 54MHz internal.
var dib8090PLLConfig12MHz = bandwidthConfig{
	internal: 54000,
	sampling: 13500,

	pllPrediv: 1,
	pllRatio:  18,
	pllRange:  3,
	pllReset:  1,
	pllBypass: 0,

	enableRefdiv: 0,
	bypclkDiv:    0,
	ioClkEnCore:  1,
	adcClkSrc:    1,
	modulo:       2,

	sadCfg: 3<<14 | 1<<12 | 599<<0,

	ifreq: 0<<25 | 0,
	timf:  20199727,

	xtalHz: 12000000,
}

// dib809xDiB8000Config is dib809x_dib8000_config[0] with the CECH-ZD1 J's output
// mode. Linux replaces any mode other than serial or parallel gated with
// FIFO at init; parallel gated is kept.
func dib809xDiB8000Config() dib8000Config {
	return dib8000Config{
		outputMPEG2In188Bytes: 1,

		agc: dib8090AGCConfig,
		pll: &dib8090PLLConfig12MHz,

		tunerIsBaseband: 1,

		gpioDir:    gpioDefaultDirections,
		gpioVal:    gpioDefaultValues,
		gpioPWMPos: gpioDefaultPWMPos,

		hostbusDiversity: 1,
		divCfg:           0x31,
		outputMode:       outmodeParallelGated,
		drives:           0x2d98,
		diversityDelay:   48,
		refclksel:        3,
	}
}

// dib8090WBDTable is dib8090_wbd_table.
var dib8090WBDTable = []wbdSlope{
	// max freq ; cold slope ; cold offset ; warm slope ; warm offset ; wbd gain
	{120, 0, 500, 0, 500, 4},      // CBAND
	{170, 0, 450, 0, 450, 4},      // CBAND
	{380, 48, 373, 28, 259, 6},    // VHF
	{860, 34, 700, 36, 616, 6},    // high UHF
	{0xFFFF, 34, 700, 36, 616, 6}, // default
}

// dib809xDiB0090Config is dib809x_dib0090_config.
func dib809xDiB0090Config() *dib0090Config {
	config := &dib0090Config{
		clkouttobamse:    1,
		analogOutput:     1,
		usePWMAGC:        1,
		clkoutdrive:      1,
		freqOffsetKHzUHF: -63,
		freqOffsetKHzVHF: -143,
		wbd:              dib8090WBDTable,
		fRefClockRatio:   6,
	}
	config.io.pllBypass = 1
	config.io.pllRange = 1
	config.io.pllPrediv = 1
	config.io.pllLoopdiv = 20
	config.io.adcClockRatio = 8
	config.io.pllIntLoopFilt = 0
	config.io.clockKHz = 12000
	return config
}

// The frequency range the frontend accepts: the tuner's 45-860MHz, which
// lies inside the demodulator's 44.25-867.25MHz.
const (
	minimumFrequencyHz = 45000000
	maximumFrequencyHz = 860000000
)

// defaultTuneTimeout bounds the demodulator's search. Its own timeouts end a
// tune within about two seconds; Linux sets no outer bound.
const defaultTuneTimeout = 5 * time.Second

// Demod is a DiB8096 reached over an I2C bus. Not safe for concurrent use: a
// caller sharing one between goroutines must serialise.
type Demod struct {
	reuseTemperature      bool
	temperatureValid      bool
	temperatureLast       time.Time
	temperatureLastReused bool
	// 保存値は個体のfrontend寿命内に限定し、Initializeで破棄する。
	useMemory      bool
	memory         [64]channel
	memoryValid    [64]bool
	memoryLastUsed bool
	memoryFallback bool

	failure failure
	channel channel
	demod   *dib8000
	tuner   *tuner
	asleep  bool
}

// New addresses a DiB8096 at Address on the given bus. Nothing is sent until
// Initialize.
func New(bus i2c.Bus) *Demod {
	board := &Demod{}
	// A wait starts once the writes before it have reached the chip, and the
	// bus may use it for work of its own.
	pause := func(duration time.Duration) {
		board.failure.record(i2c.Wait(bus, duration))
	}
	demod := &dib8000{
		bus:         bus,
		address:     Address,
		failure:     &board.failure,
		cfg:         dib809xDiB8000Config(),
		channel:     &board.channel,
		sleep:       pause,
		now:         time.Now,
		tuneTimeout: defaultTuneTimeout,
	}
	tuner := &tuner{
		bus:     tunerBus{master: &demod.master, parent: bus},
		address: tunerAddress,
		failure: &board.failure,
		config:  dib809xDiB0090Config(),
		channel: &board.channel,
		sleep:   pause,
	}

	// dib80xx_tuner_reset and dib80xx_tuner_sleep: the tuner's reset and
	// sleep inputs are the DiB8000's GPIO5 (active low) and GPIO0.
	tuner.resetLine = func(onoff int) {
		demod.setGPIO(5, 0, uint8(boolWord(onoff == 0)))
	}
	tuner.sleepLine = func(onoff int) {
		demod.setGPIO(0, 0, uint8(onoff))
	}

	// dib8090_agc_control
	demod.agcControl = func(restart uint8) {
		tuner.dccFreq(restart)
		if restart == 0 { // before AGC startup
			tuner.setDCServo(1)
		}
	}
	// dib8090_get_adc_power
	tuner.getADCPower = func() int32 { return demod.getADCPower(1) }
	demod.tunerSetParams = board.setParamOverride
	demod.tunerFrequency = tuner.getFrequency

	board.demod = demod
	board.tuner = tuner
	return board
}

// flush sends the writes the bus holds back once a call is done with it, and
// reports a failure to send them as the call's.
func (board *Demod) flush(err *error) {
	if flushErr := i2c.Flush(board.demod.bus); flushErr != nil && *err == nil {
		*err = flushErr
	}
}

// setTiming replaces the clock and sleep functions, for tests.
func (board *Demod) setTiming(now func() time.Time, sleep func(time.Duration), tuneTimeout time.Duration) {
	board.demod.now = now
	board.demod.sleep = sleep
	board.demod.tuneTimeout = tuneTimeout
	board.tuner.sleep = sleep
}

// Initialize identifies the chip, resets the demodulator, registers and
// resets the tuner behind its I2C master, wakes both as a DVB frontend's init
// would, and leaves the transport stream output in the configured mode. The
// bridge must already have pulsed the chip's reset line and run the address
// enumeration step (register 1287 = 3).
func (board *Demod) Initialize() (err error) {
	board.memoryValid = [64]bool{}
	board.temperatureValid = false
	board.temperatureLastReused = false
	board.memoryLastUsed = false
	board.memoryFallback = false
	board.tuner.capLastReused = false
	board.tuner.capMemory = [64]capMemoryEntry{}
	defer board.flush(&err)
	board.failure.err = nil

	// dib8000_init
	if err := board.demod.initialize(); err != nil {
		return err
	}

	// dib0090_register
	if err := board.tuner.register(); err != nil {
		return err
	}

	// The frontend and tuner init a DVB frontend runs when it is opened.
	board.demod.wakeup()
	board.tuner.wakeup()
	board.asleep = false

	if err := board.demod.setOutputMode(board.demod.cfg.outputMode); err != nil {
		return err
	}
	return board.failure.err
}

// SetOutputMode sets the transport stream output and makes it the mode each
// Tune ends in. Tune holds the output in high impedance while it runs.
func (board *Demod) SetOutputMode(mode OutputMode) (err error) {
	defer board.flush(&err)
	board.failure.err = nil
	outmode, err := mode.outmode()
	if err != nil {
		return err
	}
	board.demod.cfg.outputMode = outmode
	return board.demod.setOutputMode(outmode)
}

// SetGPIO drives one of the DiB8000's GPIO pins as an output, for boards that
// wire a spare pin to something of their own, such as an indicator LED. Pins
// 0 and 5 belong to the tuner and pin 3 to its band switch. Initialize returns
// every pin to an input.
func (board *Demod) SetGPIO(pin uint8, high bool) (err error) {
	defer board.flush(&err)
	board.failure.err = nil
	board.demod.setGPIO(pin, 0, uint8(boolWord(high)))
	return board.failure.err
}

// Tune tunes to the 6MHz ISDB-T channel centred on frequencyHz and runs the
// demodulator's search for its parameters to the end, returning once it has
// locked or given up; Status tells which. Up to 170MHz (CBAND, VHF 1-3) the
// gain is first found by the tuner's software AGC, see setParamOverride.
func (board *Demod) Tune(frequencyHz uint32) (err error) {
	if board.useMemory {
		return board.tuneRemembered(frequencyHz)
	}
	defer board.flush(&err)
	board.failure.err = nil

	if frequencyHz < minimumFrequencyHz || frequencyHz > maximumFrequencyHz {
		return fmt.Errorf("DiB8096: %d Hz is outside %d-%d Hz", frequencyHz, minimumFrequencyHz, maximumFrequencyHz)
	}
	if board.asleep {
		board.demod.wakeup()
		board.tuner.wakeup()
		board.asleep = false
	}

	// A DVBv5 tune from a frequency alone starts from a cleared cache.
	board.channel.clear()
	board.channel.frequency = frequencyHz

	if err := board.demod.setFrontend(); err != nil {
		return err
	}
	return board.failure.err
}

// setParamOverride is dib8096_set_param_override, installed as the tuner's
// set_params: it switches the VHF/UHF input, tunes the tuner, moves the
// demodulator's PLL ratio away from spurs near the channel, and hands the gain
// to the PWM AGC.
func (board *Demod) setParamOverride() error {
	demod, tuner, c := board.demod, board.tuner, &board.channel
	band := bandOfFrequency(c.frequency / 1000)

	switch band {
	case bandUHF:
		demod.setGPIO(3, 0, 0)
	default:
		// Linux warns that the frequency is out of the supported range and
		// uses the VHF switch.
		demod.setGPIO(3, 0, 1)
	}

	// 温度補償は同じ個体のUHFで10秒以内だけ再利用する。通信失敗で破棄する。
	board.temperatureLastReused = false
	needsTemperature := tuner.calibrate&tempCal != 0
	age := demod.now().Sub(board.temperatureLast)
	if board.reuseTemperature && board.temperatureValid && band == bandUHF && age >= 0 && age < 10*time.Second && needsTemperature {
		tuner.calibrate &^= tempCal
		board.temperatureLastReused = true
	}
	if err := tuner.setParams(); err != nil {
		board.temperatureValid = false
		board.tuner.capMemory = [64]capMemoryEntry{}
		return err
	}
	if needsTemperature && !board.temperatureLastReused {
		board.temperatureLast = demod.now()
		board.temperatureValid = true
	}

	if c.bandwidthHz != 6000000 {
		return fmt.Errorf("DiB8096: only 6MHz bandwidth is supported")
	}

	// Update PLL if needed ratio
	demod.updatePLL(&dib8090PLLConfig12MHz, c.bandwidthHz/1000, 0)

	// Get optimize PLL ratio to remove spurious
	pllRatio := dib8090ComputePLLParameters(c.frequency)
	var timf uint32
	switch pllRatio {
	case 17:
		timf = 21387946
	case 18:
		timf = 20199727
	case 19:
		timf = 19136583
	default:
		timf = 18179756
	}

	// Update ratio
	demod.updatePLL(&dib8090PLLConfig12MHz, c.bandwidthHz/1000, pllRatio)

	demod.ctrlTimf(timfSet, timf)

	if band != bandCBand {
		// dib0090_get_wbd_target is returning any possible temperature
		// compensated wbd-target
		target := (int(tuner.getWBDTarget())*8*18/33 + 1) / 2
		demod.setWBDRef(uint16(target))
	}

	if band == bandCBand {
		// tuning in CBAND - soft-AGC startup
		tuner.tuneState = agcStart
		for {
			wait := tuner.gainControl()
			tuner.msleep(wait)
			if board.failure.err != nil {
				return board.failure.err
			}
			switch tuner.tuneState {
			case agcStep0:
				demod.setGPIO(6, 0, 1)
			case agcStep1:
				// activate the external attenuator in case of very high input power
				if uint16(tuner.rfGainLimit) < 2000 {
					demod.setGPIO(6, 0, 0)
				}
			}
			if tuner.tuneState >= agcStop {
				break
			}
		}

		// switching to PWM AGC
		tuner.pwmGainReset()
		demod.pwmAGCReset()
		demod.tuneState = demodStart
	} else {
		// for everything else than CBAND we are using standard AGC
		tuner.pwmGainReset()
	}

	return board.failure.err
}

// dib8090ComputePLLParameters is dib8090_compute_pll_parameters: of the PLL
// ratios 17 to 20, the one whose ADC clock harmonics fall furthest from the
// channel.
func dib8090ComputePLLParameters(frequencyHz uint32) uint8 {
	optimalPLLRatio := uint8(20)
	var max uint32

	for pllRatio := uint8(17); pllRatio <= 20; pllRatio++ {
		freqADC := uint32(12 * int(pllRatio) * (1 << 8) / 16)
		ratio := ((frequencyHz / 1000) * (1 << 8) / 1000) / freqADC
		rest := ((frequencyHz / 1000) * (1 << 8) / 1000) - ratio*freqADC

		if rest > freqADC/2 {
			rest = freqADC - rest
		}
		if rest > max && rest > 717 {
			optimalPLLRatio = pllRatio
			max = rest
		}
	}

	return optimalPLLRatio
}

// Status is the demodulator's lock state and signal readings.
type Status struct {
	Locked bool
	// CNR is the carrier-to-noise ratio in dB, known only once locked.
	CNR    float64
	HasCNR bool
	// StrengthDBm estimates the input level from the AGC through a table Linux
	// measured on a different DiB8000 board.
	StrengthDBm float64
	HasStrength bool
	// Names lists the frontend status bits that are set, in DVB order:
	// SIGNAL, CARRIER, VITERBI, SYNC, LOCK.
	Names []string
}

// Status reads the lock state (dib8000_read_status) and the signal strength
// and CNR that dib8000_get_stats derives alongside it.
func (board *Demod) Status() (_ Status, err error) {
	defer board.flush(&err)
	board.failure.err = nil
	demod := board.demod

	stat := demod.readStatus()

	var status Status
	for _, bit := range []struct {
		mask int
		name string
	}{
		{feHasSignal, "SIGNAL"},
		{feHasCarrier, "CARRIER"},
		{feHasViterbi, "VITERBI"},
		{feHasSync, "SYNC"},
		{feHasLock, "LOCK"},
	} {
		if stat&bit.mask != 0 {
			status.Names = append(status.Names, bit.name)
		}
	}
	status.Locked = stat&feHasLock != 0

	// Signal strength, in thousandths of a dBm.
	strength := demod.readSignalStrength()
	db := int32(interpolateValue(uint32(strength), strengthToDBTable)) - 131000
	status.StrengthDBm = float64(db) / 1000
	status.HasStrength = true

	// CNR requires lock. Linux takes 10*intlog10 of the 16.16 SNR's integer
	// part; the float logarithm is the exact value it approximates.
	if status.Locked {
		snr := demod.getSNR() >> 16
		if snr != 0 {
			status.CNR = 10 * math.Log10(float64(snr))
		}
		status.HasCNR = true
	}

	if err := board.failure.err; err != nil {
		return Status{}, err
	}
	return status, nil
}

// Sleep puts the tuner to sleep, then the demodulator (dib8000_sleep: output
// high impedance, only the interface powered, ADCs off). The next Tune wakes
// both.
func (board *Demod) Sleep() (err error) {
	defer board.flush(&err)
	board.failure.err = nil

	board.tuner.sleepTuner()
	if err := board.demod.sleepChip(); err != nil {
		return err
	}
	board.asleep = true
	return board.failure.err
}
