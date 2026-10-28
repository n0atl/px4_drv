// Copyright 2026 Till0196
// Copyright (C) 2009 DiBcom (http://www.dibcom.fr/)
// SPDX-License-Identifier: GPL-2.0-only
//
// Ported from the Linux media subsystem's dib8000.c: ISDB-T channel setup,
// parameter autosearch, the tune state machine, set_frontend and the status
// readers.
//
// Only full-band (13-segment) ISDB-T is ported. The ISDB-Tsb sound
// broadcasting branches (isdbt_sb_mode set) are left out; nothing here ever
// sets that mode, so they could not be reached.

package dib8096

import (
	"fmt"
	"math/bits"
	"time"
)

// Values of the DVB API enums the driver stores and compares, with the
// numbers linux/dvb/frontend.h gives them: some are written to registers or
// used in arithmetic.
const (
	// enum fe_modulation
	modulationQPSK    = 0
	modulationQAM16   = 1
	modulationQAM32   = 2
	modulationQAM64   = 3
	modulationQAMAuto = 6
	modulationDQPSK   = 12

	// enum fe_code_rate
	fecNone = 0
	fec1_2  = 1
	fec2_3  = 2
	fec3_4  = 3
	fec4_5  = 4
	fec5_6  = 5
	fec6_7  = 6
	fec7_8  = 7
	fecAuto = 9

	// enum fe_transmit_mode
	transmission2K   = 0
	transmission8K   = 1
	transmissionAuto = 2
	transmission4K   = 3

	// enum fe_guard_interval
	guard1_32     = 0
	guard1_16     = 1
	guard1_8      = 2
	guard1_4      = 3
	guardAuto     = 4
	inversionAuto = 2

	// enum fe_delivery_system
	systemISDBT = 8
)

// Frontend status bits, enum fe_status.
const (
	feHasSignal  = 0x01
	feHasCarrier = 0x02
	feHasViterbi = 0x04
	feHasSync    = 0x08
	feHasLock    = 0x10
)

// layerParameters is one ISDB-T hierarchical layer of the property cache.
type layerParameters struct {
	fec          int
	modulation   int
	interleaving uint8
	segmentCount uint8
}

// channel is the part of struct dtv_frontend_properties the DiB8000 and
// DiB0090 read and write: Linux shares one such cache between demodulator,
// tuner and board code, and so do these.
type channel struct {
	deliverySystem        int
	frequency             uint32 // Hz
	bandwidthHz           uint32
	inversion             int
	transmissionMode      int
	guardInterval         int
	isdbtPartialReception uint8
	isdbtSbMode           uint8
	isdbtLayerEnabled     uint8
	layer                 [3]layerParameters
}

// clear is dvb_frontend_clear_cache for an ISDB-T frontend: what the cache
// holds when the caller knows nothing but the frequency.
func (c *channel) clear() {
	*c = channel{
		deliverySystem:    systemISDBT,
		transmissionMode:  transmissionAuto,
		guardInterval:     guardAuto,
		inversion:         inversionAuto,
		isdbtLayerEnabled: 7,
	}
	for i := range c.layer {
		c.layer[i] = layerParameters{fec: fecAuto, modulation: modulationQAMAuto}
	}
}

// Loop steps of dib8000_set_isdbt_loop_params, enum param_loop_step.
const (
	loopTune1 = iota
	loopTune2
)

// setLayer is dib8000_set_layer: it programs one layer and returns the
// densest constellation seen so far.
func (state *dib8000) setLayer(layerIndex uint8, maxConstellation int) int {
	c := state.channel
	layer := &c.layer[layerIndex]
	var constellation, cr uint16

	switch layer.modulation {
	case modulationDQPSK:
		constellation = 0
	case modulationQPSK:
		constellation = 1
	case modulationQAM16:
		constellation = 2
	default:
		constellation = 3
	}

	switch layer.fec {
	case fec1_2:
		cr = 1
	case fec2_3:
		cr = 2
	case fec3_4:
		cr = 3
	case fec5_6:
		cr = 5
	default:
		cr = 7
	}

	timeIntlv := uint16(bits.Len8(layer.interleaving)) // fls()
	if timeIntlv > 3 && !(timeIntlv == 4 && c.isdbtSbMode == 1) {
		timeIntlv = 0
	}

	state.writeWord(2+uint16(layerIndex), constellation<<10|(uint16(layer.segmentCount)&0xf)<<6|cr<<3|timeIntlv)
	if layer.segmentCount > 0 {
		switch maxConstellation {
		case modulationDQPSK, modulationQPSK:
			if layer.modulation == modulationQAM16 || layer.modulation == modulationQAM64 {
				maxConstellation = layer.modulation
			}
		case modulationQAM16:
			if layer.modulation == modulationQAM64 {
				maxConstellation = layer.modulation
			}
		}
	}

	return maxConstellation
}

// adpFineTune is dib8000_adp_fine_tune: channel estimation fine configuration.
func (state *dib8000) adpFineTune(maxConstellation int) uint16 {
	var anaGain uint16
	var adp *[4]uint16

	switch maxConstellation {
	case modulationQAM64:
		anaGain = 0x7
		adp = &adpQ64
	case modulationQAM16:
		anaGain = 0x7
		adp = &adpQ16
	default:
		anaGain = 0
		adp = &adpQDefault
	}

	for i := uint16(0); i < 4; i++ {
		state.writeWord(215+i, adp[i])
	}

	return anaGain
}

