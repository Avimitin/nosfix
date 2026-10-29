# PROBE_ANALYSIS - first diagnostic run vs the ABI model

Log: `logs/nosfix-probe-20261030-0219.log` (1,439 lines, 1,202 PROBE lines).
Databases re-opened this pass: `nostalgia_shared.i64` (base 0x180000000).
Tags: **[bytes]** proven from instruction bytes, **[dec]** decompiler only, **[hyp]**,
**[live]** needs a capture.

## 0. What the log actually contains

| Line kind | Count | Notes |
|---|---|---|
| `judge before` | 400 | **~300 have `event=0000000000000000`** - these are the per-frame expiry/auto pass, not input events |
| `judge after` | 400 | matches 1:1 with the before lines |
| `motion kind=3(gliss)` | 400 | **mis-labelled, see Q3** |
| `motion kind=5(velocity)` | 0 | |

Event addresses observed: `0x1813DEA20 + 28*k` for k = 0..7
(`…A20, …A3C, …A58, …A74, …A90, …AAC, …AC8`). Stride 28 confirmed exactly.

## 1. Is `0x2F6800` the normal-note judge? Yes. Corrected probe mapping.

**[bytes]** `0x1802F6800` is vtable slot 4 of `NormalNotes::vftable` at `0x180684328`
(`+0x20`). It is the normal note's input/update handler and the **only** path that reaches
the judge `0x1802F79E0` → `sub_180303E80`. So the hook target is right.

**Its real signatures:**

```
a1 = NormalNotes*      (note object, vtable 0x180684328)
a2 = int*              pointer to ONE 28-byte slot of the input-state table
                       (or NULL: the expiry/auto pass)
a3 = bool              chord flag (>=3 simultaneous)
```

`a2` field offsets **[bytes, `sub_180307170`]**: `+0x00` key, `+0x04` state (1 press /
2 repeat / 3 release / 0 idle), `+0x08` byte value, `+0x14` group id, `+0x18` repeat
counter (increments while the key is held, re-arms to 1 at 25). `a2 == NULL` makes the
handler use `a1[13]` (**note+52**) as the value and run the expiry path - that is what
~300 of the 400 logged calls are.

`a1` fields the handler touches **[dec, every one a plain member read]**:
`a1[5]`=note+20 (chart+16), `a1[6]`=note+24 (frame), `a1[7]`/`a1[8]`=note+28/+32 (x,y),
`a1[11]`=note+44, `a1[12]`=note+48 (left/right key), `a1[13]`=note+52 (centre key),
`a1[20]`=note+80, `a1[21]`=note+84, `a1[26]`=note+104, `a1[52]`=note+208.

**Probe corrections:**

| Probe field | Was read from | Correct source | Observed value is |
|---|---|---|---|
| `event_id` | `*(int*)a2` | same | key (5..25 observed) - **correct** |
| `event_kind` | `*(int*)(a2+4)` | same | state 1/2/3 - **correct** |
| `event_group` | `*(int*)(a2+??)` | `*(int*)(a2+0x14)` | 1 - plausible but unverified |
| `event_value` | `*(int*)(a2+0x20)` | `*(uint8*)(a2+0x08)` | **wrong**: +0x20 is the next slot's key |
| (missing) | - | `*(int*)(a2+0x18)` | the repeat counter |

**The pack that was wrong:** `evidence/judgment-2f7c10.md` s2 described the pair record's
`[1]` as "a 12-byte event descriptor `{lane, kind}`". It is not - it is a **pointer to a
28-byte input-state slot**. The shape (`+0` id, `+4` kind) happens to be right, which is
why nothing else broke. Corrected in place.

Also corrected there: **`chart+16` is not the 1..28 key.** The log shows `lane =
31..72` (33, 38, 45, 52, 59, 64, 69, 72). The 1..28 keys are `chart+88/+92`
(= note+44/+48), and the state-table keys observed are 5..25. Any code that used
`chart+16` as a lane/key index must switch to `chart+88/+92`.

