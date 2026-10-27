// Copyright 2026 Till0196
// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

// Package i2c is the contract between USB bridge chips and the demodulator and
// tuner ICs hanging off their I2C bus. IC drivers depend only on this package,
// so the same demodulator or tuner driver serves every board that carries it.
package i2c

import "time"

// Bus is an I2C master addressed with 8-bit (already shifted) slave addresses.
type Bus interface {
	Write(address byte, data []byte) error
	Read(address byte, length int) ([]byte, error)
}

// WriteReader is implemented by masters that can keep a repeated-start
// transaction atomic. WriteRead falls back to separate Bus operations for
// bridges that do not need special handling.
type WriteReader interface {
	WriteRead(address byte, write []byte, readLength int) ([]byte, error)
}

// Flusher is implemented by masters that hold writes back and send them
// together; Flush sends what is held.
type Flusher interface {
	Flush() error
}

// Flush sends the writes a master holds back, if it holds any back. A driver
// calls it before waiting on the hardware and when it is done with the bus
// for now.
func Flush(bus Bus) error {
	if flusher, ok := bus.(Flusher); ok {
		return flusher.Flush()
	}
	return nil
}

// Modifier is implemented by masters that can change a 16-bit register with a
// two-byte number themselves: the register is read, the bits outside keep are
// cleared, set is or'ed in and the result written back, without the read
// coming back to the caller. Like a write, it may be held back until Flush.
type Modifier interface {
	Modify16(address byte, register, keep, set uint16) error
}

// Idler is implemented by masters with work of their own that can be done
// while a driver waits on the hardware; Idle may use the time up to until.
type Idler interface {
	Idle(until time.Time)
}

// Wait is a driver's wait on the hardware: once what the master holds back has
// been sent, it lasts duration, and a master with work of its own may do that
// work in it.
func Wait(bus Bus, duration time.Duration) error {
	err := Flush(bus)
	until := time.Now().Add(duration)
	if idler, ok := bus.(Idler); ok {
		idler.Idle(until)
	}
	time.Sleep(time.Until(until))
	return err
}

// WriteRead writes to a slave and reads its answer, as one repeated-start
// transaction when the master implements WriteReader, and as separate Write
// and Read calls otherwise.
func WriteRead(bus Bus, address byte, write []byte, readLength int) ([]byte, error) {
	if combined, ok := bus.(WriteReader); ok {
		return combined.WriteRead(address, write, readLength)
	}
	if err := bus.Write(address, write); err != nil {
		return nil, err
	}
	return bus.Read(address, readLength)
}

// Gate opens and closes a demodulator's I2C pass-through to the tuner. Most
// silicon tuners sit behind such a gate rather than on the bridge bus directly.
type Gate interface {
	OpenTunerGate(open bool) error
}

// Gated returns a Bus that opens the gate around every transfer, letting a
// tuner driver stay unaware that it is reached through a demodulator.
func Gated(bus Bus, gate Gate) Bus { return gated{bus: bus, gate: gate} }

type gated struct {
	bus  Bus
	gate Gate
}

func (bus gated) Write(address byte, data []byte) error {
	if err := bus.gate.OpenTunerGate(true); err != nil {
		return err
	}
	defer bus.gate.OpenTunerGate(false)
	return bus.bus.Write(address, data)
}

func (bus gated) Read(address byte, length int) ([]byte, error) {
	if err := bus.gate.OpenTunerGate(true); err != nil {
		return nil, err
	}
	defer bus.gate.OpenTunerGate(false)
	return bus.bus.Read(address, length)
}

// Mask merges value into current under mask, the read-modify-write every
// register-oriented IC driver needs.
func Mask(current, value, mask byte) byte {
	if mask == 0 || mask == 0xff {
		return value
	}
	return value&mask | current&^mask
}