// updateAnaGain is dib8000_update_ana_gain.
func (state *dib8000) updateAnaGain(anaGain uint16) {
	state.writeWord(116, anaGain)

	// update ADC target depending on ana_gain
	if anaGain != 0 { // set -16dB ADC target for ana_gain=-1
		for i := uint16(0); i < 10; i++ {
			state.writeWord(80+i, adcTarget16dB[i])
		}
	} else { // set -22dB ADC target for ana_gain=0
		for i := uint16(0); i < 10; i++ {
			state.writeWord(80+i, adcTarget16dB[i]-355)
		}
	}
}

// loadAnaFECoefs is dib8000_load_ana_fe_coefs.
func (state *dib8000) loadAnaFECoefs(anaFE *[24]int16) {
	if state.isdbtCfgLoaded == 0 {
		for mode := uint16(0); mode < 24; mode++ {
			state.writeWord(117+mode, uint16(anaFE[mode]))
		}
	}
}

// set13SegChannel is dib8000_set_13seg_channel.
func (state *dib8000) set13SegChannel() {
	coffPow := uint16(0x2800)

	state.segMask = 0x1fff // All 13 segments enabled

	// ---- COFF ---- Carloff, the most robust ---
	if state.isdbtCfgLoaded == 0 { // if not Sound Broadcasting mode: put default values for 13 segments
		state.writeWord(180, 16<<6|9)
		state.writeWord(187, 4<<12|8<<5|0x2)
		coffPow = 0x2800
		for i := uint16(0); i < 6; i++ {
			state.writeWord(181+i, coffPow)
		}

		// P_ctrl_corm_thres4pre_freq_inh=1, P_ctrl_pre_freq_mode_sat=1
		// P_ctrl_pre_freq_mode_sat=1, P_ctrl_pre_freq_inh=0, P_ctrl_pre_freq_step = 3, P_pre_freq_win_len=1
		state.writeWord(338, 1<<12|1<<10|0<<9|3<<5|1)

		// P_ctrl_pre_freq_win_len=8, P_ctrl_pre_freq_thres_lockin=6
		state.writeWord(340, 8<<6|6<<0)
		// P_ctrl_pre_freq_thres_lockout=4, P_small_use_tmcc/ac/cp=1
		state.writeWord(341, 4<<3|1<<2|1<<1|1<<0)

		state.writeWord(228, 0)      // default value
		state.writeWord(265, 31)     // default value
		state.writeWord(205, 0x200f) // init value
	}

	// make the cpil_coff_lock more robust but slower p_coff_winlen
	// 6bits; p_coff_thres_lock 6bits (for coff lock if needed)

	if state.cfg.pll.ifreq == 0 {
		state.writeWord(266, ^state.segMask|state.segDiffMask|0x40) // P_equal_noise_seg_inh
	}

	state.loadAnaFECoefs(&anaFECoeff13Seg)
}

// setSubchannelPRBS is dib8000_set_subchannel_prbs.
func (state *dib8000) setSubchannelPRBS(initPRBS uint16) {
	reg1 := state.readWord(1)
	state.writeWord(1, initPRBS<<2|reg1&0x3) // ADDR 1
}

// smallFineTune is dib8000_small_fine_tune. The coefficient block it loads
// for ISDB-Tsb is left out.
func (state *dib8000) smallFineTune() {
	c := state.channel

	state.writeWord(352, state.segDiffMask)
	state.writeWord(353, state.segMask)

	// P_small_coef_ext_enable=ISDB-Tsb, P_small_narrow_band=ISDB-Tsb, P_small_last_seg=13, P_small_offset_num_car=5
	state.writeWord(351, uint16(c.isdbtSbMode)<<9|uint16(c.isdbtSbMode)<<8|13<<4|5)
}

