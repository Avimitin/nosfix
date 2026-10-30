# nosfix DLL change map - 2026-10-30

> Historical sections 2 and 3 describe host note/UI prediction. Those paths
> are no longer installed; the native microsecond timeline is authoritative.
> See `analyze/HIGH_REFRESH_FIRST.md` for the current runtime.

This document describes what the injected `nosfix.dll` changes in the game
process, how each change is installed, why it exists, and what remains
untouched. It is a behavior map, not a claim that the native timing migration
is complete.

## Identity and safety gates

`spice_sdk_entry_point` runs from Spice before game main. Before any hook, the
module checks the loaded `nostalgia.dll` PE, size and SHA-256. Optional
features add their own exact checks:

- `libxact.dll` size/hash before the QPC patch or ASIO route
- `afp-core.dll` size/hash before UI motion
- exact expected instruction bytes before every MinHook target
- exact import identity before the Direct3D wrapper
- all hooks validated before any detour is created; failure rolls the set back

No pattern scanning is used for a mutable game function. Hooks are explicit
RVA + expected-byte contracts for PAN-2025020501.

## Active by default

The packaged `nosfix.ini` enables these paths:

### 1. Independent presentation and fixed simulation scheduler

**What changes:** the main iteration is detoured at `nostalgia+0x1C31C0`; two
CALL sites in its tail are redirected through nearby stubs. A QPC scheduler
runs the original fixed 60 Hz iteration and calls the original render routine
at the selected display rate.

**How:** MinHook entry detour plus exact CALL patches. `target_hz=auto` uses the
primary display's current rate; explicit rates are 60..360. The Direct3D
`CreateDevice` present parameters are changed to
`D3DPRESENT_INTERVAL_IMMEDIATE` through a wrapper around the
`Direct3DCreate9` import, so vblank does not own pacing.

**Why:** the original loop is a 60 Hz simulation. Rendering at the monitor rate
without making the simulation wait for vblank removes the 60 Hz presentation
ceiling.

**Does not change:** the simulation step itself, game update order, note state,
judgment, or scoring.

### 2. `smooth_motion` note prediction

**What changes:** six note-update functions are detoured (normal, long, beat,
glissando, trill, velocity). Geometry is captured after the original update;
at the AFP draw callback, a private copy of XY vertices is predicted from the
latest two 60 Hz samples and the absolute QPC phase.

**How:** MinHook on six virtual update targets, one AFP draw callback detour,
one BM2D buffer-release detour. Predicted vertices live only in owned buffers.
The game buffer must match the captured bytes at draw time.

**Why:** at 120/144/165/240/360 presentations, a 60 Hz note position visibly
steps. Bounded linear prediction hides most of that step without feeding
anything back into the game.

**Does not change:** note fields, source buffers, colors, UVs, judgment
positions, or collision/scoring. It is prediction, not a continuous-time
rewrite; curves and topology changes still fall back.

### 3. `smooth_ui` AFP translation prediction

**What changes:** supported AFP node translations are predicted between
simulation ticks; the original matrix composition receives an aligned private
copy.

**How:** one verified CALL in `afp-core.dll` is redirected; node identity and
parent/child order are preserved. Reused nodes, changed parents/resources,
missing ticks and invalid matrices reset prediction.

**Why:** menu animation otherwise advances only on the 60 Hz tick.

**Does not change:** AFP timeline advancement, callbacks, hit testing, colors,
fades, clipping or pivot modes. The 0.3 live trial accepted zero UI captures;
this path is still unverified live.

### 4. `precise_audio_clock`

**What changes:** 44 bytes in libxact's cue elapsed calculation are replaced.

**How:** both absolute 64-bit QPC timestamps are subtracted first, the
difference is converted in double precision, and only the final result is
converted to float at the existing ABI boundary.

**Why:** the original code converted each large absolute timestamp to float
before subtracting, losing precision as system uptime grows. The cue clock is
the time base used by the chart-time rendering path.

**Does not change:** cue selection, the audio-clock fallback, the 100 ms
source-selection threshold, judgment, keysound dispatch or audio output.

### 5. `skip_monitor_check`

**What changes:** the startup FPS-calibration initializer is detoured; the
game's existing completion branch is selected and the three legacy conversion
constants are fixed at 60.

**How:** one exact initializer hook; the original completion callback and scene
transition run normally. No fake hardware state is injected.

**Why:** the calibration can replace chart conversion constants with measured
59..60 values and delays startup. Chart time must not be defined by a display
rate.

**Does not change:** hardware/network checks or the game's scene transition.
The integer frame units remain temporary for frame-based consumers.

