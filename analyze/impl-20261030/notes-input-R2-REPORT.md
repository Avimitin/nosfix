# Round 2 stream report: note types, bucket input and PANB (notes-input)

Branch `native/r2-notes-input-20261030`, based on `b26d8c7` (main after the ABI
completion pass and the build-B probe analysis). Worktree
`build/meka/impl-20261030b/notes-input`. No merge, rebase, push or deploy. No
hook, detour, thread, game-memory read, HID access or default-on path was added:
every new entry point is a portable model that nothing calls yet.

## 1. What changed

New files
- `include/nosfix/bucket_input.hpp` / `src/bucket_input.cpp`
  - `DecodeStagingSlots`: pure decode of one 28 x 28 staging frame into 28
    `BucketSlot` values plus a `BucketFrameCounts` summary. Validates the buffer
    size, slot state (0..3), key domain (1..28 for active slots), the repeat cap
    (25) and duplicate keys; leaves a free slot uninterpreted.
  - `BucketInputState`: applies a frame with a supplied `SongTime` (the song
    timestamp *and* the generation), emits ordered `BucketInputEvent`s in native
    slot order (press / repeat / release), and tracks per-key state, observed
    presses, drops, orphans, press-while-held and generation re-basing.
  - `BucketTableState`: `Rotate` (0x307090: bucket k <- k-1 for k = 29..1, then
    bucket 0 zeroed), `Compact` (0x307170 phase 1 rebuild of bucket 0 with the
    bucket-1 decay, optional phase 2 group/repeat assignment), `Count`/`Entry`/
    `Entries`, and `ToActiveInputEntries` which hands a bucket to the existing
    pure rules as an `ActiveInputEntry` span.
- `tests/bucket_input_tests.cpp`: 369 CHECK expressions and 26 static
  assertions in 16 groups (geometry, velocity distance, slot decode, all twelve
  refusals, ordered events, orphans, generations, capacity/refusal isolation,
  repeated frames, PANB threshold, rotation, compaction state machine, group
  assignment, bucket view + both gates, bridge, allocation probe).

Modified (minimum)
- `include/nosfix/native_note_types.hpp`
  - key domain settled: `kNativeInputKeyMin/Max`, `IsNativeInputKey`,
    `NativeLaneFromKey` (lane = key - 1); `ActiveInputEntry` gains `value`
    (entry+8) and `repeat` (entry+24) as defaulted trailing fields;
  - `velocity_layout`: the proven 0x1AA340 / 0x1AA4D0 offsets (+0x128 = 296,
    +0x12C = 300, +0x134 = 308, +0x13C = 316), the store RVAs, the
    `imul eax, note+316, 180` numerator, and the explicit "flag writer not
    located" / "not a two-state flag" facts;
  - `VelocitySpanDistanceUnits` (pure `180 * span / rate^2`, refusing a
    non-finite or non-positive rate);
  - the `+296/+300/+316` block rewritten as shared base storage (gliss must not
    model them, long takes its end from the caller, velocity keeps +296
    fail-closed), and the held-input section now cites the published staging
    pair instead of an "undecoded producer".
- `src/native_note_types.cpp`: `VelocitySpanDistanceUnits` definition; the two
  eligibility comments now say which type owns the +300 / +316 reading.
- `include/nosfix/panb_pressure.hpp`: `PanbConfigFromConfigByte` - the explicit
  opt-in for the proven `config+0x2C` -> `byte_1812D3E77` writer. The default
  stays `PanbThresholdSource::Unverified`.
- `include/nosfix/native_input_bridge.hpp`: `MakeNativeNoteEvent(const
  BucketInputEvent&, NativeNoteEvent&)` (kind, lane, event_id = key, group_id,
  panb_level = the raw staging value, song time).
- `CMakeLists.txt`: `src/bucket_input.cpp` in the `nosfix` target and the
  `nosfix_bucket_input_tests` target + `add_test(NAME bucket_input ...)`, both
  inside the existing `/W4 /permissive- /EHsc /utf-8` + `cxx_std_20` blocks.

Not touched: `native_note_state.*`, `native_gameplay.*`, `native_match.*`,
`native_scoring.*`, `timed_*`, `audio_*`, `nosfix.ini`, `config.*`,
`bootstrap.*`, `game_hooks.*`, `hooks.*`.

## 2. Evidence used