// setISDBTCommonChannel is dib8000_set_isdbt_common_channel.
func (state *dib8000) setISDBTCommonChannel(seq uint8, autosearching bool) {
	var pCfrLeftEdge, pCfrRightEdge uint16
	var tmccPow, nbsegDiff uint16
	maxConstellation := modulationDQPSK
	c := state.channel

	if autosearching {
		c.isdbtPartialReception = 1
	}

	// P_mode
	state.writeWord(10, uint16(seq)<<4)

	// init mode
	state.mode = state.fftToMode()

	// set guard
	tmp := state.readWord(1)
	state.writeWord(1, tmp&0xfffc|uint16(c.guardInterval)&0x3)

	state.modifyWord(274, 0xffcf, uint16(c.isdbtPartialReception&1)<<5|uint16(c.isdbtSbMode&1)<<4)

	// signal optimization parameter
	if c.isdbtPartialReception != 0 {
		state.segDiffMask = boolWord(c.layer[0].modulation == modulationDQPSK) << permuSeg[0]
		for i := 1; i < 3; i++ {
			nbsegDiff += boolWord(c.layer[i].modulation == modulationDQPSK) * uint16(c.layer[i].segmentCount)
		}
		// Segment counts are four-bit fields, so a bad TMCC reading can ask
		// for more than the table holds; C reads past its end, this stops.
		for i := uint16(0); i < nbsegDiff && int(i)+1 < len(permuSeg); i++ {
			state.segDiffMask |= 1 << permuSeg[i+1]
		}
	} else {
		// Linux accumulates into the mask left by the previous call here.
		for i := 0; i < 3; i++ {
			nbsegDiff += boolWord(c.layer[i].modulation == modulationDQPSK) * uint16(c.layer[i].segmentCount)
		}
		for i := uint16(0); i < nbsegDiff && int(i) < len(permuSeg); i++ {
			state.segDiffMask |= 1 << permuSeg[i]
		}
	}

	if state.segDiffMask != 0 {
		state.modifyWord(268, 0xf9ff, 0x0200)
	} else {
		state.writeWord(268, 2<<9|39) // init value
	}

	for i := uint8(0); i < 3; i++ {
		maxConstellation = state.setLayer(i, maxConstellation)
	}
	if !autosearching {
		state.layerBNbSeg = c.layer[1].segmentCount
		state.layerCNbSeg = c.layer[2].segmentCount
	}

	// WRITE: Mode & Diff mask
	state.writeWord(0, state.mode<<13|state.segDiffMask)

	state.differentialConstellation = uint8(boolWord(state.segDiffMask != 0))

	// channel estimation fine configuration
	anaGain := state.adpFineTune(maxConstellation)

	// update ana_gain depending on max constellation
	state.updateAnaGain(anaGain)

	// ---- ANA_FE ----
	if c.isdbtPartialReception != 0 { // 3-segments
		state.loadAnaFECoefs(&anaFECoeff3Seg)
	} else {
		state.loadAnaFECoefs(&anaFECoeff1Seg) // 1-segment
	}

	// TSB or ISDBT? Only ISDB-T is ported.
	state.set13SegChannel()
	initPRBS := uint16(0xfff)

	// SMALL
	state.smallFineTune()

	state.setSubchannelPRBS(initPRBS)

	// ---- CHAN_BLK ----
	for i := uint(0); i < 13; i++ {
		if (^state.segDiffMask>>i)&1 == 1 {
			enabled := state.segMask & ^state.segDiffMask
			if i == 0 || (enabled>>(i-1))&1 == 0 {
				pCfrLeftEdge += 1 << i
			}
			if i == 12 || (enabled>>(i+1))&1 == 0 {
				pCfrRightEdge += 1 << i
			}
		}
	}
	state.writeWord(222, pCfrLeftEdge)  // p_cfr_left_edge
	state.writeWord(223, pCfrRightEdge) // p_cfr_right_edge
	// "P_cspu_left_edge" & "P_cspu_right_edge" not used => do not care

	state.writeWord(189, ^state.segMask|state.segDiffMask) // P_lmod4_seg_inh
	state.writeWord(192, ^state.segMask|state.segDiffMask) // P_pha3_seg_inh
	state.writeWord(225, ^state.segMask|state.segDiffMask) // P_tac_seg_inh

	if !autosearching {
		state.writeWord(288, (^state.segMask|state.segDiffMask)&0x1fff) // P_tmcc_seg_eq_inh
	} else {
		// Disable equalisation of the TMCC during autosearch so the DQPSK
		// channels can be found.
		state.writeWord(288, 0x1fff)
	}

	state.writeWord(211, state.segMask&^state.segDiffMask) // P_des_seg_enabled
	state.writeWord(287, ^state.segMask|0x1000)            // P_tmcc_seg_inh

	state.writeWord(178, 32) // P_fft_powrange = 32

	// ---- TMCC ----
	for i := 0; i < 3; i++ {
		tmccPow += (boolWord(c.layer[i].modulation == modulationDQPSK)*4 + 1) * uint16(c.layer[i].segmentCount)
	}

	// Quantif of "P_tmcc_dec_thres_?k" is (0, 5+mode, 9);
	// Threshold is set at 1/4 of max power.
	tmccPow *= 1 << (9 - 2)
	state.writeWord(290, tmccPow) // P_tmcc_dec_thres_2k
	state.writeWord(291, tmccPow) // P_tmcc_dec_thres_4k
	state.writeWord(292, tmccPow) // P_tmcc_dec_thres_8k

	// ---- PHA3 ----
	if state.isdbtCfgLoaded == 0 {
		state.writeWord(250, 3285) // p_2d_hspeed_thr0
	}

	state.isdbtCfgLoaded = 0
}

func boolWord(value bool) uint16 {
	if value {
		return 1
	}
	return 0
}

// waitLock is dib8000_wait_lock: it programs P_search_end0..2 in units of the
// internal clock and returns the last value written.
func (state *dib8000) waitLock(internal, wait0ms, wait1ms, wait2ms uint32) uint32 {
	var value uint32 // P_search_end0 wait time

	for register := uint16(11); register < 16; register += 2 {
		switch register {
		case 11:
			value = internal * wait0ms
		case 13:
			value = internal * wait1ms
		case 15:
			value = internal * wait2ms
		}
		state.writeWord(register, uint16((value>>16)&0xffff))
		state.writeWord(register+1, uint16(value&0xffff))
	}
	return value
}