## Opt-in diagnostics and shadow paths

These are off in the packaged ini.

### 6. `capture_input`

**What changes:** keyboard/HID reports are copied on Spice's input thread before
its parser consumes them.

**How:** thread-specific `WH_GETMESSAGE` and `WH_CALLWNDPROC` hooks on the
verified `SpiceTools Input` window. Each packet records QPC dequeue time,
source header, sequence, stream epoch and a bounded ordered queue entry. Device
changes produce reset boundaries.

**Why:** the game's PANB/MIDI path reduces input to current state; a future
timestamped judgment needs the discrete press/release and its capture time.

**Does not change:** Spice's registration, message flow, input state, game
judgment or keysounds. This pass is diagnostic only.

### 7. `native_shadow`

**What changes:** the corrected cue time is published as an immutable
`SongClockPublication`; captured packets keep the snapshot taken before their
QPC and are mapped to song time for counters.

**Why:** prove the clock/input publication ordering without feeding judgment.
Requires `precise_audio_clock=true` and `capture_input=true`.

**Does not change:** judgment, score, rendering or sound.

### 8. `native_shadow_feed`

**What changes:** normal-note chart/note fields are collected from the update
detour; the 28x28 staging bucket at `module+0x13DEA10` is read each tick and
fed to `NativeShadowFeed`. The feed evaluates the decoded matching/scoring
model and logs `SHADOW outcome`/`SHADOW effect`.

**Why:** compare the native timing model against the original judge without
letting it affect the game.

**Does not change:** the original judge, score, keysounds or rendering. The
feed is read-only and fail-closed.

### 9. `diagnostic_probe`

**What changes:** normal judge calls at `0x2F6800`, note updates and the
staging-buffer diff emit `PROBE` lines.

**Why:** close the remaining live-only field questions (slot values,
`note+84`, velocity/gliss fields, connector clip).

**Does not change:** every probe calls the original function and only reads
memory.

### 10. `asio_output`

**What changes:** an `AsioAudioHost` opens an exact ASIO driver and
MinHook-replaces libxact's XAudio2Create helper at `libxact+0x3730`; libxact
receives the bridge engine instead of the real XAudio2 engine.

**Why:** move BGM/keysounds onto an ASIO sample timeline with low, measurable
latency.

**Does not change (on failure):** if the driver, rate, buffer, channels, hook
bytes or engine setup fail, the original engine remains and audio stays on the
original path. This route is experimental and has not been hardware-validated.
It also does not yet remove the `0x304F30` frame gate or `0x92E0` sleep.

## Compiled but not live

These components are in the DLL and tested, but no default game path invokes
them:

- `NativeMatchPolicy`, `NativeScoringState`, `NativeShadowFeed` (only the
  explicit shadow feed above uses the feed)
- `NativeLongState`, `NativeGlissState`, `NativeTrillState`, PANB pressure
- `BucketInputState` / `BucketTableState`
- `InputPipeline` and `SpiceBindingAdapter`
- `TimedUiAdapter` / `TimedUiTimeline`
- `Xa2Bridge`, `AudioMixer`, `AsioAudioHost` (only `asio_output`)
- `KeysoundDispatch` and `AudioTimeline` (only the ASIO route)
- `ChartEventCursor` and the portable timing pipeline

## Invariants: what the DLL does not change

- It does not modify the game installation. It is loaded by Spice using
  `-k nosfix.dll`; its only file writes are its own `nosfix.log` and an
  optional adjacent `nosfix.ini` edited by the configurator.
- It does not change the game's note table, note state, judgment windows,
  scoring, combo, hit effects, keysound selection or song position.
- It does not change input registration or current input state; `capture_input`
  only copies reports.
- It does not install process-wide graphics/audio API hooks. The Direct3D
  wrapper is on the game's imported `Direct3DCreate9` pointer; the ASIO route is
  a libxact module hook.
- It does not replace the D3D device, the XAudio2 device (unless the explicit
  ASIO route succeeds), AFP timelines or the game's scheduler.
- It does not make judgment frame-independent yet. The simulation and judge
  still advance at 60 Hz; chart-time rendering, captured timestamps and the
  shadow feed are separate diagnostic/visual paths.
- It does not enable the native ASIO route, shadow feed, probes or input
  observation by default.

## Failure behavior

Every optional path fails closed to the original behavior. Hook installation is
transactional across the whole nostalgia hook set; a byte mismatch, import
change or detour failure rolls back the set and the D3D import change. The
libxact, AFP, ASIO and probe paths report the exact reason in `nosfix.log`.
