// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only

package dib8096

import (
	"errors"
	"fmt"
	"strings"
	"testing"
	"time"
)

// fakeChip is a DiB8096 on the CECH-ZD1 J's bridge: the DiB8000 takes four-byte
// register writes and two-byte-address, two-byte-data register reads, and
// the DiB0090 behind its gate three-byte writes and one-byte-address,
// two-byte-data reads while the gate is open to it. Anything else is an
// error.
type fakeChip struct {
	registers map[uint16]uint16
	// queued read values per register; the last one repeats
	queues map[uint16][]uint16

	writes []registerWrite

	tunerRegisters map[uint8]uint16
	tunerWrites    []registerWrite
}

type registerWrite struct {
	register uint16
	value    uint16
}

func newFakeChip() *fakeChip {
	return &fakeChip{
		registers: map[uint16]uint16{
			896: 0x01b3,
			897: 0x8002,
		},
		queues: map[uint16][]uint16{},
		tunerRegisters: map[uint8]uint16{
			// Krosus, MP001 (9090/8096), P1G, PLL locked.
			0x1a: 0x0a27,
		},
	}
}

// gateOpen is the gate register as dibx000_i2c_gate_ctrl opens it to the
// tuner on a DiB8000.
const gateOpen = uint16(tunerAddress>>1) << 9

func (chip *fakeChip) tunerReachable(address byte) error {
	if address != tunerAddress {
		return fmt.Errorf("access to address %#02x", address)
	}
	if chip.registers[1025] != gateOpen {
		return fmt.Errorf("tuner access with the gate at %#04x", chip.registers[1025])
	}
	return nil
}

func (chip *fakeChip) Write(address byte, data []byte) error {
	if address != Address {
		if err := chip.tunerReachable(address); err != nil {
			return err
		}
		if len(data) != 3 {
			return fmt.Errorf("tuner write of %d bytes", len(data))
		}
		value := uint16(data[1])<<8 | uint16(data[2])
		chip.tunerRegisters[data[0]] = value
		chip.tunerWrites = append(chip.tunerWrites, registerWrite{uint16(data[0]), value})
		return nil
	}
	if len(data) != 4 {
		return fmt.Errorf("write of %d bytes; the DiB8000 takes 4", len(data))
	}
	register := uint16(data[0])<<8 | uint16(data[1])
	value := uint16(data[2])<<8 | uint16(data[3])
	chip.writes = append(chip.writes, registerWrite{register, value})
	chip.registers[register] = value
	return nil
}

func (chip *fakeChip) Read(address byte, length int) ([]byte, error) {
	return nil, errors.New("bare read; only register reads are expected")
}

func (chip *fakeChip) WriteRead(address byte, write []byte, readLength int) ([]byte, error) {
	if address != Address {
		if err := chip.tunerReachable(address); err != nil {
			return nil, err
		}
		if len(write) != 1 || readLength != 2 {
			return nil, fmt.Errorf("tuner read shape %d/%d", len(write), readLength)
		}
		value := chip.tunerRegisters[write[0]]
		return []byte{byte(value >> 8), byte(value)}, nil
	}
	if len(write) != 2 || readLength != 2 {
		return nil, fmt.Errorf("read shape %d/%d; the bridge only does 2/2", len(write), readLength)
	}
	register := uint16(write[0])<<8 | uint16(write[1])
	value := chip.registers[register]
	if queue := chip.queues[register]; len(queue) > 0 {
		value = queue[0]
		if len(queue) > 1 {
			chip.queues[register] = queue[1:]
		}
	}
	return []byte{byte(value >> 8), byte(value)}, nil
}

// lastWrite is the last value written to a DiB8000 register.
func (chip *fakeChip) lastWrite(register uint16) (uint16, bool) {
	for i := len(chip.writes) - 1; i >= 0; i-- {
		if chip.writes[i].register == register {
			return chip.writes[i].value, true
		}
	}
	return 0, false
}

