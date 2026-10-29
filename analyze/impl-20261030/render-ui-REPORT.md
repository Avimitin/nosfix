# nosfix 0.4 - render-ui worker report (impl-20261030)

Worktree `build/meka/impl-20261030/render-ui`, branch `native/render-ui-20261030`.
Parent owns review, merge, live wiring and deployment. Nothing here is deployed and
no new runtime hook, detour or AFP write is enabled by default.

## 1. Outcome in one table

| Kind | Status after this pass | Chart-time rebuild |
| --- | --- | --- |
| Normal | supported (unchanged) | yes |
| Long / hold | supported (unchanged) | yes |
| Trill | supported (unchanged) | yes |
| Glissando | **refused**, both blockers now named and one of them proven | no |
| Velocity zone | **refused**, width rule proven, end anchor still unproven | no |
| Beats | **refused**, proven to have no chart-time geometry at all | no (nothing to rebuild) |
| AFP UI timeline | evaluator + sink contract only; **no node is written** | n/a |

## 2. What changed

### 2.1 Whole-object parts (`timed_note_geometry.*`)

The glissando update appends both connector quads to the **single** box-fill object at
`note+216`: `sub_1800D5370(obj, ...)` is called once per connector and
`(_DWORD *)note + 54` is `note+216` in both cases (answer render-ui-001 Q3, verified from
bytes). One draw call therefore owns **0, 1 or 2** records, and the draw count is the
object's own count word at `+24` - the same field shape as `note+128`. The previous
one-part-per-buffer binding cannot express that.

Added:

- `NotePartShape::BoxObject` - a whole-object box part.
- `NoteAnchorRule::None` - the anchor-less marker a whole-object part must carry.
- `NoteObjectQuad` (`p1x,p2x,p3x,p4x,previous_y,current_y`) - the exact argument order of
  `0xD5370`, whose record builder `0xD5460` emits `p1,p2,p4,p4,p2,p3`.
- `TimedNoteGeometry::BuildObjectPart(index, quads, view, diagnostic)` and
  `PartIsObject(index)`; `kMaxObjectQuads = 2`.
- `Capture` accepts a whole-object part with 1-2 records and the `None` anchor, and
  rejects every other combination; the box slot allocation is shared with the strips and
  still allocation free.
- `BuildPart`/`Build` refuse a whole-object part with `ObjectQuadsRequired`; the object
  path refuses 0 quads with `EmptyObject`, >2 with `ObjectOverflow`, a reversed row with
  `NonFiniteProjection`, and a non-object part with `LayoutUnverified`.
- Colour/alpha for each object quad come from the captured records, so the object keeps
  the game's own palette and the documented one-tick RGB lag.

The corner coordinates are **caller supplied**: the native glue forms them from a
projector call whose distance is only now proven (see 4.1), and from the element `+0x50`
clip whose writer is still unknown. The model never derives them, so no layout is guessed.

### 2.2 Draw hook (`motion_hooks.cpp`)

- A whole-object part that cannot be rebuilt now disables the *prediction* fallback for
  that draw instead of substituting a predicted payload: prediction would invent a record
  count that two independent connectors own. The original draw is used unchanged.
- Unsupported kinds are counted by name: `unsupported_glissando`,
  `unsupported_velocity_zones`, `unsupported_beats`, plus the exact refusal in
  `geometry_diagnostics`. `object_part_builds` exists and stays 0 until the parent enables
  glissando.
- `MotionMetrics::geometry_diagnostics` grew from 16 to 24 entries because the diagnostic
  enum grew from 12 to 19 values.

### 2.3 Velocity zones and beats (`timed_rendering.*`)

- `RibbonWidthRule`, `RibbonWidthRuleFor(kind)`, `RibbonHalfWidth(rule, note_width,
  shrink, wide, out)` are the single source of truth for the two proven rules:
  long/trill `shrink*wide/2 - 10 + note+60/2`, velocity zone `note+60/2 - 10` with no
  shrink or wide term. Integer arithmetic matches the native `idiv` truncation toward zero.