// autosearchStart is dib8000_autosearch_start. From revision 0x8002 on, the
// search runs in two passes, FFT size then guard interval; older revisions
// search a fixed list in one.
func (state *dib8000) autosearchStart() {
	c := state.channel
	var slist uint8
	internal := state.cfg.pll.internal

	if state.revision >= 0x8002 && state.autosearchState == autosearchSearchingFFT {
		state.writeWord(37, 0x0065)  // P_ctrl_pha_off_max default values
		state.writeWord(116, 0x0000) // P_ana_gain to 0

		state.modifyWord(0, 0x1fff, 0<<13|1<<15) // P_mode = 0, P_restart_search=1
		state.modifyWord(1, 0xfffc, 0)           // P_guard = 0
		state.writeWord(6, 0)                    // P_lock0_mask = 0
		state.writeWord(7, 0)                    // P_lock1_mask = 0
		state.writeWord(8, 0)                    // P_lock2_mask = 0
		state.modifyWord(10, 0x200, 16<<4|0<<0)  // P_search_list=16, P_search_maxtrial=0

		// time in ms configure P_search_end0 P_search_end1 P_search_end2
		value := state.waitLock(internal, 20, 20, 20)

		state.writeWord(17, 0)
		state.writeWord(18, 200) // P_search_rstst = 200
		state.writeWord(19, 0)
		state.writeWord(20, 400)                        // P_search_rstend = 400
		state.writeWord(21, uint16((value>>16)&0xffff)) // P_search_checkst
		state.writeWord(22, uint16(value&0xffff))

		state.modifyWord(32, 0xf0ff, 9<<8) // P_corm_alpha = 3
		state.writeWord(355, 2)            // P_search_param_max = 2

		// P_search_param_select = (1 | 1<<4 | 1 << 8)
		state.writeWord(356, 0)
		state.writeWord(357, 0x111)

		state.modifyWord(770, 0xdfff, 1<<13)    // P_restart_ccg = 1
		state.modifyWord(770, 0xdfff, 0<<13)    // P_restart_ccg = 0
		state.modifyWord(0, 0x7ff, 0<<15|1<<13) // P_restart_search = 0;
	} else if state.revision >= 0x8002 && state.autosearchState == autosearchSearchingGuard {
		c.transmissionMode = transmission8K
		c.guardInterval = guard1_8
		c.inversion = 0
		c.layer[0].modulation = modulationQAM64
		c.layer[0].fec = fec2_3
		c.layer[0].interleaving = 0
		c.layer[0].segmentCount = 13

		slist = 16
		c.transmissionMode = state.foundNFFT

		state.setISDBTCommonChannel(slist, true)

		// set lock_mask values
		state.writeWord(6, 0x4)
		state.writeWord(7, 0x8)
		state.writeWord(8, 0x1000)

		// set lock_mask wait time values
		state.waitLock(internal, 50, 200, 1000) // time in ms configure P_search_end0 P_search_end1 P_search_end2

		state.writeWord(355, 3) // P_search_param_max = 3

		// P_search_param_select = 0xf; look for the 4 different guard intervals
		state.writeWord(356, 0)
		state.writeWord(357, 0xf)

		value := state.readWord(0)
		state.writeWord(0, 1<<15|value)
		state.readWord(1284) // reset the INT. n_irq_pending
		state.writeWord(0, value)
	} else {
		c.inversion = 0
		c.layer[0].modulation = modulationQAM64
		c.layer[0].fec = fec2_3
		c.layer[0].interleaving = 0
		c.layer[0].segmentCount = 13

		// choose the right list
		if c.guardInterval == guardAuto {
			if c.transmissionMode == transmissionAuto {
				c.transmissionMode = transmission8K
				c.guardInterval = guard1_8
				slist = 7
				state.modifyWord(0, 0x9fff, 1<<13) // P_mode = 1 to have autosearch start ok with mode2
			} else {
				c.guardInterval = guard1_8
				slist = 3
			}
		} else {
			if c.transmissionMode == transmissionAuto {
				c.transmissionMode = transmission8K
				slist = 2
				state.modifyWord(0, 0x9fff, 1<<13) // P_mode = 1
			} else {
				slist = 0
			}
		}

		state.setISDBTCommonChannel(slist, true)

		// set lock_mask values
		state.writeWord(6, 0x4)
		state.writeWord(7, 0x8)
		state.writeWord(8, 0x1000)

		// set lock_mask wait time values
		state.waitLock(internal, 50, 100, 1000) // time in ms configure P_search_end0 P_search_end1 P_search_end2

		value := state.readWord(0)
		state.writeWord(0, 1<<15|value)
		state.readWord(1284) // reset the INT. n_irq_pending
		state.writeWord(0, value)
	}
}

// autosearchIRQ is dib8000_autosearch_irq: 0 still pending, 1 failed,
// 2 succeeded, 3 the FFT correlation pass has finished.
func (state *dib8000) autosearchIRQ() int {
	irqPending := state.readWord(1284)

	if state.revision >= 0x8002 && state.autosearchState == autosearchSearchingFFT {
		if irqPending&0x1 != 0 {
			return 3
		}
	} else {
		if irqPending&0x1 != 0 { // failed
			return 1
		}
		if irqPending&0x2 != 0 { // succeeded
			return 2
		}
	}

	return 0 // still pending
}

// viterbiState is dib8000_viterbi_state: P_restart_chd, the channel decoder.
func (state *dib8000) viterbiState(on bool) {
	tmp := state.readWord(771)
	if on {
		state.writeWord(771, tmp&0xfffd)
	} else {
		state.writeWord(771, tmp|1<<1)
	}
}

// setDDS is dib8000_set_dds.
func (state *dib8000) setDDS(offsetKHz int32) {
	absOffsetKHz := uint32(offsetKHz)
	if offsetKHz < 0 {
		absOffsetKHz = uint32(-offsetKHz)
	}
	dds := state.cfg.pll.ifreq & 0x1ffffff
	invert := uint16(0)
	if state.cfg.pll.ifreq&(1<<25) != 0 {
		invert = 1
	}
	ratio := uint32(2)

	unitKHzDDSVal := int32(67108864 / state.cfg.pll.internal)

	if offsetKHz < 0 {
		unitKHzDDSVal *= -1
	}

	// IF tuner. The product is unsigned in C, so a negative unit wraps.
	if invert != 0 {
		dds -= absOffsetKHz * uint32(unitKHzDDSVal)
	} else {
		dds += absOffsetKHz * uint32(unitKHzDDSVal)
	}

	if absOffsetKHz <= state.cfg.pll.internal/ratio {
		// Max dds offset is the half of the demod freq
		state.writeWord(26, invert)
		state.writeWord(27, uint16(dds>>16)&0x1ff)
		state.writeWord(28, uint16(dds&0xffff))
	}
}

