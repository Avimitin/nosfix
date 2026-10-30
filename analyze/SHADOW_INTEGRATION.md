# Shadow integration - stage 1

Updated 2026-10-30. This document describes what is actually wired into the
live DLL after the first four implementation streams were merged. It is not a
claim of frame-independent judgement.

## What is wired

`native_shadow=true` makes the render thread publish a `SongClockPublication`
from the same corrected libxact cue time used by `clock_motion`. The raw input
capture takes that immutable snapshot before reading each packet QPC, so a
queued packet keeps its capture-time mapping. The game thread maps input
packets for diagnostics and logs counters every five seconds.

The implementation is:

- `include/nosfix/shadow_clock.hpp`, `src/shadow_clock.cpp`: single-writer
  owner around `SongClock` and `SongClockPublication`; bounded corrections,
  explicit `Seek` generations and `PossiblyRegressed` publication.
- `include/nosfix/timed_rendering.hpp`, `src/timed_rendering.cpp`: sample the
  corrected cue time once per presentation and feed `ShadowClock`.
- `src/game_hooks.cpp`: configure the shadow clock, pass its publication to
  `StartRawInputCapture`, map captured packets, and log counters.
- `include/nosfix/config.hpp`, `src/config.cpp`, `nosfix.ini`: the
  `native_shadow` switch, default `false`.

The shadow path does not read back into game state. It does not call a
judgement routine, change scoring, drive a note or UI primitive, or start a
keysound.

## Enabling it

The minimum isolated configuration is:

```ini
enabled=true
precise_audio_clock=true
skip_monitor_check=true
capture_input=true
native_shadow=true
clock_motion=false
smooth_motion=false
smooth_ui=false
target_hz=auto
log_level=info
```

`native_shadow` requires `precise_audio_clock=true`; startup fails closed
otherwise. `capture_input=true` is required for packets; without it the clock
publishes but the log warns that nothing consumes it.

## What to check in `nosfix.log`

On startup:

```
shadow song clock publication enabled; input packets are mapped for diagnostics only
raw input observation active with shadow song-clock snapshots; judgments unchanged
```

Every five seconds:

```
5s shadow clock: packets=N, mapped=N, unmapped=N, backwards=N, regressed=N, fences=N, generation=N
```

Expected on a normal song:

- `mapped` grows with input packets; `unmapped` is small after the first cue
  sample.
- `backwards` stays `0`; a nonzero value means a snapshot mapped a later QPC to
  an earlier song time.
- `regressed` stays `0`; nonzero means a negative correction was accepted and
  published as `PossiblyRegressed`.
- `generation` is stable during one song and changes on seek/retry, clock
  restart or a large discontinuity.
- `fences` grows even when no key is pressed (the producer heartbeat).

## What is deliberately not wired

- `NativeMatchPolicy` / `NativeScoringState` (judgement, score, effects).
- `NativeLongState` / `NativeGlissState` / `NativeTrillState` and PANB pressure.
- `TimedUiAdapter` and the AFP node/timeline path.
- `Xa2Bridge` / `AsioAudioHost` / the mixer.
- Any config key, bootstrap order, detour or hook for those modules.

Reasons are recorded in `analyze/impl-20261030/*-REPORT.md` and the evidence
packs under `build/meka/impl-20261030/coord/`. The remaining blockers are
binary/live evidence, not missing clock code: the `dword_18070A610` unit, the
bucket table at `0x13DEA20`, the native input-event producer, the AFP
property/callback ABI, the cue sample format and the frame-gate consumer.

## Diagnostic probe build

`diagnostic_probe=true` installs two read-only probes and writes `PROBE` lines:

- `0x2F6800` normal judge: `PROBE judge before/after` records the note id/rank
  fields, `dword_18070A610`, chart time, and the native event fields
  `+0/+4/+0x14/+0x20`.
- Motion update detours: `PROBE motion kind=3(gliss)` and
  `PROBE motion kind=5(velocity)` record `note+296/+300/+316`, `chart+0x50`,
  `note+32`, chart type flags and the current frame.

The probes are rate-limited to 400 judge calls and 400 updates per kind. They do
not change scoring, rendering, capture or audio. The packaged diagnostic build
is `out/diag-20261030/` with its own `nosfix.ini` and `README-PROBE.txt`.

## Stage 2: live normal-note shadow feed

`native_shadow_feed=true` starts a read-only comparison path for normal notes:

- the normal-note update detour records each note's chart pointer and decoded
  fields into a bounded 512-note table;
- `ShadowLiveTick` reads the 28x28 staging buffer published at
  `module+0x13DEA10`, opens a `NativeShadowFeed` frame at the mapped song time,
  rotates/compacts the bucket and evaluates entry 0;
- judged outcomes and score effects are written to `nosfix.log` as
  `SHADOW outcome` and `SHADOW effect` lines.

The original judge, score, keysounds and rendering are never called from this
path. `native_shadow_feed` requires `precise_audio_clock=true`; it does not
require `capture_input` because the bucket comes from the game's own note
manager. The feed waits for the judgment-window block to become non-zero after
the game's initializer, then configures itself on the first gameplay tick.

Enable it in isolation:

```ini
enabled=true
precise_audio_clock=true
skip_monitor_check=true
capture_input=false
clock_motion=false
native_shadow=false
native_shadow_feed=true
target_hz=auto
smooth_motion=false
smooth_ui=false
log_level=info
```

Expected lines:

```
SHADOW live feed configured (normal notes, diagnostic only)
5s SHADOW live: frames=..., notes=..., outcomes=..., effects=..., errors=..., last=ok
SHADOW outcome note=... lane=... kind=... rank=... grade=... at=...ms input=...ms
SHADOW effect note=... grade=... score=... combo=...->... overridden=...
```

This is a comparison feed, not a replacement judge. It still leaves the
live-only `note+84 > 1`, velocity/gliss writer and AFP ABI questions
fail-closed.

## Next integration stage

1. Add a live chart/note-table adapter and feed `NativeMatchPolicy` in shadow
   beside the existing `2F7C10` traversal; keep original scoring authoritative.
2. Add the `InputPipeline` consumer with the real HID descriptor and binding
   XML, then supply `PipelineInputEvent` to the shadow note state.
3. Only after a full song agrees, enable native judgement; keep `S2` disabled
   until the bucket table and unit are proven.
4. Audio and AFP stay separate gates; neither is reachable from this stage.