- `TimedNoteRefusal(kind)` returns the exact refusal diagnostic
  (`GlissAnchorUnproven`, `VelocityAnchorUnproven`, `ChartTimeAbsent`, or `None`).
- `BuildNoteSpec` now derives the ribbon base width through `RibbonHalfWidth` and treats
  only long/trill as having a second chart time (`has_end`), instead of `kind != Normal`.
  Supported kinds keep byte-identical specs.
- Beats are **proven** to have no chart-time geometry: `sub_1801A5990` projects at
  `distance = dword_18070A610` (the live frame counter), forces `x = 640`, clamps
  `y` to `720 - lane_word`, and takes `+196/+200` with a `max(half_height, 3.0f)` floor.
  Its `note+24` is a frame, not a chart millisecond, so there is nothing to rebuild.

### 2.4 UI adapter (new `timed_ui_adapter.*`, `timed_ui_timeline.*`)

- `UiTimelineBank::EvaluateDomain(domain, UiStateSink, context)` evaluates every live
  timeline of one clock domain and hands the fractional `UiAnimationState` to a sink. It
  primes an entry with exactly the same `Start(generation, now)` rule as `AdvanceDomain`,
  so evaluation and event delivery can never disagree about the epoch.
- `UiTimelineBank::DomainTime`/`DomainGeneration` expose the published clock read-only.
- **Bank identity fix:** `Entry` now records the `UiNodeIdentity` at `Acquire` time
  instead of relying on `UiTimeline::Node()`, which `Reset()` zeroes. Before this,
  `Acquire` of an already bound but not yet configured node returned a *second* slot,
  `Find` returned null, and the same node could occupy all 16 slots. This is the
  "acquire/release by identity" behaviour the task asks for, and it is now covered.
- New fail-closed `TimedUiAdapter` (`src/timed_ui_adapter.cpp`) with a `UiNodeSink`
  of three function pointers (`apply`, `event`, optional context). It owns a
  `UiTimelineBank` and a monotonic QPC-derived wall clock, and it writes **nothing** into
  a game module. Song timelines advance only from `SetSongTime`, wall-clock timelines only
  from QPC or a manual value; each is advanced separately and in a fixed order
  (wall before song) because `a + b` operand order is unspecified and the first version
  evaluated the domains in reverse.
- Deviations are explicit: null `apply` sink is a complete no-op (`NoSink`, nothing
  evaluated, no event consumed); a backwards QPC sample, zero frequency or a backwards
  manual value is rejected and counted; a frequency change rebases at the published value
  so the wall clock never jumps; releasing an identity drops it before the next frame.

### 2.5 Build script

`scripts/build.ps1` failed to configure in this worktree: CMake writes the RC compiler
into `CMakeRCCompiler.cmake` as an unquoted `set()` value, so a Windows path with
backslashes aborts with `Invalid character escape '\S'`. The script now hands CMake
forward-slash paths for `rc.exe`/`mt.exe`. Nothing else in it changed.

## 3. Files

Added
- `include/nosfix/timed_ui_adapter.hpp`
- `src/timed_ui_adapter.cpp`
- `tests/timed_ui_adapter_tests.cpp`
- `analyze/impl-20261030/render-ui-REPORT.md`

Modified
- `include/nosfix/timed_note_geometry.hpp`, `src/timed_note_geometry.cpp`
- `include/nosfix/timed_rendering.hpp`, `src/timed_rendering.cpp`
- `include/nosfix/timed_ui_timeline.hpp`, `src/timed_ui_timeline.cpp`
- `include/nosfix/motion_hooks.hpp`, `src/motion_hooks.cpp`
- `tests/timed_note_geometry_tests.cpp`, `tests/timed_note_geometry_native_tests.cpp`
- `tests/motion_hook_timed_tests.cpp`, `tests/timed_ui_timeline_tests.cpp` (line endings only)
- `tests/timed_rendering_tests.cpp` (line endings only)
- `CMakeLists.txt` (adapter source + `timed_ui_adapter` test target)
- `scripts/build.ps1` (RC/MT path quoting)

