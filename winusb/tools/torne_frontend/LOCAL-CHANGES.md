# Local adapter changes

The normal DriverHost now uses a generated C++ port (`dib8096_native_generated.inc`)
and a native bus adapter. The Go executable remains a diagnostic reference only.
`port_native.go.txt` is the development-only AST/type-aware porting tool;
`tests/check_torne_native.py` compares all mock I2C/wait/result events with the
independent Go fakeChip/fakeClock fixtures in `native_trace_test.go`.
The native generated sources retain the same GPL-2.0-only copyright notices.

Upstream source is pinned by `upstream.json`; the default path retains the original timing and search. Optional changes are described below.
The local `internal/coaxd/demod/dib8096/tune_result.go` adds `SearchFailed()` to expose an existing
terminal state (`statusTuneFailed` and `demodStop`) after `Tune` returns without an I/O error.
It does not implement a signal-strength threshold or shorten acquisition timeouts.

The stdio adapter reports this result as a successful `0x11` response with body `01`.
An empty body retains its legacy meaning. Transport errors remain separate error responses.
Tests exercise no-signal search completion and exclusion of pending/locked states.

The additional files use GPL-2.0-only. Retain upstream copyright and license notices and include
these adapter sources and build instructions with corresponding-source distribution.

## Fast tuning for CECHZD1J

Fast tuning is always enabled in the native host; no FastTuning, RememberChannelParameters or ReuseCaptrim INI keys are read. The Go helper remains a
comparison reference with fast features disabled unless a test enables them. No denpa code was copied.
The original acquisition path remains available internally for reference tests. Bon buffers remain 1024 packets.

Successful transmission parameters are stored per device/session in a bounded 64-slot exact-frequency
table. Initialization clears the tables. A failed remembered search invalidates its entry and retries
normal acquisition within the remaining deadline; transport errors do not cause a second search.
Software captrim reuse applies only to UHF/non-P1G and checks a new ADC sample against the saved
minimum plus 10. A mismatch runs the original search.

Fast mode reuses temperature compensation for at most 10 seconds on the same UHF frontend,
shortens DC calibration waits to 5ms units, and polls eligible demod states at 3ms minimum.
AGC/PLL and synchronization states 4-8 retain their existing wait boundaries. Data output may start
when the carrier and any MPEG layer lock; startup error packets can therefore reach the application.
Healthy frontend state survives Bon close only while the normal host process lives. Host idle exit
is unchanged; removal and termination release the retained session. No disk persistence is added.

Ring I2C batching, preferred-8K assumptions, adjacent-frequency captrim hints and research environment
variables are excluded. Comparison tests use TORNE_TEST_CHANNEL_MEMORY, TORNE_TEST_CAPTRIM_REUSE
and TORNE_TEST_FAST_TUNING; production code does not read these variables.