| Fact | Source |
|---|---|
| staging pair `xmmword_1813DEA10 = {a2, a3}`, a2 = 28x28 buffer (obj+8), a3 = companion (obj+0x31C); publisher `sub_180305B30` at 0x180305C6C; four call sites | PRODUCER_FINDING |
| bucket table 30 x 792 at 0x1813DEA20, entry 28 slots x 28 bytes + count at +784 + 4 filler; entry access `792 * dword_181366910`; selected index constant 0 | ABI_COMPLETION A1 |
| slot fields: +0 key 1-based 1..28 (`dw0-1` clamped 0..27), +4 state (1 new / 2 held / 3 release / 0 free), +8 byte value, +0x14 group, +0x18 repeat (0..24, re-armed 1 at 25); +0x0C/+0x10 unread | ABI_COMPLETION A1, PROBE_ANALYSIS s1 |
| +0x20 is **not** a field of a 28-byte slot (it is the next slot's key) | ABI_COMPLETION A1/B6, PROBE_ANALYSIS s1 |
| rotation 0x307090: 29 memmoves of 792 down, then bucket 0 zeroed | ABI_COMPLETION A1 |
| compaction 0x307170: phase 1 decay 2->1 / 3->0 against bucket 1 when its state is outside {1,2}; phase 2 group/repeat from buckets 2..5 (`key == k`, `k+1`, `k-1`, 0 forced to 1), gated on `byte_1813E4A8E == 1` | ABI_COMPLETION A1, B7 |
| frame semantics: press 1, held/repeat 2 with counter, release 3 then the slot is dropped, chords share a frame; group always 0 in staging; value 0..15 (255 on one idle slot) | PROBE_ANALYSIS s11 |
| gliss connector scan starts at bucket 1's count 0x1813DF048 / table 0x1813DEA20 and matches `note+272` (1 = wildcard) | ABI_COMPLETION A1/B7, PROBE_ANALYSIS s12 |
| release gate 0x2F4060 filters states {1,2} in `[left - dword_1813E4A84, right + dword_1813E4A84]`, inserts keys into 0x1813E46F8 and sets note+84 = 1 inside the span | ABI_COMPLETION A1/B7 |
| entry keys are 1-based and chart+88/+92 are 1-based (closes round 1's ±1 blocker) | ABI_COMPLETION A1, PROBE_ANALYSIS s1 |
| +296/+300/+316 are shared base storage: long, trill and velocity reference them, no gliss function does; `+300` is not a 0/1 flag (-1/30/69/72); `+316` frame span used as `180*frames/rate^2`; +296 writer unlocated | ABI_COMPLETION C11, PROBE_ANALYSIS s8/s9 |
| `byte_1812D3E77 = config+0x2C` written only by 0x3525D0 at 0x180352687, gate `config+0x25` | ABI_COMPLETION B8 |

## 3. Decoded semantics now in code

- A frame is 28 slots of 28 bytes with no count (fixed bound 784). Events are
  emitted in slot order, i.e. the order 0x307170 walks, so a chord keeps the
  producer's slot order.
- press = state 1 (`NativeEventKind::Press`), held = state 2 with the producer's
  repeat counter (`Repeat`), release = state 3 (`Release`), free = 0 (no event).
  A state-3 slot is present for exactly one frame; the next frame's absent key is
  counted as a `drop` instead of invented as an event.
- A held/release state whose press was never observed is reported as `orphan`
  (the native tolerates it) and only refused when `refuse_orphans` is set.
- Keys are validated as 1..28 for active slots and mapped to lane = key - 1;
  two active slots naming one key are refused (`DuplicateKey`), because one key
  cannot produce two ordered events in one frame.
- Generation: zero is refused; an older generation is `StaleGeneration`; a newer
  generation re-bases the per-key tracking (so a key still held after a seek is
  an orphan repeat rather than a phantom press) unless
  `keep_tracking_across_generations` is set. Song time may not go backwards
  inside a generation; equal timestamps are accepted (a host may publish one
  frame twice, which the native would see as two repeat frames).
- Compaction phase 1 is unconditional and uses bucket 1 exactly as the native
  does; phase 2 is opt-in (`assign_groups`) and documents its unproven match
  priority. With phase 2 off, entries keep the staging group (0) and are counted
  in `GroupPending()` instead of guessing.
- Nothing here allocates, locks, logs or blocks (a test wraps the whole
  configure/decode/compact path in the allocation probe and requires 0).

## 4. Tests

```
scripts/build.ps1 -RunTests
100% tests passed out of 48
```

48 = the 47 tests that existed at `b26d8c7` (all still passing, unchanged) plus
`bucket_input`. Compiler: MSVC 19.44.35229.0 x64, Release, `/W4 /permissive-
/EHsc /utf-8`, no warnings from the new or modified translation units.

Build note (environment, not a code change): `scripts/build.ps1` writes its
temporary `nosfix-build-env.cmd` into `%TEMP%`, which the four sibling workers
share. With three workers building at once the file is rewritten mid-run and the
wrapper aborts after a successful build with
`'rrorlevel' is not recognized ... nosfix build failed with exit code 1`; the
first run of this worktree hit exactly that. The script itself is unmodified and
passes when `TMP`/`TEMP` are scoped to this worktree before invoking it:

```powershell
$env:TEMP = "<worktree>\build\cltmp"; $env:TMP = $env:TEMP
powershell -File scripts\build.ps1 -RunTests   # 48/48
```

The same flags (`-DCMAKE_RC_COMPILER` / `-DCMAKE_MT` with forward slashes, NMake
Makefiles, `-DCMAKE_BUILD_TYPE=Release`) were also driven directly from a
worktree-local `.cmd` with the identical result.

## 5. Remaining blockers (exact)

1. **Staging filler.** `sub_180305B30` publishes the pair and four call sites
   pass `obj+8` / `obj+0x31C`, but the code that fills the buffer writes through
   a register base, so it is still unnamed. The decoder therefore takes the
   buffer as an argument and never locates it.
2. **Slot +0x0C / +0x10.** Never read in this build; they are skipped and given
   no meaning (a test proves that changing them changes nothing).
3. **Compaction phase 2 details.** The per-bucket match priority, the tie between
   `key+1` and `key-1`, and whether older entries of any state participate are
   not proven from bytes. Implemented as documented, opt-in, default off.
   Until it is on and checked live, every compacted group is 0 and the glissando
   connector scan can only take its `note+272 == 0` wildcard path.
4. **Value byte provenance.** It matches the averaged PANB nibble (0..15), but
   nothing proves it is the threshold-gated level. It is carried verbatim and
   never turned into a press/release edge; `PanbPressureState` keeps
   `PanbThresholdSource::Unverified` as its default.
5. **Live `config+0x2C`.** The writer and offset are proven; the value is a
   runtime configuration byte, so `PanbConfigFromConfigByte` requires the caller
   to pass it.
6. **Velocity +296.** Its writer is unlocated in the whole image and `+300` is
   not a 0/1 flag; only the layout and the distance arithmetic are recorded, no
   velocity judgement is modelled and glissando never derives the three fields.
7. **`note+84` / event `+0x20`** (single key vs chord) and **`note+272`'s own
   1/2/3 derivation** (0x1A7450, decompiler-only) remain caller-supplied /
   unmodelled, unchanged from round 1.
8. **Runtime tolerance.** `dword_1813E4A84` is written by chart/option code
   (`sub_180277360`, `sub_1803049C0`, `sub_180305B30`), so the release gate still
   needs the live value from whoever owns the option state.

## 6. Merge notes

- Self-contained: three new files plus five edited files (`CMakeLists.txt`,
  `native_note_types.hpp/.cpp`, `panb_pressure.hpp`,
  `native_input_bridge.hpp`). `src/panb_pressure.cpp` holds no change at all, so
  no source file outside that list was touched.
  Nothing else in the tree differs from `b26d8c7`.
- API compatibility: `ActiveInputEntry` only gains defaulted trailing fields, so
  the round-1 aggregate initialisations `{key, state, group}` still compile; no
  existing function signature changed. `git grep` finds no other stream using
  the touched symbols.
- No behaviour change in the existing note-type models: the diff is comments,
  constants and one new pure helper. The 47 pre-existing tests are untouched and
  pass.
- Suggested wiring order for the parent: (a) read the pair at `0x13DEA10` in the
  note-manager frame update and call `BucketInputState::Apply` with the song
  clock snapshot; (b) forward each event through `MakeNativeNoteEvent` into the
  long/gliss/trill states; (c) run `Rotate` then `Compact` once per frame and
  feed `ToActiveInputEntries(bucket 1, ...)` to `FindConnectorKey` and
  `ToActiveInputEntries(bucket 0, ...)` to `EvaluateReleaseGate`; (d) leave
  `assign_groups`, `refuse_orphans` and PANB `RuntimeSampled` off until the
  corresponding live sample exists.
