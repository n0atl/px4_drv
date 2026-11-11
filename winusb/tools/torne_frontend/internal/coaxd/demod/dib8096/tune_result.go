// SPDX-License-Identifier: GPL-2.0-only
package dib8096

import "strings"

// SearchFailed reports only a completed, explicitly failed search. Call after
// Tune returns nil; an unlocked status while acquisition is pending is not failure.
// This exposes the existing upstream state without changing its tuning sequence.
func (board *Demod) SearchFailed() bool {
	return board.failure.err == nil && board.demod.status == statusTuneFailed && board.demod.tuneState == demodStop
}

// TuneTimedOut reports whether err from Tune is only setFrontend's deadline,
// with every bus transfer and wait successful. A retried search can reach it
// part way through synchronization; adapters report it like a failed search
// instead of a transport error, which would discard the whole bridge session.
func (board *Demod) TuneTimedOut(err error) bool {
	return err != nil && board.failure.err == nil && strings.HasPrefix(err.Error(), "DiB8000: tune did not finish")
}

// emptyNoiseLimit は、探索失敗後のr526がこの値未満でr533が0なら信号が無いとみなす境界。
// ブースターなしの受信環境で全UHFを走査した実測では、地デジの信号が無い27チャンネルすべてで
// r526=19、r533=0だった。遠方の弱い局や同期できなかった局を含め、信号があればr526は100以上だった。
// 入力電力 (strength) は空きチャンネルと遠方局で重なり、空きでも-53dBmの例があったため使わない。
const emptyNoiseLimit = 50

// searchFoundNothing は探索に失敗した直後のr526とr533を記録し、地デジの信号がまったく無いかを返す。
// 信号が無いチャンネルは何度探索しても見つからないため、再探索を省いてチャンネルスキャンを短くする。
// 少しでも信号の痕跡があれば従来どおり再探索する。読み出しの失敗はfailureへ残り、呼び出し側が扱う。
// 省くのはskipEmptyが有効で、起動・復帰直後の選局でない場合だけ。判断自体は常に記録する。
func (board *Demod) searchFoundNothing() bool {
	demod := board.demod
	noise := demod.readWord(526)
	ber := demod.readWord(533)
	t := &demod.timing
	if t.failures < int64(len(t.failNoise)) {
		t.failNoise[t.failures] = noise
		t.failBER[t.failures] = ber
	}
	t.failures++
	t.noSignal = board.failure.err == nil && noise < emptyNoiseLimit && ber == 0
	t.skipped = t.noSignal && board.skipEmpty && !t.fresh
	return t.skipped
}
