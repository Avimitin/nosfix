# nosfix

`nosfix.dll` 0.4.0 (development) is a 64-bit Windows hook module for Nostalgia PAN-2025020501. The
project is C++20 and uses MinHook v1.3.3 at pinned commit
`9fbd087432700d73fc571118d6a9697a36443d88`.

The current runtime uses a single absolute microsecond music timeline. The
native chart loader, note timestamps, judgment windows, timeline tick and
beat/angle formulas operate in microseconds; the scheduler retires one native
update per display frame. There is no 60 Hz simulation fallback and no host
frame-prediction path. Timestamped input judgments and a hardware-validated
ASIO route remain future work. See
[the high-refresh notes](analyze/HIGH_REFRESH_FIRST.md) and
[the DLL change map](analyze/DLL_CHANGE_MAP.md).

Version 0.4 adds a guarded correction to libxact's cue-clock arithmetic, a
native conversion manifest recovered from IDA, and a startup FPS-calibration
bypass. The optional `SongClock`/`ShadowClock` publication can be connected to
the raw input observer with `native_shadow=true`, but it does not feed
judgments. The merged native match/scoring, bucket-input, timed-UI and
XAudio2/ASIO components are compiled and tested but dormant.

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
The microsecond cutover validates the complete `nostalgia.dll` conversion
manifest before writing any byte. `precise_audio_clock=true` is mandatory: it
verifies `libxact.dll` and replaces the cue-clock arithmetic so the published
song clock is QPC-correct. Cue selection and the audio backend are unchanged.
`skip_monitor_check=true` selects the game's existing FPS-check completion
path at startup without writing any rate constants. This removes the
calibration wait, not hardware/network checks. There is no frame-unit fallback:
the native timeline, chart loader, note timestamps, judgment windows and
beat/angle formulas all operate in microseconds.
The entry-point ABI follows the [Spice SDK header](https://raw.githubusercontent.com/spice2x/spice2x.github.io/main/src/spice2x/sdk/include/spicesdk.h)
and its [SDK initialization path](https://raw.githubusercontent.com/spice2x/spice2x.github.io/main/src/spice2x/sdk/sdk.cpp).

## Launch

Copy the built `nosfix.dll` and its adjacent `nosfix.ini` beside `spice64.exe`,
then add `-k nosfix.dll` to the existing
`spice64.exe` launch command. Set `target_hz=auto` to use the primary display's
current rate, or set `target_hz` to 60, 90, 120, 144, 165, 240, or 360. The
display rate is also the native simulation rate; there is no 60 Hz mode.
Host note/UI prediction and the former `clock_motion` path are removed.
`native_shadow=true` publishes that corrected cue time as an immutable
song-clock snapshot for the raw input observer. It requires
`precise_audio_clock=true` and `capture_input=true`; it maps captured QPCs and
logs mapped, unmapped and backward-mapping counters every five seconds. It
does not feed judgments, scoring, rendering, or sound. `native_shadow_feed` is
rejected in this build: the microsecond timeline is authoritative.
`diagnostic_probe=true` emits read-only `PROBE` lines for the normal judge.
The microsecond timeline is always enabled: chart times, note times, judgment
windows, the timeline tick and beat/angle formulas use absolute microseconds,
and the scheduler retires one native update per display frame. There is no
`high_refresh_timeline` switch and no frame-unit fallback.
See [the high-refresh notes](analyze/HIGH_REFRESH_FIRST.md).

`asio_output=true` is an experimental native route: it opens an exact ASIO
driver and replaces libxact's XAudio2 engine creation with the bridge engine.
Set either `asio_driver_clsid` or the exact `asio_driver_name`, plus
`asio_output_channels`, `asio_sample_rate` and `asio_buffer_frames`. The route
is fail-closed: if the driver, rate, buffer, channels or hook bytes do not
match, it logs the reason and leaves the original engine in place. It has not
been validated on real hardware.

Double-click `scripts\nosfix-config.bat` to open the configurator; it launches
`nosfix-config-gui.ps1` from the same folder. Use `scripts\nosfix-config-gui.ps1`
to edit the adjacent ini in a small WinForms
window. It lists the registered ASIO drivers from `HKLM\SOFTWARE\ASIO` and
saves the selected driver by CLSID, so the name does not need to be copied from
regedit. `-ListDrivers` prints the registry entries without opening the GUI;
`-SelfTest` builds the form without showing it. The script is optional and does
not modify the DLL.

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

## Native motion

The game's own note and UI geometry uses the microsecond timeline. The host
does not capture, predict, or rewrite AFP vertices or matrices. The former
`smooth_motion`, `smooth_ui` and `clock_motion` paths are removed from the
runtime configuration; their source remains only as dormant experimental code
and is not installed.

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
judgment. `capture_input=false` is the default. An experimental ASIO route
exists (`asio_output=true`) but is off by default and has not been validated on
a real driver; key-sound dispatch remains on the original path without it.

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

All 50 tests pass in the current merged build. The round-2 native components
(bucket table and shadow feed, staging-bucket input decoder, exact glissando
anchor, libxact cue table and frame-gate scheduler) are compiled and tested.
The normal-note shadow feed is the only live consumer and is off unless
`native_shadow_feed=true`; the other components remain dormant. Tests also execute the original normal-note projection
in an isolated linear-scroll setup, checking its float argument ABI and use of
millisecond chart time. The monitor-check test executes the original initializer
and completion callback against a synthetic scene to verify the native transition,
legacy constants, disabled behavior, and hook rollback.
Input tests exercise batched reports, validation, queue overflow/recovery,
1 kHz edges across a 1.53-second consumer stall, and a 100,000-event concurrent queue run.
An isolated Windows test runs actual thread hooks, checks that peeking does not
double-count input, forwards the original message, observes sent disconnects,
and verifies unhook behavior.

Earlier 0.4 trials used the fixed-60 simulation and host prediction; those
results are historical and do not validate the microsecond cutover.

For the current build, run the same chart and timing settings at each target
rate. Check normal notes, every long-note type, pause/retry, seek and song
transitions. `nosfix.log` reports display-rate ticks, presentations, render
cost and the timeline state every five seconds. Native ticks should approach
the selected display rate, not 60, and there is no prediction counter because
host geometry rewriting is not installed.

Scoring windows, audio drift correction, keysound dispatch, and audio output
remain on the native paths. The experimental `asio_output` route is off by
default and not hardware-validated; without it, audio remains on the original
engine. Immediate presentation is requested so vblank blocking does not own
simulation pacing; tearing may be visible. The D3D driver may also block or
reject a selected mode, so the logged render rate is an attempted rate and
must be measured on the target display.
