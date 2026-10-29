# Stream report: non-normal note types and PANB input semantics (notes-input)

Branch `native/notes-input-20261030`, based on `8291ee4` (nosfix 0.4 baseline with
native timing components). No merge, push or deployment. No live hook, no game
memory write, no default-on seam: everything added is a portable model plus an
explicit, dormant adapter.

Round 2 applies `coord/answers/notes-input-001.md` (Q1-Q5 verified; Q6 partial):
the long release gate, the trill tail handler, the glissando connector scan and
the PANB threshold provenance are now implemented instead of refused. Q6's event
producer stays refused.

## 1. What changed

New files
- `include/nosfix/native_note_types.hpp` / `src/native_note_types.cpp`
  - `NativeLongState` (long/hold, 0x2F4220 + 0x2F4E80),
  - `NativeGlissState` (glissando, 0x1A60C0 + 0x1A6820),
  - `NativeTrillState` (trill, 0x1A7AF0 + 0x1A8490 + 0x1A7910),
  - `EvaluateReleaseGate` (0x2F4060) and `FindConnectorKey` (0x1A60C0 scan),
    pure functions over a caller-supplied `ActiveInputEntry` span,
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
  their `add_test` entries, both in the existing `/W4 /permissive- /EHsc /utf-8`
  + `cxx_std_20` + `include` foreach. Round 2 changed no build file.

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
| countdown | n/a | n/a | +344 (99999 at commit, 25 per tail hit) |
| sub-element cursor | n/a | n/a | +348 (chart+36 count, chart+40 array) |
| group id | n/a | +272 (1 = event wildcard, 0 = scan wildcard) | n/a |
| trail/box primitive | +288 note object | +216 | +304 note object |

Chart record (104 bytes, 0x1A43E0): `+0` start ms, `+4` end ms, `+8` type,
`+12` flags, `+16` int16 lane, `+20/+24` derived frames, `+28` mid frame,
`+36/+40` sub-event count/pointer (24-byte records: `+0/+4` bytes, `+8` start
frame, `+12` end frame, `+16` qword cue), `+48` state, `+52` mode, `+64/+72`
connector element pointers, `+88/+92` left/right key, `+96` commit flag, `+97`
auto flag. `ChartNoteTypeFromFlags` reproduces 0x1A4C01..0x1A4C2D: bit 2 -> gliss
(2), bit 6 -> trill (4), bit 1 -> long (1), else normal (0). That type value is
also the score-table group in 0x303E80 and suppresses the effect for 1 and 4.

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
  - the same arithmetic applies to a null update, a press and a release; a press
    during the hold never re-judges.
- **Release gate 0x2F4060 (Q1, new in round 2).** For each entry of the held
  input table with state 1 or 2 (held): if its key is inside
  `[left - dword_1813E4A84, right + dword_1813E4A84]` the gate appends the key to
  the intrusive list at 0x1813E46F8 and returns 1; a key strictly inside
  `[left, right]` also sets `note+84 = 1`. The release only proceeds when the
  gate returns 0. `dword_1813E4A84` initialises to 3 and is chart/option driven;
  `dword_181366910`, the bucket index, is a constant 0. Because 0x303E80 forces
  rank 5 only for `note+84 > 1`, the gate's `1` cannot upgrade a grade. The
  trill tail release is **not** gated.
- A finished long note fades its sound only when rank < Great; Great/Just leave
  the keysound running. Kind-2 (repeat) events only append to the shared list.

### Glissando (0x1A60C0, render 0x1A6820)

- Exactly **one** judgement per glissando. `d < good_late` -> immediate
  Missed(1)/100. Otherwise a press inside a window records Just(4) and defers to
  the note time; a press at/after the note time is judged at its own rank with
  scale 100; auto notes complete as Just(4)/100 at the note time.
- The judge starts **no** keysound; it only fades an event-owned table entry on a
  release. Keysound ownership is external.
- `note+272` must equal event+20 unless it is 1 (wildcard). 0x1A7450 writes
  1/2/3 from a chart-key/midpoint comparison; that derivation is decompiler-only,
  so the caller supplies the value (default 1).
- **Connector scan (Q3, new in round 2).** With no press and `d <= 0`, the judge
  walks the held-input table at 0x1813DEA20 (30 buckets of 28 entries x 28 bytes
  plus a count at +784; entry `+0` key, `+4` state, `+20` group) starting at
  bucket 1's count field 0x1813DF048. The first entry whose key is inside
  `[chart+88, chart+92]` and whose `+20` equals `note+272` (or `note+272 == 0`)
  matches. The listing sets `note+80 = 4`, clears `note+76` and `note+84`, and
  writes `chart+0x50 = note+32` (render-only): because `note+76` ends at 0 the
  note is judged at the **captured delta's rank**, not as Just. This scan does
  not test the entry state.
