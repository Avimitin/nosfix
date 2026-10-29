# Native judgment seam - implementation report

Stream: judgment. Branch `native/judgment-20261030`. Worktree
`D:\Share\nosfix\build\meka\impl-20261030\judgment`.
Evidence base: nostalgia.dll PAN-2025020501, SHA256
`339247b9...d5638b`, image base `0x180000000`. Every RVA below is module-relative.

Coordination: request **judgment-001** -> answer **judgment-001** (partial) plus proactive
packs `coord/evidence/judgment-2f7c10.md` and `coord/evidence/judgment-303e80.md`.
No IDA, IDAPython or idalib was used by this stream; only the exported pseudocode,
the answer and the packs.

---

## 1. Decoded model: `0x1802F7C10` matching

### 1.1 Coordinates (judgment-001 items 1, 4, 6; pack 2f7c10 s1)

* 28 key columns, **1-based**: key `k` left edge `0x13E4738 + (k-1)*0x13E476C`, pitch
  `2*(640-0x13E4738)/28`. Key 0 is not a key.
* Three lane arrays are 29 entries (index 0 idle, 1..28 live):
  * `0x13E4978` - lane history, `{int32 stamp, int32 value}` x 29, zeroed by
    `sub_180102B80(0x13E4978, 0, 232)`.
  * the two `v558` / `v563` byte flags, cleared by
    `for (i = 0; i <= 28; ++i) { v558[i] = 0; v563[i] = 0; }` (`0x1802F9194`, with a
    `_report_rangecheckfailure` bound at 0x1D).
* `0x13E4968 = 3`, `0x13E496C = 1`, `0x13E4A60 = 4`, `0x13E4974 = 10` have **exactly one
  writer each** (`sub_1800CB9A0`, `0x1800CC2C4..0x1800CC2E9`); they are constants of this
  build, not frames or lane counts.
* `0x13E5724` (float) and `0x13E4A84` are **not** written by the exported initializer;
  they are layout/config values and are caller-sampled (`MatchConfig.key_spread`,
  `lane_over_filter`, gated by `layout_sampled`).

### 1.2 Pair records and candidate slots (judgment-001 item 2)

Per input event the traversal collects up to three candidate notes into an intrusive list
(`sub_1800CA9A0` push, `sub_1800CA930` first, `sub_1800CAE90` sort). A record is 64 bytes:

| offset | field |
|---|---|
| +0x10 | note object pointer (own vtable, `0x684328` for normal notes) |
| +0x18 | event descriptor: `+0` lane (1..28), `+4` kind (1 press, 3 release) |
| +0x20 | **candidate slot index 1..3** - the value later stored into `note+84` |
| +0x28 | `slot == 1` |
| +0x30 | float `|candidate key - event lane|` |
| +0x38 | partner note pointer, or NULL |

The three slots live at `note_block+1216 + 24*(slot-1)`; `+8` is the candidate key float and
`+16` the "occupied" byte. Slot selection (`0x1802F8C47..0x1802F8ED7`):

```
lo = note+44; hi = note+48; span = hi - lo
if (span >= 4) lo += (int)(0x13E5724 * -0.5f)
if (span >= 3) hi += (int)(0x13E5724 * 0.5f - 0.9f)
lo' = (chart+8 == 2) ? lo : lo - 0x13E4A84
hi' = (chart+8 == 2) ? hi : hi + 0x13E4A84
drop the candidate if entry_lane < lo' or > hi'
slot = entry_lane < lo ? 2 (left) : entry_lane > hi ? 3 (right) : 1 (primary)
keep the candidate with the smallest (note+24 - now); ties -> smaller note+56
```

The slot index is `1..3`, and it is what `vtable+0xE8` stores into `note+84`
(`if (note+84 != 1) note+84 = slot`). `note+84 > 1` therefore means *"answered from a
non-primary slot"*, which is exactly the `grade := 5, score := 100` override in
`0x180303E80` (see s2).

### 1.3 The `v558` / `v563` decisions (pack 2f7c10 s3)

For each record, in candidate order:

```
lane = *(int*)event_descriptor                       // the EVENT lane, not the note lane
horizon_ok = sub_180303BA0(note+24, lane)
if (note+68 != 0 || (!horizon_ok && !linked_open))   skip
if (v558[lane] != 0)                                 skip     // lane already judged this batch
if (note+76 == 1 && slot != 1)                       skip     // pending early needs the primary slot
if (linked) { v563[lane] = 1; note+84 = slot; note+96/104 = partner, 3;
              sub_180303C10(note+24, lane); skip }           // never judged
if (v563[lane] != 1) {
    note+84 = slot; v558[lane] = 1;
    ok = note->vtable[4](note, event, chord)             // 0x1802F6800
    if (ok || note->vtable[0xD8]() != 0) { if (kind == 1)
        sub_180303C10(chart+8 == 4 ? now + 9 : note+24, lane); }
}
```

* `v558` = one judged hit per event lane per batch (so one input sample cannot score two
  notes on a lane).
* `v563` = one qualifying event per lane per batch; it gates both the linked branch and the
  human judgement branch.
* `chord` is `count(pending notes with the same chart+56) >= 3`, used only when
  `HIDWORD(qword_1813E4730) == 1` (0 in a default boot).

### 1.4 Horizon helpers (judgment-001 item 1, pack 2f7c10 s4.2/4.3)

```
sub_180303BA0(note_frame, lane) -> bool:
    if (0x13E4968 < 0) return true
    p = table29[lane]
    v5 = p.stamp <= 0 ? 0 : p.value
    if (note_frame > 0x13E4968 + 0x13E496C + v5 && p.stamp > 0 && p.value > 0) return false
    return true
```
i.e. **a candidate whose frame is more than 4 frames past the frame already recorded for
that lane is not admitted**. Call sites pass `a1 = note+24` (the *note frame*, not a lane) and
`a2 = the event lane`.

```
sub_180303C10(value, lane):
    for k = lane .. lane+4 (bounded by 0x13E4A5C = base+228): table29[k] = {3, value}
    for k = lane-1 .. lane-4 (bounded by 0):                   table29[k] = {3, value}
```
Call sites: linked branch -> `note+24`; after a judged press -> `chart+8 == 4` ? `now + 9`
(`0x136690C + 0x1366918 + 0x18070A610 + 0x13668E0`) : `note+24`. `sub_180303B50` is only a
4-slot debug ring (`+6720`, head `+6736`) with **no timing state**.

### 1.5 Module boundaries in the traversal

`sub_1800CA1C0` is *not* the record builder: it initialises the three (four) 24-byte
candidate slots of the current note block to `{source, 2000.0f, 2000.0f, 0}`.

---

## 2. Decoded model: `0x180303E80` scoring / effects

Single caller: `0x1802F7A69` inside `sub_1802F79E0` (`vtable+0x68`), reached only from
`0x1802F6800`. Signature and argument meaning (proven at both ends):

```
sub_180303E80(id, unused, rank, unused, note_y, width_keys, chart, value, can_judge)
  id        = note+84   (candidate slot that answered, 1..3)
  rank      = the window grade 0..4
  note_y    = note+32, width_keys = note+64 (key span)
  chart     = note+8    (104-byte chart record)
  value     = 100 at every call site
  can_judge = (chart+48 != 2) && (chart+97 == 0)
```

Order of effects, exactly as coded:

1. `can_judge == 0` -> return, **no state change** (this is why an auto-play note neither
   scores nor breaks the combo).
2. `if (id > 1 && rank > 1) { value = 100; rank = 5; }` - the non-primary-slot override.
3. Combo: `combo = clamp(combo + 1, max(stored, ...), 0..9999)` for rank 2..5; **forced to 0**
   for rank 0 or 1. Running maximum in a 16-bit field at `player+20`.
4. Grade histogram at `player+22` (six 16-bit counters): `if (rank) --slot0 (floor 0)`, then
   `++slot[clamp(rank,0,5)]` (ceiling 9999).
5. Score table `player+36`, `int32[5][6]`, row `g = clamp(chart+8, 0, 4)`:
   `val = rank <= 1 ? 100 : clamp(value, 0, 100)`; `if (rank) col0 -= 100`;
   `if (rank != 1) col1 += 100 - val`; `col[rank] += val`. The six columns always sum to
   zero per call - asserted in the tests.
6. Perfect counter `player+156`, `int16[5][6]`: `++[g][rank]` only when `rank != 0` and
   `val == 100`.