// setFrequencyOffset is dib8000_set_frequency_offset: it moves the DDS by the
// difference between the requested frequency and the one the tuner actually
// set, which on this board includes its fixed per-band offset.
func (state *dib8000) setFrequencyOffset() {
	c := state.channel
	var currentRF uint32

	if state.tunerFrequency != nil {
		currentRF = state.tunerFrequency()
	} else {
		currentRF = c.frequency
	}
	currentRF /= 1000
	totalDDSOffsetKHz := int32(currentRF) - int32(c.frequency)/1000

	state.setDDS(totalDDSOffsetKHz)
}

// isdbtSymbolDuration is LUT_isdbt_symbol_duration.
var isdbtSymbolDuration = [4]uint16{26, 101, 63}

// getSymbolDuration is dib8000_get_symbol_duration, in 100us units. For a
// 6MHz channel the division truncates to zero and every mode gives 1.
func (state *dib8000) getSymbolDuration() uint32 {
	c := state.channel
	var i int

	switch c.transmissionMode {
	case transmission2K:
		i = 0
	case transmission4K:
		i = 2
	default:
		i = 1
	}

	return uint32(isdbtSymbolDuration[i])/(c.bandwidthHz/1000) + 1
}

// setISDBTLoopParams is dib8000_set_isdbt_loop_params for 13 segments.
func (state *dib8000) setISDBTLoopParams(loopStep int) {
	var reg32, reg37 uint16

	switch loopStep {
	case loopTune1:
		// 13-seg start conf offset loop parameters
		reg32 = (9-state.mode)<<12 | 6<<8 | 0x80 // P_timf_alpha = (9-P_mode, P_corm_alpha=6, P_corm_thres=0x80
		reg37 = 3<<5 | 0<<4 | (8 - state.mode)   // P_ctrl_pha_off_max=3   P_ctrl_sfreq_inh =0  P_ctrl_sfreq_step = 9
	case loopTune2:
		reg32 = (11-state.mode)<<12 | 6<<8 | 0x80 // P_timf_alpha = 8 , P_corm_alpha=6, P_corm_thres=0x80
		reg37 = (5+state.mode)<<5 | (10 - state.mode)
	}
	state.writeWord(32, reg32)
	state.writeWord(37, reg37)
}

// demodRestart is dib8000_demod_restart.
func (state *dib8000) demodRestart() {
	state.writeWord(770, 0x4000)
	state.writeWord(770, 0x0000)
}

// setSyncWait is dib8000_set_sync_wait: P_dvsy_sync_wait, reusing the mode.
func (state *dib8000) setSyncWait() {
	c := state.channel
	var syncWait uint16

	switch c.transmissionMode {
	case transmission8K:
		syncWait = 256
	case transmission4K:
		syncWait = 128
	default:
		syncWait = 64
	}

	// Add 50% SFN margin and compensate for the DVSY FIFO.
	if state.cfg.diversityDelay == 0 {
		syncWait = uint16((int(syncWait)*(1<<uint(c.guardInterval))*3)/2 + 48)
	} else {
		syncWait = uint16((int(syncWait)*(1<<uint(c.guardInterval))*3)/2 + int(state.cfg.diversityDelay))
	}

	state.modifyWord(273, 0x000f, syncWait<<4)
}

// minTuneStep is the least the tune loop waits between steps. Linux sleeps in
// 100ms steps until the callback time, so every step waited at least that
// long, and some steps need more than the callback time they ask for:
// started 1ms after the FFT search ends, the guard search can report a
// failure. 10ms held on every try, on the CECH-ZD1 J under Linux and Windows.
const minTuneStep = 10 * time.Millisecond

// getTimeout is dib8000_get_timeout: delay is in 100us units, or in symbols
// when symbolDependent.
func (state *dib8000) getTimeout(delay uint32, symbolDependent bool) time.Time {
	if symbolDependent {
		delay *= uint32(state.symbolDuration)
	}
	return state.now().Add(time.Duration(delay) * 100 * time.Microsecond)
}

// readLock is dib8000_read_lock.
func (state *dib8000) readLock() uint16 {
	return state.readWord(568)
}

// isManualMode is is_manual_mode: whether every parameter is known, so the
// autosearch can be skipped. With only a frequency known it never is.
func isManualMode(c *channel) bool {
	nSegs := 0

	// Use auto mode on DVB-T compat mode
	if c.deliverySystem != systemISDBT {
		return false
	}

	// Transmission mode and guard interval are only detected in auto mode.
	if c.transmissionMode == transmissionAuto {
		return false
	}
	if c.guardInterval == guardAuto {
		return false
	}

	// If no layer is enabled, assume auto mode, as at least one layer
	// should be enabled.
	if c.isdbtLayerEnabled == 0 {
		return false
	}

	// Check that the per-layer parameters are not auto, and disable a layer
	// whose segment count is 0 or invalid.
	for i := 0; i < 3; i++ {
		if c.isdbtLayerEnabled&(1<<i) == 0 {
			continue
		}
		if c.layer[i].segmentCount > 13 || c.layer[i].segmentCount == 0 {
			c.isdbtLayerEnabled &^= 1 << i
			continue
		}

		nSegs += int(c.layer[i].segmentCount)

		if c.layer[i].modulation == modulationQAMAuto || c.layer[i].fec == fecAuto {
			return false
		}
	}

	// A wrong number of segments falls back to auto mode.
	if nSegs == 0 || nSegs > 13 {
		return false
	}

	return true
}

