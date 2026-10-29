# render-ui round 2 - glissando anchor, chart-record time mapping

Worktree `build/meka/impl-20261030b/render-ui`, branch `native/r2-render-ui-20261030`.
Parent owns review, merge, live wiring and deployment. Nothing here installs a hook,
writes AFP or game memory, or turns a runtime path on by itself.

## 0. Outcome

| Kind | Status after round 2 | What changed |
| --- | --- | --- |
| Normal | supported (unchanged) | - |
| Long / hold | supported (unchanged) | - |
| Trill | supported (unchanged) | - |
| Glissando | **supported (new)** | exact chart-ms/frame mapping, distance-zero note quad, whole-object connector capture |
| Velocity zone | refused, `VelocityAnchorUnproven` | C11 corrections folded into the diagnostic contract; code unchanged |
| Beats | refused, `ChartTimeAbsent` | unchanged (no chart-time geometry exists) |
| AFP UI timeline | evaluator only; `TimedUiAdapter` still constructed nowhere | unchanged |

`TimedNoteSupported()` now returns true for Normal, Long, Trill **and Glissando**;
`TimedNoteRefusal(Glissando)` returns `None`.

## 1. Goal 1 - the chart-ms to frame mapping is exact, so glissando is enabled

### Evidence

`ABI_COMPLETION.md` C10 gives the writer of `element+0x14`, `sub_1801A43E0`:

```
1801A48F9  eax = src[0]                                       ; raw chart milliseconds
1801A4907  elem+0x00 = r8d                                    ; ms start   (decompiler-only)
1801A4912  elem+0x04 = (int)((float)src[0x0C] / rate)         ; ms end     (decompiler-only)
1801A4935  mulsd  xmm0, cs:qword_18070B2F8                    ; * 60.0   (double)
1801A493D  divsd  xmm0, xmm6                                  ; / 1000.0 (qword_180687218)
1801A4945  mov    [rbx+rcx+14h], eax                          ; int frame = trunc(ms*60/1000)
1801A495A  elem+0x18 = trunc(elem+0x04 * 60 / 1000)
1801A4977  elem+0x1C = trunc(r9d * 60 / 1000)                 ; r9d = src[8] / rate
```

C10 also resolves the round-1 worry: `+0`/`+4` are the record's chart milliseconds, not an
id, and the record is zero-based in the chart's own time origin - the same origin
`chart+0x00` already had for every other kind. The anchor question was therefore not a
missing field, it was a missing conversion, and the conversion is now implemented.

### Why it is exact, not approximate

For rate 60 the quotient is the reduced rational `3*ms/50`. Any non-integer value of that
fraction is at least `1/50 = 0.02` away from an integer, while one IEEE double division of
an exactly representable numerator (`ms*60 < 2^53`) errs by less than `2^-26`. The
truncation can therefore never cross an integer boundary, so
`(int)((double)ms * 60.0 / 1000.0) == trunc(ms * 3 / 50)` for every `int32` millisecond.
The implementation is that double expression (identical to the native instruction
sequence) and the test proves the equality against int64 integer arithmetic exhaustively
over `[-20000, 20000]`, on the worst-case residues (`ms = 17, 33 mod 50`, i.e. `0.02` and
`0.98` from an integer) sampled across the whole `int32` range, and at `INT32_MIN`/
`INT32_MAX`.

### New API (`timed_note_geometry.hpp`)

- `ChartMillisecondsToNativeFrame(chart_ms, rate, frame)` - the loader's expression with
  truncation toward zero, refusing non-finite input, `|ms| > 1e12`, a rate outside
  `(0, 1000]` and any result outside `int32`.
- `ChartRecordAnchorMilliseconds(chart_ms, frame, rate, anchor_ms)` - a record's fields
  must *agree* (`frame == ChartMillisecondsToNativeFrame(chart_ms)`); a foreign, malformed
  or half-written record can never become an anchor.
- `GlissLinkDistance(element_frame, song_ms, spec, distance)` - the head-connector anchor,
  `180*(element+0x14 - frame_counter - offsets)/rate^2` (`0x1801A6D20..0x1801A6DE4`),
  expressed in the same continuous millisecond domain every other kind uses:
  `element_frame * 1000 / rate` is inside `[ms - 1000/rate, ms]`, then the shared
  `DistanceFor` applies the timing offsets and `rate`. One frame of anchor error is exactly
  `180/rate^2` (0.05 distance units at 60 Hz), which the test asserts.

### What enabling glissando required beyond the mapping

The glissando update projects the note's own quad with a **literal distance of 0**
(`sub_180305430(..., a2 = 0, &note+28, &note+32, ..., note+44, note+48, ...)` at
`0x1801A6993..0x1801A6A12`), reads its extents from `note+196/+200` and writes it into the
same object at `note+128` that a normal note uses. Both connectors are appended to the
single box object at `note+216` (`0x1801A6D19` / `0x1801A7326`).