Not touched: `native_*`, `input_*`, `asio_*`, `audio_*`, `nosfix.ini`, `config.*`,
`bootstrap.*`, `game_hooks.*`, `hooks.*`, `ui_hooks.*`, `ui_motion.*`, `presentation.*`.

## 4. Evidence used

Requests raised this pass, both answered by the analysis worker:
- `coord/requests/render-ui-001.md` -> `coord/answers/render-ui-001.md` (nostalgia
  geometry), status **partial**, confidence mixed (bytes / decompiler / unproven).
- `coord/requests/render-ui-002.md` -> `coord/answers/render-ui-002.md` (afp-core UI
  binding), status **partial**, items 1, 2, 4, 5, 6 **blocked**.

Reproduced/consumed from the worktree evidence:
- `analyze/evidence/verified_0x1801a6820.c` (glissando update, both connector appends),
  `motion_0x1801aa4d0.c` (velocity zone), `motion_0x1801a5990.c` (beats).
- `reference/motion_ui-REPORT.txt` section 5, `motion_ui-REQUESTS.txt` N2-N8.

### 4.1 Facts established (and the ones still open)

Proven from instruction bytes (render-ui-001 Q1):
`XMM1 = 180.0f * (float)(element+0x14 - dword_18070A610 - HIDWORD(qword_181366914) -
HIDWORD(qword_181366908)) / (rate*rate)` at the first connector call `0x1801A6DE4`;
the second connector passes literal `0` (`chart+0x48`, guarded by `element+0x20 == 0`).
The projector ABI is confirmed again: `rcx = element+0x10` lane, `xmm1 = distance`,
`r8 = &x`, `r9 = &y`, `[rsp+0x20] = a5`, `+0x28 = &scale`, `+0x30 = &a7`,
`+0x38 = &opacity`, `+0x40 = element+0x58` left key, `+0x48 = element+0x5C` right key,
`+0x50 = &out_left_px`, `+0x58 = &out_right_px`.

Proven from bytes (render-ui-001 Q3): `note+216` is one box object, count `+24`,
capacity `+16`, buffer `+32`, stride 120, so 0-2 connector quads per draw call.

Proven from bytes (render-ui-001 Q4): velocity-zone half width is
`(note+60/2 - 10) * scale` with no `dword_1813E476C` and no `0x13E5721` reference
anywhere in `sub_1801AA4D0`; `note+0x158` selects one of two colour quadruples.

Still open (exact gaps):
- glissando: writer of the 104-byte chart element field `+0x50` (the clip Y compared
  against `note+32` at `0x1801A6DE9`). The loader only writes the link pointers
  (`chart+0x40` at `0x1801A4F09`, `chart+0x48` at `0x1801A4F2D`);
  `sub_1802F6800` writes `+0x50` only for the note's own record. The element layout is
  also inconsistent with a raw chart record (`element+0` is compared as a word id at
  `0x1801A4ED4`), so `element+0x14`'s frame -> millisecond mapping is unproven.
- velocity zone: writers of `note+296` (end), `note+300` (held flag) and `note+316`
  (held length); none is touched by the shared binder `sub_1802F73B0`.
- ribbon colour ramps `0x1366018..`, `0x12D3278..`, `0x12D3398..0x12D33D8`: no writer
  found, so a box strip's RGB may still lag one legacy tick while its geometry is exact.
- AFP: colour/opacity/visibility offsets, matrix convention and multiply order,
  evaluator time source and keyframe layout, user callback slot/ABI, pool free entry, and
  whether `node+128` may be written outside the draw traversal. The answer also corrects
  two earlier assumptions: `node+192/+196/+200` are **floats** used as a negation vector,
  and `+204`/`+205` are a split 2-bit transform mode. `sub_18010CF60` is an action-queueing
  helper, not a timeline tick. `play+32 & 4` disables the timeline and `play+336` is a
  count-prefixed track table; both are read-only facts and no code depends on them.

Consequence: `TimedNoteSupported(Glissando)` stays false and `TimedUiAdapter` writes no
node property. The whole-object capability is in place so that enabling glissando later is
a capture + anchor change, not a model change.

