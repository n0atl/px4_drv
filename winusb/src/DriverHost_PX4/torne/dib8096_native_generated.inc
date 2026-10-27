// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2026 Till0196; Copyright (C) 2005-9 DiBcom, SA et al.
// Generated C++ port of the pinned local coax dib8096 sources.
// Regenerate with tools/torne_frontend/port_native.go.txt; do not hand edit.
#include "dib8096_native_runtime.hpp"
namespace px4::torne::dibnative {
struct T_Demod;
using T_OutputMode = std::int64_t;
struct T_Status;
struct T_agcConfig;
struct T_bandwidthConfig;
struct T_channel;
struct T_dcCalibration;
struct T_dib0090Config;
struct T_dib8000;
struct T_dib8000Config;
struct T_failure;
struct T_i2cMaster;
struct T_layerParameters;
struct T_linearSegment;
struct T_pllEntry;
struct T_slope;
struct T_tuner;
struct T_tunerIdentity;
struct T_tuningEntry;
struct T_wbdSlope;
constexpr std::uint8_t v_Address = 18;
constexpr T_OutputMode v_OutputFIFO = 4;
constexpr T_OutputMode v_OutputHighZ = 0;
constexpr T_OutputMode v_OutputParallelContinuousClock = 2;
constexpr T_OutputMode v_OutputParallelGatedClock = 1;
constexpr T_OutputMode v_OutputSerial = 3;
constexpr std::int64_t v_adcOff = 3;
constexpr std::int64_t v_adcOn = 2;
constexpr std::int64_t v_adcSlowOff = 1;
constexpr std::int64_t v_adcSlowOn = 0;
constexpr std::int64_t v_adcTarget = -220;
constexpr std::int64_t v_adcVBGDisable = 5;
constexpr std::int64_t v_adcVBGEnable = 4;
constexpr std::int64_t v_agcStart = 20;
constexpr std::int64_t v_agcStep0 = 21;
constexpr std::int64_t v_agcStep1 = 22;
constexpr std::int64_t v_agcStep2 = 23;
constexpr std::int64_t v_agcStep3 = 24;
constexpr std::int64_t v_agcStep4 = 25;
constexpr std::int64_t v_agcStop = 26;
constexpr std::int64_t v_autosearchDone = 100;
constexpr std::int64_t v_autosearchSearchingFFT = 1;
constexpr std::int64_t v_autosearchSearchingGuard = 2;
constexpr std::int64_t v_autosearchStart = 0;
constexpr std::uint8_t v_bandCBand = 32;
constexpr std::uint8_t v_bandFM = 16;
constexpr std::uint8_t v_bandLBand = 1;
constexpr std::uint8_t v_bandSBand = 8;
constexpr std::uint8_t v_bandUHF = 2;
constexpr std::uint8_t v_bandVHF = 4;
constexpr std::uint32_t v_callbackTimeNever = 4294967295;
constexpr std::int64_t v_capValueMax = 40;
constexpr std::int64_t v_capValueMin = 9;
constexpr std::int64_t v_captrimCal = 8;
constexpr std::int64_t v_dcCal = 1;
constexpr std::int64_t v_defaultTuneTimeout = 5000000000;
constexpr std::int64_t v_demodSearchNext = 51;
constexpr std::int64_t v_demodStart = 30;
constexpr std::int64_t v_demodStep1 = 31;
constexpr std::int64_t v_demodStep10 = 40;
constexpr std::int64_t v_demodStep11 = 41;
constexpr std::int64_t v_demodStep2 = 32;
constexpr std::int64_t v_demodStep3 = 33;
constexpr std::int64_t v_demodStep4 = 34;
constexpr std::int64_t v_demodStep5 = 35;
constexpr std::int64_t v_demodStep6 = 36;
constexpr std::int64_t v_demodStep7 = 37;
constexpr std::int64_t v_demodStep8 = 38;
constexpr std::int64_t v_demodStep9 = 39;
constexpr std::int64_t v_demodStepLocked = 52;
constexpr std::int64_t v_demodStop = 53;
constexpr std::int64_t v_deviceDiB3000MC = 1;
constexpr std::int64_t v_deviceDiB7000 = 2;
constexpr std::int64_t v_deviceDiB7000MC = 12;
constexpr std::int64_t v_deviceDiB7000P = 11;
constexpr std::int64_t v_deviceDiB8000 = 13;
constexpr std::int64_t v_enBB = 8;
constexpr std::int64_t v_enBias = 1;
constexpr std::int64_t v_enCAB = 35049;
constexpr std::int64_t v_enCrystal = 2;
constexpr std::int64_t v_enDigClk = 128;
constexpr std::int64_t v_enIQADC = 64;
constexpr std::int64_t v_enIQANA = 2;
constexpr std::int64_t v_enLBD = 4585;
constexpr std::int64_t v_enLNA0 = 32768;
constexpr std::int64_t v_enLNA1 = 16384;
constexpr std::int64_t v_enLNA2 = 8192;
constexpr std::int64_t v_enLNA3 = 4096;
constexpr std::int64_t v_enLO = 4;
constexpr std::int64_t v_enMix0 = 2048;
constexpr std::int64_t v_enMix1 = 1024;
constexpr std::int64_t v_enMix2 = 512;
constexpr std::int64_t v_enMix3 = 256;
constexpr std::int64_t v_enPLL = 32;
constexpr std::int64_t v_enSBD = 17641;
constexpr std::int64_t v_enTX = 16;
constexpr std::int64_t v_enUHF = 8937;
constexpr std::int64_t v_enVHF = 17641;
constexpr std::int64_t v_feHasCarrier = 2;
constexpr std::int64_t v_feHasLock = 16;
constexpr std::int64_t v_feHasSignal = 1;
constexpr std::int64_t v_feHasSync = 8;
constexpr std::int64_t v_feHasViterbi = 4;
constexpr std::int64_t v_fec1_2 = 1;
constexpr std::int64_t v_fec2_3 = 2;
constexpr std::int64_t v_fec3_4 = 3;
constexpr std::int64_t v_fec4_5 = 4;
constexpr std::int64_t v_fec5_6 = 5;
constexpr std::int64_t v_fec6_7 = 6;
constexpr std::int64_t v_fec7_8 = 7;
constexpr std::int64_t v_fecAuto = 9;
constexpr std::int64_t v_fecNone = 0;
constexpr std::int64_t v_gainAlpha = 5;
constexpr std::int64_t v_gpioDefaultDirections = 65535;
constexpr std::int64_t v_gpioDefaultPWMPos = 65535;
constexpr std::int64_t v_gpioDefaultValues = 0;
constexpr std::int64_t v_guard1_16 = 1;
constexpr std::int64_t v_guard1_32 = 0;
constexpr std::int64_t v_guard1_4 = 3;
constexpr std::int64_t v_guard1_8 = 2;
constexpr std::int64_t v_guardAuto = 4;
constexpr std::int64_t v_hrMax = 40;
constexpr std::int64_t v_hrMin = 25;
constexpr std::uint16_t v_interfaceGPIO12 = 1;
constexpr std::uint16_t v_interfaceGPIO34 = 2;
constexpr std::uint16_t v_interfaceGPIO67 = 3;
constexpr std::uint16_t v_interfaceTuner = 0;
constexpr std::int64_t v_internWait = 10;
constexpr std::int64_t v_inversionAuto = 2;
constexpr std::int64_t v_krosus = 2;
constexpr std::int64_t v_krosusPLLLocked = 2048;
constexpr std::int64_t v_layerA = 1;
constexpr std::int64_t v_layerAll = -1;
constexpr std::int64_t v_layerB = 2;
constexpr std::int64_t v_layerC = 3;
constexpr std::int64_t v_loopTune1 = 0;
constexpr std::int64_t v_loopTune2 = 1;
constexpr std::int64_t v_lpf = 100;
constexpr std::int64_t v_maximumFrequencyHz = 860000000;
constexpr std::int64_t v_minTuneStep = 10000000;
constexpr std::int64_t v_minimumFrequencyHz = 45000000;
constexpr std::int64_t v_modulationDQPSK = 12;
constexpr std::int64_t v_modulationQAM16 = 1;
constexpr std::int64_t v_modulationQAM32 = 2;
constexpr std::int64_t v_modulationQAM64 = 3;
constexpr std::int64_t v_modulationQAMAuto = 6;
constexpr std::int64_t v_modulationQPSK = 0;
constexpr std::int64_t v_mp001 = 1;
constexpr std::int64_t v_mp005 = 4;
constexpr std::int64_t v_mp008 = 6;
constexpr std::int64_t v_mp009 = 7;
constexpr std::int64_t v_outmodeAnalogADC = 6;
constexpr std::int64_t v_outmodeDiversity = 4;
constexpr std::int64_t v_outmodeFIFO = 5;
constexpr std::int64_t v_outmodeHighZ = 0;
constexpr std::int64_t v_outmodeParallelCont = 2;
constexpr std::int64_t v_outmodeParallelGated = 1;
constexpr std::int64_t v_outmodeSerial = 7;
constexpr std::int64_t v_p1AB = 0;
constexpr std::int64_t v_p1C = 1;
constexpr std::int64_t v_p1DEF = 3;
constexpr std::int64_t v_p1G = 7;
constexpr std::int64_t v_p1G21R2 = 15;
constexpr std::int64_t v_polyMax = 8;
constexpr std::int64_t v_polyMin = 0;
constexpr std::int64_t v_powerAll = 0;
constexpr std::int64_t v_powerInterfaceOnly = 1;
constexpr std::int64_t v_soc = 2;
constexpr std::int64_t v_soc7090P1G11R1 = 130;
constexpr std::int64_t v_soc7090P1G21R1 = 138;
constexpr std::int64_t v_soc8090P1G11R1 = 134;
constexpr std::int64_t v_soc8090P1G21R1 = 142;
constexpr std::int32_t v_statusDataLocked = -8;
constexpr std::int32_t v_statusDemodSuccess = -6;
constexpr std::int32_t v_statusFFTSuccess = -5;
constexpr std::int32_t v_statusLocked = -7;
constexpr std::int32_t v_statusStdSuccess = -4;
constexpr std::int32_t v_statusTuneFailed = 0;
constexpr std::int32_t v_statusTunePending = -3;
constexpr std::int32_t v_statusTuneTimeTooShort = -2;
constexpr std::int32_t v_statusTuneTimedOut = -1;
constexpr std::int64_t v_systemISDBT = 8;
constexpr std::int64_t v_tempCal = 4;
constexpr std::int64_t v_timfGet = 1;
constexpr std::int64_t v_timfSet = 0;
constexpr std::int64_t v_timfUpdate = 2;
constexpr std::int64_t v_transmission2K = 0;
constexpr std::int64_t v_transmission4K = 3;
constexpr std::int64_t v_transmission8K = 1;
constexpr std::int64_t v_transmissionAuto = 2;
constexpr std::uint8_t v_tunerAddress = 192;
constexpr std::int64_t v_tunerStart = 10;
constexpr std::int64_t v_tunerStep0 = 11;
constexpr std::int64_t v_tunerStep1 = 12;
constexpr std::int64_t v_tunerStep2 = 13;
constexpr std::int64_t v_tunerStep3 = 14;
constexpr std::int64_t v_tunerStep4 = 15;
constexpr std::int64_t v_tunerStep5 = 16;
constexpr std::int64_t v_tunerStep6 = 17;
constexpr std::int64_t v_tunerStep7 = 18;
constexpr std::int64_t v_tunerStop = 19;
constexpr std::int64_t v_wbdAlpha = 6;
constexpr std::int64_t v_wbdCal = 2;
constexpr std::int64_t v_wbdMux = 1921;
struct T_Anon0 {
std::uint32_t v_clockKHz{};
std::uint8_t v_pllBypass{};
std::uint8_t v_pllRange{};
std::uint8_t v_pllPrediv{};
std::uint8_t v_pllLoopdiv{};
std::uint8_t v_adcClockRatio{};
std::uint16_t v_pllIntLoopFilt{};
};
struct T_failure {
Error v_err{};
};
struct T_layerParameters {
std::int64_t v_fec{};
std::int64_t v_modulation{};
std::uint8_t v_interleaving{};
std::uint8_t v_segmentCount{};
};
struct T_channel {
std::int64_t v_deliverySystem{};
std::uint32_t v_frequency{};
std::uint32_t v_bandwidthHz{};
std::int64_t v_inversion{};
std::int64_t v_transmissionMode{};
std::int64_t v_guardInterval{};
std::uint8_t v_isdbtPartialReception{};
std::uint8_t v_isdbtSbMode{};
std::uint8_t v_isdbtLayerEnabled{};
std::array<T_layerParameters, 3> v_layer{};
};
struct T_Anon1 {
T_failure v_failure{};
T_channel v_channel{};
T_dib8000* v_demod{};
T_tuner* v_tuner{};
bool v_asleep{};
};
struct T_tunerIdentity {
std::uint8_t v_version{};
std::uint8_t v_product{};
std::uint8_t v_p1g{};
std::uint8_t v_inSoC{};
};
struct T_Anon10 {
Bus* v_bus{};
std::uint8_t v_address{};
T_failure* v_failure{};
T_dib0090Config* v_config{};
T_channel* v_channel{};
std::function<void(std::int64_t)> v_resetLine{};
std::function<void(std::int64_t)> v_sleepLine{};
std::function<void(std::int64_t)> v_sleep{};
std::uint8_t v_currentBand{};
std::int64_t v_tuneState{};
std::uint32_t v_currentRF{};
std::uint16_t v_wbdOffset{};
std::int16_t v_wbdTarget{};
std::int16_t v_rfGainLimit{};
std::int16_t v_currentGain{};
std::uint8_t v_agcStep{};
std::uint8_t v_agcFreeze{};
std::array<std::uint16_t, 2> v_gain{};
std::array<std::uint16_t, 4> v_gainReg{};
std::function<std::int32_t()> v_getADCPower{};
Slice<std::uint16_t> v_rfRamp{};
Slice<std::uint16_t> v_bbRamp{};
std::uint16_t v_bb1Def{};
std::uint16_t v_rfLTDef{};
std::int8_t v_step{};
std::int16_t v_adcDiff{};
std::int16_t v_minADCDiff{};
std::int8_t v_captrim{};
std::int8_t v_fcaptrim{};
Slice<T_dcCalibration> v_dc{};
std::int64_t v_dcNext{};
std::uint16_t v_bb6{};
std::uint16_t v_bb7{};
T_tuningEntry* v_currentTuneTable{};
T_pllEntry* v_currentPLLTable{};
std::uint8_t v_tunerIsTuned{};
T_tunerIdentity v_identity{};
std::uint32_t v_rfRequest{};
std::uint8_t v_currentStandard{};
std::uint8_t v_calibrate{};
std::uint32_t v_rest{};
std::uint16_t v_bias{};
std::int16_t v_temperature{};
std::uint8_t v_wbdCalibrationGain{};
Slice<T_wbdSlope> v_currentWBDTable{};
std::uint16_t v_wbdmux{};
};
struct T_Anon11 {
std::int16_t v_rangeValue{};
std::int16_t v_slope{};
};
struct T_Anon12 {
Error v_err{};
};
struct T_Anon13 {
std::uint8_t v_outputMPEG2In188Bytes{};
std::uint8_t v_hostbusDiversity{};
std::uint8_t v_tunerIsBaseband{};
Slice<T_agcConfig> v_agc{};
T_bandwidthConfig* v_pll{};
std::uint16_t v_gpioDir{};
std::uint16_t v_gpioVal{};
std::uint16_t v_gpioPWMPos{};
std::uint16_t v_pwmFreqDiv{};
std::uint16_t v_drives{};
std::uint16_t v_diversityDelay{};
std::uint8_t v_divCfg{};
std::int64_t v_outputMode{};
std::uint8_t v_refclksel{};
};
struct T_dib8000Config {
std::uint8_t v_outputMPEG2In188Bytes{};
std::uint8_t v_hostbusDiversity{};
std::uint8_t v_tunerIsBaseband{};
Slice<T_agcConfig> v_agc{};
T_bandwidthConfig* v_pll{};
std::uint16_t v_gpioDir{};
std::uint16_t v_gpioVal{};
std::uint16_t v_gpioPWMPos{};
std::uint16_t v_pwmFreqDiv{};
std::uint16_t v_drives{};
std::uint16_t v_diversityDelay{};
std::uint8_t v_divCfg{};
std::int64_t v_outputMode{};
std::uint8_t v_refclksel{};
};
struct T_i2cMaster {
T_dib8000* v_demod{};
std::uint16_t v_deviceRevision{};
std::uint16_t v_baseRegister{};
std::uint16_t v_selectedInterface{};
std::uint8_t v_gate{};
};
struct T_Anon14 {
Bus* v_bus{};
std::uint8_t v_address{};
T_failure* v_failure{};
T_dib8000Config v_cfg{};
T_i2cMaster v_master{};
T_channel* v_channel{};
std::uint16_t v_wbdRef{};
std::uint8_t v_currentBand{};
T_agcConfig* v_currentAGC{};
std::uint32_t v_timf{};
std::uint32_t v_timfDefault{};
std::uint8_t v_differentialConstellation{};
std::int64_t v_diversityOnOff{};
std::uint16_t v_revision{};
std::uint8_t v_isdbtCfgLoaded{};
std::int64_t v_tuneState{};
std::int32_t v_status{};
std::uint16_t v_currentDemodBW{};
std::uint16_t v_segMask{};
std::uint16_t v_segDiffMask{};
std::uint16_t v_mode{};
std::uint8_t v_layerBNbSeg{};
std::uint8_t v_layerCNbSeg{};
std::int64_t v_channelParametersSet{};
std::uint16_t v_autosearchState{};
std::int64_t v_foundNFFT{};
std::int64_t v_foundGuard{};
std::uint8_t v_symbolDuration{};
Time v_timeout{};
std::uint8_t v_longestIntlvLayer{};
std::int64_t v_outputMode{};
std::function<Error()> v_tunerSetParams{};
std::function<std::uint32_t()> v_tunerFrequency{};
std::function<void(std::uint8_t)> v_agcControl{};
std::function<void(std::int64_t)> v_sleep{};
std::function<Time()> v_now{};
std::int64_t v_tuneTimeout{};
};
struct T_Anon15 {
std::uint16_t v_min{};
std::uint16_t v_max{};
std::uint16_t v_minThres{};
std::uint16_t v_maxThres{};
};
struct T_Anon16 {
std::int64_t v_fec{};
std::int64_t v_modulation{};
std::uint8_t v_interleaving{};
std::uint8_t v_segmentCount{};
};
struct T_Anon17 {
std::int64_t v_deliverySystem{};
std::uint32_t v_frequency{};
std::uint32_t v_bandwidthHz{};
std::int64_t v_inversion{};
std::int64_t v_transmissionMode{};
std::int64_t v_guardInterval{};
std::uint8_t v_isdbtPartialReception{};
std::uint8_t v_isdbtSbMode{};
std::uint8_t v_isdbtLayerEnabled{};
std::array<T_layerParameters, 3> v_layer{};
};
struct T_Anon18 {
std::uint32_t v_x{};
std::int32_t v_y{};
};
struct T_Anon19 {
std::uint8_t v_bandCaps{};
std::uint16_t v_setup{};
std::uint16_t v_invGain{};
std::uint16_t v_timeStabiliz{};
std::uint8_t v_alphaLevel{};
std::uint16_t v_thlock{};
std::uint8_t v_wbdInv{};
std::uint16_t v_wbdRef{};
std::uint8_t v_wbdSel{};
std::uint8_t v_wbdAlpha{};
std::uint16_t v_agc1Max{};
std::uint16_t v_agc1Min{};
std::uint16_t v_agc2Max{};
std::uint16_t v_agc2Min{};
std::uint8_t v_agc1Pt1{};
std::uint8_t v_agc1Pt2{};
std::uint8_t v_agc1Pt3{};
std::uint8_t v_agc1Slope1{};
std::uint8_t v_agc1Slope2{};
std::uint8_t v_agc2Pt1{};
std::uint8_t v_agc2Pt2{};
std::uint8_t v_agc2Slope1{};
std::uint8_t v_agc2Slope2{};
std::uint8_t v_alphaMant{};
std::uint8_t v_alphaExp{};
std::uint8_t v_betaMant{};
std::uint8_t v_betaExp{};
std::uint8_t v_performAGCSoftsplit{};
T_Anon15 v_split{};
};
struct T_Anon2 {
bool v_Locked{};
double v_CNR{};
bool v_HasCNR{};
double v_StrengthDBm{};
bool v_HasStrength{};
Slice<std::string> v_Names{};
};
struct T_Anon20 {
std::uint32_t v_internal{};
std::uint32_t v_sampling{};
std::uint8_t v_pllPrediv{};
std::uint8_t v_pllRatio{};
std::uint8_t v_pllRange{};
std::uint8_t v_pllReset{};
std::uint8_t v_pllBypass{};
std::uint8_t v_enableRefdiv{};
std::uint8_t v_bypclkDiv{};
std::uint8_t v_ioClkEnCore{};
std::uint8_t v_adcClkSrc{};
std::uint8_t v_modulo{};
std::uint16_t v_sadCfg{};
std::uint32_t v_ifreq{};
std::uint32_t v_timf{};
std::uint32_t v_xtalHz{};
};
struct T_Anon21 {
T_dib8000* v_demod{};
std::uint16_t v_deviceRevision{};
std::uint16_t v_baseRegister{};
std::uint16_t v_selectedInterface{};
std::uint8_t v_gate{};
};
struct T_Anon22 {
T_i2cMaster* v_master{};
Bus* v_parent{};
};
struct T_Anon3 {
std::int64_t v_mask{};
std::string v_name{};
};
struct T_Anon4 {
std::uint32_t v_maxFreq{};
std::uint8_t v_switchTrim{};
std::uint8_t v_lnaTune{};
std::uint16_t v_lnaBias{};
std::uint16_t v_v2i{};
std::uint16_t v_mix{};
std::uint16_t v_load{};
std::uint16_t v_tunerEnable{};
};
struct T_Anon5 {
std::uint32_t v_maxFreq{};
std::uint8_t v_vcoBand{};
std::uint8_t v_hfdivCode{};
std::uint8_t v_hfdiv{};
std::uint8_t v_topresc{};
};
struct T_Anon6 {
std::uint16_t v_maxFreq{};
std::uint16_t v_slopeCold{};
std::uint16_t v_offsetCold{};
std::uint16_t v_slopeHot{};
std::uint16_t v_offsetHot{};
std::uint8_t v_wbdGain{};
};
struct T_Anon7 {
std::uint8_t v_addr{};
std::uint8_t v_offset{};
std::uint8_t v_pga{};
std::uint16_t v_bb1{};
std::uint8_t v_i{};
};
struct T_Anon8 {
std::uint8_t v_version{};
std::uint8_t v_product{};
std::uint8_t v_p1g{};
std::uint8_t v_inSoC{};
};
struct T_Anon9 {
T_Anon0 v_io{};
std::int32_t v_freqOffsetKHzUHF{};
std::int32_t v_freqOffsetKHzVHF{};
std::uint8_t v_clkouttobamse{};
std::uint8_t v_analogOutput{};
std::uint8_t v_usePWMAGC{};
std::uint8_t v_clkoutdrive{};
std::uint16_t v_wbdVHFOffset{};
std::uint16_t v_wbdCBandOffset{};
std::uint8_t v_fRefClockRatio{};
Slice<T_wbdSlope> v_wbd{};
std::uint8_t v_forceCrystalMode{};
};
struct T_Demod {
T_failure v_failure{};
T_channel v_channel{};
T_dib8000* v_demod{};
T_tuner* v_tuner{};
bool v_asleep{};
};
struct T_Status {
bool v_Locked{};
double v_CNR{};
bool v_HasCNR{};
double v_StrengthDBm{};
bool v_HasStrength{};
Slice<std::string> v_Names{};
};
struct T_agcConfig {
std::uint8_t v_bandCaps{};
std::uint16_t v_setup{};
std::uint16_t v_invGain{};
std::uint16_t v_timeStabiliz{};
std::uint8_t v_alphaLevel{};
std::uint16_t v_thlock{};
std::uint8_t v_wbdInv{};
std::uint16_t v_wbdRef{};
std::uint8_t v_wbdSel{};
std::uint8_t v_wbdAlpha{};
std::uint16_t v_agc1Max{};
std::uint16_t v_agc1Min{};
std::uint16_t v_agc2Max{};
std::uint16_t v_agc2Min{};
std::uint8_t v_agc1Pt1{};
std::uint8_t v_agc1Pt2{};
std::uint8_t v_agc1Pt3{};
std::uint8_t v_agc1Slope1{};
std::uint8_t v_agc1Slope2{};
std::uint8_t v_agc2Pt1{};
std::uint8_t v_agc2Pt2{};
std::uint8_t v_agc2Slope1{};
std::uint8_t v_agc2Slope2{};
std::uint8_t v_alphaMant{};
std::uint8_t v_alphaExp{};
std::uint8_t v_betaMant{};
std::uint8_t v_betaExp{};
std::uint8_t v_performAGCSoftsplit{};
T_Anon15 v_split{};
};
struct T_bandwidthConfig {
std::uint32_t v_internal{};
std::uint32_t v_sampling{};
std::uint8_t v_pllPrediv{};
std::uint8_t v_pllRatio{};
std::uint8_t v_pllRange{};
std::uint8_t v_pllReset{};
std::uint8_t v_pllBypass{};
std::uint8_t v_enableRefdiv{};
std::uint8_t v_bypclkDiv{};
std::uint8_t v_ioClkEnCore{};
std::uint8_t v_adcClkSrc{};
std::uint8_t v_modulo{};
std::uint16_t v_sadCfg{};
std::uint32_t v_ifreq{};
std::uint32_t v_timf{};
std::uint32_t v_xtalHz{};
};
struct T_dcCalibration {
std::uint8_t v_addr{};
std::uint8_t v_offset{};
std::uint8_t v_pga{};
std::uint16_t v_bb1{};
std::uint8_t v_i{};
};
struct T_dib0090Config {
T_Anon0 v_io{};
std::int32_t v_freqOffsetKHzUHF{};
std::int32_t v_freqOffsetKHzVHF{};
std::uint8_t v_clkouttobamse{};
std::uint8_t v_analogOutput{};
std::uint8_t v_usePWMAGC{};
std::uint8_t v_clkoutdrive{};
std::uint16_t v_wbdVHFOffset{};
std::uint16_t v_wbdCBandOffset{};
std::uint8_t v_fRefClockRatio{};
Slice<T_wbdSlope> v_wbd{};
std::uint8_t v_forceCrystalMode{};
};
struct T_dib8000 {
Bus* v_bus{};
std::uint8_t v_address{};
T_failure* v_failure{};
T_dib8000Config v_cfg{};
T_i2cMaster v_master{};
T_channel* v_channel{};
std::uint16_t v_wbdRef{};
std::uint8_t v_currentBand{};
T_agcConfig* v_currentAGC{};
std::uint32_t v_timf{};
std::uint32_t v_timfDefault{};
std::uint8_t v_differentialConstellation{};
std::int64_t v_diversityOnOff{};
std::uint16_t v_revision{};
std::uint8_t v_isdbtCfgLoaded{};
std::int64_t v_tuneState{};
std::int32_t v_status{};
std::uint16_t v_currentDemodBW{};
std::uint16_t v_segMask{};
std::uint16_t v_segDiffMask{};
std::uint16_t v_mode{};
std::uint8_t v_layerBNbSeg{};
std::uint8_t v_layerCNbSeg{};
std::int64_t v_channelParametersSet{};
std::uint16_t v_autosearchState{};
std::int64_t v_foundNFFT{};
std::int64_t v_foundGuard{};
std::uint8_t v_symbolDuration{};
Time v_timeout{};
std::uint8_t v_longestIntlvLayer{};
std::int64_t v_outputMode{};
std::function<Error()> v_tunerSetParams{};
std::function<std::uint32_t()> v_tunerFrequency{};
std::function<void(std::uint8_t)> v_agcControl{};
std::function<void(std::int64_t)> v_sleep{};
std::function<Time()> v_now{};
std::int64_t v_tuneTimeout{};
};
struct T_linearSegment {
std::uint32_t v_x{};
std::int32_t v_y{};
};
struct T_pllEntry {
std::uint32_t v_maxFreq{};
std::uint8_t v_vcoBand{};
std::uint8_t v_hfdivCode{};
std::uint8_t v_hfdiv{};
std::uint8_t v_topresc{};
};
struct T_slope {
std::int16_t v_rangeValue{};
std::int16_t v_slope{};
};
struct T_tuner {
Bus* v_bus{};
std::uint8_t v_address{};
T_failure* v_failure{};
T_dib0090Config* v_config{};
T_channel* v_channel{};
std::function<void(std::int64_t)> v_resetLine{};
std::function<void(std::int64_t)> v_sleepLine{};
std::function<void(std::int64_t)> v_sleep{};
std::uint8_t v_currentBand{};
std::int64_t v_tuneState{};
std::uint32_t v_currentRF{};
std::uint16_t v_wbdOffset{};
std::int16_t v_wbdTarget{};
std::int16_t v_rfGainLimit{};
std::int16_t v_currentGain{};
std::uint8_t v_agcStep{};
std::uint8_t v_agcFreeze{};
std::array<std::uint16_t, 2> v_gain{};
std::array<std::uint16_t, 4> v_gainReg{};
std::function<std::int32_t()> v_getADCPower{};
Slice<std::uint16_t> v_rfRamp{};
Slice<std::uint16_t> v_bbRamp{};
std::uint16_t v_bb1Def{};
std::uint16_t v_rfLTDef{};
std::int8_t v_step{};
std::int16_t v_adcDiff{};
std::int16_t v_minADCDiff{};
std::int8_t v_captrim{};
std::int8_t v_fcaptrim{};
Slice<T_dcCalibration> v_dc{};
std::int64_t v_dcNext{};
std::uint16_t v_bb6{};
std::uint16_t v_bb7{};
T_tuningEntry* v_currentTuneTable{};
T_pllEntry* v_currentPLLTable{};
std::uint8_t v_tunerIsTuned{};
T_tunerIdentity v_identity{};
std::uint32_t v_rfRequest{};
std::uint8_t v_currentStandard{};
std::uint8_t v_calibrate{};
std::uint32_t v_rest{};
std::uint16_t v_bias{};
std::int16_t v_temperature{};
std::uint8_t v_wbdCalibrationGain{};
Slice<T_wbdSlope> v_currentWBDTable{};
std::uint16_t v_wbdmux{};
};
struct T_tuningEntry {
std::uint32_t v_maxFreq{};
std::uint8_t v_switchTrim{};
std::uint8_t v_lnaTune{};
std::uint16_t v_lnaBias{};
std::uint16_t v_v2i{};
std::uint16_t v_mix{};
std::uint16_t v_load{};
std::uint16_t v_tunerEnable{};
};
struct T_wbdSlope {
std::uint16_t v_maxFreq{};
std::uint16_t v_slopeCold{};
std::uint16_t v_offsetCold{};
std::uint16_t v_slopeHot{};
std::uint16_t v_offsetHot{};
std::uint8_t v_wbdGain{};
};
std::tuple<std::int64_t,Error> m_OutputMode_outmode(T_OutputMode v_mode);
T_dib8000Config v_dib809xDiB8000Config();
T_dib0090Config* v_dib809xDiB0090Config();
Error m_Demod_Initialize(T_Demod* v_board);
Error m_Demod_SetOutputMode(T_Demod* v_board,T_OutputMode v_mode);
Error m_Demod_SetGPIO(T_Demod* v_board,std::uint8_t v_pin,bool v_high);
Error m_Demod_Tune(T_Demod* v_board,std::uint32_t v_frequencyHz);
Error m_Demod_setParamOverride(T_Demod* v_board);
std::uint8_t v_dib8090ComputePLLParameters(std::uint32_t v_frequencyHz);
std::tuple<T_Status,Error> m_Demod_Status(T_Demod* v_board);
Error m_Demod_Sleep(T_Demod* v_board);
std::uint16_t m_tuner_readReg(T_tuner* v_state,std::uint8_t v_register);
void m_tuner_writeReg(T_tuner* v_state,std::uint8_t v_register,std::uint16_t v_value);
void m_tuner_msleep(T_tuner* v_state,std::int64_t v_milliseconds);
void m_tuner_writeRegs(T_tuner* v_state,std::uint8_t v_register,Slice<std::uint16_t> v_values);
Error m_tuner_identify(T_tuner* v_state);
void m_tuner_resetDigital(T_tuner* v_state);
void m_tuner_wakeup(T_tuner* v_state);
void m_tuner_sleepTuner(T_tuner* v_state);
void m_tuner_dccFreq(T_tuner* v_state,std::uint8_t v_fast);
void m_tuner_setBoost(T_tuner* v_state,std::uint16_t v_onoff);
void m_tuner_setRFRampPWM(T_tuner* v_state,Slice<std::uint16_t> v_cfg);
void m_tuner_setBBRampPWM(T_tuner* v_state,Slice<std::uint16_t> v_cfg);
void m_tuner_pwmGainReset(T_tuner* v_state);
void m_tuner_wbdTargetFor(T_tuner* v_state);
std::uint16_t v_rampAt(Slice<std::uint16_t> v_ramp,std::int64_t v_index);
void m_tuner_gainApply(T_tuner* v_state,std::int16_t v_gainDelta,std::int16_t v_topDelta,bool v_force);
std::int64_t m_tuner_gainControl(T_tuner* v_state);
void m_tuner_setRFRamp(T_tuner* v_state,Slice<std::uint16_t> v_cfg);
void m_tuner_setBBRamp(T_tuner* v_state,Slice<std::uint16_t> v_cfg);
void m_tuner_setDCServo(T_tuner* v_state,std::uint8_t v_dcServoCutoff);
std::uint32_t m_tuner_getSlowADCVal(T_tuner* v_state);
std::uint16_t v_slopesToScale(Slice<T_slope> v_slopes,std::int16_t v_val);
std::int16_t m_tuner_wbdToDB(T_tuner* v_state,std::uint16_t v_wbd);
std::uint16_t m_tuner_getWBDTarget(T_tuner* v_state);
void m_tuner_setDefaultConfig(T_tuner* v_state,Slice<std::uint16_t> v_n);
void m_tuner_setEFuse(T_tuner* v_state);
Error m_tuner_reset(T_tuner* v_state);
std::int8_t v_steps(std::int8_t v_u);
std::int64_t m_tuner_getOffset(T_tuner* v_state);
void m_tuner_setTrim(T_tuner* v_state);
std::int64_t v_abs(std::int64_t v_value);
std::int64_t m_tuner_dcOffsetCalibration(T_tuner* v_state);
void m_tuner_dcStep0(T_tuner* v_state);
std::int64_t m_tuner_wbdCalibration(T_tuner* v_state);
void m_tuner_setBandwidth(T_tuner* v_state);
std::int64_t m_tuner_captrimSearch(T_tuner* v_state);
std::int64_t m_tuner_getTemperature(T_tuner* v_state);
std::uint32_t m_tuner_tune(T_tuner* v_state);
std::uint32_t m_tuner_getFrequency(T_tuner* v_state);
Error m_tuner_setParams(T_tuner* v_state);
Error m_tuner_register(T_tuner* v_state);
void m_failure_record(T_failure* v_failure,Error v_err);
std::uint16_t m_dib8000_readWord(T_dib8000* v_state,std::uint16_t v_register);
std::uint32_t m_dib8000_read32(T_dib8000* v_state,std::uint16_t v_register);
void m_dib8000_writeWord(T_dib8000* v_state,std::uint16_t v_register,std::uint16_t v_value);
void m_dib8000_msleep(T_dib8000* v_state,std::int64_t v_milliseconds);
std::uint16_t m_dib8000_fftToMode(T_dib8000* v_state);
void m_dib8000_setAcquisitionMode(T_dib8000* v_state);
Error m_dib8000_setOutputMode(T_dib8000* v_state,std::int64_t v_mode);
void m_dib8000_setDiversityIn(T_dib8000* v_state,std::int64_t v_onoff);
void m_dib8000_setPowerMode(T_dib8000* v_state,std::int64_t v_mode);
void m_dib8000_setADCState(T_dib8000* v_state,std::int64_t v_no);
std::int32_t m_dib8000_getADCPower(T_dib8000* v_state,std::uint8_t v_mode);
void m_dib8000_pwmAGCReset(T_dib8000* v_state);
void m_dib8000_setBandwidth(T_dib8000* v_state,std::uint32_t v_bandwidth);
void m_dib8000_sadCalibrate(T_dib8000* v_state);
void m_dib8000_setWBDRef(T_dib8000* v_state,std::uint16_t v_value);
void m_dib8000_resetPLLCommon(T_dib8000* v_state,T_bandwidthConfig* v_bandwidth);
void m_dib8000_resetPLL(T_dib8000* v_state);
void m_dib8000_updatePLL(T_dib8000* v_state,T_bandwidthConfig* v_pll,std::uint32_t v_bandwidth,std::uint8_t v_ratio);
void m_dib8000_resetGPIO(T_dib8000* v_state);
void m_dib8000_setGPIO(T_dib8000* v_state,std::uint8_t v_num,std::uint8_t v_dir,std::uint8_t v_value);
std::tuple<std::uint16_t,Error> m_dib8000_identify(T_dib8000* v_state);
void m_dib8000_resetStats(T_dib8000* v_state);
Error m_dib8000_reset(T_dib8000* v_state);
void m_dib8000_restartAGC(T_dib8000* v_state);
Error m_dib8000_setAGCConfig(T_dib8000* v_state,std::uint8_t v_band);
std::int64_t m_dib8000_agcSoftSplit(T_dib8000* v_state);
std::int64_t m_dib8000_agcStartup(T_dib8000* v_state);
void m_dib8000_updateTimf(T_dib8000* v_state);
std::uint32_t m_dib8000_ctrlTimf(T_dib8000* v_state,std::uint8_t v_op,std::uint32_t v_timf);
void m_dib8000_wakeup(T_dib8000* v_state);
Error m_dib8000_sleepChip(T_dib8000* v_state);
Error m_dib8000_initialize(T_dib8000* v_state);
void m_channel_clear(T_channel* v_c);
std::int64_t m_dib8000_setLayer(T_dib8000* v_state,std::uint8_t v_layerIndex,std::int64_t v_maxConstellation);
std::uint16_t m_dib8000_adpFineTune(T_dib8000* v_state,std::int64_t v_maxConstellation);
void m_dib8000_updateAnaGain(T_dib8000* v_state,std::uint16_t v_anaGain);
void m_dib8000_loadAnaFECoefs(T_dib8000* v_state,std::array<std::int16_t, 24>* v_anaFE);
void m_dib8000_set13SegChannel(T_dib8000* v_state);
void m_dib8000_setSubchannelPRBS(T_dib8000* v_state,std::uint16_t v_initPRBS);
void m_dib8000_smallFineTune(T_dib8000* v_state);
void m_dib8000_setISDBTCommonChannel(T_dib8000* v_state,std::uint8_t v_seq,bool v_autosearching);
std::uint16_t v_boolWord(bool v_value);
std::uint32_t m_dib8000_waitLock(T_dib8000* v_state,std::uint32_t v_internal,std::uint32_t v_wait0ms,std::uint32_t v_wait1ms,std::uint32_t v_wait2ms);
void m_dib8000_autosearchStart(T_dib8000* v_state);
std::int64_t m_dib8000_autosearchIRQ(T_dib8000* v_state);
void m_dib8000_viterbiState(T_dib8000* v_state,bool v_on);
void m_dib8000_setDDS(T_dib8000* v_state,std::int32_t v_offsetKHz);
void m_dib8000_setFrequencyOffset(T_dib8000* v_state);
std::uint32_t m_dib8000_getSymbolDuration(T_dib8000* v_state);
void m_dib8000_setISDBTLoopParams(T_dib8000* v_state,std::int64_t v_loopStep);
void m_dib8000_demodRestart(T_dib8000* v_state);
void m_dib8000_setSyncWait(T_dib8000* v_state);
Time m_dib8000_getTimeout(T_dib8000* v_state,std::uint32_t v_delay,bool v_symbolDependent);
std::uint16_t m_dib8000_readLock(T_dib8000* v_state);
bool v_isManualMode(T_channel* v_c);
std::int64_t m_dib8000_tune(T_dib8000* v_state);
void m_dib8000_getFrontend(T_dib8000* v_state);
Error m_dib8000_setFrontend(T_dib8000* v_state);
std::int64_t m_dib8000_readStatus(T_dib8000* v_state);
std::uint32_t m_dib8000_readUncBlocks(T_dib8000* v_state);
std::uint16_t m_dib8000_readSignalStrength(T_dib8000* v_state);
std::uint32_t v_shiftLeft32(std::uint32_t v_value,std::uint32_t v_count);
std::uint32_t m_dib8000_getSNR(T_dib8000* v_state);
std::uint32_t v_interpolateValue(std::uint32_t v_value,Slice<T_linearSegment> v_segments);
std::uint8_t v_bandOfFrequency(std::uint32_t v_kHz);
void m_i2cMaster_initialize(T_i2cMaster* v_master,T_dib8000* v_demod,std::uint16_t v_deviceRevision);
void m_i2cMaster_reset(T_i2cMaster* v_master);
std::uint16_t m_i2cMaster_gateControl(T_i2cMaster* v_master,std::uint8_t v_address7,bool v_open);
void m_i2cMaster_selectInterface(T_i2cMaster* v_master,std::uint16_t v_intf);
bool m_Demod_SearchFailed(T_Demod* v_board);
void m_dib8000_modifyWord(T_dib8000*, std::uint16_t, std::uint16_t, std::uint16_t);
Slice<T_agcConfig> v_dib8090AGCConfig = Slice<T_agcConfig>{[&]() { T_agcConfig value{}; value.v_bandCaps = static_cast<std::uint8_t>((static_cast<std::uint8_t>((static_cast<std::uint8_t>((v_bandUHF) | (v_bandVHF))) | (v_bandLBand))) | (v_bandSBand)); value.v_setup = static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((Shl<std::int64_t>(0,15)) | (Shl<std::int64_t>(0,14)))) | (Shl<std::int64_t>(5,11)))) | (Shl<std::int64_t>(0,10)))) | (Shl<std::int64_t>(0,9)))) | (Shl<std::int64_t>(0,8)))) | (Shl<std::int64_t>(3,5)))) | (Shl<std::int64_t>(0,4)))) | (Shl<std::int64_t>(5,1)))) | (Shl<std::int64_t>(0,0))); value.v_invGain = 787; value.v_timeStabiliz = 10; value.v_alphaLevel = 0; value.v_thlock = 118; value.v_wbdInv = 0; value.v_wbdRef = 3530; value.v_wbdSel = 1; value.v_wbdAlpha = 5; value.v_agc1Max = 65535; value.v_agc1Min = 0; value.v_agc2Max = 65535; value.v_agc2Min = 0; value.v_agc1Pt1 = 0; value.v_agc1Pt2 = 32; value.v_agc1Pt3 = 114; value.v_agc1Slope1 = 143; value.v_agc1Slope2 = 144; value.v_agc2Pt1 = 114; value.v_agc2Pt2 = 227; value.v_agc2Slope1 = 116; value.v_agc2Slope2 = 117; value.v_alphaMant = 28; value.v_alphaExp = 26; value.v_betaMant = 31; value.v_betaExp = 51; value.v_performAGCSoftsplit = 0; return value; }(),[&]() { T_agcConfig value{}; value.v_bandCaps = v_bandCBand; value.v_setup = static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((Shl<std::int64_t>(0,15)) | (Shl<std::int64_t>(0,14)))) | (Shl<std::int64_t>(5,11)))) | (Shl<std::int64_t>(0,10)))) | (Shl<std::int64_t>(0,9)))) | (Shl<std::int64_t>(0,8)))) | (Shl<std::int64_t>(3,5)))) | (Shl<std::int64_t>(0,4)))) | (Shl<std::int64_t>(5,1)))) | (Shl<std::int64_t>(0,0))); value.v_invGain = 787; value.v_timeStabiliz = 10; value.v_alphaLevel = 0; value.v_thlock = 118; value.v_wbdInv = 0; value.v_wbdRef = 3530; value.v_wbdSel = 1; value.v_wbdAlpha = 5; value.v_agc1Max = 0; value.v_agc1Min = 0; value.v_agc2Max = 65535; value.v_agc2Min = 0; value.v_agc1Pt1 = 0; value.v_agc1Pt2 = 32; value.v_agc1Pt3 = 114; value.v_agc1Slope1 = 143; value.v_agc1Slope2 = 144; value.v_agc2Pt1 = 114; value.v_agc2Pt2 = 227; value.v_agc2Slope1 = 116; value.v_agc2Slope2 = 117; value.v_alphaMant = 28; value.v_alphaExp = 26; value.v_betaMant = 31; value.v_betaExp = 51; value.v_performAGCSoftsplit = 0; return value; }()};
T_bandwidthConfig v_dib8090PLLConfig12MHz = [&]() { T_bandwidthConfig value{}; value.v_internal = 54000; value.v_sampling = 13500; value.v_pllPrediv = 1; value.v_pllRatio = 18; value.v_pllRange = 3; value.v_pllReset = 1; value.v_pllBypass = 0; value.v_enableRefdiv = 0; value.v_bypclkDiv = 0; value.v_ioClkEnCore = 1; value.v_adcClkSrc = 1; value.v_modulo = 2; value.v_sadCfg = static_cast<std::uint16_t>((static_cast<std::int64_t>((Shl<std::int64_t>(3,14)) | (Shl<std::int64_t>(1,12)))) | (Shl<std::int64_t>(599,0))); value.v_ifreq = static_cast<std::uint32_t>((Shl<std::int64_t>(0,25)) | (0)); value.v_timf = 20199727; value.v_xtalHz = 12000000; return value; }();
Slice<T_wbdSlope> v_dib8090WBDTable = Slice<T_wbdSlope>{[&]() { T_wbdSlope value{}; value.v_maxFreq = 120; value.v_slopeCold = 0; value.v_offsetCold = 500; value.v_slopeHot = 0; value.v_offsetHot = 500; value.v_wbdGain = 4; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 170; value.v_slopeCold = 0; value.v_offsetCold = 450; value.v_slopeHot = 0; value.v_offsetHot = 450; value.v_wbdGain = 4; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 380; value.v_slopeCold = 48; value.v_offsetCold = 373; value.v_slopeHot = 28; value.v_offsetHot = 259; value.v_wbdGain = 6; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 860; value.v_slopeCold = 34; value.v_offsetCold = 700; value.v_slopeHot = 36; value.v_offsetHot = 616; value.v_wbdGain = 6; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 0xFFFF; value.v_slopeCold = 34; value.v_offsetCold = 700; value.v_slopeHot = 36; value.v_offsetHot = 616; value.v_wbdGain = 6; return value; }()};
Slice<T_slope> v_dib0090WBDSlopes = Slice<T_slope>{[&]() { T_slope value{}; value.v_rangeValue = 66; value.v_slope = 120; return value; }(),[&]() { T_slope value{}; value.v_rangeValue = 600; value.v_slope = 170; return value; }(),[&]() { T_slope value{}; value.v_rangeValue = 170; value.v_slope = 250; return value; }()};
std::array<std::uint16_t, 4> v_isdbtSymbolDuration = std::array<std::uint16_t, 4>{{26,101,63}};
std::array<std::int16_t, 24> v_anaFECoeff3Seg = std::array<std::int16_t, 24>{{81,80,78,74,68,61,54,45,37,28,19,11,4,1022,1017,1013,1010,1008,1008,1008,1008,1010,1014,1017}};
std::array<std::int16_t, 24> v_anaFECoeff1Seg = std::array<std::int16_t, 24>{{249,226,164,82,5,981,970,988,1018,20,31,26,8,1012,1000,1018,1012,8,15,14,9,3,1017,1003}};
std::array<std::int16_t, 24> v_anaFECoeff13Seg = std::array<std::int16_t, 24>{{396,305,105,Neg<std::int16_t>(51),Neg<std::int16_t>(77),Neg<std::int16_t>(12),41,31,Neg<std::int16_t>(11),Neg<std::int16_t>(30),Neg<std::int16_t>(11),14,15,Neg<std::int16_t>(2),Neg<std::int16_t>(13),Neg<std::int16_t>(7),5,8,1,Neg<std::int16_t>(6),Neg<std::int16_t>(7),Neg<std::int16_t>(3),0,1}};
Slice<std::uint16_t> v_dib8000Defaults = Slice<std::uint16_t>{3,7,0x0004,0x0400,0x0814,12,11,0x001b,0x7740,0x005b,0x8d80,0x01c9,0xc380,0x0000,0x0080,0x0000,0x0090,0x0001,0xd4c0,11,80,Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),825),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),837),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),811),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),766),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),737),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),693),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),648),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),619),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),575),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),531),117),Sub<std::uint16_t>(Sub<std::int64_t>((Shl<std::int64_t>(1,13)),501),117),4,108,0,0,0,0,1,175,0x0410,1,179,8192,6,181,0x2800,0x2800,0x2800,0x2800,0x2800,0x2800,2,193,0x0666,0x0000,2,205,0x200f,0x000f,5,215,0x023d,0x00a4,0x00a4,0x7ff0,0x3ccc,1,230,0x0000,1,263,0x800,1,268,static_cast<std::uint16_t>(((Shl<std::int64_t>(2,9))) | (39)),1,270,0x0001,1,285,0x0020,1,299,0x0062,1,338,static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,12))) | ((Shl<std::int64_t>(1,10))))) | ((Shl<std::int64_t>(0,9))))) | ((Shl<std::int64_t>(3,5))))) | ((Shl<std::int64_t>(1,0)))),0};
std::array<std::uint16_t, 11> v_adcTarget16dB = std::array<std::uint16_t, 11>{{7250,7238,7264,7309,7338,7382,7427,7456,7500,7544,7574}};
std::array<std::uint8_t, 13> v_permuSeg = std::array<std::uint8_t, 13>{{6,5,7,4,8,3,9,2,10,1,11,0,12}};
std::array<std::uint16_t, 4> v_adpQ64 = std::array<std::uint16_t, 4>{{0x0148,0xfff0,0x00a4,0xfff8}};
std::array<std::uint16_t, 4> v_adpQ16 = std::array<std::uint16_t, 4>{{0x023d,0xffdf,0x00a4,0xfff0}};
std::array<std::uint16_t, 4> v_adpQDefault = std::array<std::uint16_t, 4>{{0x099a,0xffae,0x0333,0xfff8}};
Slice<T_linearSegment> v_strengthToDBTable = Slice<T_linearSegment>{[&]() { T_linearSegment value{}; value.v_x = 55953; value.v_y = 108500; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 55394; value.v_y = 108000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 53834; value.v_y = 107000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 52863; value.v_y = 106000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 52239; value.v_y = 105000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 52012; value.v_y = 104000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 51803; value.v_y = 103000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 51566; value.v_y = 102000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 51356; value.v_y = 101000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 51112; value.v_y = 100000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 50869; value.v_y = 99000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 50600; value.v_y = 98000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 50363; value.v_y = 97000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 50117; value.v_y = 96000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 49889; value.v_y = 95000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 49680; value.v_y = 94000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 49493; value.v_y = 93000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 49302; value.v_y = 92000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 48929; value.v_y = 91000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 48416; value.v_y = 90000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 48035; value.v_y = 89000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 47593; value.v_y = 88000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 47282; value.v_y = 87000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 46953; value.v_y = 86000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 46698; value.v_y = 85000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 45617; value.v_y = 84000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 44773; value.v_y = 83000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 43845; value.v_y = 82000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 43020; value.v_y = 81000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 42010; value.v_y = 80000; return value; }(),[&]() { T_linearSegment value{}; value.v_x = 0; value.v_y = 0; return value; }()};
Slice<std::uint16_t> v_bbRampPWMNormal = Slice<std::uint16_t>{500,8,400,static_cast<std::uint16_t>(((Shl<std::int64_t>(2,9))) | (0)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,9))) | (168)),static_cast<std::uint16_t>(((Shl<std::int64_t>(2,9))) | (168)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,9))) | (400))};
Slice<std::uint16_t> v_bbRampPWMNormalSoCs = Slice<std::uint16_t>{550,static_cast<std::uint16_t>(((Shl<std::int64_t>(1,9))) | (8)),440,static_cast<std::uint16_t>(((Shl<std::int64_t>(4,9))) | (0)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,9))) | (208)),static_cast<std::uint16_t>(((Shl<std::int64_t>(4,9))) | (208)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,9))) | (440))};
Slice<std::uint16_t> v_rfRampPWMCBand7090P = Slice<std::uint16_t>{280,18,504,static_cast<std::uint16_t>(((Shl<std::int64_t>(29,10))) | (364)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (504)),static_cast<std::uint16_t>(((Shl<std::int64_t>(60,10))) | (228)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (364)),static_cast<std::uint16_t>(((Shl<std::int64_t>(34,10))) | (109)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (228)),static_cast<std::uint16_t>(((Shl<std::int64_t>(37,10))) | (0)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (109))};
Slice<std::int32_t> v_lut1000lnMant = Slice<std::int32_t>{908,7003,7090,7170,7244,7313,7377,7438,7495,7549,7600};
Slice<std::uint16_t> v_rfRampPWMCBand = Slice<std::uint16_t>{314,33,1023,static_cast<std::uint16_t>(((Shl<std::int64_t>(8,10))) | (743)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (1023)),static_cast<std::uint16_t>(((Shl<std::int64_t>(15,10))) | (469)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (742)),static_cast<std::uint16_t>(((Shl<std::int64_t>(9,10))) | (234)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (468)),static_cast<std::uint16_t>(((Shl<std::int64_t>(9,10))) | (0)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (233))};
Slice<std::uint16_t> v_rfRampPWMVHF = Slice<std::uint16_t>{398,24,954,static_cast<std::uint16_t>(((Shl<std::int64_t>(7,10))) | (0)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (290)),static_cast<std::uint16_t>(((Shl<std::int64_t>(16,10))) | (699)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (954)),static_cast<std::uint16_t>(((Shl<std::int64_t>(17,10))) | (580)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (699)),static_cast<std::uint16_t>(((Shl<std::int64_t>(7,10))) | (290)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (580))};
Slice<std::uint16_t> v_rfRampPWMUHF = Slice<std::uint16_t>{398,24,954,static_cast<std::uint16_t>(((Shl<std::int64_t>(7,10))) | (0)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (290)),static_cast<std::uint16_t>(((Shl<std::int64_t>(16,10))) | (699)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (954)),static_cast<std::uint16_t>(((Shl<std::int64_t>(17,10))) | (580)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (699)),static_cast<std::uint16_t>(((Shl<std::int64_t>(7,10))) | (290)),static_cast<std::uint16_t>(((Shl<std::int64_t>(0,10))) | (580))};
Slice<std::uint16_t> v_dib0090Defaults = Slice<std::uint16_t>{25,0x01,0x0000,0x99a0,0x6008,0x0000,0x8bcb,0x0000,0x0405,0x0000,0x0000,0x0000,0xb802,0x0300,0x2d12,0xbac0,0x7c00,0xdbb9,0x0954,0x0743,0x8000,0x0001,0x0040,0x0100,0x0000,0xe910,0x149e,1,0x1c,0xff2d,1,0x39,0x0000,2,0x1e,0x07FF,0x0007,1,0x24,static_cast<std::uint16_t>((v_enUHF) | (v_enCrystal)),2,0x3c,0x3ff,0x111,0};
Slice<std::uint16_t> v_dib0090P1GAdditionalDefaults = Slice<std::uint16_t>{1,0x05,0xabcd,1,0x11,0x00b4,1,0x1c,0xfffd,1,0x40,0x108,0};
Slice<T_dcCalibration> v_dcTable = Slice<T_dcCalibration>{[&]() { T_dcCalibration value{}; value.v_addr = 0x06; value.v_offset = 5; value.v_pga = 1; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(0,8))))) | ((Shl<std::int64_t>(26,3)))); value.v_i = 1; return value; }(),[&]() { T_dcCalibration value{}; value.v_addr = 0x07; value.v_offset = 11; value.v_pga = 1; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(0,8))))) | ((Shl<std::int64_t>(26,3)))); value.v_i = 0; return value; }(),[&]() { T_dcCalibration value{}; value.v_addr = 0x06; value.v_offset = 0; value.v_pga = 0; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(29,8))))) | ((Shl<std::int64_t>(26,3)))); value.v_i = 1; return value; }(),[&]() { T_dcCalibration value{}; value.v_addr = 0x06; value.v_offset = 10; value.v_pga = 0; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(29,8))))) | ((Shl<std::int64_t>(26,3)))); value.v_i = 0; return value; }(),[&]() { T_dcCalibration value{}; return value; }()};
Slice<T_dcCalibration> v_dcP1GTable = Slice<T_dcCalibration>{[&]() { T_dcCalibration value{}; value.v_addr = 0x06; value.v_offset = 5; value.v_pga = 1; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(0,8))))) | ((Shl<std::int64_t>(15,3)))); value.v_i = 1; return value; }(),[&]() { T_dcCalibration value{}; value.v_addr = 0x07; value.v_offset = 11; value.v_pga = 1; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(0,8))))) | ((Shl<std::int64_t>(15,3)))); value.v_i = 0; return value; }(),[&]() { T_dcCalibration value{}; value.v_addr = 0x06; value.v_offset = 0; value.v_pga = 0; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(29,8))))) | ((Shl<std::int64_t>(15,3)))); value.v_i = 1; return value; }(),[&]() { T_dcCalibration value{}; value.v_addr = 0x06; value.v_offset = 10; value.v_pga = 0; value.v_bb1 = static_cast<std::uint16_t>((static_cast<std::int64_t>(((Shl<std::int64_t>(1,13))) | ((Shl<std::int64_t>(29,8))))) | ((Shl<std::int64_t>(15,3)))); value.v_i = 0; return value; }(),[&]() { T_dcCalibration value{}; return value; }()};
Slice<T_pllEntry> v_dib0090PLLTable = Slice<T_pllEntry>{[&]() { T_pllEntry value{}; value.v_maxFreq = 56000; value.v_vcoBand = 0; value.v_hfdivCode = 9; value.v_hfdiv = 48; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 70000; value.v_vcoBand = 1; value.v_hfdivCode = 9; value.v_hfdiv = 48; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 87000; value.v_vcoBand = 0; value.v_hfdivCode = 8; value.v_hfdiv = 32; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 105000; value.v_vcoBand = 1; value.v_hfdivCode = 8; value.v_hfdiv = 32; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 115000; value.v_vcoBand = 0; value.v_hfdivCode = 7; value.v_hfdiv = 24; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 140000; value.v_vcoBand = 1; value.v_hfdivCode = 7; value.v_hfdiv = 24; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 170000; value.v_vcoBand = 0; value.v_hfdivCode = 6; value.v_hfdiv = 16; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 200000; value.v_vcoBand = 1; value.v_hfdivCode = 6; value.v_hfdiv = 16; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 230000; value.v_vcoBand = 0; value.v_hfdivCode = 5; value.v_hfdiv = 12; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 280000; value.v_vcoBand = 1; value.v_hfdivCode = 5; value.v_hfdiv = 12; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 340000; value.v_vcoBand = 0; value.v_hfdivCode = 4; value.v_hfdiv = 8; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 380000; value.v_vcoBand = 1; value.v_hfdivCode = 4; value.v_hfdiv = 8; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 450000; value.v_vcoBand = 0; value.v_hfdivCode = 3; value.v_hfdiv = 6; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 580000; value.v_vcoBand = 1; value.v_hfdivCode = 3; value.v_hfdiv = 6; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 700000; value.v_vcoBand = 0; value.v_hfdivCode = 2; value.v_hfdiv = 4; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 860000; value.v_vcoBand = 1; value.v_hfdivCode = 2; value.v_hfdiv = 4; value.v_topresc = 4; return value; }()};
Slice<T_tuningEntry> v_dib0090TuningTableFMVHFOnCBand = Slice<T_tuningEntry>{[&]() { T_tuningEntry value{}; value.v_maxFreq = 184000; value.v_switchTrim = 4; value.v_lnaTune = 1; value.v_lnaBias = 15; value.v_v2i = 0x280; value.v_mix = 0x2912; value.v_load = 0xb94e; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 227000; value.v_switchTrim = 4; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x280; value.v_mix = 0x2912; value.v_load = 0xb94e; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 380000; value.v_switchTrim = 4; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x280; value.v_mix = 0x2912; value.v_load = 0xb94e; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 520000; value.v_switchTrim = 2; value.v_lnaTune = 0; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 550000; value.v_switchTrim = 2; value.v_lnaTune = 2; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 650000; value.v_switchTrim = 2; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 750000; value.v_switchTrim = 2; value.v_lnaTune = 5; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 850000; value.v_switchTrim = 2; value.v_lnaTune = 6; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 900000; value.v_switchTrim = 2; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }()};
Slice<T_tuningEntry> v_dib0090TuningTable = Slice<T_tuningEntry>{[&]() { T_tuningEntry value{}; value.v_maxFreq = 170000; value.v_switchTrim = 4; value.v_lnaTune = 1; value.v_lnaBias = 15; value.v_v2i = 0x280; value.v_mix = 0x2912; value.v_load = 0xb94e; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 184000; value.v_switchTrim = 1; value.v_lnaTune = 1; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x4d12; value.v_load = 0xb94e; value.v_tunerEnable = v_enVHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 227000; value.v_switchTrim = 1; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x4d12; value.v_load = 0xb94e; value.v_tunerEnable = v_enVHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 380000; value.v_switchTrim = 1; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x4d12; value.v_load = 0xb94e; value.v_tunerEnable = v_enVHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 520000; value.v_switchTrim = 2; value.v_lnaTune = 0; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 550000; value.v_switchTrim = 2; value.v_lnaTune = 2; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 650000; value.v_switchTrim = 2; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 750000; value.v_switchTrim = 2; value.v_lnaTune = 5; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 850000; value.v_switchTrim = 2; value.v_lnaTune = 6; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 900000; value.v_switchTrim = 2; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }()};
Slice<T_tuningEntry> v_dib0090P1GTuningTable = Slice<T_tuningEntry>{[&]() { T_tuningEntry value{}; value.v_maxFreq = 170000; value.v_switchTrim = 4; value.v_lnaTune = 1; value.v_lnaBias = 0x820f; value.v_v2i = 0x300; value.v_mix = 0x2d22; value.v_load = 0x82cb; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 184000; value.v_switchTrim = 1; value.v_lnaTune = 1; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x4d12; value.v_load = 0xb94e; value.v_tunerEnable = v_enVHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 227000; value.v_switchTrim = 1; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x4d12; value.v_load = 0xb94e; value.v_tunerEnable = v_enVHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 380000; value.v_switchTrim = 1; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x4d12; value.v_load = 0xb94e; value.v_tunerEnable = v_enVHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 510000; value.v_switchTrim = 2; value.v_lnaTune = 0; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 540000; value.v_switchTrim = 2; value.v_lnaTune = 1; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 600000; value.v_switchTrim = 2; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 630000; value.v_switchTrim = 2; value.v_lnaTune = 4; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 680000; value.v_switchTrim = 2; value.v_lnaTune = 5; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 720000; value.v_switchTrim = 2; value.v_lnaTune = 6; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 900000; value.v_switchTrim = 2; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }()};
Slice<T_pllEntry> v_dib0090P1GPLLTable = Slice<T_pllEntry>{[&]() { T_pllEntry value{}; value.v_maxFreq = 57000; value.v_vcoBand = 0; value.v_hfdivCode = 11; value.v_hfdiv = 48; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 70000; value.v_vcoBand = 1; value.v_hfdivCode = 11; value.v_hfdiv = 48; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 86000; value.v_vcoBand = 0; value.v_hfdivCode = 10; value.v_hfdiv = 32; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 105000; value.v_vcoBand = 1; value.v_hfdivCode = 10; value.v_hfdiv = 32; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 115000; value.v_vcoBand = 0; value.v_hfdivCode = 9; value.v_hfdiv = 24; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 140000; value.v_vcoBand = 1; value.v_hfdivCode = 9; value.v_hfdiv = 24; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 170000; value.v_vcoBand = 0; value.v_hfdivCode = 8; value.v_hfdiv = 16; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 200000; value.v_vcoBand = 1; value.v_hfdivCode = 8; value.v_hfdiv = 16; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 230000; value.v_vcoBand = 0; value.v_hfdivCode = 7; value.v_hfdiv = 12; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 280000; value.v_vcoBand = 1; value.v_hfdivCode = 7; value.v_hfdiv = 12; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 340000; value.v_vcoBand = 0; value.v_hfdivCode = 6; value.v_hfdiv = 8; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 380000; value.v_vcoBand = 1; value.v_hfdivCode = 6; value.v_hfdiv = 8; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 455000; value.v_vcoBand = 0; value.v_hfdivCode = 5; value.v_hfdiv = 6; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 580000; value.v_vcoBand = 1; value.v_hfdivCode = 5; value.v_hfdiv = 6; value.v_topresc = 6; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 680000; value.v_vcoBand = 0; value.v_hfdivCode = 4; value.v_hfdiv = 4; value.v_topresc = 4; return value; }(),[&]() { T_pllEntry value{}; value.v_maxFreq = 860000; value.v_vcoBand = 1; value.v_hfdivCode = 4; value.v_hfdiv = 4; value.v_topresc = 4; return value; }()};
Slice<T_tuningEntry> v_dib0090P1GTuningTableFMVHFOnCBand = Slice<T_tuningEntry>{[&]() { T_tuningEntry value{}; value.v_maxFreq = 184000; value.v_switchTrim = 4; value.v_lnaTune = 3; value.v_lnaBias = 0x4187; value.v_v2i = 0x2c0; value.v_mix = 0x2d22; value.v_load = 0x81cb; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 227000; value.v_switchTrim = 4; value.v_lnaTune = 3; value.v_lnaBias = 0x4187; value.v_v2i = 0x2c0; value.v_mix = 0x2d22; value.v_load = 0x81cb; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 380000; value.v_switchTrim = 4; value.v_lnaTune = 3; value.v_lnaBias = 0x4187; value.v_v2i = 0x2c0; value.v_mix = 0x2d22; value.v_load = 0x81cb; value.v_tunerEnable = v_enCAB; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 520000; value.v_switchTrim = 2; value.v_lnaTune = 0; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 550000; value.v_switchTrim = 2; value.v_lnaTune = 2; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 650000; value.v_switchTrim = 2; value.v_lnaTune = 3; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 750000; value.v_switchTrim = 2; value.v_lnaTune = 5; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 850000; value.v_switchTrim = 2; value.v_lnaTune = 6; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }(),[&]() { T_tuningEntry value{}; value.v_maxFreq = 900000; value.v_switchTrim = 2; value.v_lnaTune = 7; value.v_lnaBias = 15; value.v_v2i = 0x300; value.v_mix = 0x1d12; value.v_load = 0xb9ce; value.v_tunerEnable = v_enUHF; return value; }()};
Slice<T_wbdSlope> v_dib0090WBDTableDefault = Slice<T_wbdSlope>{[&]() { T_wbdSlope value{}; value.v_maxFreq = 470; value.v_slopeCold = 0; value.v_offsetCold = 250; value.v_slopeHot = 0; value.v_offsetHot = 100; value.v_wbdGain = 4; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 860; value.v_slopeCold = 51; value.v_offsetCold = 866; value.v_slopeHot = 21; value.v_offsetHot = 375; value.v_wbdGain = 4; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 1700; value.v_slopeCold = 0; value.v_offsetCold = 800; value.v_slopeHot = 0; value.v_offsetHot = 850; value.v_wbdGain = 4; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 2900; value.v_slopeCold = 0; value.v_offsetCold = 250; value.v_slopeHot = 0; value.v_offsetHot = 100; value.v_wbdGain = 6; return value; }(),[&]() { T_wbdSlope value{}; value.v_maxFreq = 0xFFFF; value.v_slopeCold = 0; value.v_offsetCold = 0; value.v_slopeHot = 0; value.v_offsetHot = 0; value.v_wbdGain = 0; return value; }()};
// Source: board.go
// outmode maps an OutputMode to the chip's OUTMODE_* value.
std::tuple<std::int64_t,Error> m_OutputMode_outmode(T_OutputMode v_mode)
{
{
auto switch1 = v_mode;
do {
if ((switch1 == v_OutputHighZ)) {
return {v_outmodeHighZ,nullptr};
}
else if ((switch1 == v_OutputParallelGatedClock)) {
return {v_outmodeParallelGated,nullptr};
}
else if ((switch1 == v_OutputParallelContinuousClock)) {
return {v_outmodeParallelCont,nullptr};
}
else if ((switch1 == v_OutputSerial)) {
return {v_outmodeSerial,nullptr};
}
else if ((switch1 == v_OutputFIFO)) {
return {v_outmodeFIFO,nullptr};
}
} while(false); }
return {0,MakeError("DiB8096: unknown output mode %d",static_cast<std::int64_t>(v_mode))};
}
// dib809xDiB8000Config is dib809x_dib8000_config[0] with the CECH-ZD1 J's output
// mode. Linux replaces any mode other than serial or parallel gated with
// FIFO at init; parallel gated is kept.
T_dib8000Config v_dib809xDiB8000Config()
{
return [&]() { T_dib8000Config value{}; value.v_outputMPEG2In188Bytes = 1; value.v_agc = v_dib8090AGCConfig; value.v_pll = (&v_dib8090PLLConfig12MHz); value.v_tunerIsBaseband = 1; value.v_gpioDir = v_gpioDefaultDirections; value.v_gpioVal = v_gpioDefaultValues; value.v_gpioPWMPos = v_gpioDefaultPWMPos; value.v_hostbusDiversity = 1; value.v_divCfg = 0x31; value.v_outputMode = v_outmodeParallelGated; value.v_drives = 0x2d98; value.v_diversityDelay = 48; value.v_refclksel = 3; return value; }();
}
// dib809xDiB0090Config is dib809x_dib0090_config.
T_dib0090Config* v_dib809xDiB0090Config()
{
T_dib0090Config* v_config = new T_dib0090Config([&]() { T_dib0090Config value{}; value.v_clkouttobamse = 1; value.v_analogOutput = 1; value.v_usePWMAGC = 1; value.v_clkoutdrive = 1; value.v_freqOffsetKHzUHF = Neg<std::int32_t>(63); value.v_freqOffsetKHzVHF = Neg<std::int32_t>(143); value.v_wbd = v_dib8090WBDTable; value.v_fRefClockRatio = 6; return value; }());
((v_config)->v_io).v_pllBypass = 1;
((v_config)->v_io).v_pllRange = 1;
((v_config)->v_io).v_pllPrediv = 1;
((v_config)->v_io).v_pllLoopdiv = 20;
((v_config)->v_io).v_adcClockRatio = 8;
((v_config)->v_io).v_pllIntLoopFilt = 0;
((v_config)->v_io).v_clockKHz = 12000;
return v_config;
}
// Initialize identifies the chip, resets the demodulator, registers and
// resets the tuner behind its I2C master, wakes both as a DVB frontend's init
// would, and leaves the transport stream output in the configured mode. The
// bridge must already have pulsed the chip's reset line and run the address
// enumeration step (register 1287 = 3).
Error m_Demod_Initialize(T_Demod* v_board)
{
Error v_err{};
((v_board)->v_failure).v_err = nullptr;
{
Error v_err = m_dib8000_initialize((v_board)->v_demod);
if ((v_err != nullptr))
{
return v_err;
}
}
{
Error v_err = m_tuner_register((v_board)->v_tuner);
if ((v_err != nullptr))
{
return v_err;
}
}
m_dib8000_wakeup((v_board)->v_demod);
m_tuner_wakeup((v_board)->v_tuner);
(v_board)->v_asleep = false;
{
Error v_err = m_dib8000_setOutputMode((v_board)->v_demod,(((v_board)->v_demod)->v_cfg).v_outputMode);
if ((v_err != nullptr))
{
return v_err;
}
}
return ((v_board)->v_failure).v_err;
}
// SetOutputMode sets the transport stream output and makes it the mode each
// Tune ends in. Tune holds the output in high impedance while it runs.
Error m_Demod_SetOutputMode(T_Demod* v_board,T_OutputMode v_mode)
{
Error v_err{};
((v_board)->v_failure).v_err = nullptr;
auto tmp2 = m_OutputMode_outmode(v_mode);
std::int64_t v_outmode = std::get<0>(tmp2);
v_err = std::get<1>(tmp2);
if ((v_err != nullptr))
{
return v_err;
}
(((v_board)->v_demod)->v_cfg).v_outputMode = v_outmode;
return m_dib8000_setOutputMode((v_board)->v_demod,v_outmode);
}
// SetGPIO drives one of the DiB8000's GPIO pins as an output, for boards that
// wire a spare pin to something of their own, such as an indicator LED. Pins
// 0 and 5 belong to the tuner and pin 3 to its band switch. Initialize returns
// every pin to an input.
Error m_Demod_SetGPIO(T_Demod* v_board,std::uint8_t v_pin,bool v_high)
{
Error v_err{};
((v_board)->v_failure).v_err = nullptr;
m_dib8000_setGPIO((v_board)->v_demod,v_pin,0,static_cast<std::uint8_t>(v_boolWord(v_high)));
return ((v_board)->v_failure).v_err;
}
// Tune tunes to the 6MHz ISDB-T channel centred on frequencyHz and runs the
// demodulator's search for its parameters to the end, returning once it has
// locked or given up; Status tells which. Up to 170MHz (CBAND, VHF 1-3) the
// gain is first found by the tuner's software AGC, see setParamOverride.
Error m_Demod_Tune(T_Demod* v_board,std::uint32_t v_frequencyHz)
{
Error v_err{};
((v_board)->v_failure).v_err = nullptr;
if (((v_frequencyHz < v_minimumFrequencyHz) || (v_frequencyHz > v_maximumFrequencyHz)))
{
return MakeError("DiB8096: %d Hz is outside %d-%d Hz",v_frequencyHz,v_minimumFrequencyHz,v_maximumFrequencyHz);
}
if ((v_board)->v_asleep)
{
m_dib8000_wakeup((v_board)->v_demod);
m_tuner_wakeup((v_board)->v_tuner);
(v_board)->v_asleep = false;
}
m_channel_clear((&(v_board)->v_channel));
((v_board)->v_channel).v_frequency = v_frequencyHz;
{
Error v_err = m_dib8000_setFrontend((v_board)->v_demod);
if ((v_err != nullptr))
{
return v_err;
}
}
return ((v_board)->v_failure).v_err;
}
// setParamOverride is dib8096_set_param_override, installed as the tuner's
// set_params: it switches the VHF/UHF input, tunes the tuner, moves the
// demodulator's PLL ratio away from spurs near the channel, and hands the gain
// to the PWM AGC.
Error m_Demod_setParamOverride(T_Demod* v_board)
{
auto tmp3_0 = (v_board)->v_demod;
auto tmp3_1 = (v_board)->v_tuner;
auto tmp3_2 = (&(v_board)->v_channel);
T_dib8000* v_demod = tmp3_0;
T_tuner* v_tuner = tmp3_1;
T_channel* v_c = tmp3_2;
std::uint8_t v_band = v_bandOfFrequency(static_cast<std::uint32_t>(((v_c)->v_frequency) / (1000)));
{
auto switch4 = v_band;
do {
if ((switch4 == v_bandUHF)) {
m_dib8000_setGPIO(v_demod,3,0,0);
}
else
{
m_dib8000_setGPIO(v_demod,3,0,1);
}
} while(false); }
{
Error v_err = m_tuner_setParams(v_tuner);
if ((v_err != nullptr))
{
return v_err;
}
}
if (((v_c)->v_bandwidthHz != 6000000))
{
return MakeError("DiB8096: only 6MHz bandwidth is supported");
}
m_dib8000_updatePLL(v_demod,(&v_dib8090PLLConfig12MHz),static_cast<std::uint32_t>(((v_c)->v_bandwidthHz) / (1000)),0);
std::uint8_t v_pllRatio = v_dib8090ComputePLLParameters((v_c)->v_frequency);
std::uint32_t v_timf{};
{
auto switch5 = v_pllRatio;
do {
if ((switch5 == 17)) {
v_timf = 21387946;
}
else if ((switch5 == 18)) {
v_timf = 20199727;
}
else if ((switch5 == 19)) {
v_timf = 19136583;
}
else
{
v_timf = 18179756;
}
} while(false); }
m_dib8000_updatePLL(v_demod,(&v_dib8090PLLConfig12MHz),static_cast<std::uint32_t>(((v_c)->v_bandwidthHz) / (1000)),v_pllRatio);
m_dib8000_ctrlTimf(v_demod,v_timfSet,v_timf);
if ((v_band != v_bandCBand))
{
std::int64_t v_target = static_cast<std::int64_t>(((Add<std::int64_t>(static_cast<std::int64_t>((Mul<std::int64_t>(Mul<std::int64_t>(static_cast<std::int64_t>(m_tuner_getWBDTarget(v_tuner)),8),18)) / (33)),1))) / (2));
m_dib8000_setWBDRef(v_demod,static_cast<std::uint16_t>(v_target));
}
if ((v_band == v_bandCBand))
{
(v_tuner)->v_tuneState = v_agcStart;
for (;true;)
{
std::int64_t v_wait = m_tuner_gainControl(v_tuner);
m_tuner_msleep(v_tuner,v_wait);
if ((((v_board)->v_failure).v_err != nullptr))
{
return ((v_board)->v_failure).v_err;
}
{
auto switch6 = (v_tuner)->v_tuneState;
do {
if ((switch6 == v_agcStep0)) {
m_dib8000_setGPIO(v_demod,6,0,1);
}
else if ((switch6 == v_agcStep1)) {
if ((static_cast<std::uint16_t>((v_tuner)->v_rfGainLimit) < 2000))
{
m_dib8000_setGPIO(v_demod,6,0,0);
}
}
} while(false); }
if (((v_tuner)->v_tuneState >= v_agcStop))
{
break;
}
}
m_tuner_pwmGainReset(v_tuner);
m_dib8000_pwmAGCReset(v_demod);
(v_demod)->v_tuneState = v_demodStart;
}
else
{
m_tuner_pwmGainReset(v_tuner);
}
return ((v_board)->v_failure).v_err;
}
// dib8090ComputePLLParameters is dib8090_compute_pll_parameters: of the PLL
// ratios 17 to 20, the one whose ADC clock harmonics fall furthest from the
// channel.
std::uint8_t v_dib8090ComputePLLParameters(std::uint32_t v_frequencyHz)
{
std::uint8_t v_optimalPLLRatio = static_cast<std::uint8_t>(20);
std::uint32_t v_max{};
for (std::uint8_t v_pllRatio = static_cast<std::uint8_t>(17);(v_pllRatio <= 20);v_pllRatio++)
{
std::uint32_t v_freqADC = static_cast<std::uint32_t>(static_cast<std::int64_t>((Mul<std::int64_t>(Mul<std::int64_t>(12,static_cast<std::int64_t>(v_pllRatio)),(Shl<std::int64_t>(1,8)))) / (16)));
std::uint32_t v_ratio = static_cast<std::uint32_t>(((static_cast<std::uint32_t>((Mul<std::uint32_t>((static_cast<std::uint32_t>((v_frequencyHz) / (1000))),(Shl<std::uint32_t>(1,8)))) / (1000)))) / (v_freqADC));
std::uint32_t v_rest = Sub<std::uint32_t>((static_cast<std::uint32_t>((Mul<std::uint32_t>((static_cast<std::uint32_t>((v_frequencyHz) / (1000))),(Shl<std::uint32_t>(1,8)))) / (1000))),Mul<std::uint32_t>(v_ratio,v_freqADC));
if ((v_rest > static_cast<std::uint32_t>((v_freqADC) / (2))))
{
v_rest = Sub<std::uint32_t>(v_freqADC,v_rest);
}
if (((v_rest > v_max) && (v_rest > 717)))
{
v_optimalPLLRatio = v_pllRatio;
v_max = v_rest;
}
}
return v_optimalPLLRatio;
}
// Status reads the lock state (dib8000_read_status) and the signal strength
// and CNR that dib8000_get_stats derives alongside it.
std::tuple<T_Status,Error> m_Demod_Status(T_Demod* v_board)
{
Error v_err{};
((v_board)->v_failure).v_err = nullptr;
T_dib8000* v_demod = (v_board)->v_demod;
std::int64_t v_stat = m_dib8000_readStatus(v_demod);
T_Status v_status{};
{ auto&& range7 = Slice<T_Anon3>{[&]() { T_Anon3 value{}; value.v_mask = v_feHasSignal; value.v_name = "SIGNAL"; return value; }(),[&]() { T_Anon3 value{}; value.v_mask = v_feHasCarrier; value.v_name = "CARRIER"; return value; }(),[&]() { T_Anon3 value{}; value.v_mask = v_feHasViterbi; value.v_name = "VITERBI"; return value; }(),[&]() { T_Anon3 value{}; value.v_mask = v_feHasSync; value.v_name = "SYNC"; return value; }(),[&]() { T_Anon3 value{}; value.v_mask = v_feHasLock; value.v_name = "LOCK"; return value; }()};
for (std::int64_t index7=0; index7<Len(range7); ++index7) {
auto v_bit = range7[index7];
if ((static_cast<std::int64_t>((v_stat) & ((v_bit).v_mask)) != 0))
{
(v_status).v_Names = Append((v_status).v_Names,(v_bit).v_name);
}
}}
(v_status).v_Locked = (static_cast<std::int64_t>((v_stat) & (v_feHasLock)) != 0);
std::uint16_t v_strength = m_dib8000_readSignalStrength(v_demod);
std::int32_t v_db = Sub<std::int32_t>(static_cast<std::int32_t>(v_interpolateValue(static_cast<std::uint32_t>(v_strength),v_strengthToDBTable)),131000);
(v_status).v_StrengthDBm = static_cast<double>((static_cast<double>(v_db)) / (1000));
(v_status).v_HasStrength = true;
if ((v_status).v_Locked)
{
std::uint32_t v_snr = Shr<std::uint32_t>(m_dib8000_getSNR(v_demod),16);
if ((v_snr != 0))
{
(v_status).v_CNR = static_cast<double>((10) * (std::log10(static_cast<double>(v_snr))));
}
(v_status).v_HasCNR = true;
}
{
Error v_err = ((v_board)->v_failure).v_err;
if ((v_err != nullptr))
{
return {[&]() { T_Status value{}; return value; }(),v_err};
}
}
return {v_status,nullptr};
}
// Sleep puts the tuner to sleep, then the demodulator (dib8000_sleep: output
// high impedance, only the interface powered, ADCs off). The next Tune wakes
// both.
Error m_Demod_Sleep(T_Demod* v_board)
{
Error v_err{};
((v_board)->v_failure).v_err = nullptr;
m_tuner_sleepTuner((v_board)->v_tuner);
{
Error v_err = m_dib8000_sleepChip((v_board)->v_demod);
if ((v_err != nullptr))
{
return v_err;
}
}
(v_board)->v_asleep = true;
return ((v_board)->v_failure).v_err;
}
// Source: dib0090.go
// readReg is dib0090_read_reg: a one-byte register address, then a two-byte
// read, as two I2C messages.
std::uint16_t m_tuner_readReg(T_tuner* v_state,std::uint8_t v_register)
{
if ((((v_state)->v_failure)->v_err != nullptr))
{
return 0;
}
auto tmp8 = I2cWriteRead((v_state)->v_bus,(v_state)->v_address,Slice<std::uint8_t>{v_register},2);
Slice<std::uint8_t> v_data = std::get<0>(tmp8);
Error v_err = std::get<1>(tmp8);
if ((v_err != nullptr))
{
m_failure_record((v_state)->v_failure,MakeError("read DiB0090 register %#02x: %w",v_register,v_err));
return 0;
}
if ((Len(v_data) != 2))
{
m_failure_record((v_state)->v_failure,MakeError("read DiB0090 register %#02x: %d bytes, want 2",v_register,Len(v_data)));
return 0;
}
return [&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>(At(v_data,0)),8); auto right = static_cast<std::uint16_t>(At(v_data,1)); return static_cast<std::uint16_t>((left) | (right)); }();
}
// writeReg is dib0090_write_reg.
void m_tuner_writeReg(T_tuner* v_state,std::uint8_t v_register,std::uint16_t v_value)
{
if ((((v_state)->v_failure)->v_err != nullptr))
{
return ;
}
{
Error v_err = (v_state)->v_bus->Write((v_state)->v_address,Slice<std::uint8_t>{v_register,static_cast<std::uint8_t>(Shr<std::uint16_t>(v_value,8)),static_cast<std::uint8_t>(v_value)});
if ((v_err != nullptr))
{
m_failure_record((v_state)->v_failure,MakeError("write DiB0090 register %#02x: %w",v_register,v_err));
}
}
}
void m_tuner_msleep(T_tuner* v_state,std::int64_t v_milliseconds)
{
(v_state)->v_sleep(Mul<std::int64_t>(static_cast<std::int64_t>(v_milliseconds),INT64_C(1000000)));
}
// writeRegs is dib0090_write_regs: consecutive registers from values.
void m_tuner_writeRegs(T_tuner* v_state,std::uint8_t v_register,Slice<std::uint16_t> v_values)
{
{ auto&& range9 = v_values;
for (std::int64_t index9=0; index9<Len(range9); ++index9) {
auto v_value = range9[index9];
m_tuner_writeReg(v_state,v_register,v_value);
v_register++;
}}
}
// identify is dib0090_identify.
Error m_tuner_identify(T_tuner* v_state)
{
T_tunerIdentity* v_identity = (&(v_state)->v_identity);
std::uint16_t v_v = m_tuner_readReg(v_state,0x1a);
if ((((v_state)->v_failure)->v_err != nullptr))
{
return ((v_state)->v_failure)->v_err;
}
(v_identity)->v_p1g = 0;
(v_identity)->v_inSoC = 0;
v_v = static_cast<std::uint16_t>(v_v & ~(v_krosusPLLLocked));
(v_identity)->v_version = static_cast<std::uint8_t>(static_cast<std::uint16_t>((v_v) & (0xff)));
(v_identity)->v_product = static_cast<std::uint8_t>(static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_v,8))) & (0xf)));
if (((v_identity)->v_product != v_krosus))
{
return MakeError("DiB0090: unknown product (version register %#04x)",v_v);
}
if ((static_cast<std::uint8_t>(((v_identity)->v_version) & (0x3)) == v_soc))
{
return MakeError("DiB0090: SoC version %#02x belongs to a DiB7090 or DiB8096P, which this driver does not support",(v_identity)->v_version);
}
{
auto switch10 = static_cast<std::uint8_t>(((Shr<std::uint8_t>((v_identity)->v_version,5))) & (0x7));
do {
if ((switch10 == v_mp001) || (switch10 == v_mp005) || (switch10 == v_mp008) || (switch10 == v_mp009)) {
}
else
{
return MakeError("DiB0090: unknown package variant (version %#02x)",(v_identity)->v_version);
}
} while(false); }
{
auto switch11 = static_cast<std::uint8_t>(((v_identity)->v_version) & (0x1f));
do {
if ((switch11 == v_p1G21R2) || (switch11 == v_p1G)) {
(v_identity)->v_p1g = 1;
}
else if ((switch11 == v_p1DEF) || (switch11 == v_p1C)) {
}
else if ((switch11 == v_p1AB)) {
return MakeError("DiB0090: P1-A/B detected: driver is deactivated - not available");
}
else
{
return MakeError("DiB0090: unknown revision (version %#02x)",(v_identity)->v_version);
}
} while(false); }
return nullptr;
}
// resetDigital is dib0090_reset_digital. The PLL reconfiguration Linux does
// when the tuner's PLL is not bypassed is left out: this board bypasses it.
void m_tuner_resetDigital(T_tuner* v_state)
{
T_dib0090Config* v_cfg = (v_state)->v_config;
if (((v_state)->v_resetLine != nullptr))
{
if (((v_state)->v_sleepLine != nullptr))
{
(v_state)->v_sleepLine(0);
}
m_tuner_msleep(v_state,10);
(v_state)->v_resetLine(1);
m_tuner_msleep(v_state,10);
(v_state)->v_resetLine(0);
m_tuner_msleep(v_state,10);
}
m_tuner_writeReg(v_state,0x24,static_cast<std::uint16_t>((v_enPLL) | (v_enCrystal)));
m_tuner_writeReg(v_state,0x1b,static_cast<std::uint16_t>((static_cast<std::int64_t>((v_enDigClk) | (v_enPLL))) | (v_enCrystal)));
m_tuner_writeReg(v_state,0x20,static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>(static_cast<std::uint16_t>(Sub<std::uint8_t>(((v_cfg)->v_io).v_adcClockRatio,1)),11)) | (Shl<std::uint16_t>(0,10)))) | (Shl<std::uint16_t>(1,9)))) | (Shl<std::uint16_t>(1,8)))) | (Shl<std::uint16_t>(0,4)))) | (0)));
std::uint16_t v_notAnalog = static_cast<std::uint16_t>(0);
if (((v_cfg)->v_analogOutput == 0))
{
v_notAnalog = 1;
}
if (((v_cfg)->v_clkoutdrive != 0))
{
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((static_cast<std::uint16_t>(([&]() { auto left = static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>(0,15)) | (Shl<std::uint16_t>(v_notAnalog,14)))) | (Shl<std::uint16_t>(2,10)))) | (Shl<std::uint16_t>(1,9)))) | (Shl<std::uint16_t>(0,8)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_cfg)->v_clkoutdrive),5))); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_cfg)->v_clkouttobamse),4); return static_cast<std::uint16_t>((left) | (right)); }()) | (Shl<std::uint16_t>(0,2)))) | (0)));
}
else
{
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>(0,15)) | (Shl<std::uint16_t>(v_notAnalog,14)))) | (Shl<std::uint16_t>(2,10)))) | (Shl<std::uint16_t>(1,9)))) | (Shl<std::uint16_t>(0,8)))) | (Shl<std::uint16_t>(7,5)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_cfg)->v_clkouttobamse),4)))) | (Shl<std::uint16_t>(0,2)))) | (0)));
}
std::uint16_t v_pllCfg = m_tuner_readReg(v_state,0x21);
if ((((v_cfg)->v_io).v_pllBypass != 0))
{
v_pllCfg |= Shl<std::uint16_t>(static_cast<std::uint16_t>(((v_cfg)->v_io).v_pllBypass),15);
m_tuner_writeReg(v_state,0x21,v_pllCfg);
}
}
// wakeup is dib0090_wakeup.
void m_tuner_wakeup(T_tuner* v_state)
{
if (((v_state)->v_sleepLine != nullptr))
{
(v_state)->v_sleepLine(0);
}
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x23)) | (Shl<std::uint16_t>(1,14))));
}
// sleepTuner is dib0090_sleep.
void m_tuner_sleepTuner(T_tuner* v_state)
{
if (((v_state)->v_sleepLine != nullptr))
{
(v_state)->v_sleepLine(1);
}
}
// dccFreq is dib0090_dcc_freq.
void m_tuner_dccFreq(T_tuner* v_state,std::uint8_t v_fast)
{
if ((v_fast != 0))
{
m_tuner_writeReg(v_state,0x04,0);
}
else
{
m_tuner_writeReg(v_state,0x04,1);
}
}
// setBoost is dib0090_set_boost.
void m_tuner_setBoost(T_tuner* v_state,std::uint16_t v_onoff)
{
(v_state)->v_bb1Def &= 0xdfff;
(v_state)->v_bb1Def |= Shl<std::uint16_t>(v_onoff,13);
}
// setRFRampPWM is dib0090_set_rframp_pwm.
void m_tuner_setRFRampPWM(T_tuner* v_state,Slice<std::uint16_t> v_cfg)
{
(v_state)->v_rfRamp = v_cfg;
m_tuner_writeReg(v_state,0x2a,0xffff);
m_tuner_writeRegs(v_state,0x2c,Subslice(v_cfg,3,Add<std::int64_t>(3,6)));
m_tuner_writeRegs(v_state,0x3e,Subslice(v_cfg,9,Add<std::int64_t>(9,2)));
}
// setBBRampPWM is dib0090_set_bbramp_pwm.
void m_tuner_setBBRampPWM(T_tuner* v_state,Slice<std::uint16_t> v_cfg)
{
(v_state)->v_bbRamp = v_cfg;
m_tuner_setBoost(v_state,v_boolWord((At(v_cfg,0) > 500)));
m_tuner_writeReg(v_state,0x33,0xffff);
m_tuner_writeRegs(v_state,0x35,Subslice(v_cfg,3,Add<std::int64_t>(3,4)));
}
// pwmGainReset is dib0090_pwm_gain_reset: it hands the gain to the
// demodulator's PWM AGC, with the ramp for the band.
void m_tuner_pwmGainReset(T_tuner* v_state)
{
Slice<std::uint16_t> v_bbRamp = v_bbRampPWMNormal;
Slice<std::uint16_t> v_rfRamp{};
std::uint16_t v_enPWMRFMux = static_cast<std::uint16_t>(1);
if ((((v_state)->v_config)->v_usePWMAGC != 0))
{
{
auto switch12 = (v_state)->v_currentBand;
do {
if ((switch12 == v_bandCBand)) {
v_rfRamp = v_rfRampPWMCBand;
}
else if ((switch12 == v_bandVHF)) {
v_rfRamp = v_rfRampPWMVHF;
}
else if ((switch12 == v_bandUHF)) {
v_rfRamp = v_rfRampPWMUHF;
}
} while(false); }
if ((v_rfRamp != nullptr))
{
m_tuner_setRFRampPWM(v_state,v_rfRamp);
}
m_tuner_setBBRampPWM(v_state,v_bbRamp);
if (((v_rfRamp != nullptr) && ((((((v_state)->v_rfRamp != nullptr) && (At((v_state)->v_rfRamp,0) == 0))) || ((((v_state)->v_currentBand == v_bandCBand) && ((static_cast<std::uint8_t>((((v_state)->v_identity).v_version) & (0x1f))) <= v_p1DEF)))))))
{
v_enPWMRFMux = 0;
}
m_tuner_writeReg(v_state,0x32,static_cast<std::uint16_t>((Shl<std::uint16_t>(v_enPWMRFMux,12)) | (Shl<std::uint16_t>(v_enPWMRFMux,11))));
m_tuner_writeReg(v_state,0x04,1);
m_tuner_writeReg(v_state,0x39,Shl<std::uint16_t>(1,10));
}
}
// wbdTargetFor is dib0090_wbd_target.
void m_tuner_wbdTargetFor(T_tuner* v_state)
{
std::uint16_t v_offset = static_cast<std::uint16_t>(250);
if (((v_state)->v_currentBand == v_bandVHF))
{
v_offset = ((v_state)->v_config)->v_wbdVHFOffset;
}
if (((v_state)->v_currentBand == v_bandCBand))
{
v_offset = ((v_state)->v_config)->v_wbdCBandOffset;
}
(v_state)->v_wbdTarget = m_tuner_wbdToDB(v_state,Add<std::uint16_t>((v_state)->v_wbdOffset,v_offset));
}
// rampAt is a ramp entry, or 0 past the end. dib0090_gain_apply walks five RF
// amplifiers of three words each from the second word, which is 16 words, and
// the PWM ramps it is given hold 11: Linux reads whatever follows the table.
// Here that is 0, an amplifier with no gain.
std::uint16_t v_rampAt(Slice<std::uint16_t> v_ramp,std::int64_t v_index)
{
if ((v_index < Len(v_ramp)))
{
return At(v_ramp,v_index);
}
return 0;
}
// gainApply is dib0090_gain_apply: moves the gain and the take-over point by
// the errors given and writes the amplifiers.
void m_tuner_gainApply(T_tuner* v_state,std::int16_t v_gainDelta,std::int16_t v_topDelta,bool v_force)
{
std::array<std::uint16_t, 4> v_gainReg{};
if ((v_topDelta < Neg<std::int16_t>(511)))
{
v_topDelta = Neg<std::int16_t>(511);
}
if ((v_topDelta > 511))
{
v_topDelta = 511;
}
if (v_force)
{
v_topDelta = static_cast<std::int16_t>(Mul<std::int64_t>(static_cast<std::int64_t>(v_topDelta),(Shl<std::int64_t>(1,v_wbdAlpha))));
v_gainDelta = static_cast<std::int16_t>(Mul<std::int64_t>(static_cast<std::int64_t>(v_gainDelta),(Shl<std::int64_t>(1,v_gainAlpha))));
}
if ([&]() { auto left = static_cast<std::int64_t>(v_topDelta); auto right = [&]() { auto left = static_cast<std::int64_t>(static_cast<std::int16_t>(Shl<std::uint16_t>(At((v_state)->v_rfRamp,0),v_wbdAlpha))); auto right = static_cast<std::int64_t>((v_state)->v_rfGainLimit); return Sub<std::int64_t>(left,right); }(); return (left >= right); }())
{
(v_state)->v_rfGainLimit = static_cast<std::int16_t>(Shl<std::uint16_t>(At((v_state)->v_rfRamp,0),v_wbdAlpha));
}
else
{
(v_state)->v_rfGainLimit += v_topDelta;
}
if (((v_state)->v_rfGainLimit < 0))
{
(v_state)->v_rfGainLimit = 0;
}
std::uint16_t v_gain = Shl<std::uint16_t>((Add<std::uint16_t>(static_cast<std::uint16_t>(Shr<std::int16_t>((v_state)->v_rfGainLimit,v_wbdAlpha)),At((v_state)->v_bbRamp,0))),v_gainAlpha);
if ([&]() { auto left = static_cast<std::int64_t>(v_gainDelta); auto right = [&]() { auto left = static_cast<std::int64_t>(static_cast<std::int16_t>(v_gain)); auto right = static_cast<std::int64_t>((v_state)->v_currentGain); return Sub<std::int64_t>(left,right); }(); return (left >= right); }())
{
(v_state)->v_currentGain = static_cast<std::int16_t>(v_gain);
}
else
{
(v_state)->v_currentGain += v_gainDelta;
}
if (((v_state)->v_currentGain < 0))
{
(v_state)->v_currentGain = 0;
}
v_gain = static_cast<std::uint16_t>(Shr<std::int16_t>((v_state)->v_currentGain,v_gainAlpha));
std::uint16_t v_rf{};
std::uint16_t v_bb{};
if ((v_gain > static_cast<std::uint16_t>(Shr<std::int16_t>((v_state)->v_rfGainLimit,v_wbdAlpha))))
{
v_rf = static_cast<std::uint16_t>(Shr<std::int16_t>((v_state)->v_rfGainLimit,v_wbdAlpha));
v_bb = Sub<std::uint16_t>(v_gain,v_rf);
if ((v_bb > At((v_state)->v_bbRamp,0)))
{
v_bb = At((v_state)->v_bbRamp,0);
}
}
else
{
v_rf = v_gain;
v_bb = 0;
}
At((v_state)->v_gain,0) = v_rf;
At((v_state)->v_gain,1) = v_bb;
auto tmp13_0 = (v_state)->v_rfRamp;
auto tmp13_1 = 1;
Slice<std::uint16_t> v_ramp = tmp13_0;
std::int64_t v_at = tmp13_1;
std::uint16_t v_ref = v_rf;
for (std::int64_t v_i = 0;(v_i < 7);v_i++)
{
auto tmp14_0 = static_cast<std::int64_t>(v_rampAt(v_ramp,v_at));
auto tmp14_1 = static_cast<std::int64_t>(v_rampAt(v_ramp,Add<std::int64_t>(v_at,1)));
auto tmp14_2 = static_cast<std::int64_t>(v_rampAt(v_ramp,Add<std::int64_t>(v_at,2)));
std::int64_t v_g0 = tmp14_0;
std::int64_t v_g1 = tmp14_1;
std::int64_t v_g2 = tmp14_2;
std::uint16_t v_v{};
{
do {
if ((((v_g0 == 0) || (static_cast<std::int64_t>(v_ref) < Sub<std::int64_t>(v_g1,v_g0))))) {
v_v = 0;
}
else if (((static_cast<std::int64_t>(v_ref) >= v_g1))) {
v_v = static_cast<std::uint16_t>(v_g2);
}
else
{
v_v = static_cast<std::uint16_t>(static_cast<std::int64_t>((Mul<std::int64_t>((Sub<std::int64_t>(static_cast<std::int64_t>(v_ref),(Sub<std::int64_t>(v_g1,v_g0)))),v_g2)) / (v_g0)));
}
} while(false); }
{
auto switch15 = v_i;
do {
if ((switch15 == 0)) {
At(v_gainReg,0) = v_v;
}
else if ((switch15 == 1)) {
At(v_gainReg,0) |= Shl<std::uint16_t>(v_v,7);
}
else if ((switch15 == 2)) {
At(v_gainReg,1) = v_v;
}
else if ((switch15 == 3)) {
At(v_gainReg,1) |= Shl<std::uint16_t>(v_v,7);
}
else if ((switch15 == 4)) {
At(v_gainReg,2) = static_cast<std::uint16_t>((v_v) | ((v_state)->v_rfLTDef));
}
else if ((switch15 == 5)) {
At(v_gainReg,3) = Shl<std::uint16_t>(v_v,3);
}
else if ((switch15 == 6)) {
At(v_gainReg,3) |= Shl<std::uint16_t>(v_v,8);
}
} while(false); }
v_at += 3;
if ((v_i == 4))
{
auto tmp16_0 = (v_state)->v_bbRamp;
auto tmp16_1 = 1;
v_ramp = tmp16_0;
v_at = tmp16_1;
v_ref = v_bb;
}
}
At(v_gainReg,3) |= (v_state)->v_bb1Def;
At(v_gainReg,3) |= static_cast<std::uint16_t>((Mul<std::uint16_t>((static_cast<std::uint16_t>((v_bb) % (10))),100)) / (125));
{ auto&& range17 = std::array<std::uint8_t, 4>{{0x08,0x0a,0x0f,0x01}};
for (std::int64_t index17=0; index17<Len(range17); ++index17) {
auto v_i = index17;
auto v_register = range17[index17];
std::uint16_t v_v = At(v_gainReg,v_i);
if ((v_force || (At((v_state)->v_gainReg,v_i) != v_v)))
{
At((v_state)->v_gainReg,v_i) = v_v;
m_tuner_writeReg(v_state,v_register,v_v);
}
}}
}
// gainControl is dib0090_gain_control: one step of the software AGC, run from
// agcStart until the tune state reaches agcStop. It returns how long to wait
// before the next step, in ms.
std::int64_t m_tuner_gainControl(T_tuner* v_state)
{
std::int64_t v_ret = 10;
bool v_applyGainImmediately = true;
std::int16_t v_wbdError{};
std::int16_t v_adcError{};
if (((v_state)->v_tuneState == v_agcStart))
{
(v_state)->v_agcFreeze = 0;
m_tuner_writeReg(v_state,0x04,0x0);
{
do {
if (((((v_state)->v_currentBand == v_bandVHF) && (((v_state)->v_identity).v_p1g == 0)))) {
m_tuner_setRFRamp(v_state,v_rfRampPWMVHF);
m_tuner_setBBRamp(v_state,v_bbRampPWMNormal);
}
else if (((((v_state)->v_currentBand == v_bandCBand) && (((v_state)->v_identity).v_p1g == 0)))) {
m_tuner_setRFRamp(v_state,v_rfRampPWMCBand);
m_tuner_setBBRamp(v_state,v_bbRampPWMNormal);
}
else if (((((((v_state)->v_currentBand == v_bandCBand) || ((v_state)->v_currentBand == v_bandVHF))) && (((v_state)->v_identity).v_p1g != 0)))) {
m_tuner_setRFRamp(v_state,v_rfRampPWMCBand7090P);
m_tuner_setBBRamp(v_state,v_bbRampPWMNormalSoCs);
}
else
{
m_tuner_setRFRamp(v_state,v_rfRampPWMUHF);
m_tuner_setBBRamp(v_state,v_bbRampPWMNormal);
}
} while(false); }
m_tuner_writeReg(v_state,0x32,0);
m_tuner_writeReg(v_state,0x39,0);
m_tuner_wbdTargetFor(v_state);
(v_state)->v_rfGainLimit = static_cast<std::int16_t>(Shl<std::uint16_t>(At((v_state)->v_rfRamp,0),v_wbdAlpha));
(v_state)->v_currentGain = static_cast<std::int16_t>(Shl<std::uint16_t>(static_cast<std::uint16_t>(((Add<std::uint16_t>(At((v_state)->v_rfRamp,0),At((v_state)->v_bbRamp,0)))) / (2)),v_gainAlpha));
(v_state)->v_tuneState = v_agcStep0;
}
else
if (((v_state)->v_agcFreeze == 0))
{
std::int64_t v_cnt = 1;
if (((v_state)->v_tuneState == v_agcStep0))
{
v_cnt = 5;
}
std::int16_t v_wbd{};
for (std::int64_t v_i = 0;(v_i < v_cnt);v_i++)
{
v_wbd += m_tuner_wbdToDB(v_state,static_cast<std::uint16_t>(m_tuner_getSlowADCVal(v_state)));
}
v_wbd /= static_cast<std::int16_t>(v_cnt);
v_wbdError = Sub<std::int16_t>((v_state)->v_wbdTarget,v_wbd);
if (((v_state)->v_tuneState == v_agcStep0))
{
if ((((v_wbdError < 0) && ((v_state)->v_rfGainLimit > 0)) && (((v_state)->v_identity).v_p1g == 0)))
{
std::uint16_t v_ltg2 = static_cast<std::uint16_t>(((Shr<std::uint16_t>((v_state)->v_rfLTDef,10))) & (0x7));
if ((((v_state)->v_currentBand == v_bandCBand) && (v_ltg2 != 0)))
{
v_ltg2 >>= 1;
(v_state)->v_rfLTDef &= Shl<std::uint16_t>(v_ltg2,10);
}
}
else
{
(v_state)->v_agcStep = 0;
(v_state)->v_tuneState = v_agcStep1;
}
}
else
{
std::int32_t v_adc = (v_state)->v_getADCPower();
v_adc = static_cast<std::int32_t>(Shr<std::int64_t>((Add<std::int64_t>(Mul<std::int64_t>(static_cast<std::int64_t>(v_adc),355774),Shl<std::int64_t>(1,20))),21));
v_adcError = static_cast<std::int16_t>(Sub<std::int32_t>(v_adcTarget,v_adc));
if (((v_state)->v_tuneState == v_agcStep1))
{
std::uint8_t v_step = (v_state)->v_agcStep;
(v_state)->v_agcStep++;
if (((v_abs(static_cast<std::int64_t>(v_adcError)) < 50) || (v_step > 5)))
{
m_tuner_writeReg(v_state,0x02,static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((Shl<std::int64_t>(1,15)) | (Shl<std::int64_t>(3,11)))) | (Shl<std::int64_t>(6,6)))) | (32)));
m_tuner_writeReg(v_state,0x04,0x01);
(v_state)->v_tuneState = v_agcStop;
}
}
else
{
v_ret = 100;
v_applyGainImmediately = false;
}
}
}
if (((v_state)->v_agcFreeze == 0))
{
m_tuner_gainApply(v_state,v_adcError,v_wbdError,v_applyGainImmediately);
}
return v_ret;
}
// setRFRamp is dib0090_set_rframp.
void m_tuner_setRFRamp(T_tuner* v_state,Slice<std::uint16_t> v_cfg)
{
(v_state)->v_rfRamp = v_cfg;
}
// setBBRamp is dib0090_set_bbramp.
void m_tuner_setBBRamp(T_tuner* v_state,Slice<std::uint16_t> v_cfg)
{
(v_state)->v_bbRamp = v_cfg;
m_tuner_setBoost(v_state,v_boolWord((At(v_cfg,0) > 500)));
}
// setDCServo is dib0090_set_dc_servo.
void m_tuner_setDCServo(T_tuner* v_state,std::uint8_t v_dcServoCutoff)
{
if ((v_dcServoCutoff < 4))
{
m_tuner_writeReg(v_state,0x04,static_cast<std::uint16_t>(v_dcServoCutoff));
}
}
// getSlowADCVal is dib0090_get_slow_adc_val.
std::uint32_t m_tuner_getSlowADCVal(T_tuner* v_state)
{
return static_cast<std::uint32_t>(m_tuner_readReg(v_state,0x1d));
}
// slopesToScale is slopes_to_scale.
std::uint16_t v_slopesToScale(Slice<T_slope> v_slopes,std::int16_t v_val)
{
std::uint16_t v_rest{};
std::uint16_t v_ret{};
{ auto&& range18 = v_slopes;
for (std::int64_t index18=0; index18<Len(range18); ++index18) {
auto v_i = index18;
if ((v_val > (At(v_slopes,v_i)).v_rangeValue))
{
v_rest = static_cast<std::uint16_t>((At(v_slopes,v_i)).v_rangeValue);
}
else
{
v_rest = static_cast<std::uint16_t>(v_val);
}
v_ret += static_cast<std::uint16_t>([&]() { auto left = ([&]() { auto left = static_cast<std::int64_t>(v_rest); auto right = static_cast<std::int64_t>((At(v_slopes,v_i)).v_slope); return Mul<std::int64_t>(left,right); }()); auto right = static_cast<std::int64_t>((At(v_slopes,v_i)).v_rangeValue); return static_cast<std::int64_t>((left) / (right)); }());
v_val -= static_cast<std::int16_t>(v_rest);
}}
return v_ret;
}
// wbdToDB is dib0090_wbd_to_db.
std::int16_t m_tuner_wbdToDB(T_tuner* v_state,std::uint16_t v_wbd)
{
v_wbd &= 0x3ff;
if ((v_wbd < (v_state)->v_wbdOffset))
{
v_wbd = 0;
}
else
{
v_wbd -= (v_state)->v_wbdOffset;
}
return Add<std::int16_t>(Neg<std::int16_t>(640),static_cast<std::int16_t>(v_slopesToScale(v_dib0090WBDSlopes,static_cast<std::int16_t>(v_wbd))));
}
// getWBDTarget is dib0090_get_wbd_target: the wideband-detector target for
// the channel, compensated for the temperature measured at the last tune.
std::uint16_t m_tuner_getWBDTarget(T_tuner* v_state)
{
std::uint32_t v_fMHz = static_cast<std::uint32_t>((((v_state)->v_channel)->v_frequency) / (1000000));
std::int32_t v_currentTemp = static_cast<std::int32_t>((v_state)->v_temperature);
std::int64_t v_index = 0;
for (;(v_fMHz > static_cast<std::uint32_t>((At((v_state)->v_currentWBDTable,v_index)).v_maxFreq));)
{
v_index++;
}
T_wbdSlope* v_wbd = (&At((v_state)->v_currentWBDTable,v_index));
if ((v_currentTemp < 0))
{
v_currentTemp = 0;
}
if ((v_currentTemp > 128))
{
v_currentTemp = 128;
}
(v_state)->v_wbdmux = static_cast<std::uint16_t>((v_state)->v_wbdmux & ~(Shl<std::uint16_t>(7,13)));
if (((v_wbd)->v_wbdGain != 0))
{
(v_state)->v_wbdmux |= Shl<std::uint16_t>(static_cast<std::uint16_t>((v_wbd)->v_wbdGain),13);
}
else
{
(v_state)->v_wbdmux |= Shl<std::uint16_t>(4,13);
}
m_tuner_writeReg(v_state,0x10,(v_state)->v_wbdmux);
std::int32_t v_wbdThot = static_cast<std::int32_t>([&]() { auto left = static_cast<std::uint32_t>((v_wbd)->v_offsetHot); auto right = Shr<std::uint32_t>((Mul<std::uint32_t>(static_cast<std::uint32_t>((v_wbd)->v_slopeHot),v_fMHz)),6); return Sub<std::uint32_t>(left,right); }());
std::int32_t v_wbdTcold = static_cast<std::int32_t>([&]() { auto left = static_cast<std::uint32_t>((v_wbd)->v_offsetCold); auto right = Shr<std::uint32_t>((Mul<std::uint32_t>(static_cast<std::uint32_t>((v_wbd)->v_slopeCold),v_fMHz)),6); return Sub<std::uint32_t>(left,right); }());
v_wbdTcold += Shr<std::int32_t>((Mul<std::int32_t>((Sub<std::int32_t>(v_wbdThot,v_wbdTcold)),v_currentTemp)),7);
(v_state)->v_wbdTarget = m_tuner_wbdToDB(v_state,static_cast<std::uint16_t>(Add<std::int32_t>(static_cast<std::int32_t>((v_state)->v_wbdOffset),v_wbdTcold)));
return static_cast<std::uint16_t>(Add<std::int32_t>(static_cast<std::int32_t>((v_state)->v_wbdOffset),v_wbdTcold));
}
// setDefaultConfig is dib0090_set_default_config: runs of a count, the first
// register, then the values, until a zero count.
void m_tuner_setDefaultConfig(T_tuner* v_state,Slice<std::uint16_t> v_n)
{
for (;true;)
{
std::uint16_t v_count = At(v_n,0);
if ((v_count == 0))
{
return ;
}
std::uint8_t v_register = static_cast<std::uint8_t>(At(v_n,1));
v_n = Subslice(v_n,2,Len(v_n));
for (;(v_count > 0);v_count--)
{
m_tuner_writeReg(v_state,v_register,At(v_n,0));
v_n = Subslice(v_n,1,Len(v_n));
v_register++;
}
}
}
// setEFuse is dib0090_set_EFUSE: the baseband trims burnt in at the factory,
// or computed from a measurement when the fuses are blank.
void m_tuner_setEFuse(T_tuner* v_state)
{
std::uint8_t v_c{};
std::uint8_t v_h{};
std::uint8_t v_n{};
std::uint16_t v_e2 = m_tuner_readReg(v_state,0x26);
std::uint16_t v_e4 = m_tuner_readReg(v_state,0x28);
if ((((((v_state)->v_identity).v_version == v_p1DEF) || (((v_state)->v_identity).v_version == v_p1G)) || (v_e2 == 0xffff)))
{
m_tuner_writeReg(v_state,0x22,0x10);
std::uint16_t v_cal = static_cast<std::uint16_t>(((Shr<std::uint16_t>(m_tuner_readReg(v_state,0x22),6))) & (0x3ff));
if (((v_cal < 670) || (v_cal == 1023)))
{
v_cal = 850;
}
v_n = static_cast<std::uint8_t>(Sub<std::int64_t>(165,(Shr<std::int64_t>((Mul<std::int64_t>(static_cast<std::int64_t>(v_cal),10)),6))));
v_e2 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::int64_t>(3,12)) | (Shl<std::int64_t>(34,6)))) | (static_cast<std::uint16_t>(v_n)));
v_e4 = v_e2;
}
if ((v_e2 != v_e4))
{
v_e2 &= v_e4;
}
if ((v_e2 != 0xffff))
{
v_c = static_cast<std::uint8_t>(static_cast<std::uint16_t>((v_e2) & (0x3f)));
v_n = static_cast<std::uint8_t>(static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_e2,12))) & (0xf)));
v_h = static_cast<std::uint8_t>(static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_e2,6))) & (0x3f)));
if (((v_c >= v_capValueMax) || (v_c <= v_capValueMin)))
{
v_c = 32;
}
else
{
v_c += 14;
}
if (((v_h >= v_hrMax) || (v_h <= v_hrMin)))
{
v_h = 34;
}
if (((v_n >= v_polyMax) || (v_n <= v_polyMin)))
{
v_n = 3;
}
m_tuner_writeReg(v_state,0x13,Shl<std::uint16_t>(static_cast<std::uint16_t>(v_h),10));
v_e2 = [&]() { auto left = [&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>(v_n),11); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>(Shr<std::uint8_t>(v_h,2)),6); return static_cast<std::uint16_t>((left) | (right)); }(); auto right = static_cast<std::uint16_t>(v_c); return static_cast<std::uint16_t>((left) | (right)); }();
m_tuner_writeReg(v_state,0x2,v_e2);
}
}
// reset is dib0090_reset.
Error m_tuner_reset(T_tuner* v_state)
{
m_tuner_resetDigital(v_state);
{
Error v_err = m_tuner_identify(v_state);
if ((v_err != nullptr))
{
return v_err;
}
}
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(m_tuner_readReg(v_state,0x1a),5))) & (0x2)) != 0))
{
m_tuner_writeReg(v_state,0x1b,static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((v_enIQADC) | (v_enBB))) | (v_enBias))) | (v_enDigClk))) | (v_enPLL))) | (v_enCrystal)));
}
else
{
m_tuner_writeReg(v_state,0x1b,static_cast<std::uint16_t>((static_cast<std::int64_t>((v_enDigClk) | (v_enPLL))) | (v_enCrystal)));
}
m_tuner_setDefaultConfig(v_state,v_dib0090Defaults);
if ((((v_state)->v_identity).v_p1g != 0))
{
m_tuner_setDefaultConfig(v_state,v_dib0090P1GAdditionalDefaults);
}
if ((static_cast<std::uint8_t>((((v_state)->v_identity).v_version) & (0x1f)) >= v_p1DEF))
{
m_tuner_setEFuse(v_state);
}
if ((((v_state)->v_config)->v_forceCrystalMode != 0))
{
m_tuner_writeReg(v_state,0x14,static_cast<std::uint16_t>(static_cast<std::uint8_t>((((v_state)->v_config)->v_forceCrystalMode) & (3))));
}
else
if (((((v_state)->v_config)->v_io).v_clockKHz >= 24000))
{
m_tuner_writeReg(v_state,0x14,1);
}
else
{
m_tuner_writeReg(v_state,0x14,2);
}
(v_state)->v_calibrate = static_cast<std::uint8_t>((static_cast<std::int64_t>((v_dcCal) | (v_wbdCal))) | (v_tempCal));
return ((v_state)->v_failure)->v_err;
}
// steps is the steps() macro of the DC calibration.
std::int8_t v_steps(std::int8_t v_u)
{
if ((v_u > 15))
{
return Sub<std::int8_t>(v_u,16);
}
return v_u;
}
// getOffset is dib0090_get_offset.
std::int64_t m_tuner_getOffset(T_tuner* v_state)
{
std::int64_t v_ret = Mul<std::int64_t>(v_internWait,10);
{
auto switch19 = (v_state)->v_tuneState;
do {
if ((switch19 == v_tunerStep2)) {
m_tuner_writeReg(v_state,0x1f,0x7);
(v_state)->v_tuneState = v_tunerStep3;
}
else if ((switch19 == v_tunerStep3)) {
(v_state)->v_adcDiff = static_cast<std::int16_t>(m_tuner_readReg(v_state,0x1d));
m_tuner_writeReg(v_state,0x1f,0x4);
(v_state)->v_tuneState = v_tunerStep4;
}
else if ((switch19 == v_tunerStep4)) {
(v_state)->v_adcDiff = static_cast<std::int16_t>([&]() { auto left = static_cast<std::int64_t>((v_state)->v_adcDiff); auto right = static_cast<std::int64_t>(m_tuner_readReg(v_state,0x1d)); return Sub<std::int64_t>(left,right); }());
(v_state)->v_tuneState = v_tunerStep5;
v_ret = 0;
}
} while(false); }
return v_ret;
}
// setTrim is dib0090_set_trim.
void m_tuner_setTrim(T_tuner* v_state)
{
T_dcCalibration* v_dc = (&At((v_state)->v_dc,(v_state)->v_dcNext));
std::uint16_t* v_val = (&(v_state)->v_bb6);
if (((v_dc)->v_addr == 0x07))
{
v_val = (&(v_state)->v_bb7);
}
(*v_val) = static_cast<std::uint16_t>((*v_val) & ~(Shl<std::uint16_t>(0x1f,(v_dc)->v_offset)));
(*v_val) |= static_cast<std::uint16_t>(Shl<std::int32_t>(static_cast<std::int32_t>((v_state)->v_step),(v_dc)->v_offset));
m_tuner_writeReg(v_state,(v_dc)->v_addr,(*v_val));
}
std::int64_t v_abs(std::int64_t v_value)
{
if ((v_value < 0))
{
return Neg<std::int64_t>(v_value);
}
return v_value;
}
// dcOffsetCalibration is dib0090_dc_offset_calibration.
std::int64_t m_tuner_dcOffsetCalibration(T_tuner* v_state)
{
std::int64_t v_ret = 0;
{
auto switch20 = (v_state)->v_tuneState;
do {
if ((switch20 == v_tunerStart)) {
(v_state)->v_bb6 = 0;
(v_state)->v_bb7 = 0x040d;
std::uint16_t v_reg = static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x24)) & (0x0ffb));
m_tuner_writeReg(v_state,0x24,v_reg);
(v_state)->v_wbdmux = m_tuner_readReg(v_state,0x10);
m_tuner_writeReg(v_state,0x10,static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>(((v_state)->v_wbdmux) & ~((Shl<std::uint16_t>(0xff,3))))) | (Shl<std::uint16_t>(0x7,3)))) | (0x3)));
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x23)) & ~((Shl<std::uint16_t>(1,14)))));
(v_state)->v_dc = v_dcTable;
if ((((v_state)->v_identity).v_p1g != 0))
{
(v_state)->v_dc = v_dcP1GTable;
}
(v_state)->v_dcNext = 0;
m_tuner_dcStep0(v_state);
v_ret = 50;
}
else if ((switch20 == v_tunerStep0)) {
m_tuner_dcStep0(v_state);
v_ret = 50;
}
else if ((switch20 == v_tunerStep1)) {
m_tuner_setTrim(v_state);
(v_state)->v_tuneState = v_tunerStep2;
}
else if ((switch20 == v_tunerStep2) || (switch20 == v_tunerStep3) || (switch20 == v_tunerStep4)) {
v_ret = m_tuner_getOffset(v_state);
}
else if ((switch20 == v_tunerStep5)) {
T_dcCalibration* v_dc = (&At((v_state)->v_dc,(v_state)->v_dcNext));
if ((((v_state)->v_step == 0) && ((v_state)->v_adcDiff < 0)))
{
(v_state)->v_minADCDiff = Neg<std::int16_t>(1023);
}
if (((v_state)->v_step == 0))
{
if ((((v_dc)->v_pga != 0) && ((v_state)->v_adcDiff < 0)))
{
(v_state)->v_step = 0x10;
}
if ((((v_dc)->v_pga == 0) && ((v_state)->v_adcDiff > 0)))
{
(v_state)->v_step = 0x10;
}
}
if (([&]() { auto left = static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_state)->v_adcDiff)) & (0x8000)); auto right = static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_state)->v_minADCDiff)) & (0x8000)); return (left == right); }() && (v_steps((v_state)->v_step) < 15)))
{
(v_state)->v_step++;
(v_state)->v_minADCDiff = (v_state)->v_adcDiff;
(v_state)->v_tuneState = v_tunerStep1;
}
else
{
if ([&]() { auto left = v_abs(static_cast<std::int64_t>((v_state)->v_adcDiff)); auto right = v_abs(static_cast<std::int64_t>((v_state)->v_minADCDiff)); return (left > right); }())
{
(v_state)->v_step--;
}
m_tuner_setTrim(v_state);
(v_state)->v_dcNext++;
if (((At((v_state)->v_dc,(v_state)->v_dcNext)).v_addr == 0))
{
(v_state)->v_tuneState = v_tunerStep6;
}
else
{
(v_state)->v_tuneState = v_tunerStep0;
}
}
}
else if ((switch20 == v_tunerStep6)) {
m_tuner_writeReg(v_state,0x07,static_cast<std::uint16_t>(((v_state)->v_bb7) & ~(0x0008)));
m_tuner_writeReg(v_state,0x1f,0x7);
(v_state)->v_tuneState = v_tunerStart;
(v_state)->v_calibrate = static_cast<std::uint8_t>((v_state)->v_calibrate & ~(v_dcCal));
}
} while(false); }
return v_ret;
}
// dcStep0 is CT_TUNER_STEP_0 of the DC calibration, which CT_TUNER_START
// falls through into.
void m_tuner_dcStep0(T_tuner* v_state)
{
T_dcCalibration* v_dc = (&At((v_state)->v_dc,(v_state)->v_dcNext));
m_tuner_writeReg(v_state,0x01,(v_dc)->v_bb1);
m_tuner_writeReg(v_state,0x07,static_cast<std::uint16_t>(((v_state)->v_bb7) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_dc)->v_i),7))));
(v_state)->v_step = 0;
(v_state)->v_minADCDiff = 1023;
(v_state)->v_tuneState = v_tunerStep1;
}
// wbdCalibration is dib0090_wbd_calibration.
std::int64_t m_tuner_wbdCalibration(T_tuner* v_state)
{
std::uint8_t v_wbdGain{};
{
auto switch21 = (v_state)->v_tuneState;
do {
if ((switch21 == v_tunerStart)) {
std::int64_t v_index = 0;
for (;(static_cast<std::uint32_t>(((v_state)->v_currentRF) / (1000)) > static_cast<std::uint32_t>((At((v_state)->v_currentWBDTable,v_index)).v_maxFreq));)
{
v_index++;
}
T_wbdSlope* v_wbd = (&At((v_state)->v_currentWBDTable,v_index));
if (((v_wbd)->v_wbdGain != 0))
{
v_wbdGain = (v_wbd)->v_wbdGain;
}
else
{
v_wbdGain = 4;
}
if ((v_wbdGain == (v_state)->v_wbdCalibrationGain))
{
(v_state)->v_tuneState = v_tunerStart;
(v_state)->v_calibrate = static_cast<std::uint8_t>((v_state)->v_calibrate & ~(v_wbdCal));
return 0;
}
m_tuner_writeReg(v_state,0x10,static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((0x1b81) | (Shl<std::int64_t>(1,10)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>(v_wbdGain),13)))) | (Shl<std::uint16_t>(1,3))));
m_tuner_writeReg(v_state,0x24,static_cast<std::uint16_t>(((static_cast<std::int64_t>((v_enUHF) & (0x0fff)))) | (Shl<std::int64_t>(1,1))));
(v_state)->v_tuneState = v_tunerStep0;
(v_state)->v_wbdCalibrationGain = v_wbdGain;
return 90;
}
else if ((switch21 == v_tunerStep0)) {
(v_state)->v_wbdOffset = static_cast<std::uint16_t>(m_tuner_getSlowADCVal(v_state));
(v_state)->v_tuneState = v_tunerStart;
(v_state)->v_calibrate = static_cast<std::uint8_t>((v_state)->v_calibrate & ~(v_wbdCal));
}
} while(false); }
return 0;
}
// setBandwidth is dib0090_set_bandwidth.
void m_tuner_setBandwidth(T_tuner* v_state)
{
std::uint16_t v_tmp{};
std::uint32_t v_bandwidthKHz = static_cast<std::uint32_t>((((v_state)->v_channel)->v_bandwidthHz) / (1000));
{
do {
if (((v_bandwidthKHz <= 5000))) {
v_tmp = Shl<std::uint16_t>(3,14);
}
else if (((v_bandwidthKHz <= 6000))) {
v_tmp = Shl<std::uint16_t>(2,14);
}
else if (((v_bandwidthKHz <= 7000))) {
v_tmp = Shl<std::uint16_t>(1,14);
}
else
{
v_tmp = Shl<std::uint16_t>(0,14);
}
} while(false); }
(v_state)->v_bb1Def &= 0x3fff;
(v_state)->v_bb1Def |= v_tmp;
m_tuner_writeReg(v_state,0x01,(v_state)->v_bb1Def);
m_tuner_writeReg(v_state,0x03,0x6008);
m_tuner_writeReg(v_state,0x04,0x1);
m_tuner_writeReg(v_state,0x02,static_cast<std::uint16_t>((static_cast<std::int64_t>((Shl<std::int64_t>(5,11)) | (Shl<std::int64_t>(8,6)))) | (static_cast<std::int64_t>((22) & (0x3f)))));
m_tuner_writeReg(v_state,0x05,0xabcd);
}
// captrimSearch is dib0090_captrim_search. P1G silicon searches in hardware;
// earlier revisions binary-search the capacitor trim against the slow ADC.
std::int64_t m_tuner_captrimSearch(T_tuner* v_state)
{
std::int64_t v_ret = 0;
std::uint16_t v_lo4 = static_cast<std::uint16_t>(0xe900);
{
auto switch22 = (v_state)->v_tuneState;
do {
if ((switch22 == v_tunerStart)) {
m_tuner_writeReg(v_state,0x10,0x2B1);
m_tuner_writeReg(v_state,0x1e,0x0032);
if (((v_state)->v_tunerIsTuned == 0))
{
if ((((v_state)->v_identity).v_p1g == 0))
{
(v_state)->v_step = 64;
(v_state)->v_captrim = 64;
(v_state)->v_fcaptrim = 64;
}
(v_state)->v_currentRF = (v_state)->v_rfRequest;
}
else
{
if ((((v_state)->v_identity).v_p1g == 0))
{
(v_state)->v_step = 4;
(v_state)->v_captrim = static_cast<std::int8_t>(static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x18)) & (0x7f)));
(v_state)->v_fcaptrim = (v_state)->v_captrim;
}
}
(v_state)->v_adcDiff = 3000;
(v_state)->v_tuneState = v_tunerStep0;
}
else if ((switch22 == v_tunerStep0)) {
if ((((v_state)->v_identity).v_p1g != 0))
{
std::uint16_t v_ratio = static_cast<std::uint16_t>(31);
m_tuner_writeReg(v_state,0x40,static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>(3,7)) | (Shl<std::uint16_t>(v_ratio,2)))) | (Shl<std::uint16_t>(1,1)))) | (1)));
m_tuner_readReg(v_state,0x40);
v_ret = 50;
}
else
{
(v_state)->v_step /= 2;
m_tuner_writeReg(v_state,0x18,static_cast<std::uint16_t>((v_lo4) | (static_cast<std::uint16_t>((v_state)->v_captrim))));
}
(v_state)->v_tuneState = v_tunerStep1;
}
else if ((switch22 == v_tunerStep1)) {
if ((((v_state)->v_identity).v_p1g != 0))
{
m_tuner_writeReg(v_state,0x40,static_cast<std::uint16_t>((static_cast<std::int64_t>((0x18c) | (Shl<std::int64_t>(0,1)))) | (0)));
m_tuner_readReg(v_state,0x40);
(v_state)->v_fcaptrim = static_cast<std::int8_t>(static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x18)) & (0x7F)));
(v_state)->v_tuneState = v_tunerStep3;
}
else
{
std::uint16_t v_adc = static_cast<std::uint16_t>(m_tuner_getSlowADCVal(v_state));
std::int16_t v_adcTarget{};
std::int8_t v_stepSign{};
if (((v_state)->v_rest == 0))
{
v_adcTarget = 200;
}
else
{
v_adcTarget = 400;
}
if ([&]() { auto left = static_cast<std::int64_t>(v_adc); auto right = static_cast<std::int64_t>(v_adcTarget); return (left >= right); }())
{
v_adc -= static_cast<std::uint16_t>(v_adcTarget);
v_stepSign = Neg<std::int8_t>(1);
}
else
{
v_adc = Sub<std::uint16_t>(static_cast<std::uint16_t>(v_adcTarget),v_adc);
v_stepSign = 1;
}
if ([&]() { auto left = static_cast<std::int64_t>(v_adc); auto right = static_cast<std::int64_t>((v_state)->v_adcDiff); return (left < right); }())
{
(v_state)->v_adcDiff = static_cast<std::int16_t>(v_adc);
(v_state)->v_fcaptrim = (v_state)->v_captrim;
}
(v_state)->v_captrim += Mul<std::int8_t>(v_stepSign,(v_state)->v_step);
if (((v_state)->v_step >= 1))
{
(v_state)->v_tuneState = v_tunerStep0;
}
else
{
(v_state)->v_tuneState = v_tunerStep2;
}
v_ret = 25;
}
}
else if ((switch22 == v_tunerStep2)) {
m_tuner_writeReg(v_state,0x18,static_cast<std::uint16_t>((v_lo4) | (static_cast<std::uint16_t>((v_state)->v_fcaptrim))));
(v_state)->v_tuneState = v_tunerStep3;
}
else if ((switch22 == v_tunerStep3)) {
(v_state)->v_calibrate = static_cast<std::uint8_t>((v_state)->v_calibrate & ~(v_captrimCal));
(v_state)->v_tuneState = v_tunerStep0;
}
} while(false); }
return v_ret;
}
// getTemperature is dib0090_get_temperature.
std::int64_t m_tuner_getTemperature(T_tuner* v_state)
{
std::int64_t v_ret = 15;
{
auto switch23 = (v_state)->v_tuneState;
do {
if ((switch23 == v_tunerStart)) {
(v_state)->v_wbdmux = m_tuner_readReg(v_state,0x10);
m_tuner_writeReg(v_state,0x10,static_cast<std::uint16_t>((static_cast<std::uint16_t>(((v_state)->v_wbdmux) & ~((Shl<std::uint16_t>(0xff,3))))) | (Shl<std::uint16_t>(0x8,3))));
(v_state)->v_bias = m_tuner_readReg(v_state,0x13);
m_tuner_writeReg(v_state,0x13,static_cast<std::uint16_t>(((v_state)->v_bias) | (Shl<std::uint16_t>(0x3,8))));
(v_state)->v_tuneState = v_tunerStep0;
}
else if ((switch23 == v_tunerStep0)) {
(v_state)->v_adcDiff = static_cast<std::int16_t>(m_tuner_getSlowADCVal(v_state));
m_tuner_writeReg(v_state,0x13,static_cast<std::uint16_t>((static_cast<std::uint16_t>(((v_state)->v_bias) & ~((Shl<std::uint16_t>(0x3,8))))) | (Shl<std::uint16_t>(0x2,8))));
(v_state)->v_tuneState = v_tunerStep1;
}
else if ((switch23 == v_tunerStep1)) {
std::int16_t v_val = static_cast<std::int16_t>(m_tuner_getSlowADCVal(v_state));
(v_state)->v_temperature = Add<std::int16_t>(static_cast<std::int16_t>(Shr<std::int64_t>(static_cast<std::int64_t>(static_cast<std::int16_t>(Mul<std::int64_t>(([&]() { auto left = static_cast<std::int64_t>(v_val); auto right = static_cast<std::int64_t>((v_state)->v_adcDiff); return Sub<std::int64_t>(left,right); }()),180))),8)),55);
(v_state)->v_tuneState = v_tunerStep2;
}
else if ((switch23 == v_tunerStep2)) {
m_tuner_writeReg(v_state,0x13,(v_state)->v_bias);
m_tuner_writeReg(v_state,0x10,(v_state)->v_wbdmux);
(v_state)->v_tuneState = v_tunerStart;
(v_state)->v_calibrate = static_cast<std::uint8_t>((v_state)->v_calibrate & ~(v_tempCal));
if ((((v_state)->v_config)->v_analogOutput == 0))
{
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x23)) | (Shl<std::uint16_t>(1,14))));
}
}
else
{
v_ret = 0;
}
} while(false); }
return v_ret;
}
// tune is dib0090_tune, one step of the tuner's state machine. It returns
// the time to wait before the next step in 0.1ms units.
std::uint32_t m_tuner_tune(T_tuner* v_state)
{
T_tuningEntry* v_tune = (v_state)->v_currentTuneTable;
T_pllEntry* v_pll = (v_state)->v_currentPLLTable;
T_channel* v_c = (v_state)->v_channel;
std::uint16_t v_lo5{};
std::uint16_t v_lo6{};
std::uint16_t v_den{};
std::uint16_t v_tmp{};
std::uint32_t v_fbDiv{};
std::uint32_t v_rest{};
std::uint32_t v_fRef{};
std::uint32_t v_vcofKHz{};
std::uint32_t v_ret = static_cast<std::uint32_t>(10);
if (((v_state)->v_tuneState == v_tunerStart))
{
if ((static_cast<std::uint8_t>(((v_state)->v_calibrate) & ((static_cast<std::uint8_t>((static_cast<std::int64_t>((v_dcCal) | (v_tempCal))) | (v_wbdCal))))) != 0))
{
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x23)) & ~((Shl<std::uint16_t>(1,14)))));
}
else
if ((((v_state)->v_config)->v_analogOutput == 0))
{
m_tuner_writeReg(v_state,0x23,static_cast<std::uint16_t>((m_tuner_readReg(v_state,0x23)) | (Shl<std::uint16_t>(1,14))));
}
}
{
do {
if (((static_cast<std::uint8_t>(((v_state)->v_calibrate) & (v_dcCal)) != 0))) {
return static_cast<std::uint32_t>(m_tuner_dcOffsetCalibration(v_state));
}
else if (((static_cast<std::uint8_t>(((v_state)->v_calibrate) & (v_wbdCal)) != 0))) {
if (((v_state)->v_currentRF == 0))
{
(v_state)->v_currentRF = static_cast<std::uint32_t>(((v_c)->v_frequency) / (1000));
}
return static_cast<std::uint32_t>(m_tuner_wbdCalibration(v_state));
}
else if (((static_cast<std::uint8_t>(((v_state)->v_calibrate) & (v_tempCal)) != 0))) {
return static_cast<std::uint32_t>(m_tuner_getTemperature(v_state));
}
else if (((static_cast<std::uint8_t>(((v_state)->v_calibrate) & (v_captrimCal)) != 0))) {
return static_cast<std::uint32_t>(m_tuner_captrimSearch(v_state));
}
} while(false); }
{
auto switch24 = (v_state)->v_tuneState;
do {
if ((switch24 == v_tunerStart)) {
(v_state)->v_currentBand = v_bandOfFrequency(static_cast<std::uint32_t>(((v_c)->v_frequency) / (1000)));
std::int32_t v_offset = ((v_state)->v_config)->v_freqOffsetKHzVHF;
if (((v_state)->v_currentBand == v_bandUHF))
{
v_offset = ((v_state)->v_config)->v_freqOffsetKHzUHF;
}
(v_state)->v_rfRequest = Add<std::uint32_t>(static_cast<std::uint32_t>(((v_c)->v_frequency) / (1000)),static_cast<std::uint32_t>(v_offset));
if ((((v_state)->v_currentRF != (v_state)->v_rfRequest) || (static_cast<std::int64_t>((v_state)->v_currentStandard) != (v_c)->v_deliverySystem)))
{
(v_state)->v_tunerIsTuned = 0;
(v_state)->v_currentRF = 0;
(v_state)->v_currentStandard = 0;
Slice<T_tuningEntry> v_tuneTable = v_dib0090TuningTable;
if ((((v_state)->v_identity).v_p1g != 0))
{
v_tuneTable = v_dib0090P1GTuningTable;
}
v_tmp = static_cast<std::uint16_t>(static_cast<std::uint8_t>(((Shr<std::uint8_t>(((v_state)->v_identity).v_version,5))) & (0x7)));
if (((v_tmp == v_mp005) || (v_tmp == v_mp009)))
{
if (((((v_state)->v_currentBand == v_bandFM) || ((v_state)->v_currentBand == v_bandCBand)) || ((v_state)->v_currentBand == v_bandVHF)))
{
(v_state)->v_currentBand = v_bandCBand;
v_tuneTable = v_dib0090TuningTableFMVHFOnCBand;
if ((((v_state)->v_identity).v_p1g != 0))
{
v_tuneTable = v_dib0090P1GTuningTableFMVHFOnCBand;
}
}
}
Slice<T_pllEntry> v_pllTable = v_dib0090PLLTable;
if ((((v_state)->v_identity).v_p1g != 0))
{
v_pllTable = v_dib0090P1GPLLTable;
}
std::int64_t v_tuneIndex = 0;
for (;((v_state)->v_rfRequest > (At(v_tuneTable,v_tuneIndex)).v_maxFreq);)
{
v_tuneIndex++;
}
std::int64_t v_pllIndex = 0;
for (;((v_state)->v_rfRequest > (At(v_pllTable,v_pllIndex)).v_maxFreq);)
{
v_pllIndex++;
}
v_tune = (&At(v_tuneTable,v_tuneIndex));
v_pll = (&At(v_pllTable,v_pllIndex));
(v_state)->v_currentTuneTable = v_tune;
(v_state)->v_currentPLLTable = v_pll;
m_tuner_writeReg(v_state,0x0b,static_cast<std::uint16_t>((0xb800) | (static_cast<std::uint16_t>((v_tune)->v_switchTrim))));
v_vcofKHz = Mul<std::uint32_t>((Mul<std::uint32_t>(static_cast<std::uint32_t>((v_pll)->v_hfdiv),(v_state)->v_rfRequest)),2);
v_fRef = (((v_state)->v_config)->v_io).v_clockKHz;
if ((((v_state)->v_config)->v_fRefClockRatio != 0))
{
v_fRef /= static_cast<std::uint32_t>(((v_state)->v_config)->v_fRefClockRatio);
}
v_fbDiv = static_cast<std::uint32_t>((static_cast<std::uint32_t>((v_vcofKHz) / (static_cast<std::uint32_t>((v_pll)->v_topresc)))) / (v_fRef));
v_rest = Sub<std::uint32_t>(static_cast<std::uint32_t>((v_vcofKHz) / (static_cast<std::uint32_t>((v_pll)->v_topresc))),Mul<std::uint32_t>(v_fbDiv,v_fRef));
{
do {
if (((v_rest < v_lpf))) {
v_rest = 0;
}
else if (((v_rest < Mul<std::uint32_t>(2,v_lpf)))) {
v_rest = Mul<std::uint32_t>(2,v_lpf);
}
else if (((v_rest > Sub<std::uint32_t>(v_fRef,v_lpf)))) {
v_rest = 0;
v_fbDiv++;
}
else if (((v_rest > Sub<std::uint32_t>(v_fRef,Mul<std::uint32_t>(2,v_lpf))))) {
v_rest = Sub<std::uint32_t>(v_fRef,Mul<std::uint32_t>(2,v_lpf));
}
} while(false); }
v_rest = static_cast<std::uint32_t>(((Mul<std::uint32_t>(v_rest,6528))) / ((static_cast<std::uint32_t>((v_fRef) / (10)))));
(v_state)->v_rest = v_rest;
if ((v_rest == 0))
{
if (((v_pll)->v_vcoBand != 0))
{
v_lo5 = 0x049f;
}
else
{
v_lo5 = 0x041f;
}
}
else
{
{
do {
if ((((v_pll)->v_vcoBand != 0))) {
v_lo5 = 0x049e;
}
else if (((((v_state)->v_config)->v_analogOutput != 0))) {
v_lo5 = 0x041d;
}
else
{
v_lo5 = 0x041c;
}
} while(false); }
}
if ((((v_state)->v_identity).v_p1g != 0))
{
v_lo5 = 0x42c;
}
v_lo5 |= [&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_hfdivCode),11); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_vcoBand),7); return static_cast<std::uint16_t>((left) | (right)); }();
if (((((v_state)->v_config)->v_io).v_pllIntLoopFilt == 0))
{
if (((((v_state)->v_identity).v_p1g != 0) || (v_rest == 0)))
{
v_lo6 = 0xfff8;
}
else
{
v_lo6 = 0xff28;
}
}
else
{
v_lo6 = Shl<std::uint16_t>((((v_state)->v_config)->v_io).v_pllIntLoopFilt,3);
}
v_den = 1;
if ((v_rest > 0))
{
v_lo6 |= static_cast<std::uint16_t>((Shl<std::int64_t>(1,2)) | (2));
v_den = 255;
}
m_tuner_writeReg(v_state,0x15,static_cast<std::uint16_t>(v_fbDiv));
if ((((v_state)->v_config)->v_fRefClockRatio != 0))
{
m_tuner_writeReg(v_state,0x16,static_cast<std::uint16_t>((Shl<std::uint16_t>(v_den,8)) | (static_cast<std::uint16_t>(((v_state)->v_config)->v_fRefClockRatio))));
}
else
{
m_tuner_writeReg(v_state,0x16,static_cast<std::uint16_t>((Shl<std::uint16_t>(v_den,8)) | (1)));
}
m_tuner_writeReg(v_state,0x17,static_cast<std::uint16_t>(v_rest));
m_tuner_writeReg(v_state,0x19,v_lo5);
m_tuner_writeReg(v_state,0x1c,v_lo6);
v_lo6 = (v_tune)->v_tunerEnable;
if ((((v_state)->v_config)->v_analogOutput != 0))
{
v_lo6 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_lo6) & (0xff9f))) | (0x2));
}
m_tuner_writeReg(v_state,0x24,static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_lo6) | (v_enLO))) | (Mul<std::uint16_t>(static_cast<std::uint16_t>(((v_state)->v_config)->v_usePWMAGC),v_enCrystal))));
}
(v_state)->v_currentRF = (v_state)->v_rfRequest;
(v_state)->v_currentStandard = static_cast<std::uint8_t>((v_c)->v_deliverySystem);
v_ret = 20;
(v_state)->v_calibrate = v_captrimCal;
}
else if ((switch24 == v_tunerStep0)) {
std::int64_t v_index = 0;
for (;(static_cast<std::uint32_t>(((v_state)->v_currentRF) / (1000)) > static_cast<std::uint32_t>((At((v_state)->v_currentWBDTable,v_index)).v_maxFreq));)
{
v_index++;
}
T_wbdSlope* v_wbd = (&At((v_state)->v_currentWBDTable,v_index));
m_tuner_writeReg(v_state,0x1e,0x07ff);
std::uint16_t v_gain = static_cast<std::uint16_t>(4);
std::uint16_t v_i = static_cast<std::uint16_t>(3);
if (((v_wbd)->v_wbdGain != 0))
{
v_gain = static_cast<std::uint16_t>((v_wbd)->v_wbdGain);
}
(v_state)->v_wbdmux = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>(v_gain,13)) | (Shl<std::uint16_t>(v_i,11)))) | ((static_cast<std::uint16_t>((v_wbdMux) | (Shl<std::uint16_t>(static_cast<std::uint16_t>(((v_state)->v_config)->v_usePWMAGC),1))))));
m_tuner_writeReg(v_state,0x10,(v_state)->v_wbdmux);
if ((((v_tune)->v_tunerEnable == v_enCAB) && (((v_state)->v_identity).v_p1g != 0)))
{
m_tuner_writeReg(v_state,0x09,(v_tune)->v_lnaBias);
m_tuner_writeReg(v_state,0x0b,[&]() { auto left = static_cast<std::uint16_t>((0xb800) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_tune)->v_lnaTune),6))); auto right = static_cast<std::uint16_t>((v_tune)->v_switchTrim); return static_cast<std::uint16_t>((left) | (right)); }());
}
else
{
m_tuner_writeReg(v_state,0x09,static_cast<std::uint16_t>((Shl<std::uint16_t>(static_cast<std::uint16_t>((v_tune)->v_lnaTune),5)) | ((v_tune)->v_lnaBias)));
}
m_tuner_writeReg(v_state,0x0c,(v_tune)->v_v2i);
m_tuner_writeReg(v_state,0x0d,(v_tune)->v_mix);
m_tuner_writeReg(v_state,0x0e,(v_tune)->v_load);
(v_state)->v_tuneState = v_tunerStep1;
}
else if ((switch24 == v_tunerStep1)) {
(v_state)->v_rfLTDef = 0x7c00;
m_tuner_setBandwidth(v_state);
(v_state)->v_tunerIsTuned = 1;
(v_state)->v_calibrate |= v_wbdCal;
(v_state)->v_calibrate |= v_tempCal;
(v_state)->v_tuneState = v_tunerStop;
}
else
{
v_ret = v_callbackTimeNever;
}
} while(false); }
return v_ret;
}
// getFrequency is dib0090_get_frequency, in Hz.
std::uint32_t m_tuner_getFrequency(T_tuner* v_state)
{
return Mul<std::uint32_t>(1000,(v_state)->v_currentRF);
}
// setParams is dib0090_set_params: the tune state machine run to its end.
Error m_tuner_setParams(T_tuner* v_state)
{
(v_state)->v_tuneState = v_tunerStart;
for (;true;)
{
std::uint32_t v_ret = m_tuner_tune(v_state);
if (((v_ret == v_callbackTimeNever) || (((v_state)->v_failure)->v_err != nullptr)))
{
break;
}
v_ret = static_cast<std::uint32_t>((Mul<std::uint32_t>(10,(Add<std::uint32_t>(v_ret,99)))) / (100));
m_tuner_msleep(v_state,static_cast<std::int64_t>(v_ret));
if (((v_state)->v_tuneState == v_tunerStop))
{
break;
}
}
return ((v_state)->v_failure)->v_err;
}
// register is dib0090_register.
Error m_tuner_register(T_tuner* v_state)
{
if ((((v_state)->v_config)->v_wbd == nullptr))
{
(v_state)->v_currentWBDTable = v_dib0090WBDTableDefault;
}
else
{
(v_state)->v_currentWBDTable = ((v_state)->v_config)->v_wbd;
}
return m_tuner_reset(v_state);
}
// Source: dib8000.go
void m_failure_record(T_failure* v_failure,Error v_err)
{
if (((v_failure)->v_err == nullptr))
{
(v_failure)->v_err = v_err;
}
}
// readWord is dib8000_read_word.
std::uint16_t m_dib8000_readWord(T_dib8000* v_state,std::uint16_t v_register)
{
if ((((v_state)->v_failure)->v_err != nullptr))
{
return 0;
}
auto tmp25 = I2cWriteRead((v_state)->v_bus,(v_state)->v_address,Slice<std::uint8_t>{static_cast<std::uint8_t>(Shr<std::uint16_t>(v_register,8)),static_cast<std::uint8_t>(v_register)},2);
Slice<std::uint8_t> v_data = std::get<0>(tmp25);
Error v_err = std::get<1>(tmp25);
if ((v_err != nullptr))
{
m_failure_record((v_state)->v_failure,MakeError("read DiB8000 register %d: %w",v_register,v_err));
return 0;
}
if ((Len(v_data) != 2))
{
m_failure_record((v_state)->v_failure,MakeError("read DiB8000 register %d: %d bytes, want 2",v_register,Len(v_data)));
return 0;
}
return [&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>(At(v_data,0)),8); auto right = static_cast<std::uint16_t>(At(v_data,1)); return static_cast<std::uint16_t>((left) | (right)); }();
}
// read32 is dib8000_read32: two consecutive registers, high word first.
std::uint32_t m_dib8000_read32(T_dib8000* v_state,std::uint16_t v_register)
{
std::uint16_t v_high = m_dib8000_readWord(v_state,Add<std::uint16_t>(v_register,0));
std::uint16_t v_low = m_dib8000_readWord(v_state,Add<std::uint16_t>(v_register,1));
return [&]() { auto left = Shl<std::uint32_t>(static_cast<std::uint32_t>(v_high),16); auto right = static_cast<std::uint32_t>(v_low); return static_cast<std::uint32_t>((left) | (right)); }();
}
// writeWord is dib8000_write_word.
void m_dib8000_writeWord(T_dib8000* v_state,std::uint16_t v_register,std::uint16_t v_value)
{
if ((((v_state)->v_failure)->v_err != nullptr))
{
return ;
}
Error v_err = (v_state)->v_bus->Write((v_state)->v_address,Slice<std::uint8_t>{static_cast<std::uint8_t>(Shr<std::uint16_t>(v_register,8)),static_cast<std::uint8_t>(v_register),static_cast<std::uint8_t>(Shr<std::uint16_t>(v_value,8)),static_cast<std::uint8_t>(v_value)});
if ((v_err != nullptr))
{
m_failure_record((v_state)->v_failure,MakeError("write DiB8000 register %d: %w",v_register,v_err));
}
}
void m_dib8000_msleep(T_dib8000* v_state,std::int64_t v_milliseconds)
{
(v_state)->v_sleep(Mul<std::int64_t>(static_cast<std::int64_t>(v_milliseconds),INT64_C(1000000)));
}
// fftToMode is fft_to_mode: the chip's mode number for the FFT size.
std::uint16_t m_dib8000_fftToMode(T_dib8000* v_state)
{
{
auto switch26 = ((v_state)->v_channel)->v_transmissionMode;
do {
if ((switch26 == v_transmission2K)) {
return 1;
}
else if ((switch26 == v_transmission4K)) {
return 2;
}
else
{
return 3;
}
} while(false); }
}
// setAcquisitionMode is dib8000_set_acquisition_mode.
void m_dib8000_setAcquisitionMode(T_dib8000* v_state)
{
std::uint16_t v_nud = m_dib8000_readWord(v_state,298);
v_nud |= static_cast<std::uint16_t>((Shl<std::int64_t>(1,3)) | (Shl<std::int64_t>(1,0)));
m_dib8000_writeWord(v_state,298,v_nud);
}
// setOutputMode is dib8000_set_output_mode.
Error m_dib8000_setOutputMode(T_dib8000* v_state,std::int64_t v_mode)
{
std::uint16_t v_outreg{};
std::uint16_t v_fifoThreshold = static_cast<std::uint16_t>(1792);
std::uint16_t v_sram = static_cast<std::uint16_t>(0x0205);
(v_state)->v_outputMode = v_mode;
std::uint16_t v_smoMode = static_cast<std::uint16_t>((static_cast<std::uint16_t>((m_dib8000_readWord(v_state,299)) & (0x0050))) | (Shl<std::uint16_t>(1,1)));
{
auto switch27 = v_mode;
do {
if ((switch27 == v_outmodeParallelGated)) {
v_outreg = Shl<std::uint16_t>(1,10);
}
else if ((switch27 == v_outmodeParallelCont)) {
v_outreg = static_cast<std::uint16_t>((Shl<std::int64_t>(1,10)) | (Shl<std::int64_t>(1,6)));
}
else if ((switch27 == v_outmodeSerial)) {
v_outreg = static_cast<std::uint16_t>((static_cast<std::int64_t>((Shl<std::int64_t>(1,10)) | (Shl<std::int64_t>(2,6)))) | (Shl<std::int64_t>(0,1)));
}
else if ((switch27 == v_outmodeDiversity)) {
if ((((v_state)->v_cfg).v_hostbusDiversity != 0))
{
v_outreg = static_cast<std::uint16_t>((Shl<std::int64_t>(1,10)) | (Shl<std::int64_t>(4,6)));
v_sram &= 0xfdff;
}
else
{
v_sram |= 0x0c00;
}
}
else if ((switch27 == v_outmodeFIFO)) {
v_smoMode |= Shl<std::uint16_t>(3,1);
v_fifoThreshold = 512;
v_outreg = static_cast<std::uint16_t>((Shl<std::int64_t>(1,10)) | (Shl<std::int64_t>(5,6)));
}
else if ((switch27 == v_outmodeHighZ)) {
v_outreg = 0;
}
else if ((switch27 == v_outmodeAnalogADC)) {
v_outreg = static_cast<std::uint16_t>((Shl<std::int64_t>(1,10)) | (Shl<std::int64_t>(3,6)));
m_dib8000_setAcquisitionMode(v_state);
}
else
{
return MakeError("DiB8000 output mode %d not handled",v_mode);
}
} while(false); }
if ((((v_state)->v_cfg).v_outputMPEG2In188Bytes != 0))
{
v_smoMode |= Shl<std::uint16_t>(1,5);
}
m_dib8000_writeWord(v_state,299,v_smoMode);
m_dib8000_writeWord(v_state,300,v_fifoThreshold);
m_dib8000_writeWord(v_state,1286,v_outreg);
m_dib8000_writeWord(v_state,1291,v_sram);
return ((v_state)->v_failure)->v_err;
}
// setDiversityIn is dib8000_set_diversity_in.
void m_dib8000_setDiversityIn(T_dib8000* v_state,std::int64_t v_onoff)
{
std::uint16_t v_syncWait = static_cast<std::uint16_t>((m_dib8000_readWord(v_state,273)) & (0xfff0));
if (((v_state)->v_differentialConstellation == 0))
{
m_dib8000_writeWord(v_state,272,Shl<std::uint16_t>(1,9));
m_dib8000_writeWord(v_state,273,static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_syncWait) | (Shl<std::uint16_t>(1,2)))) | (2)));
}
else
{
m_dib8000_writeWord(v_state,272,0);
m_dib8000_writeWord(v_state,273,v_syncWait);
}
(v_state)->v_diversityOnOff = v_onoff;
{
auto switch28 = v_onoff;
do {
if ((switch28 == 0)) {
m_dib8000_writeWord(v_state,270,1);
m_dib8000_writeWord(v_state,271,0);
}
else if ((switch28 == 1)) {
m_dib8000_writeWord(v_state,270,6);
m_dib8000_writeWord(v_state,271,6);
}
else if ((switch28 == 2)) {
m_dib8000_writeWord(v_state,270,0);
m_dib8000_writeWord(v_state,271,1);
}
} while(false); }
if (((v_state)->v_revision == 0x8002))
{
std::uint16_t v_tmp = m_dib8000_readWord(v_state,903);
m_dib8000_writeWord(v_state,903,static_cast<std::uint16_t>((v_tmp) & ~((Shl<std::uint16_t>(1,3)))));
m_dib8000_msleep(v_state,30);
m_dib8000_writeWord(v_state,903,static_cast<std::uint16_t>((v_tmp) | (Shl<std::uint16_t>(1,3))));
}
}
// setPowerMode is dib8000_set_power_mode.
void m_dib8000_setPowerMode(T_dib8000* v_state,std::int64_t v_mode)
{
std::uint16_t v_reg774 = static_cast<std::uint16_t>(0x3fff);
std::uint16_t v_reg775 = static_cast<std::uint16_t>(0xffff);
std::uint16_t v_reg776 = static_cast<std::uint16_t>(0xffff);
std::uint16_t v_reg900 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((m_dib8000_readWord(v_state,900)) & (0xfffc))) | (0x3));
std::uint16_t v_reg1280 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((m_dib8000_readWord(v_state,1280)) & (0x00ff))) | (0xff00));
{
auto switch29 = v_mode;
do {
if ((switch29 == v_powerAll)) {
v_reg774 = 0x0000;
v_reg775 = 0x0000;
v_reg776 = 0x0000;
v_reg900 &= 0xfffc;
v_reg1280 &= 0x00ff;
}
else if ((switch29 == v_powerInterfaceOnly)) {
v_reg1280 &= 0x00ff;
}
} while(false); }
m_dib8000_writeWord(v_state,774,v_reg774);
m_dib8000_writeWord(v_state,775,v_reg775);
m_dib8000_writeWord(v_state,776,v_reg776);
m_dib8000_writeWord(v_state,900,v_reg900);
m_dib8000_writeWord(v_state,1280,v_reg1280);
}
// setADCState is dib8000_set_adc_state.
void m_dib8000_setADCState(T_dib8000* v_state,std::int64_t v_no)
{
std::uint16_t v_reg907 = m_dib8000_readWord(v_state,907);
std::uint16_t v_reg908 = m_dib8000_readWord(v_state,908);
{
auto switch30 = v_no;
do {
if ((switch30 == v_adcSlowOn)) {
v_reg908 |= static_cast<std::uint16_t>((Shl<std::int64_t>(1,1)) | (Shl<std::int64_t>(1,0)));
m_dib8000_writeWord(v_state,908,v_reg908);
v_reg908 = static_cast<std::uint16_t>(v_reg908 & ~(Shl<std::uint16_t>(1,1)));
}
else if ((switch30 == v_adcSlowOff)) {
v_reg908 |= static_cast<std::uint16_t>((Shl<std::int64_t>(1,1)) | (Shl<std::int64_t>(1,0)));
}
else if ((switch30 == v_adcOn)) {
v_reg907 &= 0x0fff;
v_reg908 &= 0x0003;
}
else if ((switch30 == v_adcOff)) {
v_reg907 = static_cast<std::uint16_t>((Shl<std::int64_t>(1,13)) | (Shl<std::int64_t>(1,12)));
v_reg908 = static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((Shl<std::int64_t>(1,6)) | (Shl<std::int64_t>(1,5)))) | (Shl<std::int64_t>(1,4)))) | (Shl<std::int64_t>(1,3)))) | (Shl<std::int64_t>(1,1)));
}
else if ((switch30 == v_adcVBGEnable)) {
v_reg907 = static_cast<std::uint16_t>(v_reg907 & ~(Shl<std::uint16_t>(1,15)));
}
else if ((switch30 == v_adcVBGDisable)) {
v_reg907 |= Shl<std::uint16_t>(1,15);
}
} while(false); }
m_dib8000_writeWord(v_state,907,v_reg907);
m_dib8000_writeWord(v_state,908,v_reg908);
}
// getADCPower is dib8000_get_adc_power: the power the ADC sees, raw or, with
// mode set, in 1/256 dB.
std::int32_t m_dib8000_getADCPower(T_dib8000* v_state,std::uint8_t v_mode)
{
std::int32_t v_val = static_cast<std::int32_t>(m_dib8000_read32(v_state,384));
if ((v_mode != 0))
{
std::uint32_t v_tmp = static_cast<std::uint32_t>(v_val);
std::uint32_t v_exp = static_cast<std::uint32_t>(0);
for (v_tmp >>= 1;(v_tmp != 0);v_tmp >>= 1)
{
v_exp++;
}
std::uint32_t v_mant = static_cast<std::uint32_t>([&]() { auto left = Mul<std::int64_t>(static_cast<std::int64_t>(v_val),1000); auto right = static_cast<std::int64_t>(Shl<std::uint32_t>(static_cast<std::uint32_t>(1),v_exp)); return static_cast<std::int64_t>((left) / (right)); }());
std::int64_t v_ix = static_cast<std::int64_t>(static_cast<std::uint8_t>(static_cast<std::uint32_t>(((Sub<std::uint32_t>(v_mant,1000))) / (100))));
if ((v_ix >= Len(v_lut1000lnMant)))
{
v_ix = Sub<std::int64_t>(Len(v_lut1000lnMant),1);
}
v_val = Sub<std::int32_t>(Add<std::int32_t>(At(v_lut1000lnMant,v_ix),Mul<std::int32_t>(693,(Sub<std::int32_t>(static_cast<std::int32_t>(v_exp),20)))),6908);
v_val = static_cast<std::int32_t>((Mul<std::int32_t>(v_val,256)) / (1000));
}
return v_val;
}
// pwmAGCReset is dib8000_pwm_agc_reset: the demodulator's ADC back on and the
// AGC set up for the band, after the tuner's software AGC has run.
void m_dib8000_pwmAGCReset(T_dib8000* v_state)
{
m_dib8000_setADCState(v_state,v_adcOn);
m_failure_record((v_state)->v_failure,m_dib8000_setAGCConfig(v_state,v_bandOfFrequency(static_cast<std::uint32_t>((((v_state)->v_channel)->v_frequency) / (1000)))));
}
// setBandwidth is dib8000_set_bandwidth. The bandwidth itself is not used: a
// DiB8000 channel is always 6MHz, and only the timing frequency is written.
void m_dib8000_setBandwidth(T_dib8000* v_state,std::uint32_t v_bandwidth)
{
std::uint32_t v_timf = (v_state)->v_timf;
if (((v_state)->v_timf == 0))
{
v_timf = (v_state)->v_timfDefault;
}
m_dib8000_writeWord(v_state,29,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Shr<std::uint32_t>(v_timf,16))) & (0xffff))));
m_dib8000_writeWord(v_state,30,static_cast<std::uint16_t>(static_cast<std::uint32_t>((v_timf) & (0xffff))));
}
// sadCalibrate is dib8000_sad_calib.
void m_dib8000_sadCalibrate(T_dib8000* v_state)
{
m_dib8000_writeWord(v_state,923,static_cast<std::uint16_t>((Shl<std::int64_t>(0,1)) | (Shl<std::int64_t>(0,0))));
m_dib8000_writeWord(v_state,924,776);
m_dib8000_writeWord(v_state,923,Shl<std::uint16_t>(1,0));
m_dib8000_writeWord(v_state,923,Shl<std::uint16_t>(0,0));
m_dib8000_msleep(v_state,1);
}
// setWBDRef is dib8000_set_wbd_ref.
void m_dib8000_setWBDRef(T_dib8000* v_state,std::uint16_t v_value)
{
if ((v_value > 4095))
{
v_value = 4095;
}
(v_state)->v_wbdRef = v_value;
m_dib8000_writeWord(v_state,106,v_value);
}
// resetPLLCommon is dib8000_reset_pll_common.
void m_dib8000_resetPLLCommon(T_dib8000* v_state,T_bandwidthConfig* v_bandwidth)
{
m_dib8000_writeWord(v_state,23,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Shr<std::uint32_t>((Mul<std::uint32_t>((v_bandwidth)->v_internal,1000)),16))) & (0xffff))));
m_dib8000_writeWord(v_state,24,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Mul<std::uint32_t>((v_bandwidth)->v_internal,1000))) & (0xffff))));
m_dib8000_writeWord(v_state,27,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Shr<std::uint32_t>((v_bandwidth)->v_ifreq,16))) & (0x01ff))));
m_dib8000_writeWord(v_state,28,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((v_bandwidth)->v_ifreq) & (0xffff))));
m_dib8000_writeWord(v_state,26,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Shr<std::uint32_t>((v_bandwidth)->v_ifreq,25))) & (0x0003))));
m_dib8000_writeWord(v_state,922,(v_bandwidth)->v_sadCfg);
}
// resetPLL is dib8000_reset_pll.
void m_dib8000_resetPLL(T_dib8000* v_state)
{
T_bandwidthConfig* v_pll = ((v_state)->v_cfg).v_pll;
m_dib8000_writeWord(v_state,901,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_pllPrediv),8); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_pllRatio),0); return static_cast<std::uint16_t>((left) | (right)); }());
std::uint16_t v_clkCfg1 = static_cast<std::uint16_t>([&]() { auto left = [&]() { auto left = static_cast<std::uint16_t>(([&]() { auto left = [&]() { auto left = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::int64_t>(1,10)) | (Shl<std::int64_t>(0,9)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_ioClkEnCore),8))); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_bypclkDiv),5); return static_cast<std::uint16_t>((left) | (right)); }(); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_enableRefdiv),4); return static_cast<std::uint16_t>((left) | (right)); }()) | (Shl<std::uint16_t>(1,3))); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_pllRange),1); return static_cast<std::uint16_t>((left) | (right)); }(); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_pllReset),0); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,902,v_clkCfg1);
v_clkCfg1 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_clkCfg1) & (0xfff7))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_pllBypass),3)));
m_dib8000_writeWord(v_state,902,v_clkCfg1);
{
do {
if ((((((v_state)->v_cfg).v_pll)->v_adcClkSrc == 0))) {
m_dib8000_writeWord(v_state,904,static_cast<std::uint16_t>(([&]() { auto left = static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::int64_t>((Shl<std::int64_t>(0,15)) | (Shl<std::int64_t>(0,12)))) | (Shl<std::int64_t>(0,10)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_modulo),8))); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_adcClkSrc),7); return static_cast<std::uint16_t>((left) | (right)); }()) | (Shl<std::uint16_t>(0,1))));
}
else if (((((v_state)->v_cfg).v_refclksel != 0))) {
m_dib8000_writeWord(v_state,904,static_cast<std::uint16_t>(([&]() { auto left = [&]() { auto left = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::int64_t>(0,15)) | (Shl<std::int64_t>(1,12)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint8_t>((((v_state)->v_cfg).v_refclksel) & (0x3))),10))); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_modulo),8); return static_cast<std::uint16_t>((left) | (right)); }(); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_adcClkSrc),7); return static_cast<std::uint16_t>((left) | (right)); }()) | (Shl<std::uint16_t>(0,1))));
}
else
{
m_dib8000_writeWord(v_state,904,static_cast<std::uint16_t>(([&]() { auto left = static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::int64_t>((Shl<std::int64_t>(0,15)) | (Shl<std::int64_t>(1,12)))) | (Shl<std::int64_t>(3,10)))) | (Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_modulo),8))); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_pll)->v_adcClkSrc),7); return static_cast<std::uint16_t>((left) | (right)); }()) | (Shl<std::uint16_t>(0,1))));
}
} while(false); }
m_dib8000_resetPLLCommon(v_state,v_pll);
}
// updatePLL is dib8000_update_pll for the DiB8000. The branch that rebuilds
// the whole PLL on a bandwidth change is left out: the demodulator's
// bandwidth is 6MHz from initialisation on and the board refuses any other,
// so only the ratio update is reachable.
void m_dib8000_updatePLL(T_dib8000* v_state,T_bandwidthConfig* v_pll,std::uint32_t v_bandwidth,std::uint8_t v_ratio)
{
std::uint16_t v_reg1856 = m_dib8000_readWord(v_state,1856);
std::uint8_t v_prediv = static_cast<std::uint8_t>(static_cast<std::uint16_t>((v_reg1856) & (0x3f)));
std::uint8_t v_loopdiv = static_cast<std::uint8_t>(static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_reg1856,6))) & (0x3f)));
if (((v_pll == nullptr) || ((((v_pll)->v_pllPrediv == v_prediv) && ((v_pll)->v_pllRatio == v_loopdiv)))))
{
return ;
}
if ((v_ratio != 0))
{
m_dib8000_writeWord(v_state,901,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((((v_state)->v_cfg).v_pll)->v_pllPrediv),8); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>(v_ratio),0); return static_cast<std::uint16_t>((left) | (right)); }());
}
}
// resetGPIO is dib8000_reset_gpio.
void m_dib8000_resetGPIO(T_dib8000* v_state)
{
m_dib8000_writeWord(v_state,1029,((v_state)->v_cfg).v_gpioDir);
m_dib8000_writeWord(v_state,1030,((v_state)->v_cfg).v_gpioVal);
m_dib8000_writeWord(v_state,1032,((v_state)->v_cfg).v_gpioPWMPos);
m_dib8000_writeWord(v_state,1037,((v_state)->v_cfg).v_pwmFreqDiv);
}
// setGPIO is dib8000_set_gpio (dib8000_cfg_gpio). Direction 0 is output.
void m_dib8000_setGPIO(T_dib8000* v_state,std::uint8_t v_num,std::uint8_t v_dir,std::uint8_t v_value)
{
((v_state)->v_cfg).v_gpioDir = m_dib8000_readWord(v_state,1029);
((v_state)->v_cfg).v_gpioDir = static_cast<std::uint16_t>(((v_state)->v_cfg).v_gpioDir & ~(Shl<std::uint16_t>(1,v_num)));
((v_state)->v_cfg).v_gpioDir |= Shl<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint8_t>((v_dir) & (0x1))),v_num);
m_dib8000_writeWord(v_state,1029,((v_state)->v_cfg).v_gpioDir);
((v_state)->v_cfg).v_gpioVal = m_dib8000_readWord(v_state,1030);
((v_state)->v_cfg).v_gpioVal = static_cast<std::uint16_t>(((v_state)->v_cfg).v_gpioVal & ~(Shl<std::uint16_t>(1,v_num)));
((v_state)->v_cfg).v_gpioVal |= Shl<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint8_t>((v_value) & (0x01))),v_num);
m_dib8000_writeWord(v_state,1030,((v_state)->v_cfg).v_gpioVal);
}
// identify is dib8000_identify. The vendor register is read twice because the
// first read after power-up sometimes glitches.
std::tuple<std::uint16_t,Error> m_dib8000_identify(T_dib8000* v_state)
{
m_dib8000_readWord(v_state,896);
{
std::uint16_t v_value = m_dib8000_readWord(v_state,896);
if ((v_value != 0x01b3))
{
if ((((v_state)->v_failure)->v_err != nullptr))
{
return {0,((v_state)->v_failure)->v_err};
}
return {0,MakeError("DiB8000: wrong vendor ID %#04x",v_value)};
}
}
std::uint16_t v_value = m_dib8000_readWord(v_state,897);
if ((((v_state)->v_failure)->v_err != nullptr))
{
return {0,((v_state)->v_failure)->v_err};
}
{
auto switch31 = v_value;
do {
if ((switch31 == 0x8000) || (switch31 == 0x8001) || (switch31 == 0x8002) || (switch31 == 0x8090)) {
return {v_value,nullptr};
}
} while(false); }
return {0,MakeError("DiB8000: wrong device ID %#04x",v_value)};
}
// resetStats is the part of dib8000_reset_stats that touches the chip: it
// reads the uncorrected-block counter to take its starting value. The DVBv5
// statistics it initialises have no counterpart here.
void m_dib8000_resetStats(T_dib8000* v_state)
{
m_dib8000_readUncBlocks(v_state);
}
// reset is dib8000_reset.
Error m_dib8000_reset(T_dib8000* v_state)
{
auto tmp32 = m_dib8000_identify(v_state);
std::uint16_t v_revision = std::get<0>(tmp32);
Error v_err = std::get<1>(tmp32);
if ((v_err != nullptr))
{
return v_err;
}
(v_state)->v_revision = v_revision;
if (((v_state)->v_revision == 0x8090))
{
return MakeError("DiB8096P (device ID 0x8090) is not supported by this driver");
}
m_dib8000_writeWord(v_state,1287,0x0003);
m_i2cMaster_reset((&(v_state)->v_master));
m_dib8000_setPowerMode(v_state,v_powerAll);
m_dib8000_setADCState(v_state,v_adcOff);
m_dib8000_writeWord(v_state,770,0xffff);
m_dib8000_writeWord(v_state,771,0xffff);
m_dib8000_writeWord(v_state,772,0xfffc);
m_dib8000_writeWord(v_state,898,0x000c);
m_dib8000_writeWord(v_state,1280,0x004d);
m_dib8000_writeWord(v_state,1281,0x000c);
m_dib8000_writeWord(v_state,770,0x0000);
m_dib8000_writeWord(v_state,771,0x0000);
m_dib8000_writeWord(v_state,772,0x0000);
m_dib8000_writeWord(v_state,898,0x0004);
m_dib8000_writeWord(v_state,1280,0x0000);
m_dib8000_writeWord(v_state,1281,0x0000);
if ((((v_state)->v_cfg).v_drives != 0))
{
m_dib8000_writeWord(v_state,906,((v_state)->v_cfg).v_drives);
}
else
{
m_dib8000_writeWord(v_state,906,0x2d98);
}
m_dib8000_resetPLL(v_state);
m_dib8000_writeWord(v_state,898,0x0004);
m_dib8000_resetGPIO(v_state);
{
Error v_err = m_dib8000_setOutputMode(v_state,v_outmodeHighZ);
if ((v_err != nullptr))
{
return v_err;
}
}
(v_state)->v_currentAGC = nullptr;
if (((((v_state)->v_cfg).v_pll)->v_ifreq == 0))
{
m_dib8000_writeWord(v_state,40,0x0755);
}
else
{
m_dib8000_writeWord(v_state,40,0x1f55);
}
Slice<std::uint16_t> v_defaults = v_dib8000Defaults;
for (;true;)
{
std::uint16_t v_count = At(v_defaults,0);
if ((v_count == 0))
{
break;
}
std::uint16_t v_register = At(v_defaults,1);
v_defaults = Subslice(v_defaults,2,Len(v_defaults));
for (;(v_count > 0);v_count--)
{
m_dib8000_writeWord(v_state,v_register,At(v_defaults,0));
v_defaults = Subslice(v_defaults,1,Len(v_defaults));
v_register++;
}
}
(v_state)->v_isdbtCfgLoaded = 0;
if ((((v_state)->v_revision != 8090) && (((v_state)->v_cfg).v_divCfg != 0)))
{
m_dib8000_writeWord(v_state,903,static_cast<std::uint16_t>(((v_state)->v_cfg).v_divCfg));
}
m_dib8000_modifyWord(v_state,1285,static_cast<std::uint16_t>(~(static_cast<std::uint16_t>(Shl<std::uint16_t>(1,1)))),0);
m_dib8000_setBandwidth(v_state,6000);
m_dib8000_setADCState(v_state,v_adcSlowOn);
m_dib8000_sadCalibrate(v_state);
m_dib8000_setADCState(v_state,v_adcSlowOff);
m_dib8000_modifyWord(v_state,285,static_cast<std::uint16_t>(~(static_cast<std::uint16_t>(0x60))),Shl<std::uint16_t>(3,5));
m_dib8000_setPowerMode(v_state,v_powerInterfaceOnly);
m_dib8000_resetStats(v_state);
return ((v_state)->v_failure)->v_err;
}
// restartAGC is dib8000_restart_agc: P_restart_iqc and P_restart_agc.
void m_dib8000_restartAGC(T_dib8000* v_state)
{
m_dib8000_writeWord(v_state,770,0x0a00);
m_dib8000_writeWord(v_state,770,0x0000);
}
// setAGCConfig is dib8000_set_agc_config.
Error m_dib8000_setAGCConfig(T_dib8000* v_state,std::uint8_t v_band)
{
if ((((v_state)->v_currentBand == v_band) && ((v_state)->v_currentAGC != nullptr)))
{
return nullptr;
}
(v_state)->v_currentBand = v_band;
T_agcConfig* v_agc{};
{ auto&& range33 = ((v_state)->v_cfg).v_agc;
for (std::int64_t index33=0; index33<Len(range33); ++index33) {
auto v_i = index33;
if ((static_cast<std::uint8_t>(((At(((v_state)->v_cfg).v_agc,v_i)).v_bandCaps) & (v_band)) != 0))
{
v_agc = (&At(((v_state)->v_cfg).v_agc,v_i));
break;
}
}}
if ((v_agc == nullptr))
{
return MakeError("DiB8000: no valid AGC configuration for band %#02x",v_band);
}
(v_state)->v_currentAGC = v_agc;
m_dib8000_writeWord(v_state,76,(v_agc)->v_setup);
m_dib8000_writeWord(v_state,77,(v_agc)->v_invGain);
m_dib8000_writeWord(v_state,78,(v_agc)->v_timeStabiliz);
m_dib8000_writeWord(v_state,101,static_cast<std::uint16_t>((Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_alphaLevel),12)) | ((v_agc)->v_thlock)));
m_dib8000_writeWord(v_state,102,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_alphaMant),5); auto right = static_cast<std::uint16_t>((v_agc)->v_alphaExp); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,103,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_betaMant),6); auto right = static_cast<std::uint16_t>((v_agc)->v_betaExp); return static_cast<std::uint16_t>((left) | (right)); }());
if (((v_state)->v_wbdRef != 0))
{
m_dib8000_writeWord(v_state,106,(v_state)->v_wbdRef);
}
else
{
m_dib8000_writeWord(v_state,106,(v_agc)->v_wbdRef);
}
m_dib8000_writeWord(v_state,107,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_wbdAlpha),9); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_performAGCSoftsplit),8); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,108,(v_agc)->v_agc1Max);
m_dib8000_writeWord(v_state,109,(v_agc)->v_agc1Min);
m_dib8000_writeWord(v_state,110,(v_agc)->v_agc2Max);
m_dib8000_writeWord(v_state,111,(v_agc)->v_agc2Min);
m_dib8000_writeWord(v_state,112,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_agc1Pt1),8); auto right = static_cast<std::uint16_t>((v_agc)->v_agc1Pt2); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,113,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_agc1Slope1),8); auto right = static_cast<std::uint16_t>((v_agc)->v_agc1Slope2); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,114,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_agc2Pt1),8); auto right = static_cast<std::uint16_t>((v_agc)->v_agc2Pt2); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,115,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_agc2Slope1),8); auto right = static_cast<std::uint16_t>((v_agc)->v_agc2Slope2); return static_cast<std::uint16_t>((left) | (right)); }());
m_dib8000_writeWord(v_state,75,static_cast<std::uint16_t>((v_agc)->v_agc1Pt3));
m_dib8000_modifyWord(v_state,923,0xffe3,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_wbdInv),4); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_agc)->v_wbdSel),2); return static_cast<std::uint16_t>((left) | (right)); }());
return nullptr;
}
// agcSoftSplit is dib8000_agc_soft_split.
std::int64_t m_dib8000_agcSoftSplit(T_dib8000* v_state)
{
T_agcConfig* v_agc = (v_state)->v_currentAGC;
if ((((v_agc == nullptr) || ((v_agc)->v_performAGCSoftsplit == 0)) || (((v_agc)->v_split).v_max == 0)))
{
return 0;
}
std::uint16_t v_global = m_dib8000_readWord(v_state,390);
std::uint16_t v_splitOffset{};
{
do {
if (((v_global > ((v_agc)->v_split).v_minThres))) {
v_splitOffset = ((v_agc)->v_split).v_min;
}
else if (((v_global < ((v_agc)->v_split).v_maxThres))) {
v_splitOffset = ((v_agc)->v_split).v_max;
}
else
{
v_splitOffset = static_cast<std::uint16_t>([&]() { auto left = [&]() { auto left = static_cast<std::int64_t>(((v_agc)->v_split).v_max); auto right = ([&]() { auto left = static_cast<std::int64_t>(v_global); auto right = static_cast<std::int64_t>(((v_agc)->v_split).v_minThres); return Sub<std::int64_t>(left,right); }()); return Mul<std::int64_t>(left,right); }(); auto right = ([&]() { auto left = static_cast<std::int64_t>(((v_agc)->v_split).v_maxThres); auto right = static_cast<std::int64_t>(((v_agc)->v_split).v_minThres); return Sub<std::int64_t>(left,right); }()); return static_cast<std::int64_t>((left) / (right)); }());
}
} while(false); }
m_dib8000_modifyWord(v_state,107,0xff00,v_splitOffset);
return 5000;
}
// agcStartup is dib8000_agc_startup. It returns the time to wait before the
// next step, in 0.1ms units.
std::int64_t m_dib8000_agcStartup(T_dib8000* v_state)
{
std::int64_t v_ret = 0;
{
auto switch34 = (v_state)->v_tuneState;
do {
if ((switch34 == v_agcStart)) {
m_dib8000_setADCState(v_state,v_adcOn);
if ((m_dib8000_setAGCConfig(v_state,v_bandOfFrequency(static_cast<std::uint32_t>((((v_state)->v_channel)->v_frequency) / (1000)))) != nullptr))
{
(v_state)->v_tuneState = v_agcStop;
(v_state)->v_status = v_statusTuneFailed;
break;
}
v_ret = 70;
(v_state)->v_tuneState = v_agcStep0;
}
else if ((switch34 == v_agcStep0)) {
if (((v_state)->v_agcControl != nullptr))
{
(v_state)->v_agcControl(1);
}
m_dib8000_restartAGC(v_state);
v_ret = 50;
(v_state)->v_tuneState = v_agcStep1;
}
else if ((switch34 == v_agcStep1)) {
v_ret = 70;
(v_state)->v_tuneState = v_agcStep2;
}
else if ((switch34 == v_agcStep2)) {
m_dib8000_agcSoftSplit(v_state);
if (((v_state)->v_agcControl != nullptr))
{
(v_state)->v_agcControl(0);
}
(v_state)->v_tuneState = v_agcStop;
}
else
{
v_ret = m_dib8000_agcSoftSplit(v_state);
}
} while(false); }
return v_ret;
}
// updateTimf is dib8000_update_timf.
void m_dib8000_updateTimf(T_dib8000* v_state)
{
(v_state)->v_timf = m_dib8000_read32(v_state,435);
std::uint32_t v_timf = (v_state)->v_timf;
m_dib8000_writeWord(v_state,29,static_cast<std::uint16_t>(Shr<std::uint32_t>(v_timf,16)));
m_dib8000_writeWord(v_state,30,static_cast<std::uint16_t>(static_cast<std::uint32_t>((v_timf) & (0xffff))));
}
// ctrlTimf is dib8000_ctrl_timf.
std::uint32_t m_dib8000_ctrlTimf(T_dib8000* v_state,std::uint8_t v_op,std::uint32_t v_timf)
{
{
auto switch35 = v_op;
do {
if ((switch35 == v_timfSet)) {
(v_state)->v_timf = v_timf;
}
else if ((switch35 == v_timfUpdate)) {
m_dib8000_updateTimf(v_state);
}
else if ((switch35 == v_timfGet)) {
}
} while(false); }
m_dib8000_setBandwidth(v_state,6000);
return (v_state)->v_timf;
}
// wakeup is dib8000_wakeup.
void m_dib8000_wakeup(T_dib8000* v_state)
{
m_dib8000_setPowerMode(v_state,v_powerAll);
m_dib8000_setADCState(v_state,v_adcOn);
m_dib8000_setADCState(v_state,v_adcSlowOn);
}
// sleepChip is dib8000_sleep.
Error m_dib8000_sleepChip(T_dib8000* v_state)
{
{
Error v_err = m_dib8000_setOutputMode(v_state,v_outmodeHighZ);
if ((v_err != nullptr))
{
return v_err;
}
}
m_dib8000_setPowerMode(v_state,v_powerInterfaceOnly);
m_dib8000_setADCState(v_state,v_adcSlowOff);
m_dib8000_setADCState(v_state,v_adcOff);
return ((v_state)->v_failure)->v_err;
}
// initialize is the chip part of dib8000_init: identify, set up the I2C
// master, reset, and restore the BER window the reset leaves.
Error m_dib8000_initialize(T_dib8000* v_state)
{
(v_state)->v_timfDefault = (((v_state)->v_cfg).v_pll)->v_timf;
{
auto tmp36 = m_dib8000_identify(v_state);
Error v_err = std::get<1>(tmp36);
if ((v_err != nullptr))
{
return v_err;
}
}
m_i2cMaster_initialize((&(v_state)->v_master),v_state,v_deviceDiB8000);
{
Error v_err = m_dib8000_reset(v_state);
if ((v_err != nullptr))
{
return v_err;
}
}
m_dib8000_modifyWord(v_state,285,static_cast<std::uint16_t>(~(static_cast<std::uint16_t>(0x60))),Shl<std::uint16_t>(3,5));
(v_state)->v_currentDemodBW = 6000;
return ((v_state)->v_failure)->v_err;
}
// Source: dib8000_tune.go
// clear is dvb_frontend_clear_cache for an ISDB-T frontend: what the cache
// holds when the caller knows nothing but the frequency.
void m_channel_clear(T_channel* v_c)
{
(*v_c) = [&]() { T_channel value{}; value.v_deliverySystem = v_systemISDBT; value.v_transmissionMode = v_transmissionAuto; value.v_guardInterval = v_guardAuto; value.v_inversion = v_inversionAuto; value.v_isdbtLayerEnabled = 7; return value; }();
{ auto&& range37 = (v_c)->v_layer;
for (std::int64_t index37=0; index37<Len(range37); ++index37) {
auto v_i = index37;
At((v_c)->v_layer,v_i) = [&]() { T_layerParameters value{}; value.v_fec = v_fecAuto; value.v_modulation = v_modulationQAMAuto; return value; }();
}}
}
// setLayer is dib8000_set_layer: it programs one layer and returns the
// densest constellation seen so far.
std::int64_t m_dib8000_setLayer(T_dib8000* v_state,std::uint8_t v_layerIndex,std::int64_t v_maxConstellation)
{
T_channel* v_c = (v_state)->v_channel;
T_layerParameters* v_layer = (&At((v_c)->v_layer,v_layerIndex));
std::uint16_t v_constellation{};
std::uint16_t v_cr{};
{
auto switch38 = (v_layer)->v_modulation;
do {
if ((switch38 == v_modulationDQPSK)) {
v_constellation = 0;
}
else if ((switch38 == v_modulationQPSK)) {
v_constellation = 1;
}
else if ((switch38 == v_modulationQAM16)) {
v_constellation = 2;
}
else
{
v_constellation = 3;
}
} while(false); }
{
auto switch39 = (v_layer)->v_fec;
do {
if ((switch39 == v_fec1_2)) {
v_cr = 1;
}
else if ((switch39 == v_fec2_3)) {
v_cr = 2;
}
else if ((switch39 == v_fec3_4)) {
v_cr = 3;
}
else if ((switch39 == v_fec5_6)) {
v_cr = 5;
}
else
{
v_cr = 7;
}
} while(false); }
std::uint16_t v_timeIntlv = static_cast<std::uint16_t>(BitLength((v_layer)->v_interleaving));
if (((v_timeIntlv > 3) && static_cast<bool>(!((((v_timeIntlv == 4) && ((v_c)->v_isdbtSbMode == 1)))))))
{
v_timeIntlv = 0;
}
m_dib8000_writeWord(v_state,Add<std::uint16_t>(2,static_cast<std::uint16_t>(v_layerIndex)),static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>(v_constellation,10)) | (Shl<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_layer)->v_segmentCount)) & (0xf))),6)))) | (Shl<std::uint16_t>(v_cr,3)))) | (v_timeIntlv)));
if (((v_layer)->v_segmentCount > 0))
{
{
auto switch40 = v_maxConstellation;
do {
if ((switch40 == v_modulationDQPSK) || (switch40 == v_modulationQPSK)) {
if ((((v_layer)->v_modulation == v_modulationQAM16) || ((v_layer)->v_modulation == v_modulationQAM64)))
{
v_maxConstellation = (v_layer)->v_modulation;
}
}
else if ((switch40 == v_modulationQAM16)) {
if (((v_layer)->v_modulation == v_modulationQAM64))
{
v_maxConstellation = (v_layer)->v_modulation;
}
}
} while(false); }
}
return v_maxConstellation;
}
// adpFineTune is dib8000_adp_fine_tune: channel estimation fine configuration.
std::uint16_t m_dib8000_adpFineTune(T_dib8000* v_state,std::int64_t v_maxConstellation)
{
std::uint16_t v_anaGain{};
std::array<std::uint16_t, 4>* v_adp{};
{
auto switch41 = v_maxConstellation;
do {
if ((switch41 == v_modulationQAM64)) {
v_anaGain = 0x7;
v_adp = (&v_adpQ64);
}
else if ((switch41 == v_modulationQAM16)) {
v_anaGain = 0x7;
v_adp = (&v_adpQ16);
}
else
{
v_anaGain = 0;
v_adp = (&v_adpQDefault);
}
} while(false); }
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);(v_i < 4);v_i++)
{
m_dib8000_writeWord(v_state,Add<std::uint16_t>(215,v_i),At((*v_adp),v_i));
}
return v_anaGain;
}
// updateAnaGain is dib8000_update_ana_gain.
void m_dib8000_updateAnaGain(T_dib8000* v_state,std::uint16_t v_anaGain)
{
m_dib8000_writeWord(v_state,116,v_anaGain);
if ((v_anaGain != 0))
{
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);(v_i < 10);v_i++)
{
m_dib8000_writeWord(v_state,Add<std::uint16_t>(80,v_i),At(v_adcTarget16dB,v_i));
}
}
else
{
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);(v_i < 10);v_i++)
{
m_dib8000_writeWord(v_state,Add<std::uint16_t>(80,v_i),Sub<std::uint16_t>(At(v_adcTarget16dB,v_i),355));
}
}
}
// loadAnaFECoefs is dib8000_load_ana_fe_coefs.
void m_dib8000_loadAnaFECoefs(T_dib8000* v_state,std::array<std::int16_t, 24>* v_anaFE)
{
if (((v_state)->v_isdbtCfgLoaded == 0))
{
for (std::uint16_t v_mode = static_cast<std::uint16_t>(0);(v_mode < 24);v_mode++)
{
m_dib8000_writeWord(v_state,Add<std::uint16_t>(117,v_mode),static_cast<std::uint16_t>(At((*v_anaFE),v_mode)));
}
}
}
// set13SegChannel is dib8000_set_13seg_channel.
void m_dib8000_set13SegChannel(T_dib8000* v_state)
{
std::uint16_t v_coffPow = static_cast<std::uint16_t>(0x2800);
(v_state)->v_segMask = 0x1fff;
if (((v_state)->v_isdbtCfgLoaded == 0))
{
m_dib8000_writeWord(v_state,180,static_cast<std::uint16_t>((Shl<std::int64_t>(16,6)) | (9)));
m_dib8000_writeWord(v_state,187,static_cast<std::uint16_t>((static_cast<std::int64_t>((Shl<std::int64_t>(4,12)) | (Shl<std::int64_t>(8,5)))) | (0x2)));
v_coffPow = 0x2800;
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);(v_i < 6);v_i++)
{
m_dib8000_writeWord(v_state,Add<std::uint16_t>(181,v_i),v_coffPow);
}
m_dib8000_writeWord(v_state,338,static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((Shl<std::int64_t>(1,12)) | (Shl<std::int64_t>(1,10)))) | (Shl<std::int64_t>(0,9)))) | (Shl<std::int64_t>(3,5)))) | (1)));
m_dib8000_writeWord(v_state,340,static_cast<std::uint16_t>((Shl<std::int64_t>(8,6)) | (Shl<std::int64_t>(6,0))));
m_dib8000_writeWord(v_state,341,static_cast<std::uint16_t>((static_cast<std::int64_t>((static_cast<std::int64_t>((Shl<std::int64_t>(4,3)) | (Shl<std::int64_t>(1,2)))) | (Shl<std::int64_t>(1,1)))) | (Shl<std::int64_t>(1,0))));
m_dib8000_writeWord(v_state,228,0);
m_dib8000_writeWord(v_state,265,31);
m_dib8000_writeWord(v_state,205,0x200f);
}
if (((((v_state)->v_cfg).v_pll)->v_ifreq == 0))
{
m_dib8000_writeWord(v_state,266,static_cast<std::uint16_t>((static_cast<std::uint16_t>((static_cast<std::uint16_t>(~((v_state)->v_segMask))) | ((v_state)->v_segDiffMask))) | (0x40)));
}
m_dib8000_loadAnaFECoefs(v_state,(&v_anaFECoeff13Seg));
}
// setSubchannelPRBS is dib8000_set_subchannel_prbs.
void m_dib8000_setSubchannelPRBS(T_dib8000* v_state,std::uint16_t v_initPRBS)
{
std::uint16_t v_reg1 = m_dib8000_readWord(v_state,1);
m_dib8000_writeWord(v_state,1,static_cast<std::uint16_t>((Shl<std::uint16_t>(v_initPRBS,2)) | (static_cast<std::uint16_t>((v_reg1) & (0x3)))));
}
// smallFineTune is dib8000_small_fine_tune. The coefficient block it loads
// for ISDB-Tsb is left out.
void m_dib8000_smallFineTune(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
m_dib8000_writeWord(v_state,352,(v_state)->v_segDiffMask);
m_dib8000_writeWord(v_state,353,(v_state)->v_segMask);
m_dib8000_writeWord(v_state,351,static_cast<std::uint16_t>((static_cast<std::uint16_t>(([&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_c)->v_isdbtSbMode),9); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>((v_c)->v_isdbtSbMode),8); return static_cast<std::uint16_t>((left) | (right)); }()) | (Shl<std::uint16_t>(13,4)))) | (5)));
}
// setISDBTCommonChannel is dib8000_set_isdbt_common_channel.
void m_dib8000_setISDBTCommonChannel(T_dib8000* v_state,std::uint8_t v_seq,bool v_autosearching)
{
std::uint16_t v_pCfrLeftEdge{};
std::uint16_t v_pCfrRightEdge{};
std::uint16_t v_tmccPow{};
std::uint16_t v_nbsegDiff{};
std::int64_t v_maxConstellation = v_modulationDQPSK;
T_channel* v_c = (v_state)->v_channel;
if (v_autosearching)
{
(v_c)->v_isdbtPartialReception = 1;
}
m_dib8000_writeWord(v_state,10,Shl<std::uint16_t>(static_cast<std::uint16_t>(v_seq),4));
(v_state)->v_mode = m_dib8000_fftToMode(v_state);
std::uint16_t v_tmp = m_dib8000_readWord(v_state,1);
m_dib8000_writeWord(v_state,1,static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_tmp) & (0xfffc))) | (static_cast<std::uint16_t>((static_cast<std::uint16_t>((v_c)->v_guardInterval)) & (0x3)))));
m_dib8000_modifyWord(v_state,274,0xffcf,[&]() { auto left = Shl<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint8_t>(((v_c)->v_isdbtPartialReception) & (1))),5); auto right = Shl<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint8_t>(((v_c)->v_isdbtSbMode) & (1))),4); return static_cast<std::uint16_t>((left) | (right)); }());
if (((v_c)->v_isdbtPartialReception != 0))
{
(v_state)->v_segDiffMask = Shl<std::uint16_t>(v_boolWord(((At((v_c)->v_layer,0)).v_modulation == v_modulationDQPSK)),At(v_permuSeg,0));
for (std::int64_t v_i = 1;(v_i < 3);v_i++)
{
v_nbsegDiff += [&]() { auto left = v_boolWord(((At((v_c)->v_layer,v_i)).v_modulation == v_modulationDQPSK)); auto right = static_cast<std::uint16_t>((At((v_c)->v_layer,v_i)).v_segmentCount); return Mul<std::uint16_t>(left,right); }();
}
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);((v_i < v_nbsegDiff) && [&]() { auto left = Add<std::int64_t>(static_cast<std::int64_t>(v_i),1); auto right = Len(v_permuSeg); return (left < right); }());v_i++)
{
(v_state)->v_segDiffMask |= Shl<std::uint16_t>(1,At(v_permuSeg,Add<std::uint16_t>(v_i,1)));
}
}
else
{
for (std::int64_t v_i = 0;(v_i < 3);v_i++)
{
v_nbsegDiff += [&]() { auto left = v_boolWord(((At((v_c)->v_layer,v_i)).v_modulation == v_modulationDQPSK)); auto right = static_cast<std::uint16_t>((At((v_c)->v_layer,v_i)).v_segmentCount); return Mul<std::uint16_t>(left,right); }();
}
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);((v_i < v_nbsegDiff) && [&]() { auto left = static_cast<std::int64_t>(v_i); auto right = Len(v_permuSeg); return (left < right); }());v_i++)
{
(v_state)->v_segDiffMask |= Shl<std::uint16_t>(1,At(v_permuSeg,v_i));
}
}
if (((v_state)->v_segDiffMask != 0))
{
m_dib8000_modifyWord(v_state,268,0xf9ff,0x0200);
}
else
{
m_dib8000_writeWord(v_state,268,static_cast<std::uint16_t>((Shl<std::int64_t>(2,9)) | (39)));
}
for (std::uint8_t v_i = static_cast<std::uint8_t>(0);(v_i < 3);v_i++)
{
v_maxConstellation = m_dib8000_setLayer(v_state,v_i,v_maxConstellation);
}
if (static_cast<bool>(!(v_autosearching)))
{
(v_state)->v_layerBNbSeg = (At((v_c)->v_layer,1)).v_segmentCount;
(v_state)->v_layerCNbSeg = (At((v_c)->v_layer,2)).v_segmentCount;
}
m_dib8000_writeWord(v_state,0,static_cast<std::uint16_t>((Shl<std::uint16_t>((v_state)->v_mode,13)) | ((v_state)->v_segDiffMask)));
(v_state)->v_differentialConstellation = static_cast<std::uint8_t>(v_boolWord(((v_state)->v_segDiffMask != 0)));
std::uint16_t v_anaGain = m_dib8000_adpFineTune(v_state,v_maxConstellation);
m_dib8000_updateAnaGain(v_state,v_anaGain);
if (((v_c)->v_isdbtPartialReception != 0))
{
m_dib8000_loadAnaFECoefs(v_state,(&v_anaFECoeff3Seg));
}
else
{
m_dib8000_loadAnaFECoefs(v_state,(&v_anaFECoeff1Seg));
}
m_dib8000_set13SegChannel(v_state);
std::uint16_t v_initPRBS = static_cast<std::uint16_t>(0xfff);
m_dib8000_smallFineTune(v_state);
m_dib8000_setSubchannelPRBS(v_state,v_initPRBS);
for (std::uint64_t v_i = static_cast<std::uint64_t>(0);(v_i < 13);v_i++)
{
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(static_cast<std::uint16_t>(~((v_state)->v_segDiffMask)),v_i))) & (1)) == 1))
{
std::uint16_t v_enabled = static_cast<std::uint16_t>(((v_state)->v_segMask) & (static_cast<std::uint16_t>(~((v_state)->v_segDiffMask))));
if (((v_i == 0) || (static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_enabled,(Sub<std::uint64_t>(v_i,1))))) & (1)) == 0)))
{
v_pCfrLeftEdge += Shl<std::uint16_t>(1,v_i);
}
if (((v_i == 12) || (static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_enabled,(Add<std::uint64_t>(v_i,1))))) & (1)) == 0)))
{
v_pCfrRightEdge += Shl<std::uint16_t>(1,v_i);
}
}
}
m_dib8000_writeWord(v_state,222,v_pCfrLeftEdge);
m_dib8000_writeWord(v_state,223,v_pCfrRightEdge);
m_dib8000_writeWord(v_state,189,static_cast<std::uint16_t>((static_cast<std::uint16_t>(~((v_state)->v_segMask))) | ((v_state)->v_segDiffMask)));
m_dib8000_writeWord(v_state,192,static_cast<std::uint16_t>((static_cast<std::uint16_t>(~((v_state)->v_segMask))) | ((v_state)->v_segDiffMask)));
m_dib8000_writeWord(v_state,225,static_cast<std::uint16_t>((static_cast<std::uint16_t>(~((v_state)->v_segMask))) | ((v_state)->v_segDiffMask)));
if (static_cast<bool>(!(v_autosearching)))
{
m_dib8000_writeWord(v_state,288,static_cast<std::uint16_t>(((static_cast<std::uint16_t>((static_cast<std::uint16_t>(~((v_state)->v_segMask))) | ((v_state)->v_segDiffMask)))) & (0x1fff)));
}
else
{
m_dib8000_writeWord(v_state,288,0x1fff);
}
m_dib8000_writeWord(v_state,211,static_cast<std::uint16_t>(((v_state)->v_segMask) & ~((v_state)->v_segDiffMask)));
m_dib8000_writeWord(v_state,287,static_cast<std::uint16_t>((static_cast<std::uint16_t>(~((v_state)->v_segMask))) | (0x1000)));
m_dib8000_writeWord(v_state,178,32);
for (std::int64_t v_i = 0;(v_i < 3);v_i++)
{
v_tmccPow += [&]() { auto left = (Add<std::uint16_t>(Mul<std::uint16_t>(v_boolWord(((At((v_c)->v_layer,v_i)).v_modulation == v_modulationDQPSK)),4),1)); auto right = static_cast<std::uint16_t>((At((v_c)->v_layer,v_i)).v_segmentCount); return Mul<std::uint16_t>(left,right); }();
}
v_tmccPow *= Shl<std::uint16_t>(1,(Sub<std::int64_t>(9,2)));
m_dib8000_writeWord(v_state,290,v_tmccPow);
m_dib8000_writeWord(v_state,291,v_tmccPow);
m_dib8000_writeWord(v_state,292,v_tmccPow);
if (((v_state)->v_isdbtCfgLoaded == 0))
{
m_dib8000_writeWord(v_state,250,3285);
}
(v_state)->v_isdbtCfgLoaded = 0;
}
std::uint16_t v_boolWord(bool v_value)
{
if (v_value)
{
return 1;
}
return 0;
}
// waitLock is dib8000_wait_lock: it programs P_search_end0..2 in units of the
// internal clock and returns the last value written.
std::uint32_t m_dib8000_waitLock(T_dib8000* v_state,std::uint32_t v_internal,std::uint32_t v_wait0ms,std::uint32_t v_wait1ms,std::uint32_t v_wait2ms)
{
std::uint32_t v_value{};
for (std::uint16_t v_register = static_cast<std::uint16_t>(11);(v_register < 16);v_register += 2)
{
{
auto switch42 = v_register;
do {
if ((switch42 == 11)) {
v_value = Mul<std::uint32_t>(v_internal,v_wait0ms);
}
else if ((switch42 == 13)) {
v_value = Mul<std::uint32_t>(v_internal,v_wait1ms);
}
else if ((switch42 == 15)) {
v_value = Mul<std::uint32_t>(v_internal,v_wait2ms);
}
} while(false); }
m_dib8000_writeWord(v_state,v_register,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Shr<std::uint32_t>(v_value,16))) & (0xffff))));
m_dib8000_writeWord(v_state,Add<std::uint16_t>(v_register,1),static_cast<std::uint16_t>(static_cast<std::uint32_t>((v_value) & (0xffff))));
}
return v_value;
}
// autosearchStart is dib8000_autosearch_start. From revision 0x8002 on, the
// search runs in two passes, FFT size then guard interval; older revisions
// search a fixed list in one.
void m_dib8000_autosearchStart(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
std::uint8_t v_slist{};
std::uint32_t v_internal = (((v_state)->v_cfg).v_pll)->v_internal;
if ((((v_state)->v_revision >= 0x8002) && ((v_state)->v_autosearchState == v_autosearchSearchingFFT)))
{
m_dib8000_writeWord(v_state,37,0x0065);
m_dib8000_writeWord(v_state,116,0x0000);
m_dib8000_modifyWord(v_state,0,0x1fff,static_cast<std::uint16_t>((Shl<std::int64_t>(0,13)) | (Shl<std::int64_t>(1,15))));
m_dib8000_modifyWord(v_state,1,0xfffc,0);
m_dib8000_writeWord(v_state,6,0);
m_dib8000_writeWord(v_state,7,0);
m_dib8000_writeWord(v_state,8,0);
m_dib8000_modifyWord(v_state,10,0x200,static_cast<std::uint16_t>((Shl<std::int64_t>(16,4)) | (Shl<std::int64_t>(0,0))));
std::uint32_t v_value = m_dib8000_waitLock(v_state,v_internal,20,20,20);
m_dib8000_writeWord(v_state,17,0);
m_dib8000_writeWord(v_state,18,200);
m_dib8000_writeWord(v_state,19,0);
m_dib8000_writeWord(v_state,20,400);
m_dib8000_writeWord(v_state,21,static_cast<std::uint16_t>(static_cast<std::uint32_t>(((Shr<std::uint32_t>(v_value,16))) & (0xffff))));
m_dib8000_writeWord(v_state,22,static_cast<std::uint16_t>(static_cast<std::uint32_t>((v_value) & (0xffff))));
m_dib8000_modifyWord(v_state,32,0xf0ff,Shl<std::uint16_t>(9,8));
m_dib8000_writeWord(v_state,355,2);
m_dib8000_writeWord(v_state,356,0);
m_dib8000_writeWord(v_state,357,0x111);
m_dib8000_modifyWord(v_state,770,0xdfff,Shl<std::uint16_t>(1,13));
m_dib8000_modifyWord(v_state,770,0xdfff,Shl<std::uint16_t>(0,13));
m_dib8000_modifyWord(v_state,0,0x7ff,static_cast<std::uint16_t>((Shl<std::int64_t>(0,15)) | (Shl<std::int64_t>(1,13))));
}
else
if ((((v_state)->v_revision >= 0x8002) && ((v_state)->v_autosearchState == v_autosearchSearchingGuard)))
{
(v_c)->v_transmissionMode = v_transmission8K;
(v_c)->v_guardInterval = v_guard1_8;
(v_c)->v_inversion = 0;
(At((v_c)->v_layer,0)).v_modulation = v_modulationQAM64;
(At((v_c)->v_layer,0)).v_fec = v_fec2_3;
(At((v_c)->v_layer,0)).v_interleaving = 0;
(At((v_c)->v_layer,0)).v_segmentCount = 13;
v_slist = 16;
(v_c)->v_transmissionMode = (v_state)->v_foundNFFT;
m_dib8000_setISDBTCommonChannel(v_state,v_slist,true);
m_dib8000_writeWord(v_state,6,0x4);
m_dib8000_writeWord(v_state,7,0x8);
m_dib8000_writeWord(v_state,8,0x1000);
m_dib8000_waitLock(v_state,v_internal,50,200,1000);
m_dib8000_writeWord(v_state,355,3);
m_dib8000_writeWord(v_state,356,0);
m_dib8000_writeWord(v_state,357,0xf);
std::uint16_t v_value = m_dib8000_readWord(v_state,0);
m_dib8000_writeWord(v_state,0,static_cast<std::uint16_t>((Shl<std::uint16_t>(1,15)) | (v_value)));
m_dib8000_readWord(v_state,1284);
m_dib8000_writeWord(v_state,0,v_value);
}
else
{
(v_c)->v_inversion = 0;
(At((v_c)->v_layer,0)).v_modulation = v_modulationQAM64;
(At((v_c)->v_layer,0)).v_fec = v_fec2_3;
(At((v_c)->v_layer,0)).v_interleaving = 0;
(At((v_c)->v_layer,0)).v_segmentCount = 13;
if (((v_c)->v_guardInterval == v_guardAuto))
{
if (((v_c)->v_transmissionMode == v_transmissionAuto))
{
(v_c)->v_transmissionMode = v_transmission8K;
(v_c)->v_guardInterval = v_guard1_8;
v_slist = 7;
m_dib8000_modifyWord(v_state,0,0x9fff,Shl<std::uint16_t>(1,13));
}
else
{
(v_c)->v_guardInterval = v_guard1_8;
v_slist = 3;
}
}
else
{
if (((v_c)->v_transmissionMode == v_transmissionAuto))
{
(v_c)->v_transmissionMode = v_transmission8K;
v_slist = 2;
m_dib8000_modifyWord(v_state,0,0x9fff,Shl<std::uint16_t>(1,13));
}
else
{
v_slist = 0;
}
}
m_dib8000_setISDBTCommonChannel(v_state,v_slist,true);
m_dib8000_writeWord(v_state,6,0x4);
m_dib8000_writeWord(v_state,7,0x8);
m_dib8000_writeWord(v_state,8,0x1000);
m_dib8000_waitLock(v_state,v_internal,50,100,1000);
std::uint16_t v_value = m_dib8000_readWord(v_state,0);
m_dib8000_writeWord(v_state,0,static_cast<std::uint16_t>((Shl<std::uint16_t>(1,15)) | (v_value)));
m_dib8000_readWord(v_state,1284);
m_dib8000_writeWord(v_state,0,v_value);
}
}
// autosearchIRQ is dib8000_autosearch_irq: 0 still pending, 1 failed,
// 2 succeeded, 3 the FFT correlation pass has finished.
std::int64_t m_dib8000_autosearchIRQ(T_dib8000* v_state)
{
std::uint16_t v_irqPending = m_dib8000_readWord(v_state,1284);
if ((((v_state)->v_revision >= 0x8002) && ((v_state)->v_autosearchState == v_autosearchSearchingFFT)))
{
if ((static_cast<std::uint16_t>((v_irqPending) & (0x1)) != 0))
{
return 3;
}
}
else
{
if ((static_cast<std::uint16_t>((v_irqPending) & (0x1)) != 0))
{
return 1;
}
if ((static_cast<std::uint16_t>((v_irqPending) & (0x2)) != 0))
{
return 2;
}
}
return 0;
}
// viterbiState is dib8000_viterbi_state: P_restart_chd, the channel decoder.
void m_dib8000_viterbiState(T_dib8000* v_state,bool v_on)
{
std::uint16_t v_tmp = m_dib8000_readWord(v_state,771);
if (v_on)
{
m_dib8000_writeWord(v_state,771,static_cast<std::uint16_t>((v_tmp) & (0xfffd)));
}
else
{
m_dib8000_writeWord(v_state,771,static_cast<std::uint16_t>((v_tmp) | (Shl<std::uint16_t>(1,1))));
}
}
// setDDS is dib8000_set_dds.
void m_dib8000_setDDS(T_dib8000* v_state,std::int32_t v_offsetKHz)
{
std::uint32_t v_absOffsetKHz = static_cast<std::uint32_t>(v_offsetKHz);
if ((v_offsetKHz < 0))
{
v_absOffsetKHz = static_cast<std::uint32_t>(Neg<std::int32_t>(v_offsetKHz));
}
std::uint32_t v_dds = static_cast<std::uint32_t>(((((v_state)->v_cfg).v_pll)->v_ifreq) & (0x1ffffff));
std::uint16_t v_invert = static_cast<std::uint16_t>(0);
if ((static_cast<std::uint32_t>(((((v_state)->v_cfg).v_pll)->v_ifreq) & ((Shl<std::uint32_t>(1,25)))) != 0))
{
v_invert = 1;
}
std::uint32_t v_ratio = static_cast<std::uint32_t>(2);
std::int32_t v_unitKHzDDSVal = static_cast<std::int32_t>(static_cast<std::uint32_t>((67108864) / ((((v_state)->v_cfg).v_pll)->v_internal)));
if ((v_offsetKHz < 0))
{
v_unitKHzDDSVal *= Neg<std::int32_t>(1);
}
if ((v_invert != 0))
{
v_dds -= Mul<std::uint32_t>(v_absOffsetKHz,static_cast<std::uint32_t>(v_unitKHzDDSVal));
}
else
{
v_dds += Mul<std::uint32_t>(v_absOffsetKHz,static_cast<std::uint32_t>(v_unitKHzDDSVal));
}
if ((v_absOffsetKHz <= static_cast<std::uint32_t>(((((v_state)->v_cfg).v_pll)->v_internal) / (v_ratio))))
{
m_dib8000_writeWord(v_state,26,v_invert);
m_dib8000_writeWord(v_state,27,static_cast<std::uint16_t>((static_cast<std::uint16_t>(Shr<std::uint32_t>(v_dds,16))) & (0x1ff)));
m_dib8000_writeWord(v_state,28,static_cast<std::uint16_t>(static_cast<std::uint32_t>((v_dds) & (0xffff))));
}
}
// setFrequencyOffset is dib8000_set_frequency_offset: it moves the DDS by the
// difference between the requested frequency and the one the tuner actually
// set, which on this board includes its fixed per-band offset.
void m_dib8000_setFrequencyOffset(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
std::uint32_t v_currentRF{};
if (((v_state)->v_tunerFrequency != nullptr))
{
v_currentRF = (v_state)->v_tunerFrequency();
}
else
{
v_currentRF = (v_c)->v_frequency;
}
v_currentRF /= 1000;
std::int32_t v_totalDDSOffsetKHz = [&]() { auto left = static_cast<std::int32_t>(v_currentRF); auto right = static_cast<std::int32_t>((static_cast<std::int32_t>((v_c)->v_frequency)) / (1000)); return Sub<std::int32_t>(left,right); }();
m_dib8000_setDDS(v_state,v_totalDDSOffsetKHz);
}
// getSymbolDuration is dib8000_get_symbol_duration, in 100us units. For a
// 6MHz channel the division truncates to zero and every mode gives 1.
std::uint32_t m_dib8000_getSymbolDuration(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
std::int64_t v_i{};
{
auto switch43 = (v_c)->v_transmissionMode;
do {
if ((switch43 == v_transmission2K)) {
v_i = 0;
}
else if ((switch43 == v_transmission4K)) {
v_i = 2;
}
else
{
v_i = 1;
}
} while(false); }
return Add<std::uint32_t>(static_cast<std::uint32_t>((static_cast<std::uint32_t>(At(v_isdbtSymbolDuration,v_i))) / ((static_cast<std::uint32_t>(((v_c)->v_bandwidthHz) / (1000))))),1);
}
// setISDBTLoopParams is dib8000_set_isdbt_loop_params for 13 segments.
void m_dib8000_setISDBTLoopParams(T_dib8000* v_state,std::int64_t v_loopStep)
{
std::uint16_t v_reg32{};
std::uint16_t v_reg37{};
{
auto switch44 = v_loopStep;
do {
if ((switch44 == v_loopTune1)) {
v_reg32 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>((Sub<std::uint16_t>(9,(v_state)->v_mode)),12)) | (Shl<std::uint16_t>(6,8)))) | (0x80));
v_reg37 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::int64_t>(3,5)) | (Shl<std::int64_t>(0,4)))) | ((Sub<std::uint16_t>(8,(v_state)->v_mode))));
}
else if ((switch44 == v_loopTune2)) {
v_reg32 = static_cast<std::uint16_t>((static_cast<std::uint16_t>((Shl<std::uint16_t>((Sub<std::uint16_t>(11,(v_state)->v_mode)),12)) | (Shl<std::uint16_t>(6,8)))) | (0x80));
v_reg37 = static_cast<std::uint16_t>((Shl<std::uint16_t>((Add<std::uint16_t>(5,(v_state)->v_mode)),5)) | ((Sub<std::uint16_t>(10,(v_state)->v_mode))));
}
} while(false); }
m_dib8000_writeWord(v_state,32,v_reg32);
m_dib8000_writeWord(v_state,37,v_reg37);
}
// demodRestart is dib8000_demod_restart.
void m_dib8000_demodRestart(T_dib8000* v_state)
{
m_dib8000_writeWord(v_state,770,0x4000);
m_dib8000_writeWord(v_state,770,0x0000);
}
// setSyncWait is dib8000_set_sync_wait: P_dvsy_sync_wait, reusing the mode.
void m_dib8000_setSyncWait(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
std::uint16_t v_syncWait{};
{
auto switch45 = (v_c)->v_transmissionMode;
do {
if ((switch45 == v_transmission8K)) {
v_syncWait = 256;
}
else if ((switch45 == v_transmission4K)) {
v_syncWait = 128;
}
else
{
v_syncWait = 64;
}
} while(false); }
if ((((v_state)->v_cfg).v_diversityDelay == 0))
{
v_syncWait = static_cast<std::uint16_t>(Add<std::int64_t>(static_cast<std::int64_t>(((Mul<std::int64_t>([&]() { auto left = static_cast<std::int64_t>(v_syncWait); auto right = (Shl<std::int64_t>(1,static_cast<std::uint64_t>((v_c)->v_guardInterval))); return Mul<std::int64_t>(left,right); }(),3))) / (2)),48));
}
else
{
v_syncWait = static_cast<std::uint16_t>([&]() { auto left = static_cast<std::int64_t>(((Mul<std::int64_t>([&]() { auto left = static_cast<std::int64_t>(v_syncWait); auto right = (Shl<std::int64_t>(1,static_cast<std::uint64_t>((v_c)->v_guardInterval))); return Mul<std::int64_t>(left,right); }(),3))) / (2)); auto right = static_cast<std::int64_t>(((v_state)->v_cfg).v_diversityDelay); return Add<std::int64_t>(left,right); }());
}
m_dib8000_modifyWord(v_state,273,0x000f,Shl<std::uint16_t>(v_syncWait,4));
}
// getTimeout is dib8000_get_timeout: delay is in 100us units, or in symbols
// when symbolDependent.
Time m_dib8000_getTimeout(T_dib8000* v_state,std::uint32_t v_delay,bool v_symbolDependent)
{
if (v_symbolDependent)
{
v_delay *= static_cast<std::uint32_t>((v_state)->v_symbolDuration);
}
return (v_state)->v_now().Add(Mul<std::int64_t>(Mul<std::int64_t>(static_cast<std::int64_t>(v_delay),100),INT64_C(1000)));
}
// readLock is dib8000_read_lock.
std::uint16_t m_dib8000_readLock(T_dib8000* v_state)
{
return m_dib8000_readWord(v_state,568);
}
// isManualMode is is_manual_mode: whether every parameter is known, so the
// autosearch can be skipped. With only a frequency known it never is.
bool v_isManualMode(T_channel* v_c)
{
std::int64_t v_nSegs = 0;
if (((v_c)->v_deliverySystem != v_systemISDBT))
{
return false;
}
if (((v_c)->v_transmissionMode == v_transmissionAuto))
{
return false;
}
if (((v_c)->v_guardInterval == v_guardAuto))
{
return false;
}
if (((v_c)->v_isdbtLayerEnabled == 0))
{
return false;
}
for (std::int64_t v_i = 0;(v_i < 3);v_i++)
{
if ((static_cast<std::uint8_t>(((v_c)->v_isdbtLayerEnabled) & ((Shl<std::uint8_t>(1,v_i)))) == 0))
{
continue;
}
if ((((At((v_c)->v_layer,v_i)).v_segmentCount > 13) || ((At((v_c)->v_layer,v_i)).v_segmentCount == 0)))
{
(v_c)->v_isdbtLayerEnabled = static_cast<std::uint8_t>((v_c)->v_isdbtLayerEnabled & ~(Shl<std::uint8_t>(1,v_i)));
continue;
}
v_nSegs += static_cast<std::int64_t>((At((v_c)->v_layer,v_i)).v_segmentCount);
if ((((At((v_c)->v_layer,v_i)).v_modulation == v_modulationQAMAuto) || ((At((v_c)->v_layer,v_i)).v_fec == v_fecAuto)))
{
return false;
}
}
if (((v_nSegs == 0) || (v_nSegs > 13)))
{
return false;
}
return true;
}
// tune is dib8000_tune, one step of the demodulator's state machine. It
// returns the time to wait before the next step in 100us units, or 0 once the
// machine has stopped.
std::int64_t m_dib8000_tune(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
std::uint16_t v_locks{};
std::uint16_t v_deeperInterleaver{};
std::int64_t v_ret = 1;
Time v_now = (v_state)->v_now();
{
auto switch46 = (v_state)->v_tuneState;
do {
if ((switch46 == v_demodStart)) {
m_dib8000_resetStats(v_state);
(v_state)->v_status = v_statusTunePending;
if (v_isManualMode(v_c))
{
(v_state)->v_channelParametersSet = 1;
}
else
{
(v_state)->v_channelParametersSet = 0;
}
m_dib8000_viterbiState(v_state,false);
m_dib8000_modifyWord(v_state,285,0x60,0);
m_dib8000_setFrequencyOffset(v_state);
m_dib8000_setBandwidth(v_state,static_cast<std::uint32_t>(((v_c)->v_bandwidthHz) / (1000)));
if (((v_state)->v_channelParametersSet == 0))
{
(v_state)->v_autosearchState = v_autosearchSearchingFFT;
(v_state)->v_foundNFFT = v_transmissionAuto;
(v_state)->v_foundGuard = v_guardAuto;
(v_state)->v_tuneState = v_demodSearchNext;
}
else
{
(v_state)->v_autosearchState = v_autosearchDone;
(v_state)->v_tuneState = v_demodStep3;
}
(v_state)->v_symbolDuration = static_cast<std::uint8_t>(m_dib8000_getSymbolDuration(v_state));
}
else if ((switch46 == v_demodSearchNext)) {
m_dib8000_autosearchStart(v_state);
v_ret = 15;
(v_state)->v_tuneState = v_demodStep1;
}
else if ((switch46 == v_demodStep1)) {
{
auto switch47 = m_dib8000_autosearchIRQ(v_state);
do {
if ((switch47 == 1)) {
(v_state)->v_status = v_statusTuneFailed;
(v_state)->v_autosearchState = v_autosearchDone;
(v_state)->v_tuneState = v_demodStop;
}
else if ((switch47 == 2)) {
(v_state)->v_status = v_statusFFTSuccess;
(v_state)->v_tuneState = v_demodStep3;
if (((v_state)->v_autosearchState == v_autosearchSearchingGuard))
{
(v_state)->v_tuneState = v_demodStep2;
}
else
{
(v_state)->v_autosearchState = v_autosearchDone;
}
}
else if ((switch47 == 3)) {
(v_state)->v_tuneState = v_demodStep2;
}
} while(false); }
}
else if ((switch46 == v_demodStep2)) {
{
auto switch48 = (v_state)->v_autosearchState;
do {
if ((switch48 == v_autosearchSearchingFFT)) {
std::array<std::uint32_t, 4> v_corm{};
At(v_corm,2) = [&]() { auto left = Shl<std::uint32_t>(static_cast<std::uint32_t>(m_dib8000_readWord(v_state,594)),16); auto right = static_cast<std::uint32_t>(m_dib8000_readWord(v_state,595)); return static_cast<std::uint32_t>((left) | (right)); }();
At(v_corm,1) = [&]() { auto left = Shl<std::uint32_t>(static_cast<std::uint32_t>(m_dib8000_readWord(v_state,596)),16); auto right = static_cast<std::uint32_t>(m_dib8000_readWord(v_state,597)); return static_cast<std::uint32_t>((left) | (right)); }();
At(v_corm,0) = [&]() { auto left = Shl<std::uint32_t>(static_cast<std::uint32_t>(m_dib8000_readWord(v_state,598)),16); auto right = static_cast<std::uint32_t>(m_dib8000_readWord(v_state,599)); return static_cast<std::uint32_t>((left) | (right)); }();
std::int64_t v_maxValue = 0;
for (std::int64_t v_findIndex = 1;(v_findIndex < 3);v_findIndex++)
{
if ((At(v_corm,v_maxValue) < At(v_corm,v_findIndex)))
{
v_maxValue = v_findIndex;
}
}
{
auto switch49 = v_maxValue;
do {
if ((switch49 == 0)) {
(v_state)->v_foundNFFT = v_transmission2K;
}
else if ((switch49 == 1)) {
(v_state)->v_foundNFFT = v_transmission4K;
}
else
{
(v_state)->v_foundNFFT = v_transmission8K;
}
} while(false); }
(v_state)->v_tuneState = v_demodSearchNext;
(v_state)->v_autosearchState = v_autosearchSearchingGuard;
v_ret = 10;
}
else if ((switch48 == v_autosearchSearchingGuard)) {
(v_state)->v_foundGuard = static_cast<std::int64_t>(static_cast<std::uint16_t>((m_dib8000_readWord(v_state,570)) & (0x3)));
(v_state)->v_tuneState = v_demodStep3;
}
else
{
(v_state)->v_status = v_statusTuneFailed;
(v_state)->v_autosearchState = v_autosearchDone;
(v_state)->v_tuneState = v_demodStop;
}
} while(false); }
}
else if ((switch46 == v_demodStep3)) {
m_dib8000_setISDBTLoopParams(v_state,v_loopTune1);
m_dib8000_setISDBTCommonChannel(v_state,0,false);
(v_state)->v_tuneState = v_demodStep4;
}
else if ((switch46 == v_demodStep4)) {
m_dib8000_demodRestart(v_state);
m_dib8000_setSyncWait(v_state);
m_dib8000_setDiversityIn(v_state,(v_state)->v_diversityOnOff);
v_locks = static_cast<std::uint16_t>(((Shr<std::uint16_t>(m_dib8000_readWord(v_state,180),6))) & (0x3f));
(v_state)->v_timeout = m_dib8000_getTimeout(v_state,Mul<std::uint32_t>(2,static_cast<std::uint32_t>(v_locks)),true);
(v_state)->v_tuneState = v_demodStep5;
}
else if ((switch46 == v_demodStep5)) {
v_locks = m_dib8000_readLock(v_state);
if ((static_cast<std::uint16_t>((v_locks) & ((Shl<std::uint16_t>(0x3,11)))) != 0))
{
m_dib8000_updateTimf(v_state);
if (((v_state)->v_differentialConstellation == 0))
{
(v_state)->v_timeout = m_dib8000_getTimeout(v_state,Mul<std::uint32_t>(20,static_cast<std::uint32_t>(static_cast<std::uint16_t>(((Shr<std::uint16_t>(m_dib8000_readWord(v_state,188),5))) & (0x1f)))),true);
(v_state)->v_tuneState = v_demodStep7;
}
else
{
(v_state)->v_tuneState = v_demodStep8;
}
}
else
if (v_now.After((v_state)->v_timeout))
{
(v_state)->v_tuneState = v_demodStep6;
}
}
else if ((switch46 == v_demodStep6)) {
m_dib8000_viterbiState(v_state,true);
m_dib8000_setISDBTLoopParams(v_state,v_loopTune2);
(v_state)->v_tuneState = v_demodStop;
(v_state)->v_status = v_statusTuneFailed;
}
else if ((switch46 == v_demodStep7)) {
v_locks = m_dib8000_readLock(v_state);
if ((static_cast<std::uint16_t>((v_locks) & ((Shl<std::uint16_t>(1,10)))) != 0))
{
v_ret = 14;
(v_state)->v_tuneState = v_demodStep8;
}
else
if (v_now.After((v_state)->v_timeout))
{
(v_state)->v_tuneState = v_demodStep6;
}
}
else if ((switch46 == v_demodStep8)) {
m_dib8000_viterbiState(v_state,true);
m_dib8000_setISDBTLoopParams(v_state,v_loopTune2);
(v_state)->v_tuneState = v_demodStep9;
(v_state)->v_status = v_statusLocked;
}
else if ((switch46 == v_demodStep9)) {
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(m_dib8000_readWord(v_state,1291),9))) & (0x1)) != 0))
{
for (std::int64_t v_i = 0;(v_i < 3);v_i++)
{
if ((static_cast<std::uint16_t>((At((v_c)->v_layer,v_i)).v_interleaving) >= v_deeperInterleaver))
{
if (((At((v_c)->v_layer,v_i)).v_segmentCount > 0))
{
v_deeperInterleaver = static_cast<std::uint16_t>((At((v_c)->v_layer,0)).v_interleaving);
(v_state)->v_longestIntlvLayer = static_cast<std::uint8_t>(v_i);
}
}
}
{
auto switch50 = v_deeperInterleaver;
do {
if ((switch50 == 0)) {
v_locks = 2;
}
else if ((switch50 == 3)) {
v_locks = 8;
}
else
{
v_locks = Mul<std::uint16_t>(2,v_deeperInterleaver);
}
} while(false); }
if (((v_state)->v_diversityOnOff != 0))
{
v_locks *= 2;
}
(v_state)->v_timeout = v_now.Add(Mul<std::int64_t>(static_cast<std::int64_t>(Mul<std::int64_t>(200,static_cast<std::int64_t>(v_locks))),INT64_C(1000000)));
(v_state)->v_tuneState = v_demodStep10;
}
else
{
(v_state)->v_tuneState = v_demodStop;
}
}
else if ((switch46 == v_demodStep10)) {
v_locks = m_dib8000_readLock(v_state);
if ((static_cast<std::uint16_t>((v_locks) & ((Shl<std::uint16_t>(1,(Sub<std::uint8_t>(7,(v_state)->v_longestIntlvLayer)))))) != 0))
{
(v_state)->v_status = v_statusDataLocked;
(v_state)->v_tuneState = v_demodStop;
}
else
if (v_now.After((v_state)->v_timeout))
{
if ((static_cast<std::uint16_t>((v_locks) & ((Shl<std::uint16_t>(0x7,5)))) != 0))
{
(v_state)->v_status = v_statusDataLocked;
}
else
{
(v_state)->v_status = v_statusTuneFailed;
}
(v_state)->v_tuneState = v_demodStop;
}
}
} while(false); }
if (((v_state)->v_tuneState == v_demodStop))
{
v_ret = 0;
}
if (((v_ret > 0) && ((v_state)->v_tuneState > v_demodStep3)))
{
return Mul<std::int64_t>(v_ret,static_cast<std::int64_t>((v_state)->v_symbolDuration));
}
if (((v_ret > 0) && (v_ret < static_cast<std::int64_t>((v_state)->v_symbolDuration))))
{
return static_cast<std::int64_t>((v_state)->v_symbolDuration);
}
return v_ret;
}
// getFrontend is dib8000_get_frontend: it reads the parameters the
// autosearch found back into the channel.
void m_dib8000_getFrontend(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
(v_c)->v_bandwidthHz = 6000000;
if ((static_cast<std::int64_t>((m_dib8000_readStatus(v_state)) & (v_feHasSync)) == 0))
{
return ;
}
m_dib8000_readWord(v_state,508);
std::uint16_t v_val = m_dib8000_readWord(v_state,570);
(v_c)->v_inversion = static_cast<std::int64_t>(Shr<std::uint16_t>((static_cast<std::uint16_t>((v_val) & (0x40))),6));
{
auto switch51 = Shr<std::uint16_t>((static_cast<std::uint16_t>((v_val) & (0x30))),4);
do {
if ((switch51 == 1)) {
(v_c)->v_transmissionMode = v_transmission2K;
}
else if ((switch51 == 2)) {
(v_c)->v_transmissionMode = v_transmission4K;
}
else
{
(v_c)->v_transmissionMode = v_transmission8K;
}
} while(false); }
{
auto switch52 = static_cast<std::uint16_t>((v_val) & (0x3));
do {
if ((switch52 == 0)) {
(v_c)->v_guardInterval = v_guard1_32;
}
else if ((switch52 == 1)) {
(v_c)->v_guardInterval = v_guard1_16;
}
else if ((switch52 == 2)) {
(v_c)->v_guardInterval = v_guard1_8;
}
else if ((switch52 == 3)) {
(v_c)->v_guardInterval = v_guard1_4;
}
} while(false); }
v_val = m_dib8000_readWord(v_state,505);
(v_c)->v_isdbtPartialReception = static_cast<std::uint8_t>(static_cast<std::uint16_t>((v_val) & (1)));
for (std::uint16_t v_i = static_cast<std::uint16_t>(0);(v_i < 3);v_i++)
{
v_val = static_cast<std::uint16_t>((m_dib8000_readWord(v_state,Add<std::uint16_t>(493,v_i))) & (0x0f));
(At((v_c)->v_layer,v_i)).v_segmentCount = static_cast<std::uint8_t>(v_val);
v_val = static_cast<std::uint16_t>((m_dib8000_readWord(v_state,Add<std::uint16_t>(499,v_i))) & (0x3));
if ((v_val == 3))
{
v_val = 4;
}
(At((v_c)->v_layer,v_i)).v_interleaving = static_cast<std::uint8_t>(v_val);
v_val = m_dib8000_readWord(v_state,Add<std::uint16_t>(481,v_i));
{
auto switch53 = static_cast<std::uint16_t>((v_val) & (0x7));
do {
if ((switch53 == 1)) {
(At((v_c)->v_layer,v_i)).v_fec = v_fec1_2;
}
else if ((switch53 == 2)) {
(At((v_c)->v_layer,v_i)).v_fec = v_fec2_3;
}
else if ((switch53 == 3)) {
(At((v_c)->v_layer,v_i)).v_fec = v_fec3_4;
}
else if ((switch53 == 5)) {
(At((v_c)->v_layer,v_i)).v_fec = v_fec5_6;
}
else
{
(At((v_c)->v_layer,v_i)).v_fec = v_fec7_8;
}
} while(false); }
v_val = m_dib8000_readWord(v_state,Add<std::uint16_t>(487,v_i));
{
auto switch54 = static_cast<std::uint16_t>((v_val) & (0x3));
do {
if ((switch54 == 0)) {
(At((v_c)->v_layer,v_i)).v_modulation = v_modulationDQPSK;
}
else if ((switch54 == 1)) {
(At((v_c)->v_layer,v_i)).v_modulation = v_modulationQPSK;
}
else if ((switch54 == 2)) {
(At((v_c)->v_layer,v_i)).v_modulation = v_modulationQAM16;
}
else
{
(At((v_c)->v_layer,v_i)).v_modulation = v_modulationQAM64;
}
} while(false); }
}
}
// setFrontend is dib8000_set_frontend for a single frontend: tune the tuner,
// start the AGC, then run the demodulator's state machine to the end, waiting
// between its steps as minTuneStep says.
Error m_dib8000_setFrontend(T_dib8000* v_state)
{
T_channel* v_c = (v_state)->v_channel;
if (((v_c)->v_frequency == 0))
{
return MakeError("DiB8000: no frequency to tune to");
}
if (((v_c)->v_bandwidthHz == 0))
{
(v_c)->v_bandwidthHz = 6000000;
}
(v_c)->v_deliverySystem = v_systemISDBT;
m_dib8000_setDiversityIn(v_state,1);
{
Error v_err = m_dib8000_setOutputMode(v_state,v_outmodeHighZ);
if ((v_err != nullptr))
{
return v_err;
}
}
if (((v_state)->v_tunerSetParams != nullptr))
{
{
Error v_err = (v_state)->v_tunerSetParams();
if ((v_err != nullptr))
{
return v_err;
}
}
}
(v_state)->v_tuneState = v_agcStart;
m_dib8000_setDiversityIn(v_state,0);
for (;true;)
{
std::int64_t v_wait = m_dib8000_agcStartup(v_state);
if (((v_wait == 0) || (((v_state)->v_failure)->v_err != nullptr)))
{
break;
}
v_wait = static_cast<std::int64_t>((Mul<std::int64_t>(10,(Add<std::int64_t>(v_wait,99)))) / (100));
m_dib8000_msleep(v_state,v_wait);
if (((v_state)->v_tuneState == v_agcStop))
{
break;
}
}
if ((((v_state)->v_failure)->v_err != nullptr))
{
return ((v_state)->v_failure)->v_err;
}
(v_state)->v_tuneState = v_demodStart;
Time v_giveUp = (v_state)->v_now().Add((v_state)->v_tuneTimeout);
bool v_active = true;
for (;v_active;)
{
Time v_callbackTime{};
bool v_haveCallback = false;
std::int64_t v_delay = m_dib8000_tune(v_state);
if ((v_delay != 0))
{
v_callbackTime = (v_state)->v_now().Add(Mul<std::int64_t>(Mul<std::int64_t>(static_cast<std::int64_t>(v_delay),100),INT64_C(1000)));
v_haveCallback = true;
}
if (((v_state)->v_channelParametersSet == 0))
{
if ((((v_state)->v_status == v_statusDemodSuccess) || ((v_state)->v_status == v_statusFFTSuccess)))
{
m_dib8000_getFrontend(v_state);
(v_state)->v_channelParametersSet = 1;
}
}
if (((((v_state)->v_status == v_statusTuneFailed) || ((v_state)->v_status == v_statusLocked)) || ((v_state)->v_status == v_statusDataLocked)))
{
v_active = ((v_state)->v_tuneState != v_demodStop);
}
if ((v_active && static_cast<bool>(!(v_haveCallback))))
{
v_active = false;
}
if (v_active)
{
(v_state)->v_sleep(GoMax(v_callbackTime.Sub((v_state)->v_now()),v_minTuneStep));
}
if ((((v_state)->v_failure)->v_err != nullptr))
{
return ((v_state)->v_failure)->v_err;
}
if ((v_active && (v_state)->v_now().After(v_giveUp)))
{
return MakeError("DiB8000: tune did not finish within %v (state %d)",(v_state)->v_tuneTimeout,(v_state)->v_tuneState);
}
}
return m_dib8000_setOutputMode(v_state,((v_state)->v_cfg).v_outputMode);
}
// readStatus is the lock half of dib8000_read_status. The DVBv5 statistics
// it then gathers are read separately, by the caller that wants them.
std::int64_t m_dib8000_readStatus(T_dib8000* v_state)
{
std::uint16_t v_lock = m_dib8000_readLock(v_state);
std::int64_t v_stat = 0;
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_lock,13))) & (1)) != 0))
{
v_stat |= v_feHasSignal;
}
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_lock,8))) & (1)) != 0))
{
v_stat |= v_feHasCarrier;
}
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_lock,1))) & (0xf)) == 0xf))
{
v_stat |= v_feHasSync;
}
if (((static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_lock,12))) & (1)) != 0) && (static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_lock,5))) & (7)) != 0)))
{
v_stat |= v_feHasLock;
}
if ((static_cast<std::uint16_t>(((Shr<std::uint16_t>(v_lock,12))) & (1)) != 0))
{
if ((static_cast<std::uint16_t>((m_dib8000_readWord(v_state,554)) & (0x01)) != 0))
{
v_stat |= v_feHasViterbi;
}
if ((static_cast<std::uint16_t>((m_dib8000_readWord(v_state,555)) & (0x01)) != 0))
{
v_stat |= v_feHasViterbi;
}
if ((static_cast<std::uint16_t>((m_dib8000_readWord(v_state,556)) & (0x01)) != 0))
{
v_stat |= v_feHasViterbi;
}
}
return v_stat;
}
// readUncBlocks is dib8000_read_unc_blocks: packet errors over 13 segments.
std::uint32_t m_dib8000_readUncBlocks(T_dib8000* v_state)
{
return static_cast<std::uint32_t>(m_dib8000_readWord(v_state,565));
}
// readSignalStrength is dib8000_read_signal_strength for one frontend: the
// inverted global AGC value.
std::uint16_t m_dib8000_readSignalStrength(T_dib8000* v_state)
{
return Sub<std::uint16_t>(65535,m_dib8000_readWord(v_state,390));
}
// shiftLeft32 is C's u32 << count as x86 executes it: the count is taken
// modulo 32. dib8000_get_snr can hand it a wrapped negative exponent.
std::uint32_t v_shiftLeft32(std::uint32_t v_value,std::uint32_t v_count)
{
return Shl<std::uint32_t>(v_value,(static_cast<std::uint32_t>((v_count) & (31))));
}
// getSNR is dib8000_get_snr: signal over noise as a 16.16 fixed-point ratio,
// or 0xffffffff when there is no noise reading.
std::uint32_t m_dib8000_getSNR(T_dib8000* v_state)
{
std::uint16_t v_val = m_dib8000_readWord(v_state,542);
std::uint32_t v_n = static_cast<std::uint32_t>((static_cast<std::uint32_t>(Shr<std::uint16_t>(v_val,6))) & (0xff));
std::uint32_t v_exp = static_cast<std::uint32_t>(static_cast<std::uint16_t>((v_val) & (0x3f)));
if ((static_cast<std::uint32_t>((v_exp) & (0x20)) != 0))
{
v_exp -= 0x40;
}
v_n = v_shiftLeft32(v_n,Add<std::uint32_t>(v_exp,16));
v_val = m_dib8000_readWord(v_state,543);
std::uint32_t v_s = static_cast<std::uint32_t>((static_cast<std::uint32_t>(Shr<std::uint16_t>(v_val,6))) & (0xff));
v_exp = static_cast<std::uint32_t>(static_cast<std::uint16_t>((v_val) & (0x3f)));
if ((static_cast<std::uint32_t>((v_exp) & (0x20)) != 0))
{
v_exp -= 0x40;
}
v_s = v_shiftLeft32(v_s,Add<std::uint32_t>(v_exp,16));
if ((v_n > 0))
{
std::uint32_t v_t = Shl<std::uint32_t>((static_cast<std::uint32_t>((v_s) / (v_n))),16);
return Add<std::uint32_t>(v_t,static_cast<std::uint32_t>(((Sub<std::uint32_t>((Shl<std::uint32_t>(v_s,16)),Mul<std::uint32_t>(v_n,v_t)))) / (v_n)));
}
return 0xffffffff;
}
// interpolateValue is interpolate_value: a descending table, linear between
// its entries.
std::uint32_t v_interpolateValue(std::uint32_t v_value,Slice<T_linearSegment> v_segments)
{
std::int64_t v_length = Len(v_segments);
if ((v_value >= (At(v_segments,0)).v_x))
{
return static_cast<std::uint32_t>((At(v_segments,0)).v_y);
}
if ((v_value < (At(v_segments,Sub<std::int64_t>(v_length,1))).v_x))
{
return static_cast<std::uint32_t>((At(v_segments,Sub<std::int64_t>(v_length,1))).v_y);
}
std::int64_t v_i = 1;
for (;(v_i < Sub<std::int64_t>(v_length,1));v_i++)
{
if ((v_value == (At(v_segments,v_i)).v_x))
{
return static_cast<std::uint32_t>((At(v_segments,v_i)).v_y);
}
if ((v_value > (At(v_segments,v_i)).v_x))
{
break;
}
}
std::int32_t v_dy = Sub<std::int32_t>((At(v_segments,Sub<std::int64_t>(v_i,1))).v_y,(At(v_segments,v_i)).v_y);
std::uint32_t v_dx = Sub<std::uint32_t>((At(v_segments,Sub<std::int64_t>(v_i,1))).v_x,(At(v_segments,v_i)).v_x);
std::uint64_t v_tmp64 = static_cast<std::uint64_t>(Sub<std::uint32_t>(v_value,(At(v_segments,v_i)).v_x));
v_tmp64 *= static_cast<std::uint64_t>(static_cast<std::int64_t>(v_dy));
v_tmp64 /= static_cast<std::uint64_t>(v_dx);
return static_cast<std::uint32_t>([&]() { auto left = static_cast<std::int32_t>((At(v_segments,v_i)).v_y); auto right = static_cast<std::int32_t>(v_tmp64); return Add<std::int32_t>(left,right); }());
}
// Source: dibx000.go
// bandOfFrequency is BAND_OF_FREQUENCY. The FM test sits after CBAND's wider
// bound and so can never match; the macro is kept as written.
std::uint8_t v_bandOfFrequency(std::uint32_t v_kHz)
{
{
do {
if (((v_kHz <= 170000))) {
return v_bandCBand;
}
else if (((v_kHz <= 115000))) {
return v_bandFM;
}
else if (((v_kHz <= 250000))) {
return v_bandVHF;
}
else if (((v_kHz <= 863000))) {
return v_bandUHF;
}
else if (((v_kHz <= 2000000))) {
return v_bandLBand;
}
else
{
return v_bandSBand;
}
} while(false); }
}
// initialize is dibx000_init_i2c_master: it picks the register base and
// closes the gate.
void m_i2cMaster_initialize(T_i2cMaster* v_master,T_dib8000* v_demod,std::uint16_t v_deviceRevision)
{
(v_master)->v_demod = v_demod;
(v_master)->v_deviceRevision = v_deviceRevision;
if (((v_deviceRevision == v_deviceDiB7000P) || (v_deviceRevision == v_deviceDiB8000)))
{
(v_master)->v_baseRegister = 1024;
}
else
{
(v_master)->v_baseRegister = 768;
}
m_dib8000_writeWord((v_master)->v_demod,Add<std::uint16_t>((v_master)->v_baseRegister,1),m_i2cMaster_gateControl(v_master,0,false));
(v_master)->v_gate = 0;
}
// reset is dibx000_reset_i2c_master.
void m_i2cMaster_reset(T_i2cMaster* v_master)
{
m_dib8000_writeWord((v_master)->v_demod,Add<std::uint16_t>((v_master)->v_baseRegister,1),m_i2cMaster_gateControl(v_master,0,false));
(v_master)->v_gate = 0;
(v_master)->v_selectedInterface = 0xff;
m_i2cMaster_selectInterface(v_master,v_interfaceTuner);
}
// gateControl is dibx000_i2c_gate_ctrl's register value. Bit 7 chooses between
// the master and the gate: clear, the gate is open to address. Later chips moved
// the field up one bit.
std::uint16_t m_i2cMaster_gateControl(T_i2cMaster* v_master,std::uint8_t v_address7,bool v_open)
{
std::uint16_t v_value{};
if (v_open)
{
v_value = Shl<std::uint16_t>(static_cast<std::uint16_t>(v_address7),8);
}
else
{
v_value = Shl<std::uint16_t>(1,7);
}
if (((v_master)->v_deviceRevision > v_deviceDiB7000))
{
v_value <<= 1;
}
return v_value;
}
// selectInterface is dibx000_i2c_select_interface.
void m_i2cMaster_selectInterface(T_i2cMaster* v_master,std::uint16_t v_intf)
{
if ((((v_master)->v_deviceRevision > v_deviceDiB3000MC) && ((v_master)->v_selectedInterface != v_intf)))
{
(v_master)->v_selectedInterface = v_intf;
m_dib8000_writeWord((v_master)->v_demod,Add<std::uint16_t>((v_master)->v_baseRegister,4),v_intf);
}
}
// Source: tables.go
// Source: tune_result.go
// SearchFailed reports only a completed, explicitly failed search. Call after
// Tune returns nil; an unlocked status while acquisition is pending is not failure.
// This exposes the existing upstream state without changing its tuning sequence.
bool m_Demod_SearchFailed(T_Demod* v_board)
{
return (((((v_board)->v_failure).v_err == nullptr) && (((v_board)->v_demod)->v_status == v_statusTuneFailed)) && (((v_board)->v_demod)->v_tuneState == v_demodStop));
}
} // namespace px4::torne::dibnative
