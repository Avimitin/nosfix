# High-refresh correctness first - revised priority

Updated 2026-10-30. This supersedes the previous "native judgment first"
ordering. The primary goal is now:

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

## Proposed architecture

### One elapsed-time source

Every normal update samples QPC once and exposes:

- exact elapsed QPC ticks;
- a 60 Hz-equivalent fractional game time (`elapsed_seconds * 60`);
- a fractional second remainder for timers.

The native frame counter should no longer be incremented by the call count.
It becomes an accumulator advanced from the same elapsed time. A sub-frame
remainder must be retained; integer truncation would recreate 60 Hz steps.

### Two clocks, not one global speed-up

- **Gameplay clock**: 60 units/second, fractional. Existing chart records,
  judgment windows and note timestamps keep their frame units.
- **Render/UI clock**: display rate, elapsed time. Animation, menus, effects
  and note drawing consume elapsed time or the fractional gameplay clock.

This lets the game run its render/UI work at 120/144/165/240/360 while the
judge still sees its documented 60 Hz frame domain. It is the lowest-risk way
to get high-refresh correctness without changing scoring rules.

### Audit and conversion order

1. `sub_1802FC9D0`: replace `counter += 1` with an elapsed-time accumulator;
   preserve the fractional visual phase at `+280/+284/+292`.
2. `sub_1802FDB30`: remove/parameterize the hold/double catch-up; make the
   frame-gated cue comparison use the gameplay clock.
3. AFP update in `sub_1801C3FE0`: replace the fixed animation delta with
   elapsed time; audit the AFP ordinal that consumes it.
4. Menu, scene, result, tutorial and service countdowns: convert call-count
   thresholds to elapsed authored units, one owner at a time.
5. Note spawn/removal, long-note hold state, hit effects, keysound scheduling
   and chart event dispatch: ensure one crossing per event, not one per call.
6. Presentation: retain the current high-refresh renderer; remove any
   prediction path that becomes unnecessary once the update itself is smooth.

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

## What is deferred

- ASIO engine replacement remains experimental and off.
- `native_shadow_feed`, `diagnostic_probe`, `capture_input` remain compiled but
  are not the active path.
- Timestamped judgment, native matching and bucket-driven scoring remain
  dormant until the high-refresh simulation is correct.

## First code milestone

Introduce the elapsed-time context and make the scheduler's simulation rate
configurable, but keep the default at 60 until the frame-counter and timer
patches are validated. Then convert `sub_1802FC9D0` and `sub_1802FDB30` behind
a new opt-in high-refresh flag.