## 5. Tests

Command (exactly):

```
cd D:\Share\nosfix\build\meka\impl-20261030\render-ui
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -RunTests
```

Result: `100% tests passed out of 40` (the 39 pre-existing tests plus the new
`timed_ui_adapter` target). Release, `/W4 /permissive- /EHsc /utf-8`, no new warnings.

New coverage:
- `timed_note_geometry`: `TestWholeObjectCaptureAndBuild` (capture, `ObjectQuadsRequired`
  from the anchor path, 1-quad and 2-quad corner order `p1,p2,p4,p4,p2,p3`, 2nd record
  colour template, empty/overflow/NaN/reversed-row/part-shape refusals, source buffer
  unchanged), `TestWholeObjectHorizontalAndBounds`, `TestWholeObjectDescriptorRejections`
  (anchored object, `None` on a strip, 3 records, 0 records, short buffer),
  `TestWholeObjectMixedNote`, `TestWholeObjectNoAllocation` (200 rebuilds and a failing
  allocator), plus the new diagnostic names.
- `timed_ui_adapter` (new): identity/reuse with a recycled pool stamp, capacity refusal,
  `Release`/`ReleaseNode`/`Clear`, song evaluation at 250/500 ms, paused song freeze,
  QPC ticks (zero frequency, negative, backwards, frequency rebase, manual rebase),
  null-sink no-op that consumes nothing, event ordering and once-per-epoch delivery,
  stale generation, seek with the inclusive start policy, generation-not-new, release
  stops evaluation, the two domains staying separate with a fixed wall-before-song order,
  and a zero-allocation 30-frame run over 8 nodes.
- `timed_note_geometry_native` (real modules): full `RibbonWidthRuleFor` matrix, both
  half-width rules including truncation toward zero, all rejections, and the
  `TimedNoteSupported` matrix.
- `motion_hook_timed` (real modules): `unsupported_glissando`/`_beats`/`_velocity_zones`
  and `geometry_diagnostics[GlissAnchorUnproven|ChartTimeAbsent|VelocityAnchorUnproven]`
  counters, `object_part_builds == 0`, and the `TimedNoteRefusal` matrix.

## 6. Remaining blockers (fail closed, in priority order)

1. Glissando clip: writer of the chart element `+0x50`. Until then the only safe behaviour
   is the `+0x50 <= 0` branch, which cannot be assumed at runtime.
2. Glissando anchor: proof that `element+0x14` (a frame) maps to chart milliseconds the
   same way `chart+0/+4` do. Without it the proven distance formula has no chart-time input.
3. Velocity zones: writers of `note+296/+300/+316` (end anchor and held-head state).
4. Ribbon ramps: writer of the gradient tables, to remove the one-tick RGB lag.
5. AFP: the six blocked items in answer render-ui-002.

Nothing in this pass makes any of those worse: every one of them is a refusal that keeps
the original path and is counted by name.

## 7. Merge notes for the parent

- `TimedNoteSupported` still returns true only for Normal, Long and Trill; the live
  prediction path in `ui_hooks`/`ui_motion` is untouched and `TimedUiAdapter` is not
  constructed anywhere in the library, so no new behaviour is reachable without an
  explicit wiring change.
- `TimedUiAdapter` is compiled into `nosfix.dll` but dormant. Wiring it means calling
  `Bind`/`SetSongTime`/`Advance` and, later, supplying a sink that writes AFP - which must
  not happen before answer render-ui-002 items 1, 2, 4 and 5 land.
- The bank identity fix changes `UiTimelineBank` behaviour for a *not yet configured*
  entry (it is now addressable); configured-entry behaviour is unchanged and
  `timed_ui_timeline` still passes.
- `MotionMetrics` grew four fields and its diagnostic array grew from 16 to 24; anything
  that memcpy's or serialises `MotionMetrics` by size must be rebuilt.
- `scripts/build.ps1` change is a configure-time path fix only; no compiler flags moved.
- No merge, no push, no deployment was performed from this worktree.
