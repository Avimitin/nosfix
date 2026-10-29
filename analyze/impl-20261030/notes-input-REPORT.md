# Stream report: non-normal note types and PANB input semantics (notes-input)

Branch `native/notes-input-20261030`, based on `8291ee4` (nosfix 0.4 baseline with
native timing components). No merge, push or deployment. No live hook, no game
memory write, no default-on seam: everything added is a portable model plus an
explicit, dormant adapter.

## 1. What changed

New files
- `include/nosfix/native_note_types.hpp` / `src/native_note_types.cpp`
  - `NativeLongState` (long/hold, 0x2F4220 + 0x2F4E80),
  - `NativeGlissState` (glissando, 0x1A60C0 + 0x1A6820),
  - `NativeTrillState` (trill, 0x1A7AF0 + 0x1A8490),
  - shared layout/constants, `ChartNoteTypeFromFlags`, `LongEligible`,
    `TrillEligible`, event/outcome/status types.
- `include/nosfix/panb_pressure.hpp` / `src/panb_pressure.cpp`
  - 28-key status-buffer decode, integer averaging, threshold gate, hold
    counter, press/release edge derivation, reset, explicit `Verified()`.
- `include/nosfix/native_input_bridge.hpp` (Windows-free glue)
  - `MakeNativeNoteEvent`, `PanbLevelFromNormalized`, `MakePanbReleaseEdge`.
- `tests/native_note_types_tests.cpp`, `tests/panb_pressure_tests.cpp`.

Modified (minimum)
- `CMakeLists.txt`: two sources in the `nosfix` target, two test executables and
  their `add_test` entries, both added to the existing `/W4 /permissive- /EHsc
  /utf-8` + `cxx_std_20` + `include` foreach.

Not touched: `native_note_state.*`, `native_gameplay.*`, `native_match.*`,
`native_scoring.*`, `nosfix.ini`, `config.*`, `bootstrap.*`, `game_hooks.*`,
`hooks.*`, `input_pipeline.*`, `input_decoder.*`, `spice_binding_adapter.*`.

## 2. Decoded semantics

### Object model (verified from the judge pseudocode)

All three classes share the normal note base (`+8` chart, `+20` lane, `+24` note
time from chart+20, `+44/+48` keys from chart+88/+92, `+52` mid key, `+68` done,
`+76` pending-early, `+80` pending rank, `+84` event value, `+120` rank result,
`+124` mode). Type-specific fields:

| field | long | gliss | trill |
|---|---|---|---|
| phase | +300 (0 first press, 1 held, other done) | none | +316 |
| end time | +296 (chart+24 frame) | n/a (single time) | +312 |
| head rank | +304 | n/a | +320 |
| effect index | +308 | +276 | +324 |
| start-commit pulse | +312 | +108 flag | +328 |
| stale arm | +313 | n/a | +329 |
| recorded delta | +320 | n/a | +332 |
| sound/event id | +316 | n/a | n/a |
| scale | +324 | n/a | +352 |
| alternate counter | n/a | n/a | +344 (init 99999) |
| group id | n/a | +272 (1 = wildcard) | n/a |
| trail/box primitive | +288 note object | +216 | +304 note object |

Chart record (104 bytes, 0x1A43E0): `+0` start ms, `+4` end ms, `+8` type,
`+12` flags, `+16` int16 lane, `+20/+24` derived frames, `+28` mid frame,
`+36/+40` sub-event count/pointer (24-byte records), `+48` state, `+52` mode,
`+64/+72` connector element pointers, `+88/+92` left/right key, `+96` commit
flag, `+97` auto flag. `ChartNoteTypeFromFlags` reproduces 0x1A4C01..0x1A4C2D:
bit 2 -> gliss (2), bit 6 -> trill (4), bit 1 -> long (1), else normal.

### Long/hold (0x2F4220, update 0x2F4E80)

- Rank from `d = note - frame - judgment offset` against the inclusive
  windows; the helper 0x305A80 returns the same 0..4 code.
- `d < good_late` (rank 1): the start is committed **and** the note finishes in
  the same call as Missed(1), scale 100; the sound is started and immediately
  faded.