- The two connector pointers `chart+64`/`chart+72` point at other 104-byte chart
  elements (loader writes at 0x1801A4F09/0x1801A4F2D). They are render-only:
  0x1A6820 appends one 120-byte box record per connector into the primitive at
  note+216 between its vtable slots +24/+32, and the judge only tests the pair
  for non-null and calls both slots again.
- The head connector's clip `element+0x50` is written per frame at 0x1801A647C
  from the note's own projected Y; 0x1A6820 tolerates `+0x50 <= 0`, so the model
  never needs it.

### Trill (0x1A7AF0, eligibility 0x1A8490, tail handler 0x1A7910)

- Head: same rank/deferral structure as long; `d < good_late` commits and
  finishes as Missed(1)/100; `0 < d <= great_early` defers as 3/4; otherwise the
  head commits (`+328`, phase -> tail, `note+344 = 99999`) and the note-object
  slot +56 plus effect 21 are called. Auto commits the head as Just(4) at the
  note time with no sound.
- Tail: `remaining = end - recorded - captured - judgment offset`. Completes only
  when the stale arm `+329` is set or `remaining < 1 unit`; there is no 15-unit
  band. Rank and scale come from the `+344` countdown: positive -> head rank with
  scale 100, non-positive -> Good(2) with scale 40. A tail release increments the
  native counter and fades the sound unconditionally but does not finish early.
- **Tail handler 0x1A7910 (Q2, new in round 2).** A kind-1 tail press does not
  judge. It re-triggers the sound of the chart sub-element at
  `chart+40 + 24*note+348`: `drive = (chart+12 & 8) == 0` selects the event id,
  otherwise a fresh rolling serial; `len = elem+12 - elem+8`, clamped to 1 when
  not driven and non-positive; the call is
  `0x304F30(id, elem+16, elem+0, elem+4, len, 0)`. It then increments
  `note+348`, wraps at `chart+36`, sets `note+344 = 25` and restarts the visual
  (draw object, loop clip). The 25-frame countdown is what makes a tail that has
  not alternated for 25 frames score Good/40.
- The 24-byte sub-element array (`chart+36` count, `chart+40` pointer) is
  therefore part of the model: `NativeTrillSpec` carries up to 32 elements and an
  over-long count is rejected at Configure.

### PANB pressure (0x3520D0, 0x352560, 0x353600)

- Buffer: byte 0 = sample-set count N, bytes 1..2 unused, then N 16-byte records
  whose first 14 bytes hold the 28 keys; key k (1-based) is nibble `k-1`: even
  `k-1` is the high nibble, odd is the low nibble of byte `(k-1)/2`. Record
  stride 16, first record at offset 3.
- Per key: `sum` of the N nibbles, `average = sum / N` (unsigned, unclamped
  0..15), `level = average < byte_1812D3E77 ? 0 : average` (unsigned `cmp/jae`),
  `held = level ? held + 1 : 0`. Output record `{average, level, held}` (3
  dwords, stride 12, 28 records; the raw average is stored ungated).
- `byte_1812D3E77` (Q4, corrected in round 2) **is written** by `sub_1803525D0`
  at `0x180352687` with the x64 absolute-address form
  `mov cs:byte_1812D3E77, cl` after `movzx ecx, byte ptr [rax+2Ch]`: the value is
  `config+0x2C`, and the writer returns before the store when
  `*(config+0x25) == 0`. A RIP-relative sweep over `.text` cannot see a moffs
  store, which is why round 1 concluded "no writer". Before the config runs the
  byte is the loader's zero fill (the range is not file-backed: .data virtual
  0x4455AAC against 0x1A200 raw), while an analysis database shows 0xFF there as
  its uninitialized filler; the two readings have opposite effects (0 gates
  nothing, 255 gates everything), so the model still takes the value from the
  caller and documents both constants.
- Judgement consumes nothing from this path: the judge chain never calls
  0x3520D0, and 0x353600 is the LED/key-flash update. The model therefore derives
  edges from `level != 0` for consumers only and never affects a rank. A test
  asserts that presses with PANB 1 and PANB 14 give identical outcomes.

## 3. Interface for the judgment branch

