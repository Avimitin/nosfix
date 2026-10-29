# Judgment / scoring - Round 2 implementation report

Stream: judgment. Branch `native/r2-judgment-20261030`. Worktree
`D:\Share\nosfix\build\meka\impl-20261030b\judgment`. Baseline `b26d8c7`.
Evidence: `analyze/impl-20261030/ABI_COMPLETION.md`, `PROBE_ANALYSIS.md`,
`PRODUCER_FINDING.md`, plus the round-1 report `judgment-REPORT.md`.
No IDA, IDAPython or idalib was used, no game memory was read or written, and no
hook, detour or default-on path was added.

---

## 1. What changed

| File | Change |
|---|---|
| `include/nosfix/native_match.hpp` | +A5 song-unit section, +A1 `NativeBucketTable`/`NativeBucketSlot`/`NativeBucketEntry`/`NativeBucketCompactConfig`/`NativeBucketCompactReport`/`BucketStatus`, +A2 `NativeCandidateKey`/`NativeCandidatePrecedes`/`SortCandidateKeys`, +A4 `NativeChartFields`/`NativeNoteFields`/`DecodeNormalNote`/`SpriteOffsetForChart`; `MatchCandidate::keysound_id`, `MatchDecision::{left_key,right_key,chart_group,keysound_id,candidate_index}`, four new `MatchStatus` values, `NativeMatchPolicy::SelectSlot` |
| `src/native_match.cpp` | implementations of all of the above; `Select` refactored into `SelectCore(lane, const NativeBucketSlot*, candidates)` with identical behaviour for the R1 caller |
| `include/nosfix/native_scoring.hpp` | `kSlotOverrideVerified` (false), `SlotOverrideDecision`, `DecideSlotOverride` - a fail-closed query for the `note+84 > 1` branch. `NativeScoringState::Apply` is **unchanged** |
| `src/native_scoring.cpp` | `SlotOverrideDecisionName` only |
| `include/nosfix/native_shadow_adapter.hpp`, `src/native_shadow_adapter.cpp` | new shadow feed (`NativeShadowFeed`, `NativeShadowEvidence`, `ShadowFeedConfig`, `ShadowFeedCounts`, `ShadowFeedRecord`, `MakeMatchCandidate`) |
| `tests/native_shadow_adapter_tests.cpp` | new executable, 18 test functions |
| `CMakeLists.txt` | one library source, one test target, `add_test(native_shadow_adapter)`, one entry in the `/W4` target list |

No file outside the stream's ownership was touched: `config.cpp`, `bootstrap.cpp`,
`game_hooks.cpp`, `hooks.cpp`, `nosfix.ini`, `native_note_state.*`,
`native_note_types.*`, `panb_pressure.*`, `timed_*`, `input_*`, `audio_*` are
byte-identical.

## 2. Evidence used, item by item

### 2.1 The bucket table `0x1813DEA20` (A1)

* **Layout [bytes]**: 30 entries x 792 bytes at `0x1813DEA20`
  (`0x1813E43D8 - 29*792`); 28 slots of 28 bytes (7 dwords) at `+0..+783`; live
  count as a dword at `+784`; four filler bytes at `+788..+791`.
  Encoded as `kNativeBucketEntryCount/SlotCount/SlotBytes/EntryBytes/CountOffset/
  FillerBytes` and the seven field offsets; the tests assert
  `28 * 28 == 784`, `784 + 4 + 4 == 792` and
  `kNativeSlotRepeatOffset + 4 == 28`.
* **Fields**: `+0x00` key 1..28, `+0x04` state 1/2/3/0, `+0x08` byte value,
  `+0x14` group 1..3, `+0x18` repeat 0..24. `+0x0C`/`+0x10` are **not read by any
  function found** in the image, so they are named offsets only - no meaning is
  invented and no type member exists for them.
* **Rotation `0x180307090`**: `NativeBucketTable::RotateDown()` copies
  `entry[k] = entry[k-1]` from k = 29 down to 1 and then zeroes entry 0, i.e. the
  29-step memmove of 792 bytes plus the 792-byte zero at the base.
* **Compaction `0x180307170`**: `CompactFromStaging()` appends every staging slot
  with `state != 0`, applying the record-1 decay first (a key that record 1
  already holds with a state outside {1,2} demotes 2 -> 1 and 3 -> 0). It refuses
  unless entry 0 is empty, validates the whole 28-slot staging buffer before
  writing, and reports `staging_slots/appended/decayed_to_new/decayed_to_free`.
  `Advance()` is rotate-then-compact, the native call order at `0x18019761B`.
* **Used by the policy**: `NativeMatchPolicy::SelectSlot(table, entry, slot,
  candidates)` takes the event lane from the slot's `key` and refuses anything
  whose state is not New (1). The R1 `Select(lane, candidates)` path is
  unchanged for callers that pass their own lane.