// tune is dib8000_tune, one step of the demodulator's state machine. It
// returns the time to wait before the next step in 100us units, or 0 once the
// machine has stopped.
func (state *dib8000) tune() int {
	c := state.channel
	var locks, deeperInterleaver uint16
	ret := 1 // 1 symbol duration (in 100us unit) delay most of the time

	now := state.now()

	switch state.tuneState {
	case demodStart: // 30
		state.resetStats()

		state.status = statusTunePending
		if isManualMode(c) {
			state.channelParametersSet = 1
		} else {
			state.channelParametersSet = 0
		}

		state.viterbiState(false) // force chan dec in restart

		// Layer monitor
		state.modifyWord(285, 0x60, 0)

		state.setFrequencyOffset()
		state.setBandwidth(c.bandwidthHz / 1000)

		if state.channelParametersSet == 0 { // The channel struct is unknown, search it!
			state.autosearchState = autosearchSearchingFFT
			state.foundNFFT = transmissionAuto
			state.foundGuard = guardAuto
			state.tuneState = demodSearchNext
		} else { // we already know the channel struct so TUNE only!
			state.autosearchState = autosearchDone
			state.tuneState = demodStep3
		}
		state.symbolDuration = uint8(state.getSymbolDuration())

	case demodSearchNext: // 51
		state.autosearchStart()
		ret = 15
		state.tuneState = demodStep1

	case demodStep1: // 31
		switch state.autosearchIRQ() {
		case 1: // fail
			state.status = statusTuneFailed
			state.autosearchState = autosearchDone
			state.tuneState = demodStop // else we are done here
		case 2: // Success
			// Signal to the upper layer that a channel was found and the
			// parameters can be read.
			state.status = statusFFTSuccess
			state.tuneState = demodStep3
			if state.autosearchState == autosearchSearchingGuard {
				state.tuneState = demodStep2
			} else {
				state.autosearchState = autosearchDone
			}
		case 3: // Autosearch FFT max correlation ended
			state.tuneState = demodStep2
		}

	case demodStep2:
		switch state.autosearchState {
		case autosearchSearchingFFT:
			// searching for the correct FFT
			var corm [4]uint32
			corm[2] = uint32(state.readWord(594))<<16 | uint32(state.readWord(595))
			corm[1] = uint32(state.readWord(596))<<16 | uint32(state.readWord(597))
			corm[0] = uint32(state.readWord(598))<<16 | uint32(state.readWord(599))

			maxValue := 0
			for findIndex := 1; findIndex < 3; findIndex++ {
				if corm[maxValue] < corm[findIndex] {
					maxValue = findIndex
				}
			}

			switch maxValue {
			case 0:
				state.foundNFFT = transmission2K
			case 1:
				state.foundNFFT = transmission4K
			default:
				state.foundNFFT = transmission8K
			}

			state.tuneState = demodSearchNext
			state.autosearchState = autosearchSearchingGuard
			ret = 10
		case autosearchSearchingGuard:
			// searching for the correct guard interval
			state.foundGuard = int(state.readWord(570) & 0x3)

			state.tuneState = demodStep3
		default:
			// the demod should never be in this state
			state.status = statusTuneFailed
			state.autosearchState = autosearchDone
			state.tuneState = demodStop // else we are done here
		}

	case demodStep3: // 33
		state.setISDBTLoopParams(loopTune1)
		state.setISDBTCommonChannel(0, false) // setting the known channel parameters here
		state.tuneState = demodStep4

	case demodStep4: // (34)
		state.demodRestart()

		state.setSyncWait()
		state.setDiversityIn(state.diversityOnOff)

		locks = (state.readWord(180) >> 6) & 0x3f // P_coff_winlen ?
		// coff should lock over P_coff_winlen ofdm symbols : give 3 times this length to lock
		state.timeout = state.getTimeout(2*uint32(locks), true)
		state.tuneState = demodStep5

	case demodStep5: // (35)
		locks = state.readLock()
		if locks&(0x3<<11) != 0 { // coff-lock and off_cpil_lock achieved
			state.updateTimf() // we achieved a coff_cpil_lock - it's time to update the timf
			if state.differentialConstellation == 0 {
				// 2 times lmod4_win_len + 10 symbols (pipe delay after coff + nb to compute a 1st correlation)
				state.timeout = state.getTimeout(20*uint32((state.readWord(188)>>5)&0x1f), true)
				state.tuneState = demodStep7
			} else {
				state.tuneState = demodStep8
			}
		} else if now.After(state.timeout) {
			state.tuneState = demodStep6 // goto check for diversity input connection
		}

	case demodStep6: // (36) if there is an input (diversity)
		// There is no diversity frontend on this board.
		state.viterbiState(true) // start viterbi chandec
		state.setISDBTLoopParams(loopTune2)
		state.tuneState = demodStop // else we are done here ; step 8 will close the loops and exit
		state.status = statusTuneFailed

	case demodStep7: // 37
		locks = state.readLock()
		if locks&(1<<10) != 0 { // lmod4_lock
			ret = 14 // wait for 14 symbols
			state.tuneState = demodStep8
		} else if now.After(state.timeout) {
			state.tuneState = demodStep6 // goto check for diversity input connection
		}

	case demodStep8: // 38
		state.viterbiState(true) // start viterbi chandec
		state.setISDBTLoopParams(loopTune2)

		state.tuneState = demodStep9
		state.status = statusLocked

	case demodStep9: // 39
		if (state.readWord(1291)>>9)&0x1 != 0 { // fe capable of deinterleaving : esram
			// defines timeout for mpeg lock depending on interleaver length of longest layer
			for i := 0; i < 3; i++ {
				if uint16(c.layer[i].interleaving) >= deeperInterleaver {
					if c.layer[i].segmentCount > 0 { // valid layer
						// Linux takes layer A's interleaving here whichever
						// layer matched; kept.
						deeperInterleaver = uint16(c.layer[0].interleaving)
						state.longestIntlvLayer = uint8(i)
					}
				}
			}

			switch deeperInterleaver {
			case 0:
				locks = 2 // locks is the tmp local variable name
			case 3:
				locks = 8
			default:
				locks = 2 * deeperInterleaver
			}

			if state.diversityOnOff != 0 { // because of diversity sync
				locks *= 2
			}

			state.timeout = now.Add(time.Duration(200*int(locks)) * time.Millisecond) // give the mpeg lock 800ms if sram is present

			state.tuneState = demodStep10
		} else {
			state.tuneState = demodStop
		}

	case demodStep10: // 40
		locks = state.readLock()
		// 最長層を待たず、搬送波といずれかのMPEG層がロックしたら出力を許可する。
		if locks&(1<<(7-state.longestIntlvLayer)) != 0 || (state.earlyData && locks&0x1000 != 0 && locks&0x00e0 != 0) { // mpeg lock : check the longest one
			state.status = statusDataLocked
			state.tuneState = demodStop
		} else if now.After(state.timeout) {
			// The longest interleaver's layer did not lock; see whether
			// another one did in the meantime.
			if locks&(0x7<<5) != 0 {
				state.status = statusDataLocked
			} else {
				state.status = statusTuneFailed
			}
			state.tuneState = demodStop
		}
	}

	// tuning is finished - cleanup the demod
	if state.tuneState == demodStop { // (42)
		ret = 0
	}

	if ret > 0 && state.tuneState > demodStep3 {
		return ret * int(state.symbolDuration)
	}
	if ret > 0 && ret < int(state.symbolDuration) {
		return int(state.symbolDuration) // at least one symbol
	}
	return ret
}