```cpp
NoteTypeStatus Configure(const NativeLongSpec&,  const NativeTypeConfig&);
NoteTypeStatus Configure(const NativeGlissSpec&, const NativeTypeConfig&);
NoteTypeStatus Configure(const NativeTrillSpec&, const NativeTypeConfig&);
NoteTypeStatus Begin(std::uint64_t generation, double position_us);
NoteTypeResult Press   (const NativeNoteEvent&, double decision_us,
                        std::span<const ActiveInputEntry> held = {});   // long
NoteTypeResult Release (const NativeNoteEvent&, double decision_us,
                        std::span<const ActiveInputEntry> held = {});   // long
NoteTypeResult Advance (double up_to_us, double decision_us,
                        bool hold_commit_missed = false);                // long/trill
NoteTypeResult Advance (double up_to_us, double decision_us,
                        std::span<const ActiveInputEntry> entries = {}); // gliss
```

- `ActiveInputEntry { key, state, group }` is the caller's view of the 28-entry
  input table; both gates read it only when `release_gate_available` /
  `connector_scan_available` is set, so the default is still fail-closed.
- `NoteTypeResult` is a value type: status, up to 4 `NoteTypeOutcome`, and the
  counters `ignored/unsupported/repeats/penalties/cuts/refused_releases`. No
  allocation, no exceptions, `noexcept` throughout.
- Outcome kinds include `ReleaseRefused` (with `gate_key`, `gate_note84`,
  `gate_appended`) and `TrillTailHit` (with `sub_element`, `cue_reference`,
  `length_frames`, `countdown`). `judged == true` means "call the note's rank
  virtual (vtable slot 13 / 0x2F79E0 family) with (rank, mode, scale)"; `mode` is
  the dispatcher's chord flag and is not carried because it never affects a
  decision here.
- Events: `MakeNativeNoteEvent(song_time, input, event_id, group_id, out)` from
  `PipelineInputEvent.input` + `.song_time`, or from a `TimedInputEvent`.
  `event_id` is the native key at event+0; `group_id` is the event's group
  (0x1A60C0 reads a dword at +20; the answer reports a byte at +5 - the value
  stays opaque).
- Pressure: feed `PanbPressureState::Apply(buffer, samples, edges, counts)`, then
  translate each `PanbEdge` into an event carrying `level`. `Reset(edges)` must be
  called on reconnect/seek/reset so a held key cannot stick.

## 4. Tests and result

- `tests/native_note_types_tests.cpp`: configuration/status rejection paths;
  chart type precedence; eligibility short-circuit and inclusive union;
  `EvaluateReleaseGate` (empty table, state filtering, widened span boundaries,
  inside-span `note+84`, negative tolerance, match count) and `FindConnectorKey`
  (span, group wildcard 0, group mismatch, state ignored, no match); long
  press/hold/end, pending early (update and press resolution), Good head, late
  Missed, too-early ignore, gate refused without a table, release with a held key
  in the span (refused, `gate_key`/`gate_note84`/count, state unchanged, later
  release completes), early release Good/50 with cut and penalty, in-band release
  Just/100, release after the end, hold-to-end without a release, stale-hold arm,
  auto head and auto finish, repeat inertness, head expiry, sound-serial wrap;
  glissando press-at-note, deferred, connector counts 0/1/2, late Missed, auto,
  repeat refused and opted-in, scan refused once then silent, scan hit at the
  delta rank with `connector_key` (inside span, group mismatch, group-0
  wildcard), group-id wildcard/mismatch, event-owned release cut, head expiry;
  trill head/tail, countdown 0 vs positive, pending and late, tail press refused
  then the decoded tail hit (sub-element cursor advance and wrap, event-id vs
  rolling serial, cue reference, length clamp, 25 re-arm), empty and over-long
  sub-element arrays, tail release with and without completion, kind-2
  inertness, head expiry, auto tail; chords; capacity bound; bridge mapping.
- Cadence: one scenario (press at 1.0 s, release at 2.8 s of a 1.0..3.0 s long
  note) is replayed at 60/90/120/144/165/240/360 Hz, plain and with per-frame
  jitter plus a 700 ms stall; the outcome sequence must be exactly
  `head-committed,keysound-cut,scored` every time.
- `tests/panb_pressure_tests.cpp`: unverified refusal, nibble geometry, integer
  averaging, threshold boundaries (`>=`, unsigned), every level 0..15, native
  acceptance of 15 vs the opt-in guard, threshold 255 (database filler: all
  gated) vs 0 (pre-config: none gated) vs a mid-range value, malformed/empty/
  insufficient-capacity rejection, hold counter and reset, level change without
  an edge, reset releasing held lanes, chord edge order, eight rapid same-key
  taps, poll-rate invariance, level independence of judgement, and agreement of
  `PanbLevelFromNormalized` with `input_decoder.hpp`'s `PanbVelocity`.