- `rank 0`: nothing at all.
- `0 < d <= great_early`: deferred; `+76 = 1`, `+80 = 3 + (rank == Just)`. The
  first event or null update that reaches `note - judgment offset` commits the
  start with that recorded rank. A deferred press keeps its recorded rank even
  if a later event commits it.
- Otherwise the start commits at the actual rank and the hold begins. A commit
  by a real event draws a fresh keysound serial (10000, pre-increment, reset to
  10000 above 20000); a commit by a null update stores -1 (no sound).
- Held phase: `remaining = end - recorded - captured - judgment offset`.
  - end reached (`remaining < 1 unit`): finish with the head rank, scale 100.
  - early release or stale hold (`+313`) with `remaining > 15 units`: rank
    Good(2), scale 50, and the native counter `dword_1813E572C` increments.
  - release inside the 15-unit band: head rank, scale 100.
  - the same 15-unit/1-unit arithmetic applies to a null update, a press and a
    release; a press during the hold never re-judges.
- A finished long note fades its sound only when rank < Great; Great/Just leave
  the keysound running.
- Kind-2 (repeat) events only append to the shared id list; they never judge.

### Glissando (0x1A60C0, render 0x1A6820)

- Exactly **one** judgement per glissando. `d < good_late` -> immediate
  Missed(1)/100. Otherwise a press inside a window records Just(4) and defers to
  the note time; a press at/after the note time is judged at its own rank with
  scale 100; auto notes complete as Just(4)/100 at the note time.
- The judge starts **no** keysound; it only fades an event-owned table entry on a
  release. Keysound ownership is external.
- `note+272` must equal event+5 unless it is 1 (wildcard).
- The two connector pointers `chart+64`/`chart+72` point at other 104-byte chart
  elements (loader writes at 0x1801A4F09/0x1801A4F2D; backwards scan by key with
  `element+12 & 0x30 == 0` for the head, forward scan for the tail). They are
  render-only: 0x1A6820 appends one 120-byte box record per connector into the
  primitive at note+216 between its vtable slots +24/+32, and the judge only
  tests the pair for non-null and calls both slots again. Connector contents,
  count and identity never affect rank, scale, sound or judgement count.
- Refused: the post-note connector scan over the runtime table at 0x1813DF048
  and the 0x1813E46F8 repeat-id list (kind-2 events).
- The head connector's clip value `element+0x50` (a per-frame screen Y in
  0x1A6820's interpolation) has no known writer. It is render-only: the judge
  never reads it and 0x1A6820 tolerates `+0x50 <= 0` by skipping the clip, so the
  model never needs it and the parent can leave it inactive.
- The 24-byte sub-event array (chart+36 count, chart+40 pointer) is allocated by
  the chart loader but is never read by 0x1A60C0, 0x2F4220 or 0x1A7AF0; only the
  geometry paths use it, so it is outside this judgement model.

### Trill (0x1A7AF0, eligibility 0x1A8490)

- Head: same rank/deferral structure as long; `d < good_late` commits and
  finishes as Missed(1)/100; `0 < d <= great_early` defers as 3/4; otherwise the
  head commits (`+328`, phase -> tail) and the note-object slot +56 plus effect
  21 are called. Auto commits the head as Just(4) at the note time, no sound.
- Tail: `remaining = end - recorded - captured - judgment offset`. Completes only
  when the stale arm `+329` is set or `remaining < 1 unit`; there is no 15-unit
  band. Rank and scale come from the alternation counter at +344: positive ->
  head rank with scale 100, non-positive -> Good(2) with scale 40.
- A tail release increments the native counter and fades the sound
  unconditionally (no 0x2F4060 gate) but does not finish the tail early.
- A tail press is handed to 0x1A7910, which is not decoded (see blockers).

### PANB pressure (0x3520D0, 0x352560, 0x353600)

- Buffer: byte 0 = sample-set count N, bytes 1..2 unused, then N 16-byte records
  whose first 14 bytes hold the 28 keys; key k (1-based) is nibble `k-1`: even
  `k-1` is the high nibble, odd is the low nibble of byte `(k-1)/2`. Record
  stride 16, first record at offset 3.
- Per key: `sum` of the N nibbles, `average = sum / N` (unsigned), `level =
  average < byte_1812D3E77 ? 0 : average` (unsigned `cmp/jae`), `held =
  level ? held + 1 : 0`. Output record `{average, level, held}` (3 dwords, stride
  12, 28 records; the raw average is stored ungated).
