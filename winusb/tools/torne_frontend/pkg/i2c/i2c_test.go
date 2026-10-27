// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

package i2c

import "testing"

type recordingBus struct{ events []string }

func (bus *recordingBus) Write(address byte, data []byte) error {
	bus.events = append(bus.events, "write")
	return nil
}

func (bus *recordingBus) Read(address byte, length int) ([]byte, error) {
	bus.events = append(bus.events, "read")
	return make([]byte, length), nil
}

type recordingGate struct{ bus *recordingBus }

func (gate recordingGate) OpenTunerGate(open bool) error {
	if open {
		gate.bus.events = append(gate.bus.events, "open")
	} else {
		gate.bus.events = append(gate.bus.events, "close")
	}
	return nil
}

func TestGatedWrapsTransfers(t *testing.T) {
	t.Parallel()
	inner := &recordingBus{}
	bus := Gated(inner, recordingGate{bus: inner})
	if err := bus.Write(0x60, []byte{1}); err != nil {
		t.Fatal(err)
	}
	if _, err := bus.Read(0x60, 2); err != nil {
		t.Fatal(err)
	}
	want := []string{"open", "write", "close", "open", "read", "close"}
	if len(inner.events) != len(want) {
		t.Fatalf("events = %v; want %v", inner.events, want)
	}
	for index, event := range want {
		if inner.events[index] != event {
			t.Fatalf("events = %v; want %v", inner.events, want)
		}
	}
}

func TestMask(t *testing.T) {
	t.Parallel()
	tests := []struct {
		current, value, mask, want byte
	}{
		{0xf0, 0x0a, 0x0f, 0xfa},
		{0xf0, 0x0a, 0x00, 0x0a}, // no mask means a plain write
		{0xf0, 0x0a, 0xff, 0x0a},
	}
	for _, test := range tests {
		if got := Mask(test.current, test.value, test.mask); got != test.want {
			t.Errorf("Mask(%#02x, %#02x, %#02x) = %#02x; want %#02x", test.current, test.value, test.mask, got, test.want)
		}
	}
}