7. Optional 28-byte record ring at `0x184AD0220` (count `0x184C4E80`, limit 3000) gated by
   `word_1813E470C != 0` (0 in a default boot): `{chart+56, chart+8, rank, combo,
   player+16, value}`.
8. Rank latch `HIDWORD(0x1813668C8)` - the six-case switch, sticky at 1 and 5.
9. Popup dispatch `sub_1803075A0(rank, x, note+32, note+64, 0, 0)` with
   `x = base + pitch*(chart+88 - 1)`, `x += (base + pitch*chart+92 - x)/2`,
   `base = 0x13E4738 (10)`, `pitch = 0x13E476C`. Dispatched when
   `HIDWORD(qword_1813E4730) != 1 || byte_18136691C != 0 || HIBYTE(word_1813E5720) != 0 ||
   chart+8 in {1,4} || rank == 1`, otherwise suppressed.

**The leading sustain loop (`0x180303F48..0x1803042F0`) is not score.** It drives
`qword_1813E4A94` / `dword_1813E4A90` / `dword_1813E4ABC`, calls a libxact reset and branches
on `avs2_core_203() % 1000 < 5` / `% 100 < 10` - a **random** visual spawner. It cannot be
reproduced deterministically and is deliberately excluded from score equivalence; `value`,
combo, counters, table, latch and the popup arguments are the reproducible part.

Note (pack 303e80 s4): the counter object `sub_180304A70()` is a 584-byte **CLibXact**
instance (`0x181ABE7E0`, 5 of them); the judgement score table physically lives inside
libxact's per-voice object. Nothing in this stream touches it.

---

## 3. What was implemented

| File | Purpose |
|---|---|
| `include/nosfix/native_match.hpp` | window sampler, horizon config, evidence record, `NativeMatchPolicy`, `NativeShadowComparison` |
| `src/native_match.cpp` | `SampleJudgmentWindows`, `Configure`, `BeginUpdate`/`BeginGroup`, `Select`, `CommitJudge`, `HorizonAllows`, shadow comparator |
| `include/nosfix/native_scoring.hpp` | `ScoreInput`/`ScoreEffect`/`ScoreRingRecord`, `NativeScoringState`, `ScoreGate`, `AdvanceRankState` |
| `src/native_scoring.cpp` | the deterministic 303E80 transition, O(1) and allocation-free |
| `tests/native_match_tests.cpp` | 16 cases |
| `tests/native_scoring_tests.cpp` | 12 cases |
| `CMakeLists.txt` | two sources in `nosfix`, two test targets + `add_test` |

### 3.1 Dormancy

* `kNativeMatchModelVerified` is **false** (header constant, not configuration).
* `NativeMatchPolicy::Configure` returns `EvidenceIncomplete` unless the caller supplies all
  ten evidence booleans, `HorizonNotSampled` / `LayoutNotSampled` unless the horizon and
  layout were sampled, and `InvalidWindows`/`InvalidRate`/`NonFiniteTime` otherwise.
* `NativeNoteState::Configure` is untouched and still returns `UnsupportedSelection` for
  `NoteSelectionPolicy::Native2F7C10`; a test asserts this, so no ini value can reach the new
  policy.
* Every decision path is allocation-free; `ScoringState::Apply` was measured with the test
  harness's allocation counters (0 allocations over 64 calls).

### 3.2 Window table

`SampleJudgmentWindows(span, units_per_second, post_initialization_sample)` reads
`0x13668E0..0x1366904` (effect band + the **eight** window globals at `0x13668E8`, i.e.
Good/Great/Just plus the `±9999` outer clamps), and refuses:

| status | condition |
|---|---|
| `NotPostInitialization` | caller did not assert the initializer has run |
| `Unavailable` | span shorter than `0x1366908` |
| `InvalidRate` | rate not finite, `<= 0`, or `> 1e6` |
| `ZeroStaticBlock` | all eight globals zero (proven-invalid pre-init image) |
| `InconsistentEffectBand` | band or outer clamp outside its tier |
| `InvalidUnits` | sign/nesting violation from `ConvertLegacyNormalWindows` |

Since judgment-001 item 7 proves `sub_1800CB9A0` is the only writer, the values are
constants of this build; the sampler still demands the post-init assertion and the explicit
rate rather than hardcoding 60.

### 3.3 Shadow path