Command and result (MSVC 19.44 x64, `/std:c++20 /W4 /permissive- /EHsc /utf-8
/O2 /DNDEBUG`, SDK 10.0.26100.0 through `.tools/sdk`):

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ... && cmake --build build
ctest --test-dir build --output-on-failure
branch:       100% tests passed out of 41   (39 branch-baseline + my 2)
merged tree:  100% tests passed out of 46   (main's 44 other targets + my 2)
```

The 46-test figure was verified without merging: `git archive main` was extracted
into a scratch directory inside this worktree, the seven changed source/test files
were overlaid, and the resulting tree was configured, built and tested. No other
stream references the changed API (`git grep` over `main` finds only my own
files), so the API changes cannot break the merge.

`scripts/build.ps1 -RunTests` still cannot configure in this sandbox: cmake 4.4.4
writes `CMAKE_RC_COMPILER` into `build/CMakeFiles/4.4.4/CMakeRCCompiler.cmake`
with unescaped backslashes and fails in `enable_language(RC)` ("Invalid character
escape '\S'"), which reproduces on the unmodified baseline. The identical
configure with forward-slash `-DCMAKE_RC_COMPILER`/`-DCMAKE_MT` (and
`-DFETCHCONTENT_SOURCE_DIR_MINHOOK` pointing at the already-fetched source)
builds the same tree and runs the same tests.

## 5. Blockers and requests

Request `coord/requests/notes-input-001.md`, answered as
`coord/answers/notes-input-001.md` (Q1-Q5 verified from bytes, Q6 partial). What
remains blocked:

1. The input-event producer (Q6). The consumer-side layout is proven (event+0 key,
   +4 kind, +20 group), and the descriptors live in 64-byte records in a vector at
   the note-manager `+1168`, but nothing in the export set pushes into it. The
   model keeps `event_id`/`group_id` caller-supplied; no producer is guessed.
2. The writer of `entry+20` (the group tag) and the exact tick that decrements
   `note+344`: both live in the same unlocated producer/tick. `SetTailCountdown`
   is the explicit hook for the tick.
3. `dword_1813E4A84` (release tolerance) is chart/option driven; the model defaults
   to the initialiser's 3 and takes a caller override.
4. The entry-key domain: the answer reports entry keys as 0..27 and chart+88/+92 as
   1-based; `EvaluateReleaseGate`/`FindConnectorKey` compare raw values and never
   convert, so the ±1 question must be settled before live wiring.
5. `note+272`'s 1/2/3 derivation in 0x1A7450 is decompiler-only; the caller
   supplies the value.
6. The live value of `config+0x2C` (the PANB threshold): `PanbThresholdSource::
   Unverified` is the default, and both documented pre-config readings (0 and
   0xFF) are covered by tests rather than chosen.
7. Environment: `scripts/build.ps1` RC-path quoting under cmake 4.4.4.

## 6. Merge notes

- Self-contained: the only shared file touched is `CMakeLists.txt` (round 1). No
  `native_*` judgment files, config, bootstrap, hooks or the input pipeline were
  edited, so this branch merges independently of the judgment stream.
- The new sources compile into `nosfix.dll` but nothing calls them; there is no
  new hook, detour, thread or default-on behaviour. Every unproven native gate
  defaults to its refusal value.
- API changes since the merged snapshot: `release_gate_proven` ->
  `release_gate_available` (+ `release_gate_tolerance_keys`),
  `gliss_connector_scan_proven` -> `connector_scan_available`,
  `NativeGlissState::Advance(bool)` -> `Advance(span<const ActiveInputEntry>)`,
  `SetAlternationCounter`/`AlternationCounter` ->
  `SetTailCountdown`/`TailCountdown`. No other stream references them.
- Suggested wiring order: (a) build the note table from the chart type flags and
  the 104-byte records, including the trill sub-element array; (b) drive
  `Press/Release` through `MakeNativeNoteEvent`, passing the held-input entry
  span when the producer is available; (c) call `Advance` on the audio-owned
  timeline with the certified watermark and the ticked `note+344`; (d) forward
  `judged` outcomes to the existing rank virtual and `KeysoundCut`/`sound_id`/
  `TrillTailHit` to the keysound dispatcher; (e) keep PANB edges at `Unverified`
  until the threshold is read from `config+0x2C` or the live byte.
