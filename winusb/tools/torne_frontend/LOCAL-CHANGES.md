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

## Acquisition stability (from aki0429/coax)

Ported from aki0429/coax `docs/boards/cech-zd1j-stability.md` (same upstream base
`2d9f2c263a3b080ecef3a8ede1eb86c6afa48405`):

- `minTuneStep` is 100ms, the Linux floor, instead of 10ms. Starting the guard search too soon
  after the FFT search made some weak and strong channels fail with `no_lock`.
- `Tune` (the non-memory path) retries a failed or unlocked acquisition up to three times, 100ms
  apart, clearing the channel property cache before each attempt. All attempts share one absolute
  deadline (`tuneTimeout`); no new search starts with less than one second left. Bus errors and the
  state-machine timeout are returned as errors and are not retried.

The same change is applied by hand to `dib8096_native_generated.inc`; regenerate with
`tests/check_torne_native.py` once Go is available to confirm the generator output matches.

## Fast tuning for CECHZD1J

Fast tuning is not available in the native host. Remembered parameters and captrim reuse are always
enabled; the former `RememberChannelParameters` and `ReuseCaptrim` keys were removed. The former `FastTuning`
key was removed and is ignored if present: its features fail more searches on weak and very strong
channels, and remembered parameters with captrim reuse already bring a repeated tune to 1.2-1.4s. The 3ms
polling of fast mode is shorter than the 10ms step that already failed on some channels, so the
default follows the acquisition path validated on hardware by aki0429/coax. The Go helper remains a
comparison reference with fast features disabled unless a test enables them. No denpa code was copied.
The original acquisition path remains available internally for reference tests. Bon buffers remain 1024 packets.

Successful transmission parameters are stored per device/session in a bounded 64-slot exact-frequency
table. Initialization clears the tables. Remembered parameters that fail or do not lock invalidate their
entry and fall back at once to normal acquisition, which is retried like `Tune` (up to three searches,
100ms apart, within the remaining deadline); transport errors do not cause a second search.
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
and TORNE_TEST_FAST_TUNING (Go reference only); production code does not read these variables.

## Tune diagnostics

`setFrontend` adds up, per `Tune`, the number of acquisitions and the time spent in the tuner
(including captrim), the AGC startup, the FFT/guard autosearch and the synchronization after it. The
native host logs them as `torne: tune phases ...`. The clock is only read; no wait or register access
changes.

## Empty-channel search

After a failed full search, `searchFoundNothing` reads r526 and r533. When r526 is below 50 and r533 is 0,
no ISDB-T signal is present and the remaining retries are skipped (both `Tune` and the remembered fallback).
On a full UHF scan without a booster, all 27 empty channels read r526=19 and r533=0, while every channel
with any signal (including distant stations that never decode) read r526 of 100 or more. Signal strength
overlaps between empty channels and distant stations and is not used. The values of each failure are kept
for the host log (`torne: search failures ...`).

The skip is enabled by `SkipEmptyChannels=true` (`false` by default, which always searches up to three
times, because the check also applies to tunes for recordings and was measured in one environment only) and never applies to the first tune after `Initialize` or `Sleep`. On that tune a receivable distant
station (-87dBm, C/N 10dB on every later tune) once read r526=19 and -109dBm, as if no signal were present.