- `byte_1812D3E77` is a configuration byte copied from `config+44` by
  `sub_1803525D0` at 0x180352687; it is BSS, so the image has no initial value
  and the live value must be sampled. Default in this model: `Unverified` ->
  `UnverifiedRule`, no level, no edge, no state change.
- Judgement consumes nothing from this path: the judge chain never calls
  0x3520D0 and 0x353600 is the LED/key-flash update. The model therefore derives
  edges from `level != 0` for consumers only and never affects a rank. This is
  asserted by a test that presses with PANB 1 and PANB 14 and gets identical
  rank, scale, sound and outcome kind.

## 3. Interface for the judgment branch

```cpp
NoteTypeStatus Configure(const NativeLongSpec&,  const NativeTypeConfig&);
NoteTypeStatus Configure(const NativeGlissSpec&, const NativeTypeConfig&);
NoteTypeStatus Configure(const NativeTrillSpec&, const NativeTypeConfig&);
NoteTypeStatus Begin(std::uint64_t generation, double position_us);
NoteTypeResult Press   (const NativeNoteEvent&, double decision_us);
NoteTypeResult Release (const NativeNoteEvent&, double decision_us);
NoteTypeResult Advance (double up_to_us, double decision_us,
                        bool hold_commit_missed = false)   // gliss: connector_scan_hit
```

- `NoteTypeResult` is a value type: status, up to 4 `NoteTypeOutcome`, and the
  counters `ignored/unsupported/repeats/penalties/cuts`. No allocation, no
  exceptions, `noexcept` throughout, fixed arrays only.
- `NoteTypeOutcome` carries `kind, rank, effect_grade, scale, sound_id,
  event_id, lane, connectors, judged, sound_started, sound_cut, at_us,
  delta_us, remaining_us, reason`. `judged == true` means "call the note's rank
  virtual (vtable slot 13 / 0x2F79E0 family) with (rank, mode, scale)". `mode` is
  the dispatcher's chord flag; it is not carried because it never affects a
  decision here.
- Events: build them with `MakeNativeNoteEvent(timed, event_id, group_id, out)`
  from `PipelineInputEvent.input` + `.song_time` (immutable capture), or
  `MakeNativeNoteEvent(timed_input, ...)` from a `TimedInputEvent`. `event_id`
  and `group_id` are opaque host values; the native producer is not decoded.
- Pressure: feed `PanbPressureState::Apply(buffer, samples, edges, counts)` with
  `InputPipeline::AnalogLevels()`-equivalent data or a decoded status buffer,
  then translate each `PanbEdge` into an event (press/release on `lane`,
  carrying `level`). `Reset(edges)` must be called on reconnect/seek/reset so a
  held key cannot stick.
- Ranks/scales/sounds are the native codes; keysound ids follow the native
  serial so the host can map them to its own sound table.

## 4. Tests and result

- `tests/native_note_types_tests.cpp`: configuration/status rejection paths
  (windows not sampled, invalid windows/offset/rate/spec, non-finite, zero and
  non-new generation, generation mismatch, watermark behind/ahead, backward
  time, skipped note); chart type precedence; eligibility short-circuit and
  inclusive union; long press/hold/end, pending early (update and press
  resolution), Good head, late Missed, too-early ignore, release gate refused,
  early release Good/50 with cut and penalty, in-band release Just/100, release
  after the end, hold-to-end without a release, stale-hold arm, auto head and
  auto finish (press and null-update paths), repeat inertness, head expiry with
  `judged == false`, sound-serial wrap; glissando press-at-note, deferred,
  connector counts 0/1/2, late Missed, auto, repeat refused and opted-in,
  connector scan refused once then silent, scan hit when proven, group-id
  wildcard/mismatch, event-owned release cut, head expiry; trill head/tail,
  alternation counter 0 vs positive, pending and late, tail press refused, tail
  release with and without completion, kind-2 inertness, head expiry, auto tail;
  chords across two lanes; outcome-capacity bound; bridge mapping.
- Cadence: one scenario (press at 1.0 s, release at 2.8 s of a 1.0..3.0 s long
  note) is replayed at 60/90/120/144/165/240/360 Hz, plain and with per-frame
  jitter plus a 700 ms stall; the outcome sequence must be exactly
  `head-committed,keysound-cut,scored` every time.