// fakeClock stands in for time: sleeping advances it.
type fakeClock struct {
	now   time.Time
	slept time.Duration
}

func (clock *fakeClock) Now() time.Time { return clock.now }

func (clock *fakeClock) Sleep(duration time.Duration) {
	clock.now = clock.now.Add(duration)
	clock.slept += duration
}

func newTestDemod(chip *fakeChip) (*Demod, *fakeClock) {
	clock := &fakeClock{now: time.Unix(1000, 0)}
	demod := New(chip)
	demod.setTiming(clock.Now, clock.Sleep, 5*time.Second)
	return demod, clock
}

func TestInitialize(t *testing.T) {
	chip := newFakeChip()
	demod, _ := newTestDemod(chip)
	if err := demod.Initialize(); err != nil {
		t.Fatal(err)
	}
	if demod.demod.revision != 0x8002 {
		t.Errorf("revision = %#04x, want 0x8002", demod.demod.revision)
	}
	if demod.tuner.identity.p1g != 1 {
		t.Errorf("tuner not identified as P1G: %+v", demod.tuner.identity)
	}

	// The output ends parallel with a gated clock, as the CECH-ZD1 J's bridge
	// takes it (1286 = 0x0400, 300 = 1792, 1291 = 0x0205, 299 with the
	// 188-byte and bit-1 flags).
	for register, want := range map[uint16]uint16{1286: 0x0400, 300: 1792, 1291: 0x0205} {
		if got, _ := chip.lastWrite(register); got != want {
			t.Errorf("register %d = %#04x, want %#04x", register, got, want)
		}
	}
	if got, _ := chip.lastWrite(299); got&0x22 != 0x22 {
		t.Errorf("register 299 = %#04x, want bits 0x22 set", got)
	}

	// The master's tuner interface was selected and the tuner's defaults
	// arrived through it.
	if got, ok := chip.lastWrite(1028); !ok || got != 0 {
		t.Errorf("interface select 1028 = %#x (written %v), want 0", got, ok)
	}
	if got := chip.tunerRegisters[0x14]; got != 2 {
		t.Errorf("tuner crystal mode 0x14 = %d, want 2 for a 12MHz clock", got)
	}
	if demod.tuner.calibrate != dcCal|wbdCal|tempCal {
		t.Errorf("tuner calibrations pending = %#x", demod.tuner.calibrate)
	}
}

func TestInitializeRejectsUnknownChip(t *testing.T) {
	chip := newFakeChip()
	chip.registers[897] = 0x7000
	demod, _ := newTestDemod(chip)
	if err := demod.Initialize(); err == nil || !strings.Contains(err.Error(), "device ID") {
		t.Fatalf("Initialize = %v, want a device ID error", err)
	}
}

// TestTunerThroughGate checks the gated transfer: the gate opens to the
// tuner once, and the messages go out on the host's bus.
func TestTunerThroughGate(t *testing.T) {
	chip := newFakeChip()
	demod, _ := newTestDemod(chip)
	demod.demod.master.initialize(demod.demod, deviceDiB8000)
	demod.demod.master.reset()
	chip.writes = nil

	demod.tuner.writeReg(0x24, 0x1234)
	if got := demod.tuner.readReg(0x24); got != 0x1234 {
		t.Errorf("tuner register 0x24 reads %#04x, want 0x1234", got)
	}
	if err := demod.failure.err; err != nil {
		t.Fatal(err)
	}
	want := []registerWrite{{1025, gateOpen}}
	if fmt.Sprint(chip.writes) != fmt.Sprint(want) {
		t.Fatalf("DiB8000 writes = %x, want %x", chip.writes, want)
	}
	if gateOpen != 0xc000 {
		t.Fatalf("gate value drifted: %#04x", gateOpen)
	}
	// A master reset closes the gate, and the next message opens it again.
	demod.demod.master.reset()
	chip.writes = nil
	demod.tuner.writeReg(0x24, 0x4321)
	if got, _ := chip.lastWrite(1025); got != gateOpen {
		t.Errorf("gate after a reset = %#04x, want it opened again", got)
	}
}

