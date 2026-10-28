// SPDX-License-Identifier: GPL-2.0-only
// Opt-in transmission memory; lock criteria and calibration remain unchanged.
package dib8096

import "fmt"

func (board *Demod) tuneRemembered(hz uint32) (err error) {
	defer board.flush(&err)
	board.failure.err = nil
	board.memoryLastUsed = false
	board.memoryFallback = false
	if hz < minimumFrequencyHz || hz > maximumFrequencyHz {
		return fmt.Errorf("DiB8096: invalid memory frequency %d", hz)
	}
	slot := int((hz / 6000000) % 64)
	saved := board.memory[slot]
	known := board.memoryValid[slot] && saved.frequency == hz && saved.bandwidthHz == 6000000 && isManualMode(&saved)
	if board.asleep {
		board.demod.wakeup()
		board.tuner.wakeup()
		board.asleep = false
	}
	board.channel.clear()
	board.channel.frequency = hz
	if known {
		board.channel = saved
		board.memoryLastUsed = true
	}
	budget := board.demod.tuneTimeout
	deadline := board.demod.now().Add(budget)
	err = board.demod.setFrontend()
	if err != nil {
		board.memoryValid[slot] = false
		return err
	}
	if known && board.SearchFailed() {
		board.memoryValid[slot] = false
		board.memoryFallback = true
		remaining := deadline.Sub(board.demod.now())
		if remaining <= 0 {
			return fmt.Errorf("DiB8096: remembered tune deadline expired")
		}
		board.demod.tuneTimeout = remaining
		board.channel.clear()
		board.channel.frequency = hz
		err = board.demod.setFrontend()
		board.demod.tuneTimeout = budget
		if err != nil {
			return err
		}
	}
	if board.failure.err != nil {
		board.memoryValid[slot] = false
		return board.failure.err
	}
	candidate := board.channel
	if board.demod.status == statusDataLocked && isManualMode(&candidate) {
		board.memory[slot] = candidate
		board.memoryValid[slot] = true
	} else {
		board.memoryValid[slot] = false
	}
	return nil
}
