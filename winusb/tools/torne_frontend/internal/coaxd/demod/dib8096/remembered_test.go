// SPDX-License-Identifier: GPL-2.0-only
package dib8096

import (
	"errors"
	"testing"
	"time"
)

func TestRememberedTuning(t *testing.T) {
	chip := lockedChip()
	board, clock := newTestDemod(chip)
	board.useMemory = true
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	const hz = uint32(641143000)
	if err := board.Tune(hz); err != nil {
		t.Fatal(err)
	}
	if board.memoryLastUsed {
		t.Fatal("first tune used memory")
	}
	slot := int((hz / 6000000) % 64)
	if !board.memoryValid[slot] {
		t.Fatal("successful transmission not retained")
	}
	chip.queues[1284] = []uint16{0} // remembered tune must not enter autosearch
	begin := clock.now
	if err := board.Tune(hz); err != nil {
		t.Fatal(err)
	}
	if !board.memoryLastUsed || board.demod.status != statusDataLocked {
		t.Fatal("remembered tune not locked")
	}
	t.Logf("remembered simulated waits=%v", clock.now.Sub(begin))
	// No lock: invalidate remembered data and permit ordinary search only inside remaining budget.
	chip.registers[568] = 0
	chip.queues[1284] = []uint16{1, 0, 1}
	begin = clock.now
	if err := board.Tune(hz); err != nil {
		t.Fatal(err)
	}
	if !board.memoryFallback || board.memoryValid[slot] || !board.SearchFailed() {
		t.Fatal("failed memory not discarded")
	}
	if clock.now.Sub(begin) > 5*time.Second {
		t.Fatal("fallback extended total budget")
	}
	// Automatic recovery on the next explicit tune.
	chip.registers[568] = 0x3ffe
	chip.queues[1284] = []uint16{1, 0, 2}
	if err := board.Tune(hz); err != nil {
		t.Fatal(err)
	}
	if board.memoryLastUsed || !board.memoryValid[slot] {
		t.Fatal("automatic recovery failed")
	}
	// A fresh initialization must discard transmission memory.
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	if board.memoryValid[slot] || board.memoryLastUsed || board.memoryFallback || board.tuner.capLastReused {
		t.Fatal("initialization kept old tuning state")
	}
}

type memoryFaultBus struct {
	*fakeChip
	fail bool
}

func (bus *memoryFaultBus) Write(a byte, w []byte) error {
	if bus.fail {
		return errors.New("memory experiment transport fault")
	}
	return bus.fakeChip.Write(a, w)
}
func (bus *memoryFaultBus) WriteRead(a byte, w []byte, n int) ([]byte, error) {
	if bus.fail {
		return nil, errors.New("memory experiment transport fault")
	}
	return bus.fakeChip.WriteRead(a, w, n)
}
func TestMemoryTransportErrorDoesNotRetry(t *testing.T) {
	chip := lockedChip()
	bus := &memoryFaultBus{fakeChip: chip}
	board := New(bus)
	clock := &fakeClock{now: time.Unix(1000, 0)}
	board.setTiming(clock.Now, clock.Sleep, 5*time.Second)
	board.useMemory = true
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	const hz = uint32(641143000)
	if err := board.Tune(hz); err != nil {
		t.Fatal(err)
	}
	bus.fail = true
	if err := board.Tune(hz); err == nil {
		t.Fatal("transport fault hidden")
	}
	if board.memoryFallback {
		t.Fatal("transport fault retried as search failure")
	}
	slot := int((hz / 6000000) % 64)
	if board.memoryValid[slot] {
		t.Fatal("failed transport kept known channel")
	}
}
func TestMemorySlotCollision(t *testing.T) {
	chip := lockedChip()
	board, _ := newTestDemod(chip)
	board.useMemory = true
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	for _, hz := range []uint32{641143000, 257143000, 641143000} {
		chip.queues[1284] = []uint16{1, 0, 2}
		if err := board.Tune(hz); err != nil {
			t.Fatal(err)
		}
		if board.memoryLastUsed {
			t.Fatal("collision reused another frequency")
		}
	}
}

func TestCaptrimReuseAndMismatch(t *testing.T) {
	chip := lockedChip()
	board, _ := newTestDemod(chip)
	board.tuner.reuseCaptrim = true
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	board.tuner.identity.p1g = 0
	for _, hz := range []uint32{641143000, 647143000, 641143000} {
		chip.queues[1284] = []uint16{1, 0, 2}
		if err := board.Tune(hz); err != nil {
			t.Fatal(err)
		}
	}
	if !board.tuner.capLastReused {
		t.Fatal("verified capacitor not reused")
	}
	chip.tunerRegisters[0x1d] = 1023
	chip.queues[1284] = []uint16{1, 0, 2}
	if err := board.Tune(641143000); err != nil {
		t.Fatal(err)
	}
	if board.tuner.capLastReused {
		t.Fatal("ADC mismatch accepted")
	}
	if board.demod.status != statusDataLocked {
		t.Fatal("full recalibration did not finish")
	}
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	for _, entry := range board.tuner.capMemory {
		if entry.valid {
			t.Fatal("reset kept capacitor memory")
		}
	}
}