### 2.2 Candidate ordering `0x1800CAE90` / `0x1800CB0E0` / `0x1800CA230` (A2)

`NativeCandidatePrecedes` implements the seven keys in the documented order:
frame band (0x13E496C) -> slot-1 flag -> frame -> NULL partner -> distance ->
event lane -> `note+52`. `SortCandidateKeys` is a stable bottom-up merge taking the
right run only when it precedes the left one, with a caller-supplied scratch span
(no allocation); a short scratch refuses and leaves the input untouched.

Two transcription points are recorded rather than guessed silently:

* **Key 5 direction.** A2 writes the test as "`dA > dB` -> A first" and glosses it
  "(smaller distance first)". For a float `|candidate time - event time|` the two
  cannot both hold. The gloss is implemented (`a.distance < b.distance` -> A
  first), because it is the only reading consistent with key 6 (lane ascending)
  and with the candidate arming at `0x1802F8C47`, which keeps the closest note.
  **Residual risk**: if the field at payload+0x18 were a slack (larger = closer)
  rather than a distance, the direction would flip. One live sample of two
  candidates on the same lane with different distances settles it.
* **Key 4 asymmetry.** The image tests only `otherA == NULL && otherB != NULL`, so
  a non-NULL A against a NULL B falls through to key 5 instead of ordering B
  first. That asymmetry is reproduced literally and is asserted by a test.

### 2.3 Note / chart fields (A4) and the song unit (A5)

`DecodeNormalNote` computes `note+52 = (left + right) / 2` (C truncation),
`note+56 = (float)(left + right) * 0.5f`, `note+64 = right - left`, carries
`note+84` (1..3) and fixes `note+112 = -1`, `note+204 = 1`.
`chart+0x30` (chart+48) is carried as `keysound_id`; `chart+0x64` (chart+100) maps
1 -> 6, 3 -> 7, else 0 **only** when `HIDWORD(qword_1813E4730) == 1`, which is an
explicit `gate` argument (0 in a default boot).
A5 is encoded as `kProvenSongUnitsPerSecond = 60.0`, `SongUnitsToUs`,
`UsToSongUnits` and `ChartMsToSongUnits` (`(int)(ms * 60.0 / 1000.0)`, truncation
toward zero, negative input not floored).

`note+168` is **not** derived: `sub_180301CB0(chart+48, chart+0x30)` is undecoded,
so `NativeNoteFields::keysound_index` stays 0 with `keysound_index_known == false`
and `NativeNoteOutcome::sound_id` is 0. The "the note owns only the index, the
ledger object owns the handle" rule is recorded in the header so nothing frees
through it.

### 2.4 `note+84 > 1 -> rank 5` stays fail-closed

`kSlotOverrideVerified` is `false` and nothing in this stream can flip it.
`DecideSlotOverride(event_state, grade, verified)` answers
`NotApplicable` / `Unverified` / `Applies`; the shadow feed reports `Unverified`
through `ShadowFeedCounts::slot_override_unproven` and through
`ShadowFeedRecord::slot_override_unproven`, and feeds the scoring state as the
primary slot so the unproven branch cannot fire. The R1 arithmetic in
`NativeScoringState::Apply` (byte-proven `0x180303EDC`) is untouched, so the
existing `native_scoring` tests still exercise it.

### 2.5 The shadow feed (the parent's live-adapter seam)

`NativeShadowFeed` consumes, per frame: the 28 staging slots
(`0x1813DEA10`, produced by `sub_180305B30`), the decoded note table
(`MatchCandidate` views, normally built by `MakeMatchCandidate`) and, optionally, a
caller-decoded bucket sample via `LoadBucket`. It runs rotate -> compact ->
evaluate entry 0 (one `BeginGroup` per record, matching the `v558`/`v563` reset at
`0x1802F9194`) and emits `NativeNoteOutcome` + `ScoreEffect` records:

| bucket slot | result |
|---|---|
| state 0 / 2 | nothing (idle / still held) |
| state 3 | one `KeysoundCut` outcome, no score (release only gates a keysound) |
| state 1, no candidate | `no_candidate` counter |
| state 1, linked pair | `linked` counter, never judged or scored |
| state 1, rank TooEarly | `early_ignored`, no outcome (non-consuming) |
| state 1, Good/Great/Just | `Scored` outcome, `0x303E80` effect via `NativeScoringState` |
| state 1, Missed | `Scored` outcome with rank Missed, combo reset (`late_presses`) |

Rank classification is `ClassifyNormalDelta(note_us - now - judgment_offset_us)`,
`effect_grade` is 0 Just / 1 early non-Just / 2 late non-Just, and
`scoring_enabled = ScoreGate(chart+48, false)` so `chart+48 == 2` judges without
scoring (`not_scored`).

