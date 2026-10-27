# Local adapter changes

The normal DriverHost now uses a generated C++ port (`dib8096_native_generated.inc`)
and a native bus adapter. The Go executable remains a diagnostic reference only.
`port_native.go.txt` is the development-only AST/type-aware porting tool;
`tests/check_torne_native.py` compares all mock I2C/wait/result events with the
independent Go fakeChip/fakeClock fixtures in `native_trace_test.go`.
The native generated sources retain the same GPL-2.0-only copyright notices.

Upstream source is pinned by `upstream.json`; original demodulator timing and search code are retained.
The local `internal/coaxd/demod/dib8096/tune_result.go` adds `SearchFailed()` to expose an existing
terminal state (`statusTuneFailed` and `demodStop`) after `Tune` returns without an I/O error.
It does not implement a signal-strength threshold or shorten acquisition timeouts.

The stdio adapter reports this result as a successful `0x11` response with body `01`.
An empty body retains its legacy meaning. Transport errors remain separate error responses.
Tests exercise no-signal search completion and exclusion of pending/locked states.

The additional files use GPL-2.0-only. Retain upstream copyright and license notices and include
these adapter sources and build instructions with corresponding-source distribution.