Implemented:

- `NoteAnchorRule::Fixed` - a point anchor at distance zero. It is a legal point anchor for
  a textured part, and illegal on a strip or on a whole object (still `None` only).
  `BuildPart` projects it at `0.0f` and never derives a distance from the clock; a missing
  projector, a non-finite song time and a rejected projection are still refused.
- `kGlissParts`: `{0x80, TexturedQuad, Fixed, +196, +200, required}` and
  `{0xD8, BoxObject, None, optional}`. The connector object is optional because with no
  surviving link it holds no record and draws nothing; `CaptureTimedNote` now skips an
  optional part whose object count is zero instead of failing the note.
- `BuildNoteSpec`: width/height at `+196/+200` for glissando (it previously fell into the
  long-note `+240/+244` branch), and the record-anchor check above - a mismatch returns
  `InvalidChart`, which the hooks count as `timed_note_failures`.

## 2. Goal 2 - `chart+0x50`, the three clip branches

Writer `0x1801A647C` (`mov [rcx+50h], eax` with `rcx = *(note+8)`, `eax = note+32`) inside
the connector scan makes `element+0x50` the partner note's own projected screen Y, rewritten
every frame. It is consumed, never derived, by `BuildGlissConnectorQuads`:

| `element+0x50` | Native (`0x1801A6DE9..0x1801A6E2B`) | Model |
| --- | --- | --- |
| `<= 0` | far end keeps the projected point | same (also covers 0) |
| `>= note_y`, equal | far y = clip, far x unchanged | same |
| `>= note_y`, above | far y = clip, far x interpolated on the note->far segment | same |
| `< note_y` | far end collapses onto the note point | same |

All four are covered by `TestGlissConnectorHeadClip` (values 0, -1, NaN refused, 480 =
partner y, 400 = interpolated to 253.33/213.33, 300 = collapse, 200 = collapse above the
note, infinite half width refused) and the tail connector is re-checked to ignore the head
clip in `TestGlissConnectorTailAndOrder`.

`PROBE_ANALYSIS.md` s5 confirms the build-B `chart50=0` sample is invalid (the hooked
objects were velocity zones, so the writer never ran). It is **not** used as evidence here:
the branches come from the instruction bytes, and the `<= 0` branch is the native's own
"skip the clip" path.

## 3. Goal 3 - velocity zones, beats and the UI adapter stay fail-closed

- Velocity zone: `TimedNoteSupported` false, `TimedNoteRefusal` = `VelocityAnchorUnproven`.
  Comments now record the C11 corrections instead of the withdrawn readings: the writers of
  `note+296/+300/+316` are unknown, `+296` was observed in `{0,1}` (a flag, not a frame),
  `+300` in `{-1, 30, 69, 72}` (not a 0/1 flag), `+316` unrefuted. Nothing derives an end
  anchor, and `+296` is never read from `chart+4`.
- Beats: unchanged; `0x1801A5990` projects at the live frame counter with `x = 640`, so
  there is no chart time to rebuild and `ChartTimeAbsent` stays.
- `TimedUiAdapter` is still not constructed anywhere in the library and no AFP node is
  written. No AFP ABI was guessed.

## 4. Behaviour at the draw hook

- A glissando note is captured (quad + connector object) and its quad is rebuilt at
  distance zero, which is exact and clock independent.
- The connector object is a whole-object part: `BuildPart` refuses it with
  `ObjectQuadsRequired`, the hook then keeps the **original** payload *and* disables pixel
  prediction for that buffer (round 1 finding: a predicted payload would invent a record
  count that two independent connectors own). Corner coordinates remain caller-supplied
  through `BuildObjectPart`, fed by `GlissLinkDistance` + `BuildGlissConnectorQuads`.
- `GlissAnchorUnproven` and `GlissClipUnproven` are retired: both blockers they named are
  closed and no code returns them. The values stay in the enum so the
  `MotionMetrics::geometry_diagnostics` indices do not move; `MotionMetrics` keeps its
  exact size and field set.
- No allocation, lock or log was added to any capture/build path;
  `TestNoAllocationAfterSetup`, `TestAllocationFailureIsContained`,
  `TestWholeObjectNoAllocation` and the zero-allocation frame loop still pass.

## 5. Files

Modified
- `include/nosfix/timed_note_geometry.hpp` - `NoteAnchorRule::Fixed`; the three chart-time
  functions; retired-diagnostic and `BoxObject` contracts.
- `src/timed_note_geometry.cpp` - the three functions, `Fixed` in `ValidAnchors`/`Capture`/
  `BuildPart`.
- `src/timed_rendering.cpp` - `kGlissParts`, glissando width offsets, record-anchor check,
  optional-part skip, `TimedNoteSupported`/`TimedNoteRefusal`.
- `include/nosfix/timed_rendering.hpp` - supported-kind contract.
- `tests/timed_note_geometry_tests.cpp` - `TestChartFrameMapping`,
  `TestChartRecordAnchor`, `TestGlissLinkDistance`, `TestFixedAnchor`.