// getFrontend is dib8000_get_frontend: it reads the parameters the
// autosearch found back into the channel.
func (state *dib8000) getFrontend() {
	c := state.channel

	c.bandwidthHz = 6000000

	// Called too early, this makes the tune either not lock or not sync;
	// so do nothing until the frontend has TMCC sync.
	if state.readStatus()&feHasSync == 0 {
		return
	}

	// Register 508 bit 0 is ISDB-Tsb mode. Linux copies it into the cache;
	// the sound broadcasting paths are not ported, so it is read and dropped.
	state.readWord(508)

	val := state.readWord(570)
	c.inversion = int((val & 0x40) >> 6)
	switch (val & 0x30) >> 4 {
	case 1:
		c.transmissionMode = transmission2K
	case 2:
		c.transmissionMode = transmission4K
	default:
		c.transmissionMode = transmission8K
	}

	switch val & 0x3 {
	case 0:
		c.guardInterval = guard1_32
	case 1:
		c.guardInterval = guard1_16
	case 2:
		c.guardInterval = guard1_8
	case 3:
		c.guardInterval = guard1_4
	}

	val = state.readWord(505)
	c.isdbtPartialReception = uint8(val & 1)

	for i := uint16(0); i < 3; i++ {
		val = state.readWord(493+i) & 0x0f
		c.layer[i].segmentCount = uint8(val)

		val = state.readWord(499+i) & 0x3
		// Interleaving can be 0, 1, 2 or 4
		if val == 3 {
			val = 4
		}
		c.layer[i].interleaving = uint8(val)

		val = state.readWord(481 + i)
		switch val & 0x7 {
		case 1:
			c.layer[i].fec = fec1_2
		case 2:
			c.layer[i].fec = fec2_3
		case 3:
			c.layer[i].fec = fec3_4
		case 5:
			c.layer[i].fec = fec5_6
		default:
			c.layer[i].fec = fec7_8
		}

		val = state.readWord(487 + i)
		switch val & 0x3 {
		case 0:
			c.layer[i].modulation = modulationDQPSK
		case 1:
			c.layer[i].modulation = modulationQPSK
		case 2:
			c.layer[i].modulation = modulationQAM16
		default:
			c.layer[i].modulation = modulationQAM64
		}
	}
}