// lockedChip answers the autosearch and lock registers as a chip receiving a
// good 13-segment signal would.
func lockedChip() *fakeChip {
	chip := newFakeChip()
	// FFT pass finishes (bit 0), the read that clears the interrupt, then
	// the guard pass succeeds (bit 1).
	chip.queues[1284] = []uint16{0x1, 0x0, 0x2}
	chip.registers[568] = 0x3ffe                    // every lock bit
	chip.registers[554] = 1                         // Viterbi layer A
	chip.registers[570] = 3<<4 | 2                  // 8K, 1/8
	chip.registers[493] = 1                         // layer A: 1 segment
	chip.registers[494] = 12                        // layer B: 12 segments
	chip.registers[487], chip.registers[488] = 1, 3 // QPSK, 64QAM
	chip.registers[481], chip.registers[482] = 2, 3 // 2/3, 3/4
	chip.registers[499], chip.registers[500] = 2, 2 // interleaving 2
	chip.registers[505] = 1                         // partial reception
	chip.registers[542] = 1 << 6                    // noise 1<<16
	chip.registers[543] = 100 << 6                  // signal 100<<16
	chip.registers[390] = 65535 - 50117             // -35 dBm
	return chip
}

func TestTune(t *testing.T) {
	chip := lockedChip()
	demod, clock := newTestDemod(chip)
	if err := demod.Initialize(); err != nil {
		t.Fatal(err)
	}
	start := clock.now

	const frequency = 473142857 // UHF channel 13
	if err := demod.Tune(frequency); err != nil {
		t.Fatal(err)
	}
	if demod.demod.status != statusDataLocked || demod.demod.tuneState != demodStop {
		t.Errorf("status %d, state %d; want data locked and stopped", demod.demod.status, demod.demod.tuneState)
	}
	// The first tune after reset runs the tuner's DC offset calibration;
	// against a fake ADC that never changes sign it takes all 16 steps on
	// each of its four paths, about 4s. Real silicon stops sooner.
	if elapsed := clock.now.Sub(start); elapsed > 7*time.Second {
		t.Errorf("first tune took %v of simulated time", elapsed)
	}

	// The parameters the autosearch found were read back.
	c := demod.channel
	if c.transmissionMode != transmission8K || c.guardInterval != guard1_8 ||
		c.layer[1].segmentCount != 12 || c.layer[1].modulation != modulationQAM64 || c.layer[1].fec != fec3_4 {
		t.Errorf("channel after tune = %+v", c)
	}

	// UHF: GPIO3 low.
	if got, _ := chip.lastWrite(1030); got&(1<<3) != 0 {
		t.Errorf("GPIO values %#04x, want GPIO3 low for UHF", got)
	}

	// The tuner is 63kHz low on UHF and the DDS makes up for it:
	// 63 * (u32)-1242, as dib8000_set_dds computes it.
	if got, _ := chip.lastWrite(27); got != 0x1fe {
		t.Errorf("DDS high = %#x, want 0x1fe", got)
	}
	if got, _ := chip.lastWrite(28); got != 0xce5a {
		t.Errorf("DDS low = %#x, want 0xce5a", got)
	}

	// The PLL ratio and its timing frequency.
	ratio := dib8090ComputePLLParameters(frequency)
	if got, _ := chip.lastWrite(901); got != 1<<8|uint16(ratio) {
		t.Errorf("PLL 901 = %#x, want ratio %d", got, ratio)
	}

	// The output is back to parallel gated after the tune held it high-Z.
	if got, _ := chip.lastWrite(1286); got != 0x0400 {
		t.Errorf("1286 = %#04x after tune", got)
	}

	// The tuner was programmed for the channel less its UHF offset.
	if demod.tuner.currentRF != frequency/1000-63 {
		t.Errorf("tuner at %d kHz", demod.tuner.currentRF)
	}

	status, err := demod.Status()
	if err != nil {
		t.Fatal(err)
	}
	if !status.Locked || strings.Join(status.Names, ",") != "SIGNAL,CARRIER,VITERBI,SYNC,LOCK" {
		t.Errorf("status = %+v", status)
	}
	if !status.HasCNR || status.CNR != 20 {
		t.Errorf("CNR = %v (%v), want 20 dB", status.CNR, status.HasCNR)
	}
	if !status.HasStrength || status.StrengthDBm != -35 {
		t.Errorf("strength = %v (%v), want -35 dBm", status.StrengthDBm, status.HasStrength)
	}

	// A second tune, after a sleep, runs to completion too.
	if err := demod.Sleep(); err != nil {
		t.Fatal(err)
	}
	if got, _ := chip.lastWrite(1286); got != 0 {
		t.Errorf("1286 = %#04x asleep, want high-Z", got)
	}
	chip.queues[1284] = []uint16{0x1, 0x0, 0x2}
	start = clock.now
	if err := demod.Tune(213142857); err != nil { // VHF, above CBAND
		t.Fatal(err)
	}
	if elapsed := clock.now.Sub(start); elapsed > 3*time.Second {
		t.Errorf("second tune took %v of simulated time", elapsed)
	}
	t.Logf("second tune: %v simulated", clock.now.Sub(start))
	if got, _ := chip.lastWrite(1030); got&(1<<3) == 0 {
		t.Errorf("GPIO values %#04x, want GPIO3 high for VHF", got)
	}
}

