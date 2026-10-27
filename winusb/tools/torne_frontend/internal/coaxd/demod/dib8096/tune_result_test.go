// SPDX-License-Identifier: GPL-2.0-only
package dib8096

import "testing"

func TestSearchFailedOnlyWhenTerminal(t *testing.T) {
	chip := newFakeChip()
	demod, _ := newTestDemod(chip)
	if demod.SearchFailed() { t.Fatal("new frontend is not a completed failed search") }
	if err := demod.Initialize(); err != nil { t.Fatal(err) }
	chip.queues[1284] = []uint16{0x1, 0x0, 0x1}
	if err := demod.Tune(557142857); err != nil { t.Fatal(err) }
	if !demod.SearchFailed() { t.Fatalf("failed search not reported: status=%d state=%d", demod.demod.status, demod.demod.tuneState) }
	for _, status := range []int32{statusTunePending, statusFFTSuccess, statusDemodSuccess, statusLocked, statusDataLocked} {
		demod.demod.status = status
		if demod.SearchFailed() { t.Fatalf("pending/locked status %d treated as failure", status) }
	}
	demod.demod.status = statusTuneFailed
	demod.demod.tuneState = demodStart
	if demod.SearchFailed() { t.Fatal("failure value before search completion treated as terminal") }
}
