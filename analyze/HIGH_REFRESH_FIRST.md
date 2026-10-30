# High-refresh correctness first - revised priority

Updated 2026-10-30. This supersedes the previous "native judgment first"
ordering and the earlier two-clock proposal. The primary goal is now:

1. The game runs correctly at 60/90/120/144/165/240/360 Hz.
2. Animation, menus, effects, note motion and timing stay correct at any rate.
3. ASIO output is addressed after that.
4. Timestamped input judgment precision is last.

The existing native judge/input/ASIO work stays in the tree but is no longer
the active integration target.

## What "correct at high refresh" means

There are two different 60 Hz assumptions:

- **Presentation:** the game renders once per simulation tick. `nosfix`
  already separates presentation from the 60 Hz simulation and predicts note
  and UI vertices.
- **Simulation:** the game updates once per 60 Hz tick and many counters,
  timers, animation deltas and song-clock filters assume exactly 60 calls per
  second. This is the remaining problem.

Running the original update at 120 Hz without repairing the second category
would make menus, animation, note motion and scoring run at the wrong speed.
The target is an elapsed-time update with the original state machines retained,
not merely interpolation.

## No compatibility clock

There will be no `legacy_frame` view, no permanent 60-units/second gameplay
clock, and no dual write of a 60 Hz counter and a microsecond clock.

A temporary compatibility view would be read by every unmigrated function,
would become the path of least resistance, and would still be present after
the high-refresh work. It is therefore excluded from the design.

The single authoritative unit is **microseconds**:

- one `SongTimeUs` value, written from QPC/audio elapsed time;
- chart times converted from milliseconds to microseconds at load;
- note timestamps, judgment windows, timers and effects expressed in
  microseconds;
- no code path derives a 1/60-second value at runtime.

The old 60-unit frame counter is retired, not aliased. In the native image its
32-bit global is repurposed as a song-scoped microsecond counter; 2^32
microseconds is about 71 minutes, longer than any chart. Non-song/application
timers that need a longer range use the 64-bit host clock. This is one scale
with two storage widths, not two time bases.

Judgment windows authored in 1/60-second frames are converted once, when the
chart/window data is loaded, using exact integer rounding. That conversion is
data preparation, not a runtime adapter.

## Current audit

IDA references in the supported `nostalgia.dll`:

- `dword_18070A610` (song/frame counter): **105 references in 57 functions**;
  14 writes, 91 reads.
- `dword_18070B300` (legacy rate `60.0f`): **47 references in 29 functions**.
- `qword_18070B2F8` (legacy rate `60.0` double): **21 references in 6 functions**.
- `dword_18070A740` (frame adjustment): 6 references.
- `dword_18070A750` (clock correction state): 4 references.

Key paths:

- `sub_1801C3FE0` — main loop, device IO, AFP update, scene update, render.
- `sub_1802FC9D0` — increments `dword_18070A610` by exactly 1 per call and
  derives fractional visual time from it.
- `sub_1802FDB30` — gameplay controller; holds/doubles updates to chase audio,
  schedules frame-gated sounds, updates score/clear state.
- `sub_1802F7C10` / `sub_1802F6800` / `sub_180303E80` — traversal, normal
  judge and scoring; all compare integer frame values.
- `sub_1801A43E0` — chart loader; converts chart milliseconds with the literal
  `60.0 / 1000.0` at `0x1801A4935..0x1801A4945`.

The current `FrameScheduler` has `kLegacySimulationHz = 60`; it calls the
original game tail update at 60 and the render routine at the display rate.
That is why the rest of the game still advances at 60.

## Direct cutover plan

This is a single-scale migration, not an incremental dual path. Every one of
the 105 frame-counter sites must be accounted for before the branch merges.
The old semantics are deleted in the same change that introduces microseconds;
there is no fallback that keeps counting frames.

### 0. Site inventory (blocking)

Build an IDA-backed table of all 105 references: address, function, read or
write, what the value means at that site, and the replacement expression.
Classify each as clock read, note timestamp, judgment window, timer, visual
phase, hash/comparison or dead code. No patch is written for an unclassified
site.

### 1. One clock

- Define the host-side `SongTimeUs` type with no implicit conversion to or
  from integers.
- Sample QPC once per normal update and write the **absolute** song time. Do
  not add elapsed time per call: the update can run zero, one or two times in
  one display frame. A repeated call sees the same time; a skipped call loses
  nothing.
- Replace `sub_1802FC9D0`'s `counter += 1` with that absolute clock write.
- Delete the frame-adjustment and hold/double catch-up semantics in
  `sub_1802FDB30`; it must consume time, not fabricate frames.

### 2. Chart and judgment data

- Chart loader `sub_1801A43E0`: replace `60.0 / 1000.0` with exact
  milliseconds-to-microseconds conversion.
- Note records, chart events, BPM/stop data and long-note fields: store
  microseconds consistently.
- Judgment windows: convert frame-authored boundaries to microseconds once at
  load; use rounded integer boundaries so behavior is deterministic.
- No runtime multiplication by 60 and no fractional-frame accumulator.

### 3. Timers and animation

- AFP delta in `sub_1801C3FE0`: replace the fixed per-call delta with elapsed
  microseconds.
- Menu, scene, result, tutorial and service countdowns: elapsed time, not call
  counts.
- Hit effects, note fade, long-note ribbon and keyflash: elapsed time or
  absolute song time.
- Note spawn/removal, hold state and keysound scheduling: fire on a time
  crossing; a delayed update must not duplicate an event.

### 4. Presentation

- Render at the display rate from the same song clock.
- Remove prediction/interpolation that only existed to hide the 60 Hz update.
- Keep the existing high-refresh renderer only where it is still needed for
  output smoothing.

### Validation

- 60/75/90/95/120/144/165/240/360 Hz, steady and jittered;
- duplicate updates in one render frame and a single update across multiple
  frames;
- stalls and catch-up without duplicate score/effects/sounds;
- pause, seek, retry, song transitions and device reset;
- menu countdowns, results, tutorials and scene waits at each rate;
- side-by-side 60 Hz and 120 Hz screenshots/counters for the same chart;
- scoring parity at 60 Hz first, then at high rates.

The same input stream may be replayed at all rates, but judgment precision is
not the acceptance criterion for this phase; state/timer correctness is.

## Anti-drift gates

- No type, field or function named `legacy_frame`, `frame60`, `kLegacy...` or
  equivalent may exist in the merged timing path.
- CI/search gate: the active timing path must not contain `counter += 1` for
  song time, `60.0 / 1000.0`, or a runtime `* 60` conversion.
- A migration table must show **105/105** frame-counter sites resolved.
- No runtime switch that selects between 60-unit and microsecond semantics.
  If an A/B flag is needed during development, it lives only on the feature
  branch and is deleted before merge.
- The old frame-counter semantics are removed in the same commit as the new
  clock; no TODO is allowed to preserve them.

## What is deferred

- ASIO engine replacement remains experimental and off.
- `native_shadow_feed`, `diagnostic_probe`, `capture_input` remain compiled but
  are not the active path.
- Timestamped judgment, native matching and bucket-driven scoring remain
  dormant until the high-refresh simulation is correct.

## First code milestone

Create the microsecond clock and complete the 105-site inventory. Then convert
`sub_1802FC9D0`, `sub_1802FDB30` and the chart loader in one branch, deleting
the frame increment and the 60 Hz rate constant as part of that change.
