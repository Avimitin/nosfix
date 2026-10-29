# nosfix

`nosfix.dll` 0.4.0 (development) is a 64-bit Windows hook module for Nostalgia PAN-2025020501. The
project is C++20 and uses MinHook v1.3.3 at pinned commit
`9fbd087432700d73fc571118d6a9697a36443d88`.

The current runtime still has a 60 Hz simulation and independent presentation.
The `clock_motion` path evaluates normal, long, trill and glissando note
geometry from chart time; velocity zones and beat lines remain on the legacy
path. It does **not** yet implement timestamped input judgments, fully
time-based UI animation, or ASIO output. The new work targets an
audio-referenced timeline with QPC input timestamps, independent of display
cadence. See [the integration contracts and remaining work](analyze/NATIVE_TIMING.txt).

Version 0.4 adds a guarded correction to libxact's cue-clock arithmetic,
chart-time note projection sampled from that clock, and a startup
FPS-calibration bypass. It also includes an optional HID/keyboard report
observer. A `SongClock`/`ShadowClock` publication is tested and, with
`native_shadow=true`, connected to the raw input observer, but it does not feed
judgments. The experimental chart-time path is disabled in the distributed
configuration by default. The merged native match/scoring, bucket-input,
timed-UI and XAudio2/ASIO components are compiled and tested but remain
dormant; no live hook reaches them.

## Build

Requirements: Visual Studio 2022 C++ x64 tools, a Windows 10/11 SDK, CMake
3.24 or newer, and Git. With an installed SDK, from an x64 Visual Studio
developer shell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

`scripts/build.ps1` uses the SDK files under `.tools/sdk` when the machine has
no system SDK installation. Pass `-RunTests` to run scheduler, geometry,
synthetic game-hook, plugin-load, real-image hook installation/rollback, and
call-patch tests.

The DLL and adjacent `nosfix.ini` are written to `build/Release/` with the
Visual Studio generator or `build/` with the local NMake script. CMake fetches
the pinned MinHook source at configure time.

## Runtime behavior