// Without a search interrupt the demodulator's state machine would wait
// forever; Tune gives up instead.
func TestSetGPIOSurvivesTune(t *testing.T) {
	chip := lockedChip()
	demod, _ := newTestDemod(chip)
	if err := demod.Initialize(); err != nil {
		t.Fatal(err)
	}
	if err := demod.SetGPIO(1, true); err != nil {
		t.Fatal(err)
	}
	if got, _ := chip.lastWrite(1029); got&(1<<1) != 0 {
		t.Errorf("GPIO directions %#04x, want GPIO1 an output", got)
	}
	if got, _ := chip.lastWrite(1030); got&(1<<1) == 0 {
		t.Errorf("GPIO values %#04x, want GPIO1 high", got)
	}

	// The band switch rewrites the GPIO registers; GPIO1 must stay as set.
	if err := demod.Tune(473142857); err != nil {
		t.Fatal(err)
	}
	if got, _ := chip.lastWrite(1030); got&(1<<1) == 0 || got&(1<<3) != 0 {
		t.Errorf("GPIO values after tune %#04x, want GPIO1 high and GPIO3 low", got)
	}

	if err := demod.SetGPIO(1, false); err != nil {
		t.Fatal(err)
	}
	if got, _ := chip.lastWrite(1030); got&(1<<1) != 0 {
		t.Errorf("GPIO values %#04x, want GPIO1 low", got)
	}
}

func TestTuneGivesUp(t *testing.T) {
	chip := newFakeChip()
	demod, clock := newTestDemod(chip)
	if err := demod.Initialize(); err != nil {
		t.Fatal(err)
	}
	start := clock.now
	err := demod.Tune(557142857)
	if err == nil || !strings.Contains(err.Error(), "did not finish") {
		t.Fatalf("Tune = %v, want a timeout", err)
	}
	if elapsed := clock.now.Sub(start); elapsed > 10*time.Second {
		t.Errorf("gave up after %v", elapsed)
	}
}