## 2. Does the observed key/state/value sequence close B6/B7?

**B6 (slot field set): closed except for the value byte, which the probe never read.**
The observed `kind` sequence is exactly the state set `sub_180307170` handles -
1 (press) → repeated 2 (repeat) → 3 (release), with the documented remaps in the
compressor (`2 -> 1`, `3 -> 0` when a newer bucket already holds that key with a
different state). **[bytes]** `180307170: if (v7==2) v7=1; else if (v7==3) v7=0;`

**B7 (`entry+20` group and `note+272`): consistent, not closed.** Every logged line has
`event_group=1` because the probe read a different offset; with `+0x14` it will produce
real values and B7's "`v34 == 1 || v34 == entry[5]`" test can then be exercised.

**Verdict:** a single re-run with `event_value = *(uint8*)(slot+8)` and
`event_group = *(int*)(slot+0x14)` closes B6 and gives B7 its data.

## 3. Are the kind=3 lines glissando or velocity? **[RETRACTED - run 2 proves glissando; see s8]**

**[bytes]** Displacement census over the note-type functions:

| function | +272 | +296 | +300 | +316 | +344 |
|---|---|---|---|---|---|
| `sub_1801A6820` gliss render | - | **0** | **0** | **0** | - |
| `sub_1801A60C0` gliss input | - | **0** | **0** | **0** | - |
| `sub_1801A7450` gliss binder | 3 | 0 | 0 | 0 | - |
| `sub_1801AA4D0` velocity render | 1 | **2** | **3** | **2** | 2 |
| `sub_1801AB410` velocity slot 1 | - | **1** | - | - | 2 |

Only the **velocity** type touches `+0x128/+0x12C/+0x13C`. The logged objects have live,
changing values in all three, so they are **velocity zones**, not glissando.

**Why no kind=5:** the hook's kind mapping is inverted. The two vtables are adjacent
(`0x180679AB8` glissando, `0x180679D68` velocity), and a swap is the only explanation
consistent with "400 gliss, 0 velocity".