Gates: `NativeShadowEvidence::Complete()` (six facts this round proved) **and** the
round-1 `NativeMatchEvidence` behind `NativeMatchPolicy::Configure`, and
`GroupPhaseNotEvidenced` if the compaction phase 2 is requested without
`group_phase_known`. `slot_override_known` and `group_phase_known` are the two
unproven rules; they are feature gates, not Configure gates, so they stay false by
default and are only enabled deliberately (tests).

## 3. Test command and result

```
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -RunTests
```

```
100% tests passed out of 48
```

48 = the 47 pre-existing tests (**all still passing, unmodified**) + the new
`native_shadow_adapter`. No compiler warning is emitted for any changed file under
`/W4 /permissive- /EHsc /utf-8`.

New coverage (`tests/native_shadow_adapter_tests.cpp`, 18 functions, 422 checks):
song-unit conversion and `frame = (int)(ms*60/1000)` including negative input;
bucket constants and field offsets; slot range refusals; append/full/find;
rotation by 1 and 2 frames; compaction refusals (`InvalidStaging`,
`EntryNotEmpty`, `InvalidSlot`), decay both ways, group phase on/off, repeat
re-arm at 25, `Validate` refusals, `Slot` bounds; comparator keys 1..7 including
the band, the one-sided partner test, NaN distance, and a widened band; stable
sort ordering, primary-first ordering, scratch refusal; note decode including the
odd sum, sprite gate, keysound-known flag, every refusal and `MakeMatchCandidate`
(rate, group, units); `SelectSlot` success and every refusal; the override gate
(`NotApplicable`/`Unverified`/`Applies`); feed Configure refusals for each evidence
flag, the group-phase gate, the R1 config gates and the scoring config; frame
lifecycle (no frame, non-finite, backwards, capacity, staging size); a Just press
end to end (outcome, effect, combo, history, record ring); Great/Good/Missed/
TooEarly ranks and the `chart+48 == 2` gate; held/release/no-candidate; the
fail-closed override in the feed and its enabled counterpart; shadow divergence
through `NativeShadowComparison`; captured-bucket replay and its refusal; and the
`NativeNoteState` refusal of `Native2F7C10`.

## 4. Remaining blockers (fail-closed)

1. **`note+84 > 1 -> rank 5`** - still `[live]`. `PROBE_ANALYSIS` s10 had
   `id == 1` on all 500 calls. Needs a single judgement where two notes share a
   lane inside one candidate window (a jack), so that slot 2 or 3 answers.
   `kSlotOverrideVerified` stays false until then.
2. **Compaction phase 2 window** - `[dec]`. A1 names "records 2..5
   (`0x1813DF360`, `+792 x 4`)", but `0x1813DF360` is entry 2's count field
   (entry 2 starts at `0x1813DF050`, entry 3 at `0x1813DF368`), so the window base
   is unresolved. The phase is OFF by default, and its same-key precedence inside
   one record (my `break`) is a documented reading, not a proof.
3. **Comparator key 5 direction** - see 2.2. One live sample of two candidates on
   one lane with different distances settles it. The rest of the order is
   byte-proven.
4. **Comparator key 4** is one-sided in the image; whether the reverse case
   (non-NULL A, NULL B) is meant to order B first cannot be read from the bytes.
   Reproduced as written.
5. **`note+168` / `sub_180301CB0`** - the keysound selector is undecoded, so the
   handle index is a caller input (`keysound_index_known == false` here) and
   `NativeNoteOutcome::sound_id` stays 0.
6. **Tick call rate** - A5 proves 1 unit = 1/60 s but not that the timeline update
   runs once per rendered frame. `MatchConfig::units_per_second` stays explicit.
7. **`chart+8` semantics** - the row index is proven (clamp 0..4); what the five
   rows *mean* is not. `MakeMatchCandidate` refuses a group outside 0..4 instead of
   clamping silently.
8. **`chart+56`** is carried as `chord_key` and deliberately unused.

## 5. Merge notes

* `CMakeLists.txt` adds one library source, one test target and one `/W4` list
  entry; nothing else.
* `NativeMatchPolicy::Select` keeps its R1 semantics exactly (a test-visible
  behaviour change is limited to the new `MatchDecision` echo fields).
* **Environment note (no code change)**: `scripts/build.ps1` writes
  `%TEMP%\nosfix-build-env.cmd` by fixed name. Other workers running in parallel
  on this machine overwrite it, which made one of my runs execute another
  worktree's script and fail with `'fig' is not recognized ... exit code 9009`.
  Setting `TMP`/`TEMP` to a private directory (`build\tmp`, git-ignored) before
  invoking the script makes the run deterministic. The parent may want the script
  to use `$env:TEMP\nosfix-build-env-$PID.cmd`.
* Nothing was pushed, merged or deployed; `TASK*.md` and build logs are not
  committed.