// setFrontend is dib8000_set_frontend for a single frontend: tune the tuner,
// start the AGC, then run the demodulator's state machine to the end, waiting
// between its steps as minTuneStep says.
func (state *dib8000) setFrontend() error {
	c := state.channel

	if c.frequency == 0 {
		return fmt.Errorf("DiB8000: no frequency to tune to")
	}

	if c.bandwidthHz == 0 {
		c.bandwidthHz = 6000000
	}

	c.deliverySystem = systemISDBT

	// set output mode and diversity input
	state.setDiversityIn(1)
	if err := state.setOutputMode(outmodeHighZ); err != nil {
		return err
	}

	// tune the tuner
	if state.tunerSetParams != nil {
		if err := state.tunerSetParams(); err != nil {
			return err
		}
	}

	state.tuneState = agcStart

	// turn off the diversity of the last chip
	state.setDiversityIn(0)

	// start up the AGC
	for {
		wait := state.agcStartup()
		if wait == 0 || state.failure.err != nil {
			break
		}

		// agcStartup counts in 0.1ms, but Linux sleeps in whole jiffies and
		// found that anything below 10ms can keep the tuner from locking; so
		// it rounds up to 10ms steps.
		wait = 10 * (wait + 99) / 100
		state.msleep(wait)
		if state.tuneState == agcStop {
			break
		}
	}
	if state.failure.err != nil {
		return state.failure.err
	}

	state.tuneState = demodStart

	giveUp := state.now().Add(state.tuneTimeout)
	active := true
	for active {
		var callbackTime time.Time
		haveCallback := false

		delay := state.tune()
		if delay != 0 {
			callbackTime = state.now().Add(time.Duration(delay) * 100 * time.Microsecond)
			haveCallback = true
		}

		// we are in autosearch
		if state.channelParametersSet == 0 {
			if state.status == statusDemodSuccess || state.status == statusFFTSuccess {
				// Read the channel parameters from the frontend that found them.
				state.getFrontend()
				state.channelParametersSet = 1
			}
		}

		// Tuning is done when the frontend is done, failed or locked.
		if state.status == statusTuneFailed || state.status == statusLocked || state.status == statusDataLocked {
			active = state.tuneState != demodStop
		}

		if active && !haveCallback {
			// Linux: "strange callback time something went wrong".
			active = false
		}

		if active {
			// ポーリングだけを短縮し、同期・タイムアウトに関わる段4～8は維持する。
			minimum := minTuneStep
			if state.fastPoll && state.tuneState != demodStep4 && state.tuneState != demodStep5 && state.tuneState != demodStep6 && state.tuneState != demodStep7 && state.tuneState != demodStep8 {
				minimum = 3 * time.Millisecond
			}
			state.sleep(max(callbackTime.Sub(state.now()), minimum))
		}

		if state.failure.err != nil {
			return state.failure.err
		}
		// Linux has no bound here; a chip that never raises its search
		// interrupt would hold it forever.
		if active && state.now().After(giveUp) {
			return fmt.Errorf("DiB8000: tune did not finish within %v (state %d)", state.tuneTimeout, state.tuneState)
		}
	}

	// set output mode
	return state.setOutputMode(state.cfg.outputMode)
}

// readStatus is the lock half of dib8000_read_status. The DVBv5 statistics
// it then gathers are read separately, by the caller that wants them.
func (state *dib8000) readStatus() int {
	lock := state.readLock()
	stat := 0

	if (lock>>13)&1 != 0 {
		stat |= feHasSignal
	}

	if (lock>>8)&1 != 0 { // Equal
		stat |= feHasCarrier
	}

	if (lock>>1)&0xf == 0xf { // TMCC_SYNC
		stat |= feHasSync
	}

	if (lock>>12)&1 != 0 && (lock>>5)&7 != 0 { // FEC MPEG
		stat |= feHasLock
	}

	if (lock>>12)&1 != 0 {
		if state.readWord(554)&0x01 != 0 { // Viterbi Layer A
			stat |= feHasViterbi
		}
		if state.readWord(555)&0x01 != 0 { // Viterbi Layer B
			stat |= feHasViterbi
		}
		if state.readWord(556)&0x01 != 0 { // Viterbi Layer C
			stat |= feHasViterbi
		}
	}
	return stat
}

// readUncBlocks is dib8000_read_unc_blocks: packet errors over 13 segments.
func (state *dib8000) readUncBlocks() uint32 {
	return uint32(state.readWord(565))
}

// readSignalStrength is dib8000_read_signal_strength for one frontend: the
// inverted global AGC value.
func (state *dib8000) readSignalStrength() uint16 {
	return 65535 - state.readWord(390)
}

// shiftLeft32 is C's u32 << count as x86 executes it: the count is taken
// modulo 32. dib8000_get_snr can hand it a wrapped negative exponent.
func shiftLeft32(value, count uint32) uint32 {
	return value << (count & 31)
}

// getSNR is dib8000_get_snr: signal over noise as a 16.16 fixed-point ratio,
// or 0xffffffff when there is no noise reading.
func (state *dib8000) getSNR() uint32 {
	val := state.readWord(542)
	n := uint32(val>>6) & 0xff
	exp := uint32(val & 0x3f)
	if exp&0x20 != 0 {
		exp -= 0x40
	}
	n = shiftLeft32(n, exp+16)

	val = state.readWord(543)
	s := uint32(val>>6) & 0xff
	exp = uint32(val & 0x3f)
	if exp&0x20 != 0 {
		exp -= 0x40
	}
	s = shiftLeft32(s, exp+16)

	if n > 0 {
		t := (s / n) << 16
		return t + ((s<<16)-n*t)/n
	}
	return 0xffffffff
}

// linearSegment is struct linear_segments.
type linearSegment struct {
	x uint32
	y int32
}

// interpolateValue is interpolate_value: a descending table, linear between
// its entries.
func interpolateValue(value uint32, segments []linearSegment) uint32 {
	length := len(segments)
	if value >= segments[0].x {
		return uint32(segments[0].y)
	}
	if value < segments[length-1].x {
		return uint32(segments[length-1].y)
	}

	i := 1
	for ; i < length-1; i++ {
		// If value is identical, no need to interpolate
		if value == segments[i].x {
			return uint32(segments[i].y)
		}
		if value > segments[i].x {
			break
		}
	}

	// Linear interpolation between the two (x,y) points
	dy := segments[i-1].y - segments[i].y
	dx := segments[i-1].x - segments[i].x

	tmp64 := uint64(value - segments[i].x)
	tmp64 *= uint64(int64(dy))
	tmp64 /= uint64(dx)
	return uint32(int32(segments[i].y) + int32(tmp64))
}