**Exact change:** in the motion probe, either
(a) swap the two vtables it compares against, or
(b) better - log `*(void**)note` (the object's own vtable) and classify by comparing it to
`0x180684328` (normal), `0x1806841D0` (long), `0x180679960` (beats), `0x180679AB8`
(glissando), `0x180679D68` (velocity), `0x180679C10` (trill). Logging the raw pointer makes
the mapping self-evident and cannot drift.

## 4. Does `note+296/+300/+316` close C11? **No - it partially refutes C11.**

Observed on real velocity notes: `note296 ∈ {0,1}`, `note300 ∈ {-1, 30, 69, 72}`,
`note316 ∈ {0, 562, 820}`.

- C11 said **`+300` is a two-state flag (0/1)**. **[bytes-refuted]** by `-1`, `30`, `69`,
  `72`. Those magnitudes match the key/lane space (the same lines have `lane` 64/69/72), so
  `+300` is most likely a **key or group id**, not a flag. **[hyp]**
- C11 said **`+296` is a frame value**. Observed `0/1` says it is more likely a **flag or
  small index**. **[hyp]**
- C11 said **`+316` is a frame count used as `180*frames/rate^2`**. `562`/`820` are
  consistent with that (≈9.4 s / 13.7 s at 60 Hz). **Not refuted, not proven.**

**What is missing:** a *writer* trace. The only proven writers are
`1801AA453 note+316 = note+296 - note+316` and `1801AA479 note+300 = 1` inside
`sub_1801AA340`; the values seen (-1, 30, 69, 72) are never produced by those two stores.
So at least one more writer exists, and it uses a non-symbol base. **[live]**

## 5. Does `chart50=0` support the `+0x50 <= 0` fallback? **No - the sample is invalid.**

Two independent reasons:

1. The hooked objects are velocity notes (Q3); the writer of `chart+0x50` is
   `0x1801A647C` inside the **glissando connector scan**, which never runs for them.
2. Even for a glissando note the writer only fires when the scan finds a **held key inside
   `[chart+88, chart+92]` whose group matches `note+272`**. In a chart with no held
   connector key it stays 0 by design.

So `chart50 = 0` is exactly what the model predicts and carries **no** information about
the fallback. Do not use it to justify `+0x50 <= 0` being disabled; the fallback remains
correct as a *safety* choice (the native code also skips clipping when `+0x50 <= 0`), but
it is still unexercised.

## 6. Second diagnostic pass - exact targets

**A. Fix the two judging probes (normal note).**

| Item | RVA | Log |
|---|---|---|
| `sub_1802F6800` entry | `0x2F6800` | `a2` raw pointer, `a2==0`, and when non-null: `*(int*)a2`, `*(int*)(a2+4)`, `*(uint8*)(a2+8)`, `*(int*)(a2+0x14)`, `*(int*)(a2+0x18)` |
| note fields | - | `note+52` (centre key), `note+44`, `note+48`, `chart+8`, `chart+56`, `chart+88`, `chart+92`, `chart+97` (as a **byte**), `note+20` (keep, but label it `channel`, not `lane`) |

Do not read `chart+97` as an int: it produced `-16777216` = `0xFF000000`, i.e. the byte is
0 and the probe straddled `chart+97..+100`.

**B. Fix the motion probe.**

Log `*(void**)note` first, then classify. Only after that compare fields per class.

**C. Input-state producer path** (`this` is the note manager, `0x184B6906C` calls it):

| Item | RVA | Why |
|---|---|---|
| `sub_180211720` entry | `0x211720` | the per-frame input update; calls `sub_180307090` then `sub_180307170` and caches `*(this+224) = &bucket[792*dword_181366910]` |
| `sub_180307090` | `0x307090` | bucket rotation (`memmove` of 29 x 792 bytes) - snapshot a bucket before/after to see the shift |
| `sub_180307170` | `0x307170` | compressor staging → bucket 0; **log one full slot (28 bytes) of `0x1813DEA10` and of bucket 0 each frame** |
| staging buffer | `0x1813DEA10` | the 784-byte (28 x 28) source of the compaction; its only symbol refs are the initialiser and the chart loader, so the per-frame filler writes through a register - a frame-to-frame **diff of this buffer** identifies the producer without needing to hook it |

A bucket diff over two frames is enough to prove the `+0x08` byte value and the `+0x14`
group id; no breakpoint on the unknown producer is needed.

**D. Glissando-only run** (to exercise `chart+0x50`): hook `0x1801A647C` (the
`chart+0x50 = note+32` store) and `0x1801A6458` (the `note+272` group compare), and play a
chart that contains a glissando whose connector key is held. Until that fires, nothing
about `+0x50` is testable.

## 7. Corrections applied to existing packs

| Pack | Section | Correction |
|---|---|---|
| `judgment-2f7c10.md` | s2, s3 | `a2` is a **28-byte input-state slot pointer**, not a 12-byte event descriptor; `chart+16` is a channel id in 31..72, not the 1..28 key |
| `input-panb.md` | s5 | same lane/channel correction; keys are `chart+88/+92` |
| `ABI_COMPLETION.md` | C11 | `+300` is **not** a 0/1 flag (observed -1/30/69/72); `+296` is **not** a frame value (observed 0/1); `+316` unrefuted |
| `ABI_COMPLETION.md` | B6 | value byte is at slot `+0x08`, group at `+0x14`, plus the new `+0x18` repeat counter |

**Unchanged and still correct:** the judgement windows and rank mapping, `id > 1 && rank > 1

---

# Run 2 (build B, `nosfix-probe-20261030-0243.log`, session from 02:27:15)

Build-B lines: 2,403 (500 `judge before`, 500 `judge after`, 400 `motion`, 400 `input`).
All build-A conclusions below are re-checked against it; where they conflict, **run 2 wins**.

## 8. RETRACTION: the motion objects are glissando, not velocity

Build B logs `vtable=0000000180679AB8 rva=0x679AB8 vt=gliss` on all 400 motion lines.
`0x180679AB8` is `GlissandoNotes::vftable` (slot 3 = `sub_1801A6820`). **The
kind-mapping is NOT inverted.** Run 1's inference was made without a vtable field and is
withdrawn; the displacement census it rested on was also too narrow (3 functions).

Corrected census, whole image (all heads, any addressing form):

| displacement | functions that reference it |
|---|---|
| `+0x128` (296) | `0x1A7910`, `0x1A84F0`, `0x1A9AB0`, `0x1AA340`, `0x1AA4D0`, `0x1AB410`, `0x1AB800`, `0x2F4220`, `0x2F4E80`, `0x2F6190`, ... |
| `+0x12C` (300) | `0x1AA340`, `0x1AA4D0`, `0x2F4220`, `0x2F4E80`, `0x2F4E20`, ... |
| `+0x13C` (316) | `0x1A7AF0`, `0x1A84F0`, `0x1AA340`, `0x1AA4D0`, `0x1AB800`, `0x2F4220`, `0x2F4E80`, `0x2F6190`, ... |

**No glissando function references any of the three**, yet gliss objects carry live values
there. So the writer is not gliss-specific: it is a shared/base initialiser or a bulk copy.
The long-note binder `sub_1802F6190` writes exactly these slots
(`*(a1+316) = -1`, `*(a1+296) = chart+20 + chart+24`), so the gliss object most likely
inherits long-note base storage that the gliss class neither writes nor reads.

## 9. Q3 reclassification: gliss `note+296/+300/+316` are inert shared base storage

Observed on 400 real glissando draws: `+296 ∈ {0, 1, 257}`, `+300 ∈ {-1} ∪ [42..57]`,
`+316 ∈ {0} ∪ [600..730]`, transition 0/-1/0 -> 1/xx/yyy once per note.

- The gliss renderer `sub_1801A6820` reads only `+52`, `+64`, `+196`, `+200`, `+204`,
  `+208`, `+216`, `+128`, `+168`. It **never reads these three**, so they cannot affect
  gliss geometry.
- C11's "velocity-only" and "+300 is a 0/1 flag" statements are **withdrawn** (see the
  ABI_COMPLETION C11 correction). The *velocity* reading of the same slots by
  `sub_1801AA4D0` is unaffected.
- **Implementation rule: do not model `+296/+300/+316` for glissando at all.** Treat them
  as uninitialised base storage; do not derive them from the chart.

## 10. Q1: `note+84` / chord semantics are NOT closed - `id > 1` was never exercised

All 500 build-B judge calls have `id = 1`. `note+84` is written by vtable slot 29
(`sub_1800A2C80`, only when the current value is not 1) from the candidate record's
`+0x10` field, which is the **candidate slot index 1..3**. So `id = 1` means every
judged note was answered by candidate slot 1; slots 2 and 3 were never taken.

**What would exercise it:** a second note of the same key inside the candidate window of
the same input event - i.e. two notes on one key within roughly one judgement band
(a jack / fast repeat on a single lane, or a chart where a key carries two close notes).
A chord does *not* do it (a chord gives each key its own slot; it sets the `a3` chord flag
instead). Until a run contains `id=2` or `id=3`, the `id > 1 && rank > 1 -> rank 5`
scoring rule stays unverified and must remain fail-closed.

## 11. Q2: the staging diff closes B6's layout and state machine, not the producer's identity

Build-B `PROBE input` lines (400) are a diff of the **staging buffer** `0x1813DEA10`
(note-manager `+0x13DEA10`), 28 slots x 28 bytes:

```
slot=0 key=11 state=1 value=6 group=0 repeat=0 frame=120
slot=1 key=12 state=1 value=9 group=0 repeat=0 frame=120
slot=9 key=0  state=0 value=2 group=0 repeat=0 frame=120
```

Distributions over the run: `state` 1 = 95, 2 = 111, 3 = 95, 0 = 99;
`value` 0..12 with 255 on one idle slot; **`group` = 0 on all 400**.

**Closed (B6):** slot layout and the exact transition sequence

1. **press** - a slot appears with `state = 1`, `value` = the averaged PANB nibble
   (0..15; observed 1..12), `repeat = 0`, `group = 0`;
2. **repeat** - every subsequent frame while held: `state = 2`, `repeat` increments
   (cap 25, re-armed to 1 by `sub_180307170` phase 2);
3. **release** - one frame with `state = 3`, then the slot is dropped;
4. **chord** - several slots with the same `state` in the same frame (visible in the log);
5. the compacted bucket keeps only `state != 0` slots, and `state=3` decays to 0 unless a
   newer bucket already holds that key.

**Not closed:** the *identity* of the function that writes the staging buffer. The diff
observes contents, not the writer; no function references `0x1813DEA10` except the
initialiser and the chart loader, so the filler writes through a register base.
**Smallest next evidence:** a byte write watchpoint on `0x1813DEA10` during one key press.

**New and important:** `group` is **always 0 in the staging buffer** because the group id
is only computed later, by `sub_180307170` phase 2, into **bucket 0**
(`v1[5] = v17`, inheriting from key, key+-1, or defaulting to 1). It reaches the glissando
connector only after the rotation by `sub_180307090`, which moves bucket 0 into **bucket 1**
(`0x1813DF048`), and bucket 1 is what the connector scan actually walks.

## 12. Q4: `chart50 = 0` is still invalid, and now for a precise reason

The connector scan (`0x1801A6410..0x1801A6466`) requires, for a held entry:
`entry[0]` (key) inside `[chart+88, chart+92]`, **and** `note+272 == entry+0x14`
(`note+272` is 1/2/3 from the binder; the `== 0` wildcard can never apply because the
binder always writes 1/2/3).

In build B every staging slot had `group = 0`, and the staging buffer is not what the
scan reads anyway. Two independent reasons why `0x1801A647C` never ran:

1. the scan reads **bucket 1**, not the staging buffer;
2. even in bucket 1 the entry's group must equal 1..3, and the run never demonstrated a
   bucket-1 entry with a non-zero group co-timed with a glissando note.

**So the `+0x50 <= 0` fallback is neither supported nor refuted.** It remains the correct
*safety* choice (native also skips clipping when `+0x50 <= 0`), but it is unexercised.

**Exact condition the next run must hold:** a glissando note on screen while a key is
**held down** with `chart+88 <= key <= chart+92`, for long enough that the slot has been
compacted into bucket 1 with a group equal to that note's `note+272` (hold through at
least two frames). Log `*(int*)(0x1813DED38 + 28*k + 0x14)` (bucket-1 group) and
`0x1801A647C` execution.

## 13. Q5: what stays blocked, and can the streams start?

| Stream | Can start now? | Basis |
|---|---|---|
| judgment / scoring | **yes** | windows, rank map, `303E80` arithmetic, slot layout and states all proven. Fail-closed only the `id > 1 -> rank 5` branch |
| notes-input / PANB | **yes** | B6 closed; level at slot+8 (0..15) proven; PANB gate = `config+44`. Fail-closed: the producer's identity and PANB key order |
| render / UI | **yes for gliss and beats** | gliss geometry + connector objects proven; beats proven. Velocity zones: `+316/+300` semantics stay fail-closed, and the `+0x50` clip must stay inactive |
| audio | **yes** | engine/voice offsets proven; source-voice order is SDK order [hyp] and every call is failure-counted |
| AFP UI | **no** | colour/opacity offsets, evaluator, callback ABI and pool free are still unread |

Genuinely blocked (no further progress from static analysis):
`note+296/+300` writers (now known to be inert for gliss), the staging-buffer producer,
`element+0x50` under a held connector key, `note+84 > 1`, and the AFP timeline.

**Still correct from run 1 (unchanged):** the judgement windows and rank mapping, the
`id > 1 && rank > 1 -> rank 5` rule (unexercised), the 28-key model for *keys*, the
`note+216` connector object, and the `CreateFX`/voice-table results.
