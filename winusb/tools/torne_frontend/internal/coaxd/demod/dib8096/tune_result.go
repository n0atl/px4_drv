// SPDX-License-Identifier: GPL-2.0-only
package dib8096

// SearchFailed reports only a completed, explicitly failed search. Call after
// Tune returns nil; an unlocked status while acquisition is pending is not failure.
// This exposes the existing upstream state without changing its tuning sequence.
func (board *Demod) SearchFailed() bool {
	return board.failure.err == nil && board.demod.status == statusTuneFailed && board.demod.tuneState == demodStop
}
