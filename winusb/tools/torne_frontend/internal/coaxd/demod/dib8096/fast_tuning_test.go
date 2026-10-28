// SPDX-License-Identifier: GPL-2.0-only
package dib8096

import (
	"testing"
	"time"
)

func TestFastDCReadWait(t *testing.T) {
	for _, fast := range []bool{false, true} {
		b := New(newFakeChip())
		if err := b.Initialize(); err != nil {
			t.Fatal(err)
		}
		s := b.tuner
		s.fastDC = fast
		for _, step := range []int{tunerStep2, tunerStep3} {
			s.tuneState = step
			got := s.getOffset()
			want := 100
			if fast {
				want = 50
			}
			if got != want {
				t.Fatalf("fast=%v got=%d want=%d", fast, got, want)
			}
		}
	}
}

func TestEarlyDataRequiresCarrierAndLayer(t *testing.T) {
	for _, fast := range []bool{false, true} {
		for _, locks := range []uint16{0, 0x1000, 0x0040, 0x1040, 0x1080} {
			chip := newFakeChip()
			b, _ := newTestDemod(chip)
			s := b.demod
			if err := b.Initialize(); err != nil {
				t.Fatal(err)
			}
			s.earlyData = fast
			s.tuneState = demodStep10
			s.longestIntlvLayer = 0
			chip.registers[568] = locks
			// 期限前に判定し、別層だけのロックと搬送波の欠落を区別する。
			s.timeout = s.now().Add(1000000000)
			s.tune()
			want := locks&0x80 != 0 || (fast && locks&0x1000 != 0 && locks&0xe0 != 0)
			if (s.status == statusDataLocked) != want {
				t.Fatalf("fast=%v locks=%x status=%d", fast, locks, s.status)
			}
		}
	}
}

func TestTemperatureReuseBound(t *testing.T) {
	chip := lockedChip()
	board, clock := newTestDemod(chip)
	board.reuseTemperature = true
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	if err := board.Tune(641143000); err != nil {
		t.Fatal(err)
	}
	if !board.temperatureValid || board.temperatureLastReused {
		t.Fatal("first temperature not measured")
	}
	chip.queues[1284] = []uint16{1, 0, 2}
	if err := board.Tune(647143000); err != nil {
		t.Fatal(err)
	}
	if !board.temperatureLastReused {
		t.Fatal("recent temperature not reused")
	}
	clock.now = clock.now.Add(11 * time.Second)
	chip.queues[1284] = []uint16{1, 0, 2}
	if err := board.Tune(641143000); err != nil {
		t.Fatal(err)
	}
	if board.temperatureLastReused {
		t.Fatal("expired temperature reused")
	}
	// Clock rollback must also force a fresh calibration.
	clock.now = clock.now.Add(-time.Minute)
	chip.queues[1284] = []uint16{1, 0, 2}
	if err := board.Tune(647143000); err != nil {
		t.Fatal(err)
	}
	if board.temperatureLastReused {
		t.Fatal("clock rollback accepted")
	}
	if err := board.Initialize(); err != nil {
		t.Fatal(err)
	}
	if board.temperatureValid {
		t.Fatal("reset kept old temperature")
	}
}

func TestFastPollingKeepsFailureAndLockOutcomes(t *testing.T) {
	for _, signal := range []bool{false, true} {
		chip := newFakeChip()
		if signal {
			chip = lockedChip()
		}
		board, clock := newTestDemod(chip)
		board.demod.fastPoll = true
		if err := board.Initialize(); err != nil {
			t.Fatal(err)
		}
		if !signal {
			chip.queues[1284] = []uint16{1, 0, 1}
		}
		begin := clock.now
		if err := board.Tune(641143000); err != nil {
			t.Fatal(err)
		}
		if signal && board.demod.status != statusDataLocked {
			t.Fatal("lock lost")
		}
		if !signal && !board.SearchFailed() {
			t.Fatal("no-signal accepted")
		}
		t.Logf("signal=%v simulated wait=%v", signal, clock.now.Sub(begin))
	}
}