// A search that fails ends the tune without error, unlocked.
func TestTuneNoSignal(t *testing.T) {
	chip := newFakeChip()
	demod, _ := newTestDemod(chip)
	if err := demod.Initialize(); err != nil {
		t.Fatal(err)
	}
	chip.queues[1284] = []uint16{0x1, 0x0, 0x1}
	if err := demod.Tune(557142857); err != nil {
		t.Fatal(err)
	}
	if demod.demod.status != statusTuneFailed {
		t.Errorf("status %d, want failed", demod.demod.status)
	}
	status, err := demod.Status()
	if err != nil {
		t.Fatal(err)
	}
	if status.Locked || status.HasCNR || len(status.Names) != 0 {
		t.Errorf("status = %+v", status)
	}
}

// VHF 1 is in CBAND, where the tuner's software AGC finds the gain before the
// demodulator's PWM AGC takes over.
func TestTuneCBand(t *testing.T) {
	chip := lockedChip()
	demod, _ := newTestDemod(chip)
	if err := demod.Initialize(); err != nil {
		t.Fatal(err)
	}
	chip.queues[1284] = []uint16{0x1, 0x0, 0x2}
	if err := demod.Tune(93142857); err != nil {
		t.Fatal(err)
	}
	tuner := demod.tuner
	if tuner.currentBand != bandCBand {
		t.Errorf("tuner band %#x, want CBAND", tuner.currentBand)
	}
	if tuner.tuneState < agcStop {
		t.Errorf("software AGC left at state %d", tuner.tuneState)
	}
	// Once it has, the PWM ramps are back in place.
	if &tuner.bbRamp[0] != &bbRampPWMNormal[0] {
		t.Errorf("baseband ramp not handed back to the PWM AGC")
	}
	if got, _ := chip.lastWrite(1030); got&(1<<3) == 0 {
		t.Errorf("GPIO values %#04x, want GPIO3 high (VHF input)", got)
	}
	status, err := demod.Status()
	if err != nil {
		t.Fatal(err)
	}
	if !status.Locked {
		t.Errorf("status = %+v", status)
	}
}

func TestTuneRejects(t *testing.T) {
	demod, _ := newTestDemod(newFakeChip())
	for _, frequency := range []uint32{0, 44000000, 870000000} {
		if err := demod.Tune(frequency); err == nil {
			t.Errorf("Tune(%d) accepted", frequency)
		}
	}
}

func TestComputePLLParameters(t *testing.T) {
	// Worked by hand from dib8090_compute_pll_parameters: at 473.142857MHz
	// the rests for ratios 17..20 are 356, 164, 740 and 1756 (folded from
	// 2084 past half of 3840); 19 and 20 clear the 717 threshold and 20's
	// is the larger.
	if got := dib8090ComputePLLParameters(473142857); got != 20 {
		t.Errorf("ratio = %d, want 20", got)
	}
	for _, frequency := range []uint32{213142857, 557142857, 767142857} {
		got := dib8090ComputePLLParameters(frequency)
		if got < 17 || got > 20 {
			t.Errorf("ratio at %d = %d", frequency, got)
		}
	}
}

func TestSNRArithmetic(t *testing.T) {
	chip := newFakeChip()
	demod, _ := newTestDemod(chip)
	chip.registers[542] = 1 << 6
	chip.registers[543] = 100 << 6
	if got := demod.demod.getSNR(); got != 100<<16 {
		t.Errorf("SNR = %#x, want 100<<16", got)
	}
	chip.registers[542] = 0
	if got := demod.demod.getSNR(); got != 0xffffffff {
		t.Errorf("SNR with no noise = %#x", got)
	}
}

func TestInterpolateStrength(t *testing.T) {
	tests := []struct {
		value uint32
		want  int32
	}{
		{60000, 108500},
		{55953, 108500},
		{50117, 96000},
		{50000, 95486}, // between 50117 and 49889: 95000 + 111*1000/228
		{10000, 19043}, // Linux interpolates towards its {0, 0} sentinel
	}
	for _, test := range tests {
		if got := int32(interpolateValue(test.value, strengthToDBTable)); got != test.want {
			t.Errorf("interpolateValue(%d) = %d, want %d", test.value, got, test.want)
		}
	}
}
