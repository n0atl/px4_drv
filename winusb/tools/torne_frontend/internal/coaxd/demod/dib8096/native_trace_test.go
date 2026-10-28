// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
package dib8096

import (
	"encoding/binary"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"testing"
	"time"
)

// Optional trace oracle for the C++ port. The existing Go fakeChip and fakeClock
// remain independent of the native implementation; no USB device is opened.
type nativeTraceBus struct {
	chip             *fakeChip
	clock            *fakeClock
	file             *os.File
	count            int
	failIO, failWait bool
}

func (b *nativeTraceBus) Write(a byte, w []byte) error {
	b.count++
	fmt.Fprintf(b.file, "W %d %x\n", a, w)
	if b.failIO && b.count == 12 {
		fmt.Fprintln(b.file, "IOFAIL")
		return fmt.Errorf("injected bus failure")
	}
	return b.chip.Write(a, w)
}
func (b *nativeTraceBus) Read(a byte, n int) ([]byte, error) {
	return nil, fmt.Errorf("unexpected bare read")
}
func (b *nativeTraceBus) WriteRead(a byte, w []byte, n int) ([]byte, error) {
	b.count++
	if b.failIO && b.count == 12 {
		fmt.Fprintf(b.file, "R %d %x IOFAIL\n", a, w)
		return nil, fmt.Errorf("injected bus failure")
	}
	r, e := b.chip.WriteRead(a, w, n)
	fmt.Fprintf(b.file, "R %d %x %x\n", a, w, r)
	return r, e
}
func TestNativePortTrace(t *testing.T) {
	dir := os.Getenv("TORNE_NATIVE_TRACE_DIR")
	if dir == "" {
		t.Skip("native comparison is opt-in")
	}
	if err := os.MkdirAll(dir, 0755); err != nil {
		t.Fatal(err)
	}
	for _, mode := range []string{"locked", "nosignal", "timeout", "unknown", "iofail", "waitfail", "cband", "remembered", "capremembered"} {
		t.Run(mode, func(t *testing.T) {
			if (mode == "remembered" || mode == "capremembered") && os.Getenv("TORNE_TEST_CHANNEL_MEMORY") != "1" {
				t.Skip("memory tests are opt-in")
			}
			if mode == "capremembered" && os.Getenv("TORNE_TEST_CAPTRIM_REUSE") != "1" {
				t.Skip("captrim test is opt-in")
			}
			chip := newFakeChip()
			if mode == "locked" || mode == "cband" || mode == "remembered" || mode == "capremembered" {
				chip = lockedChip()
			}
			if mode == "unknown" {
				chip.registers[897] = 0x7000
			}
			if mode == "capremembered" {
				chip.tunerRegisters[0x1a] = 0x0a23
			}
			clock := &fakeClock{now: time.Unix(1000, 0)}
			file, err := os.Create(filepath.Join(dir, mode+".go.trace"))
			if err != nil {
				t.Fatal(err)
			}
			defer file.Close()
			bus := &nativeTraceBus{chip: chip, clock: clock, file: file, failIO: mode == "iofail", failWait: mode == "waitfail"}
			board := New(bus)
			board.reuseTemperature = os.Getenv("TORNE_TEST_FAST_TUNING") == "1"
			board.tuner.fastDC = board.reuseTemperature
			board.demod.fastPoll = board.reuseTemperature
			board.demod.earlyData = board.reuseTemperature
			board.useMemory = os.Getenv("TORNE_TEST_CHANNEL_MEMORY") == "1"
			board.tuner.reuseCaptrim = os.Getenv("TORNE_TEST_CAPTRIM_REUSE") == "1"
			board.setTiming(clock.Now, func(d time.Duration) {
				fmt.Fprintf(file, "S %d\n", d)
				if bus.failWait {
					board.failure.record(fmt.Errorf("injected wait failure"))
					return
				}
				clock.Sleep(d)
			}, 5*time.Second)
			run := func(kind byte, hz uint32) {
				var e error
				var body []byte
				switch kind {
				case 0x10:
					e = board.Initialize()
					if e == nil {
						e = board.SetGPIO(1, true)
					}
				case 0x11:
					e = board.Tune(hz)
					if e == nil && board.SearchFailed() {
						body = []byte{1}
					}
				case 0x12:
					e = board.SetOutputMode(OutputHighZ)
				case 0x13:
					var status Status
					status, e = board.Status()
					if e == nil {
						body = make([]byte, 24)
						if status.Locked {
							body[0] = 1
						}
						if status.HasCNR {
							body[1] = 1
						}
						if status.HasStrength {
							body[2] = 1
						}
						binary.LittleEndian.PutUint64(body[8:], math.Float64bits(status.CNR))
						binary.LittleEndian.PutUint64(body[16:], math.Float64bits(status.StrengthDBm))
					}
				case 0x14:
					e = board.SetGPIO(1, false)
					if e == nil {
						e = board.Sleep()
					}
				}
				ok := 1
				if e != nil {
					ok = 0
				}
				fmt.Fprintf(file, "C %d %d %x\n", kind, ok, body)
			}
			run(0x10, 0)
			if mode == "unknown" || mode == "iofail" || mode == "waitfail" {
				return
			}
			if mode == "nosignal" {
				chip.queues[1284] = []uint16{1, 0, 1}
			}
			frequency := uint32(473142857)
			if mode == "cband" {
				frequency = 93142857
			}
			run(0x11, frequency)
			run(0x13, 0)
			run(0x12, 0)
			run(0x14, 0)
			if mode == "remembered" || mode == "capremembered" {
				chip.queues[1284] = []uint16{0}
				run(0x11, frequency)
				run(0x13, 0)
				run(0x14, 0)
			}
			if mode == "locked" {
				chip.queues[1284] = []uint16{1, 0, 2}
				run(0x11, 213142857)
				run(0x13, 0)
				run(0x14, 0)
			}
		})
	}
}