`NativeShadowComparison::Observe(adapter, native)` compares, in order: kind, note index,
lane, rank, effect grade, score, then time with a configured tolerance, and latches the
first divergence with note index, song time, rank, score and effect plus the outcome
ordinal. `FromAdapter(NativeNoteOutcome, running_score)` removes the adapter-specific
conversion. The parent feeds both engines' outcome streams from one capture; nothing has to
be enabled for that.

**Parent wiring later**: enable the adapter as today, run `NativeMatchPolicy` in shadow
(one `BeginUpdate` per traversal call, one `BeginGroup` per bucket, `Select` per bucket
entry, `CommitJudge` with the judge's own result), convert its decisions to
`ShadowOutcome`, and compare against the adapter's `NativeNoteOutcome` stream. Then flip
`kNativeMatchModelVerified` only after a full song agrees.

---

## 4. Test command and results

```
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -RunTests
```

`ctest` result after the change (log `build/ctest_all3.log`):

```
100% tests passed, 0 tests failed out of 41
```

41 = the 39 pre-existing tests (all still passing) + `native_match` + `native_scoring`.
Both new executables are also listed in the `/W4 /permissive- /EHsc /utf-8` target loop.

Covered: exact match choice, ties (equal time -> smaller `note+56`), same-time events in two
lanes, lane masking and group reset, pending-early primary-slot rule, auto notes, done and
consumed notes, chart types 3/5 exclusion, linked pairs (state + auto slot + horizon),
horizon admission boundary (4 frames), `CommitJudge` for hit/miss/release/type-4, over-filter
and slot classification, 60/90/120/144/165/240/360 Hz with jitter and a 30-frame stall,
shadow divergence in every field plus latching and tolerance, window sampling for all
refusal paths, policy configuration refusal for every gate, adapter refusal of
`Native2F7C10`, and the full rank-state table.

---

## 5. Remaining blockers (fail-closed, nothing guessed)

1. **`dword_18070A610` unit and rate** (judgment-001 s7, pack 2f7c10 s8.1). Everything is
   self-consistent in integer frames but nothing proves one unit is 1/60 s. This is why
   `MatchConfig.units_per_second` is explicit and `ScoringConfig` never converts. A live
   sample of `(note+24, dword_18070A610, wall clock)` over one measure is required.
2. **Bucket table `0x13DEA20`** (judgment-001 item 3, partial). 30 buckets x 792 bytes
   (`784` of entries + a qword count at `+784`), 28 entries of 7 ints, rotated by
   `sub_180307090`/`sub_180307170`; the **filler and the meaning of ints 1..6 are unknown**.
   `NativeMatchEvidence::bucket_table_known` stays false until they are; the policy therefore
   never configures in production. The model takes the entry lane and the candidate list from
   the caller instead of guessing.
3. **`chart+8` / `chart+56` semantics** (pack 303e80 s8.2). `chart+8` indexes the 5-group
   score table and suppresses the popup at 1 and 4; it is not the lane. `chart+56` is the
   duplicate key and the record field. Both are carried through unchanged, but their meaning
   is unproven.
4. **`note+96` partner identity** (judgment-001 item 4, strong reading only): another note
   object with `+80` (auto) and `+96` (claimed) fields. Modelled as caller-supplied
   `linked_open` / `auto_slot_empty`.
5. **Candidate sort comparator** (pack 2f7c10 s8.3): `sub_1800CAE90`'s comparator was not
   read; the "closest wins" model comes from the slot-arming code, and the caller's candidate
   order is respected.
6. **Live values of `byte_18136691C`, `word_1813E5720`, `HIDWORD(qword_1813E4730)`** for the
   effect-dispatch gates; they init to 0 and are modelled as explicit inputs.

## 6. Merge notes

* `CMakeLists.txt` adds two library sources and two test targets; no other build change.
* `scripts/build.ps1` carries a **one-line environment fix**: `rc.exe`/`mt.exe` are passed to
  CMake with forward slashes, because CMake 4.4 writes `CMAKE_RC_COMPILER` unescaped into
  `CMakeRCCompiler.cmake` and a native path fails to configure (`Invalid character escape
  '\S'`). Revert if the parent prefers another fix; without it the tree cannot configure
  from scratch on this machine.
* `NativeNoteState`, `normal_judgment`, config/bootstrap/hooks/game_hooks are untouched.
* No detour, no game memory access, no default-enabled seam.