- `tests/panb_pressure_tests.cpp`: unverified refusal, nibble geometry, integer
  averaging, threshold boundaries (`>=`, unsigned), every level 0..15, native
  acceptance of 15 vs the opt-in guard, malformed/empty/insufficient-capacity
  rejection, hold counter 1..n and reset, level change without an edge, reset
  releasing held lanes and the counter restart, three-note chord edge order,
  eight rapid same-key taps, poll-rate invariance of the edge sequence, level
  independence of judgement, and agreement of `PanbLevelFromNormalized` with
  `input_decoder.hpp`'s `PanbVelocity` over 1001 samples.

Command and result (MSVC 19.44 x64, `/std:c++20 /W4 /permissive- /EHsc /utf-8
/O2 /DNDEBUG`, SDK 10.0.26100.0 through `.tools/sdk`):

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ... && cmake --build build
ctest --test-dir build --output-on-failure
-> 100% tests passed, 0 tests failed out of 41
```

41 = the 39 pre-existing tests plus `native_note_types` and `panb_pressure`.
`scripts/build.ps1 -RunTests` could not be used as shipped in this sandbox: it
fails in `enable_language(RC)` because cmake 4.4.4 writes
`build/CMakeFiles/4.4.4/CMakeRCCompiler.cmake` with unescaped backslashes in
`CMAKE_RC_COMPILER` ("Invalid character escape '\S'"). Running the identical
configure with forward-slash `-DCMAKE_RC_COMPILER`/`-DCMAKE_MT` values (and
`-DFETCHCONTENT_SOURCE_DIR_MINHOOK` pointing at the already-fetched source)
builds the same tree and runs the same tests; the failure is environmental and
reproduces on the unmodified baseline. `build/` is gitignored and not committed.

## 5. Blockers and requests

Request `coord/requests/notes-input-001.md` (non-blocking; everything below has a
fail-closed implementation and a test). No answer appeared in `coord/answers/`
during the polling window (about seven minutes over three polls, under the
ten-minute limit), so the refusals remain.

1. `sub_1802F4060` (0x2F4060), the long/trill release gate: not in the exported
   pseudocode. `NativeTypeConfig::release_gate_proven = false` makes every long
   release return `Unsupported` with no state change.
2. `sub_1801A7910` (0x1A7910), the trill tail press handler: undecoded. Tail
   presses return `Unsupported`; the alternation counter is caller-owned through
   `SetAlternationCounter`.
3. The glissando connector scan over 0x1813DF048 (and its `dword_1813E4974`
   count) and the 0x1813E46F8 repeat-id list: refused once per note / per event.
4. `byte_1812D3E77`: live value of `config+44` (the threshold) is not provable
   from the image; `PanbThresholdSource::Unverified` is the default.
5. `note+272` (glissando group id) producer: the model treats 1 as the wildcard
   observed in the compare and otherwise requires an exact event group id.
6. The native input-event producer (event+0 lane/id, +4 kind, +0x14 group id,
   +0x20 value) is not decoded; `event_id`/`group_id` stay opaque host values and
   the +0x20 value is deliberately not modelled.
7. Environment: `scripts/build.ps1` cannot configure with the sandbox's cmake
   4.4.4 RC path quoting (see section 4).

## 6. Merge notes

- Self-contained: the only shared file touched is `CMakeLists.txt`. No
  `native_*` judgment files, config, bootstrap, hooks or the input pipeline were
  edited, so this branch merges independently of the judgment stream.
- The new sources compile into `nosfix.dll` but nothing calls them; there is no
  new hook, detour, thread or default-on behaviour. Every unproven native gate
  defaults to its refusal value.
- Suggested wiring order for the parent: (a) build the note table from the chart
  loader's type flags and the 104-byte records; (b) drive `Press/Release` from
  `PipelineInputEvent` through `MakeNativeNoteEvent`; (c) call `Advance` on the
  audio-owned timeline with the certified watermark; (d) forward `judged`
  outcomes to the existing rank virtual and `KeysoundCut`/`sound_id` to the
  keysound dispatcher; (e) keep PANB edges at `Unverified` until the threshold is
  sampled live.