- `tests/timed_note_geometry_native_tests.cpp` - glissando capture/build against the real
  module, loader-constant reads, mismatch and missing-connector refusals.
- `tests/motion_hook_timed_tests.cpp` - glissando capture, distance-zero quad draw,
  connector fallback with `ObjectQuadsRequired`, mismatch refusal.
- `analyze/impl-20261030/render-ui-R2-REPORT.md` (this file).

Not touched: `native_*`, `input_*`, `audio_*`, `asio_*`, `nosfix.ini`, `config.*`,
`bootstrap.*`, `game_hooks.*`, `hooks.*`, `ui_hooks.*`, `ui_motion.*`, `presentation.*`,
`primitive_motion.*`, `motion_hooks.*` (header and source both stay at the round-1
version), `scripts/build.ps1`, `CMakeLists.txt`.

## 6. Tests

Command (exactly, from this worktree):

```
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -RunTests
```

Result: `100% tests passed out of 47` on this branch (the 47 baseline tests, all
pre-existing targets plus the extended render-ui ones). Release, `/W4 /permissive- /EHsc
/utf-8`; no warning is emitted for any file in this change.

New coverage
- `timed_note_geometry` (pure): mapping exactness (exhaustive range, worst-case residues
  across the int32 range, both int32 ends, every guard, int32 overflow refusal); record
  anchor agreement and one-frame/unwritten refusals; `GlissLinkDistance` (exact zero at the
  record's own instant, `0.05` per frame, song clock, timing offset, refusals); `Fixed`
  anchor (projects at distance 0 with the note's lane and keys, x shift, clock
  independence, `TimeRange`, `ProjectionRejected`, refused on a strip and on a whole
  object, and coexist with a chart-anchored part).
- `timed_note_geometry_native`: the loader's own constants (`double 60.0` at `0x70B2F8`,
  `double 1000.0` at `0x687218`) read from the real image; glissando capture yields the
  quad + connector object; `BuildPart(0)` at distance zero for two different song times
  produces identical bytes; `BuildPart(1)` refuses with `ObjectQuadsRequired`; a
  `+0x14 = +0x00 frame + 1` record is refused with `InvalidChart`; a zero-record connector
  is skipped; a glissando note is not accepted under the velocity kind.
- `motion_hook_timed`: glissando is captured (1 capture, 0 unsupported), its quad draw is a
  private rebuild at (110, 690) regardless of song time, the connector draw keeps the game
  buffer and increments `fallback_draws` + `ObjectQuadsRequired`, and the mismatch case is
  counted as a capture failure.

## 7. Remaining blockers (fail closed)

1. Velocity zone end anchor: the writers of `note+296` (end), `note+300` (state) and
   `note+316` (span) are still unknown (`ABI_COMPLETION.md` C11). Needed: the writer of
   `+0x128` reached with a symbol-relative base in `sub_1801A5DB0`, `sub_1801AB3A0` or
   `sub_1801AB410`.
2. Ribbon colour: the gradient ramps are read-only data (C12), so no one-tick lag has to be
   reproduced; the model still reuses the captured record palette. Copying the ramps is
   optional fidelity work, not a blocker.
3. AFP UI timeline: the six blocked items of answer `render-ui-002` (colour/opacity/
   visibility offsets, evaluator time source, user callback ABI, pool free entry,
   `node+128` writability). Until they land `TimedUiAdapter` must stay unconstructed.
4. Glissando connector corners: by design still caller-supplied (no projection and no field
   reads in the model). The integration must supply rows projected with
   `GlissLinkDistance` and the note's own point, then `BuildGlissConnectorQuads` ->
   `BuildObjectPart`.

## 8. Merge notes for the parent

- Seven files change; `main` holds all of them at the round-1 version, so the merge replaces
  them wholesale. `motion_hooks.*`, `primitive_motion.*` and `CMakeLists.txt` are untouched,
  so no new target or hook spec is introduced.
- `TimedNoteSupported(Glissando)` is now true. That is inert until
  `PrepareTimedRendering` + `ConfigureTimedRendering` run; then `unsupported_glissando`
  stops counting and glissando notes are captured. The connector draw never uses a
  prediction afterwards.
- `MotionMetrics` keeps its exact layout (`geometry_diagnostics` still 24 entries). Two
  diagnostic names are retired but still mapped in `GeometryDiagnosticName`.
- Harness pitfall found while testing: `note+196/+200` (the glissando quad extents) overlap
  the synthetic long-note tail object header at `note+0xA8`, whose buffer pointer lives at
  `note+0xC8`. A test that reuses one note buffer for every kind must re-attach that object
  before a long/velocity kind reads it. This is a harness artefact only; the native note
  has no tail object at `0xA8`.
- No merge, no rebase, no push, no deployment, no hook install, no AFP write.