Spice calls the exported `spice_sdk_entry_point` synchronously, outside the
loader lock and before the game creates its D3D device. `DllMain` does no work;
CRT thread notifications remain enabled for the static runtime. Configuration loading, logging, version checks, and
hook setup run from that SDK entry point. The adjacent `nosfix.ini` enables
hooks by default and uses `target_hz=auto` to select the primary display's
current refresh rate. Initialization verifies the loaded x64 PE and the
on-disk `nostalgia.dll` file size and SHA-256 before any hook code can proceed.
UI smoothing independently verifies the installed `afp-core.dll` size and SHA-256.
`precise_audio_clock=true` also verifies `libxact.dll` and replaces only the
cue-clock arithmetic: subtract 64-bit QPC timestamps first, calculate in
double precision, and convert to float at the existing ABI boundary. Cue
selection and the existing audio-clock fallback remain unchanged. This
correction alone does not remove the game's frame-based judgments.
`skip_monitor_check=true` selects the game's existing FPS-check completion
path at startup and fixes the three legacy chart conversion constants at 60.
This removes the calibration wait, not hardware/network checks. Those units
are temporary compatibility for frame-based consumers: integer judgments
still have roughly 16.67 ms granularity. The native judgment migration will
use retained millisecond chart times and timestamped input instead.
The entry-point ABI follows the [Spice SDK header](https://raw.githubusercontent.com/spice2x/spice2x.github.io/main/src/spice2x/sdk/include/spicesdk.h)
and its [SDK initialization path](https://raw.githubusercontent.com/spice2x/spice2x.github.io/main/src/spice2x/sdk/sdk.cpp).

## Launch

Copy the built `nosfix.dll` and its adjacent `nosfix.ini` beside `spice64.exe`,
then add `-k nosfix.dll` to the existing
`spice64.exe` launch command. Set `target_hz=auto` to use the primary display's
current rate, or set `target_hz` to 60, 90, 120, 144, 165, 240, or 360.
Keep `smooth_motion=true` for the bounded prediction pass. Set it to `false`
for a comparison at the same presentation rate with the original geometry.
`smooth_ui=true` enables supported AFP UI translations; set it to `false`
to compare menus with their original motion. Both switches are read on startup.
At 60 Hz, prediction passes through without capturing or predicting geometry.
`clock_motion=true` enables the experimental chart-time projection for normal,
long, trill and glissando notes at any presentation rate and requires
`precise_audio_clock=true`. Velocity zones and beat lines keep the legacy path.
During its live trial, `smooth_motion=false` and `smooth_ui=false` isolate it.
`native_shadow=true` publishes that corrected cue time as an immutable
song-clock snapshot for the raw input observer. It requires
`precise_audio_clock=true` and `capture_input=true`; it maps captured QPCs and
logs mapped, unmapped and backward-mapping counters every five seconds. It
does not feed judgments, scoring, rendering, or sound. `diagnostic_probe=true`
emits read-only `PROBE` lines for the normal judge and gliss/velocity note
updates; it is a diagnostic build aid, not a gameplay path. See
[the shadow integration notes](analyze/SHADOW_INTEGRATION.md).

Hook sets are supplied as explicit module-relative RVAs and exact expected
instruction bytes. The installer validates the complete set before creating
detours and applies queued hooks together. A failure rolls back the hook set
and the Direct3D import change. The render routine has one entry detour; two
specific CALL instructions in its dispatch tail are redirected through nearby
stubs. Note motion adds six virtual note-update hooks, an AFP draw callback
hook, and a BM2D buffer-release hook to the same transaction.
UI motion adds one verified call in `afp-core.dll` to the call-patch transaction.
The render-tail helper functions retain their original entry points and
register behavior for all other callers. No process-wide API hooks are installed.

Logs are appended to `nosfix.log` beside the DLL. The module does not launch,
restart, or modify the original game installation.

## Note motion

With `clock_motion=true`, normal, long, trill and glissando notes use their
chart timestamps, retaining integer milliseconds before conversion to legacy
frame units. Each presentation queries the corrected cue clock once, applies
the existing song/visual offsets, and evaluates the game's native projection
with a float distance argument. It rebuilds private quads and ribbons from the
resulting center, scale and opacity. Previous rendered positions do not
advance this clock. The original projector still rounds centers to integer
pixels; scaled extents remain fractional intentionally. Source texture
dimensions retain their original integer division, including height/2.
Glissando uses its 104-byte record's own chart-ms/frame pair and link-frame
distance; velocity zones and beat lines still fall back to the original path.
Spawn/removal logic, judgments and UI timelines still use legacy updates.
Pause/retry and every scroll mode have not yet been validated live.

The following describes the separate `smooth_motion` prediction path.

The original loop runs at fixed 60 Hz simulation boundaries. Above 60 Hz,
the motion pass captures geometry after the original update of normal notes,
long notes, beat lines, glissando notes, trill notes, and velocity-zone notes.
It predicts fractional vertex positions from the latest two consecutive
snapshots using the absolute QPC simulation phase. Every presentation starts
from the latest game snapshot; predicted positions are never fed back into
the game or accumulated. It adds no interpolation frame of latency.

Only a private copy of XY vertex coordinates is changed at the AFP draw
callback. Note fields, source buffers, colors, texture coordinates, and
judgment positions remain on the original paths. Both textured heads and
filled ribbons are covered. The downstream draw calls consume the private
copy synchronously, as specified by Microsoft's
[DrawPrimitiveUP contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-drawprimitiveup).

Prediction is limited to one simulation step. New notes, missing updates,
music-frame holds/seeks/double updates, hold-state changes, buffer reuse,
changed textures/UVs, changed ribbon segment counts, and invalid or unusually
large movement fall back to the latest original geometry. The game buffer
must still match the captured bytes at draw time. Allocations happen during
capture; drawing reuses owned buffers.

This is bounded linear prediction, not an exact continuous-time rewrite of
the game's nonlinear projection. Curved motion, integer position rounding,
topology changes, and state transitions may still show small corrections or
60 Hz steps. AFP menu timelines, hit effects, colors, input polling, and
judgment processing remain at their original cadence.

## UI motion

The UI pass tracks individual AFP display nodes during draw traversal. It
predicts their local translation from the latest two consecutive simulation
ticks, using the same QPC phase as note motion. The original matrix composition
receives an aligned private copy of the node's transform fields, so parent and
child transforms retain their normal composition order. AFP node memory,
timeline advancement, callbacks, and input hit testing are unchanged.

This first UI pass covers translation in regular affine transform mode.
Rotation, scale, fades, pivot modes, clipping bounds, and animation frame changes
retain their original cadence. It is not a full AFP tween-engine replacement;
moving clipping edges can still step. Reused nodes, changed resources/parents,
missing ticks, multiple distinct samples in one tick, invalid matrices, and
large jumps reset prediction. Histories are bounded and owned by the render
thread. See [the verified AFP contracts](analyze/AFP_UI_TRANSFORM.md).

## Input observation

`capture_input=true` observes HID/keyboard reports on Spice's dedicated input
thread. It requires the verified test version of `spice64.exe` and a unique
registered `SpiceTools Input` window in the same process. Thread-specific
Windows message hooks copy reports and chain to Spice without altering its
registration, messages, or current input state. Capture failure disables this
optional observer and records the reason in the log.
Observation begins at the first game-loop render, after initial loading, so
reports generated before the diagnostic consumer exists do not fill the queue.

Packets retain QPC dequeue time, source RAWINPUT header, report order, sequence
and stream epoch. Batched HID reports share one timestamp; this is not a device
timestamp or proof of physical switch latency. A bounded queue holds up to 8191
input packets plus a reserved reset record. Overflow, malformed/oversize data,
read failure and device changes produce ordered reset boundaries and counters.
Queue operations do not allocate or lock; the callback does not log or invoke
game/audio code. Storage is about 17 MB, allocated once before capture starts.

This pass is diagnostic. The game thread drains copies and logs only aggregate
packet counts, errors and maximum time until consumption; raw keys/reports are
not written to disk. Consuming them on a legacy tick does not change their
capture timestamps, but judgment has not yet been connected to them. The
descriptor decoder, binding adapter, bounded `InputPipeline` and
`SongClockPublication` are implemented and tested; the live game still uses
only the diagnostic drain. With `native_shadow=true` the corrected song clock
is published to the capture and packet QPCs are mapped for diagnostics, not
judgment. `capture_input=false` is the default. ASIO and key-sound dispatch
remain unchanged.

The portable timing kernels are built and tested: decoded-control
normalization with held-state recovery, continuous-time normal-note judgment,
and a chart event cursor that retains every crossed event after a stall. They
use captured song times and explicit playback generations. A combined replay
checks identical input edges, judgments and chart-event order at all seven
requested refresh rates, with jitter, limited consumer capacity and a 1.53-second
stall. These components have no new live detours: native note matching, expiry,
pending-hit resolution and sound dispatch still require migration. The review
and integration boundaries are recorded in [TIMING_PIPELINE.txt](analyze/TIMING_PIPELINE.txt).

## Display selection

Spice's `Change Main Monitor` option can override Windows' primary display.
Choose the fast monitor there, or clear the option and select it in Windows.
For a single-screen test, Windows Display settings can disable the 60 Hz
screen without unplugging it. Restart the game after changing displays.
The verified test setup uses DISPLAY1 (DP); its D3D9 mode reports 119 Hz.
DISPLAY2 (YSNO) is the 60 Hz touchscreen. These names describe this machine,
not a portable monitor numbering rule.

## Validation and live check

Automated tests cover all seven requested display rates, ten-minute timing
and nonaccumulation runs, both vertex layouts, all six note layouts, source
buffer preservation, and hook installation in the copied game image. Hook-only
tests map images without executing game instructions. UI tests additionally
exercise node identity, repeated renders, source preservation, mode bypass,
and the actual cross-module hook transaction.

Live 0.2 testing on 2026-10-29 reached the title and attract-mode gameplay on
the fast monitor, with about 119 presentations/second and 60 simulation
ticks/second. Note prediction counters were active during the demo. This
does not validate manual play, every chart/note type, long-session drift, or
all requested display rates. The 0.3 UI trial reached the title screen but
reported zero accepted UI captures; smooth UI animation is unverified.

Version 0.4 tests additionally execute isolated copies of the original and
corrected QPC instruction blocks, exercise missing-libxact rejection and patch
rollback in an inert image, and replay clock timestamps under 60/90/120/144/
165/240/360 Hz sampling schedules with jitter and a stall. These are clock-core
and patch tests, not proof of frame-independent gameplay.

All 49 tests pass in the current merged build. The round-2 native components
(bucket table and shadow feed, staging-bucket input decoder, exact glissando
anchor, libxact cue table and frame-gate scheduler) are compiled and tested but
remain dormant; no config key or live hook reaches them. Tests also execute the original normal-note projection
in an isolated linear-scroll setup, checking its float argument ABI and use of
millisecond chart time. The monitor-check test executes the original initializer
and completion callback against a synthetic scene to verify the native transition,
legacy constants, disabled behavior, and hook rollback.
Input tests exercise batched reports, validation, queue overflow/recovery,
1 kHz edges across a 1.53-second consumer stall, and a 100,000-event concurrent queue run.
An isolated Windows test runs actual thread hooks, checks that peeking does not
double-count input, forwards the original message, observes sent disconnects,
and verifies unhook behavior.

The live 0.4 trial on 2026-10-29 produced about 119 presentations/second and
60 legacy ticks/second, with nonzero song-time normal-note draw counters and
prediction disabled. The subsequent build logged the FPS-calibration bypass
at 20:37:00 and proceeded to the startup logos. Counters prove execution of the
new path; they do not establish correct manual judgment or long-session drift.

The final input-observation trial started at 21:10:49. It reached attract-mode
gameplay with about 1000 HID reports/s, 119 presentations/s, and nonzero
song-time normal draws. A measured 904 ms scene-loading stall caused no reported
queue overflow, oversize packet, read error or malformed packet. These are raw
report observations, not proof of 1 ms physical input accuracy or scoring latency.
That trial ran with `clock_motion=true`, `capture_input=true`, and both
prediction switches disabled. The packaged configuration leaves the
experimental features disabled until explicitly selected.

For a live comparison, use the same chart, scroll speed, and timing settings
with `smooth_motion=false` and then `true`. Check normal notes and each long
note type near the judgment line, plus pause/retry and song transitions.
`nosfix.log` reports legacy ticks, presentations, predicted/song-time draw counts,
guarded fallbacks, and render/Present cost every five seconds. Expected legacy
ticks stay near 60 while presentations approach the selected display rate.
Prediction counters should become nonzero during moving notes above 60 Hz;
they count drawing calls, not unique notes or display frames.

Scoring windows, the existing audio drift correction, keysound dispatch, and
audio output remain on the original paths. ASIO is unchanged and is not enabled
by this DLL. Immediate presentation is requested so vblank blocking does not
own simulation pacing; tearing may be visible. The D3D driver may also block or
reject a selected mode, so the logged render rate is an attempted rate and
must be measured on the target display.
