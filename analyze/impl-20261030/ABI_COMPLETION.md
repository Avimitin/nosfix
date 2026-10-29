# ABI completion pass - final evidence (2026-10-30, session 2)

Databases (shared copies only, opened read-only, never saved or renamed):
`nostalgia_shared.i64` (20,472 funcs, base 0x180000000), `libxact_shared.i64` (480 funcs,
full mangled symbols), `afp-core_shared.dll` (3,038 funcs, 127 ordinal-only exports).

Tags: **[bytes]** = proven from instruction/data bytes, **[dec]** = decompiler-only
(consistent pseudocode, no byte-level confirmation), **[hyp]** = hypothesis,
**[live]** = needs a live sample. All addresses are absolute VAs (RVA = VA - 0x180000000).

Headline results of this pass:

1. **`dword_18070A610` is 1/60 s per unit - now proven from bytes** (A5, C10). The chart
   loader converts chart milliseconds with the literal doubles `60.0 / 1000.0`, and the
   timeline tick advances the counter by exactly 1.
2. **The candidate sort order is now fully known** (A2) - the previous pack's record layout
   was wrong and is corrected below.
3. **The 28x28 input-state bucket entry is decoded** (A1) and its tag writer found (B7).
4. **`note+272` (the 1/2/3 tag) is derived in the glissando binder** (B7) - proven.
5. **libxact's cue table is 1024 x 568 bytes at `this+8`, command word at `this+12`** (E17),
   with the field list printed by the binary's own diagnostic format string.
6. AFP: **matrix convention and multiply order proven** (D14); the rest of D remains
   unproven (smallest missing range given).

---

# A. Judgment and note data

## A1. The bucket table at `0x1813DEA20` (30 x 792)

**Status: proven-bytes** (layout, rotation, consumption) / **decompiler-only** (field labels).

### Layout [bytes]

`sub_180307090` (the rotate-down) does `memmove` in a loop of 29 with stride `0x318` from
the top of the array downwards, then zeroes the base:

```
180307090  v1 = (char*)&unk_1813E43D8 ; 29 iterations
            avs2_core_240(v1, v1 - 792, 792) ; memmove(dst=entry k, src=entry k-1, 792)
            v1 -= 792
            ... then  sub_180102B80(&unk_1813DEA20, 0, 792)   ; zero entry 0
```

`0x1813E43D8 - 29*792 = 0x1813DEA20`, so the array is **30 entries x 792 bytes** at
`0x1813DEA20`, entry *k* at `0x1813DEA20 + 792*k`; the same function zeroes the *other*
published array (`sub_180102B80(&unk_181366920, 0, 16392)` - 30 entries of 16392, unrelated
to this item).

Each entry = **28 slots x 28 bytes (7 dwords)** at `+0 .. +783`, then the **count as a dword
at +784** (`v3[196]` in `sub_1802F4060`, `LODWORD(qword_1813DED30) = n` at `0x18030729C`),
then **4 filler bytes at +788..+791** (never read or written; IDA names `+784` as a qword
only because of the 8-byte alignment, only the low dword is used).

Entry access is `&unk_1813DEA20 + 792 * dword_181366910`
(`0x1802F7E55..0x1802F7E78`, `0x1802F407C`, and the vtable thunk `0x180304E40`).
`dword_181366910` has exactly one writer, the initialiser `sub_1800CB9A0`
(`0x1800CC18B`), so the *selected* record index is a constant in this build; record 0 is
the live one and record 1 is "one frame ago" (proven by the 1->2 state step below).

### Slot fields [bytes] for +0, +4, +8; [dec] for +5, +6

| off | type | meaning | evidence |
|---|---|---|---|
| `+0x00` | int | **key / lane, 1-based 1..28** | used as `dw0-1` clamped to 0..27 to index the per-key arrays at `note-manager+360` (`0x1801AC...`, `0x1802F7...`: `mov eax,[v179]; sub eax,1; cmp eax,27`); compared against the note's left/right key in `sub_1802F4060`; used verbatim in the sprite name `"cmp_00_usr/key_down_%02d_usr"` (`0x1801B5D30`, `0x180309310`) |
| `+0x04` | int | **state**: 1 = new this frame, 2 = held, 3 = release, 0 = free/idle | `sub_1802F4060` filters `(dw1-1) <= 1` i.e. {1,2}; `sub_1802F7C10` handles `dw1 == 1` as a new event (`0x1802F...` line 2165) and `dw1 == 3` as a release trigger for `sub_1802BCAD0(&xmmword_1813E4AD8)` (lines 1707 and 2140); the note managers rewrite 1 -> 2 (`*v40 == 1 -> *v40 = 2`, `0x180196390`/`0x1802FDB30`) |
| `+0x08` | byte (low byte of dw2) | per-key **value** consumed as an int (`v60 = *((_BYTE*)v48 + 8)`) | `0x1802F7C10` line 785 |
| `+0x0C` | int | not read by any function found | - |
| `+0x10` | int | not read | - |
| `+0x14` | int | **group id, 1..3** | written by `sub_180307170` phase 2 (`v1[5] = v17`, `0x1803073xx`); compared with `note+272` in `sub_1801A60C0` (`v34 = note+272; if (v34 == 1 \|\| v34 == entry[5])`) |
| `+0x18` | int | **repeat/merge counter** (0..24) | `sub_180307170`: `v1[6] = other[6] + 1`; reset to 0 when `>= 25` |

There is **no field at +0x20** inside a 28-byte slot. The `+0x14`/`+0x20` offsets in the
task brief belong to the *candidate node payload* of the matcher list, which is a different
object (see A2).

### The rotation [bytes]

`sub_180307170` (called immediately after `sub_180307090` from all five note managers,
e.g. `0x18019761B/0x180197620`) rebuilds record 0 from the **source array**:

* source = `*(qword*)(0x1813DEA10)` (`0x1803071D0: mov rax, qword ptr cs:xmmword_1813DEA10`
  - `xmmword_1813DEA10` is a **pointer variable**, not a 16-byte matrix; the decompiler's
  `i + xmmword_1813DEA10` is really `i + *ptr`). The source holds the same 28x28 slot
  layout, iterated with a fixed bound `cmp rbx, 310h` (28 slots, no count).
* for each source slot with `dw1 != 0`:
  * scan **record 1** (`r14 = 0x1813DED3C`, count `qword_1813DF048` = record 1 +784) for the
    same key with `dw1 not in {1,2}`; if found, decay **2 -> 1** and **3 -> 0**
    (`0x18030721F..0x180307232`).
  * if the state survives non-zero, append the slot to record 0 (`0x180307263..0x180307278`)
    and increment the record-0 count.
* then (only if `byte_1813E4A8E == 1`, a constant 1 written only by `sub_1800CB9A0`)
  phase 2 recomputes `dw5` (group id) and `dw6` (repeat counter) by comparing every record-0
  slot against records 2..5 (`0x1813DF360`, `+792` x 4) on `key == k`, `key == k+1`,
  `key == k-1`; a slot with `dw6 >= 25` re-arms group 1.

`byte_1813E4A8E == 1` also gates the glissando `note+272` derivation (B7), so group-id
computation and the glissando side tag are switched on by the same constant.

### How `sub_1802F7C10` consumes an entry [dec, exact offsets]

`v526 = &unk_1813DEA20 + 792*dword_181366910` (`0x1802F7E6E`). Two consumers:

```
; A) count-driven walk, "held" slots with a value in [v55,v56]
v59 = *(int*)((char*)&qword_1813DED30 + v16);            ; record+784 = count
v48 = (int*)v526;
do { if (*v48 >= v55 && *v48 <= v56 && v48[1] == 2)
       take *((_BYTE*)v48 + 8) as the value;
     v48 += 7; } while (--v59);
; B) release / new-event walk
if (v372[1] == 3 && *v372 >= 0) sub_1802BCAD0(&xmmword_1813E4AD8);   ; state 3 -> sound gate
if (v349[1] == 1 && *v349 <= 28) { ... }                             ; state 1 -> new event
```

Also `sub_1802F4060(note)` walks the same record and inserts every key in
`[note_left_key - dword_1813E4A84, note_right_key + dword_1813E4A84]` whose state is
{1,2} into the per-chart `std::set` at `qword_1813E46F8` (count `qword_1813E4700`),
then flags the note (`slot 29` / `a1[21] = 1`). `dword_1813E4A84` defaults to **3**
(`0x1800CC3B7`) and is also written by `sub_180277360`, `sub_1803049C0`, `sub_180305B30`.

**Safe to rely on:** sizes, stride, count offset, key/state/value/group/counter semantics,
the 29-entry table29 decay in the same function, and the record-0/record-1 relationship.
**Fail-closed:** `+0x0C`/`+0x10` are unused in this build - do not invent meaning for them.
**Next evidence (not required for a faithful port):** the writer of the 28x28 source array
published at `0x1813DEA10`. No instruction in the image stores through that pointer; it is
published by `sub_180305B30` (`0x180305C69/C70`) from its `rdi`/`rsi` arguments, which the
four call sites load as `obj+8` and `obj+0x31C` (note manager fields). Smallest next step:
a write watchpoint on the note-manager `+8` array while a key is pressed.

## A2. Candidate ordering - `sub_1800CAE90` / `sub_1800CB0E0` / `sub_1800CA230`

**Status: proven-bytes.**

`sub_1800CAE90(list, mode)` is a thin wrapper:
`sub_1800CB0E0(list, &out, list->head, list->head, &mode, list->count)`.
`sub_1800CB0E0` is a **recursive stable merge sort of the intrusive doubly-linked list**
(`node+0 = next`, `node+8 = prev`), merging right-run-first only when
`sub_1800CA230(right, left) != 0`. The `mode` argument is passed down and **never read**.

**Node layout [bytes]** (correcting `judgment-2f7c10.md` s2): the list node is
`{qword next, qword prev, payload[40]}`; the payload starts at `node+0x10`:

```
payload+0x00  note*                                  (0x1802F9097)
payload+0x08  input-event descriptor*  -> *(int*)    = event lane   (0x1802F909E)
payload+0x10  int  candidate slot index 1..3         (0x1802F90A5)
payload+0x14  byte (slot == 1)                       (0x1802F90AE, sete)
payload+0x18  float |candidate time - event time|    (0x1802F90B5)
payload+0x20  qword partner note* or NULL            (0x1802F90BD)
```

`sub_1800CA230(A, B)` returns 1 when **A sorts before B**, evaluating in this exact order
(all operands re-read at every step, so this is the effective key order):

1. `|fA - fB| > dword_1813E496C` (= 1)  ->  `fA < fB`   (`f` = `note+24`, vtable slot 18)
2. else compare `payload+0x14` (the "slot == 1" flag): if different, the **slot-1** record
   first; if equal keep going
3. `fA < fB` -> A first; `fA > fB` -> B first
4. partner pointer: `otherA == NULL && otherB != NULL` -> A first (NULL partners first)
5. `dA > dB` -> A first (smaller distance first)
6. event lane (`*(int*)payload+8`) ascending
7. `note+52` (vtable slot 10, the note's integer centre key) ascending

Because the sort is stable and the comparator never returns true for equal records, equal
records keep insertion order (candidates are pushed in slot order 1,2,3 from the fixed
3-entry array at `note_block+1216`, stride 24).

**Safe to rely on:** the seven keys, their order, the direction of each test, and
stability. **Fail-closed:** `mode` is dead - do not thread it into a comparator.
**ERRATUM** applied to `judgment-2f7c10.md` s2 (offsets were +0x10/+0x18/+0x20/+0x28).

## A3. `chart+8`, `chart+56`, `note+96` partner, auto slot

**Status: proven-bytes** for the partner latch and `chart+56`; **decompiler-only/partial**
for `chart+8`.

* **`chart+8`** is read in `sub_180303E80` as `a7[2]` and used as
  `g = clamp(chart+8, 0, 4)` to index the 5-group score table, and compared against
  1/3/4/5 to suppress the judgement effect. In the 104-byte record loader
  `sub_1801A43E0` it is written as a **literal zero**
  (`0x1801A48B6 xor r11d,r11d` -> `0x1801A49B7 mov [rbx+rax+8], r11d`). So on that
  loader's records `g` is always 0. Records that carry non-zero `chart+8` come from a
  loader not identified in this pass (see below).
* **`chart+56` (+0x38)** is written by the same loader as `elem[0x38] = src[0]`
  (`0x1801A48FC`) and read in `sub_180303E80` only as the first field of the optional
  per-note record ring (`p->rec[count].+0 = a7[14]`, gated on
  `HIBYTE(word_1813E470C) != 0`, which the initialiser leaves 0). **It has no scoring
  effect in a default boot.**
* **`note+96` partner identity**: `sub_1802F79C0` (vtable slot 12) is a **first-writer-wins
  latch**:
  ```
  if (*(qword*)(note+96) == 0) { *(qword*)(note+96) = other; *(int*)(note+104) = dword_1813E4A88; }
  ```
  `note+104` therefore carries the auto-slot index/value that was current at pairing time;
  a second call is a no-op. The binder clears both (`*(qword*)(note+96)=0; *(int*)(note+104)=0`).
* **Auto-slot branch** in `sub_1802F7C10`: when the note's vtable slot 22 / `note+17` marks
  it as autoplay, the code sets `note+84` (slot 29), calls slot 12 with the partner, and
  records the frame via `sub_180303C10`; `v563[lane]` latches so the autoplay path cannot
  fire twice per batch. **[dec]** for the label, **[bytes]** for the offsets and the latch.

**Safe to rely on:** the offsets, the latch semantics, `chart+56` being inert by default.
**Fail-closed:** `g` may only be 0 when the record came from `sub_1801A43E0`; do not assume
`g == 0` globally.
**Smallest next evidence for `chart+8`:** the writer of `chart+8` for non-glissando
records - it must be a second 0x68-record builder; search for `mov [r*+r*+8], <1..5>`
after `imul r*, 68h`.

## A4. Normal-note / chart fields, keysound handle

**Status: proven-bytes** (`sub_1802F73B0`, the normal-note binder, full body read).

```
note+16  = (word)1                     note+20  = chart[4]  = chart+0x10  lane
note+8   = chart (qword)               note+24  = chart[5]  = chart+0x14  frame (int)
note+44  = chart[22] = chart+0x58  left key      note+48 = chart[23] = chart+0x5C right key
note+52  = (left + right) / 2   (signed, C truncation)
note+56  = (float)(left + right) * 0.5f
note+60  = out_left_px - out_right_px            note+64 = chart+0x5C - chart+0x58
note+68  = byte 0   note+76 = byte 0   note+80..87 = 0 (so note+84 starts 0)
note+88  = chart[25] = chart+0x64      note+96 = 0  note+104 = 0  note+108 = byte 0
note+112 = -1   note+116 = byte 0
note+168 = sub_180301CB0(sprite_off, chart+0x30) (+ up to 9 by width)
note+172..   out buffer of a vtable slot-17 call on qword_184AEF6D0[note+168]
note+204 = (qword)1   note+208 = 0
```

* `note+84` is **not** written by the binder - its only writer is vtable slot 29
  (`sub_1800A2C90` setter), fed from the input event's `+0x20` dword.
* `chart+48` (0x30) is the id passed to the keysound/sprite selector `sub_180301CB0`.
* `chart+100` (0x64) selects the sprite offset: `1 -> 6`, `3 -> 7`, else `0`
  (only when `HIDWORD(qword_1813E4730) == 1`).
* `chart+104` is **outside** the 104-byte record (`+0 .. +0x67`); it is not a field. Do not
  read it.

**Keysound handle lifetime: decompiler-only.** `note+168` is an index into the global
array `qword_184AEF6D0` (1984 bytes zeroed once, `0x1802F73B0`), and the handle payload is
copied into `note+172` by that object's slot 17 called with `(obj, 0, note+172)`. So the
handle is **owned by the shared sprite/keysound object, not by the note**; the note only
keeps an index plus a copy. Consequence for a port: do not free anything from the note
destructor based on `note+168`.
**Fail-closed:** the exact lifetime/refcount of `qword_184AEF6D0[i]` is unproven.

## A5. Unit of `dword_18070A610` - **1/60 s, proven**

**Status: proven-bytes.** This closes the last live-sample blocker for judgement.

1. `sub_180197330` ends with `lea rcx, dword_18070A610; jmp sub_1802FC9D0`
   (`0x1801975E4`), i.e. the timeline tick gets `&dword_18070A610`.
2. `sub_1802FC9D0` (`0x1802FC9D0`) does `v3 = *a1 + 1; *a1 = v3;` - the counter advances by
   **exactly 1 per call**, then `beats += (frame - last_frame) * tempo / rate^2`
   (`v9 = (float)(v3 - *(a1+308)) / ((rate*rate) / *(a1+276)) + *(a1+300)`).
3. `dword_18070B300` (the `rate`) holds **60.0f** (`0x18070B300` file bytes `00007042`,
   written only by `sub_180218AA0` and two sites near `0x18033A49F`) and the default tempo
   `*(float*)(0x18070A724)` is **100.0f**.
4. With `beats = frames * tempo / 60^2`, the only consistent reading is
   `frames x (1/60 s)` and `tempo` in beats/minute.
5. Independently, the chart loader converts chart **milliseconds** to frames with the
   literal constants `60.0` (double at `0x18070B2F8`) and `1000.0` (double at `0x180687218`):
   ```
   1801A4935  mulsd  xmm0, cs:qword_18070B2F8    ; * 60.0
   1801A493D  divsd  xmm0, xmm6                  ; xmm6 = qword_180687218 = 1000.0
   1801A4945  mov    [rbx+rcx+14h], eax          ; int frame = (int)(ms*60/1000)
   ```
   `frame = (int)(ms * 0.06)`.

**Safe to rely on:** 1 unit = 1/60 s; judgement windows `+2/-2, +4/-4, +7/-6, +9/-11` are in
those units; `frame = (int)(ms * 60 / 1000)` with **truncation toward zero**.
**Fail-closed:** the *call rate* of the tick (once per rendered frame, presumably at
60 Hz) is not provable from the image; if the port decouples logic from render rate it must
advance this counter at exactly 60 Hz.
**Live sample (only if the tick rate is desired):** frame value at the start of a chart plus
wall clock at the end of the same measure; 1 unit = 1/60 s is already implied.

---

# B. Input producer

## B6. Input event fields `+0`, `+4`, `+0x14`, `+0x20` and their producer

**Status: partial - proven-bytes for the state set, not-found for the final writer.**

What the bytes show:

* The 28x28 state set (A1) *is* the per-frame input state: `sub_180211720` (called from
  `0x184B6906C`, the note manager's frame update) runs `sub_180307090` then
  `sub_180307170` and caches `*(qword*)(this+224) = &bucket[792*dword_181366910]`.
* The "event" the matcher consumes is a **slot of that set**, addressed as 7 dwords:
  `+0` key, `+4` state (kind 1/2/3), `+8` byte value, `+0x14` group id.
* The 64-byte record at note-manager `+1168` is a **different** list: `sub_1800CA9A0`
  (append) and `sub_1800CA930` (first) have exactly one caller each, `sub_1802F7C10`, and
  the payload offsets are those in A2 (`+0x14` = "slot==1" byte, `+0x20` = partner note
  pointer). **The brief's `+0x20 = value` does not hold for that record.**

**Refuted:** "input event `+0` = lane / `+4` = kind / `+0x20` = value" cannot be a single
record type in this build: `+0x14` is group id in the 28-byte state slot, but
"slot==1" in the 40-byte candidate payload, and neither has a value at `+0x20`.

**Fail-closed:** do not read `+0x20` from a state slot (it lands in the next slot).
**Smallest next evidence:** the writer of the 28x28 array published at
`qword_1813DEA10` - the four callers of `sub_180305B30` pass `obj+8` and `obj+0x31C`, so a
write watchpoint on note-manager `+8` during a key press names it in one hit. No static
xref exists because the pointer is only ever dereferenced in `sub_180307170`.

## B7. `note+272` 1/2/3, the `0x1813E46F8` list, and the `entry+20` tag writer

**Status: proven-bytes for all three sub-questions except `note+84 == 1`.**

**`note+272` (0x110) derivation - `sub_1801A7450` (glissando binder), tail:**

```
1801A76B0  mov dword ptr [rdi+110h], 1          ; default
1801A76BA  cmp cs:byte_1813E4A8E, 1
1801A76C1  jnz  done                            ; disabled -> stays 1
1801A76C3  mov rcx,[rdi+8]                      ; chart
1801A76C7  mov rdx,[rcx+40h]  ; head element    (NULL -> stays 1)
1801A76D0  mov rcx,[rcx+48h]  ; tail element    (NULL -> stays 1)
1801A76D9  eax = head[0x5C] + head[0x58]; eax /= 2 (signed, trunc)   ; lo = head centre key
1801A76E7  eax = tail[0x5C] + tail[0x58]; eax /= 2 (signed, trunc)   ; hi = tail centre key
1801A76ED  ecx = [rdi+34h]                     ; k = note centre key (note+52)
1801A76F5  if (lo >= k) goto L1
1801A76FA  if (k >= hi) goto L2  else  note+272 = 3
L2/L1:     if (lo > k && k > hi)      note+272 = 2
```

So: **3** = the note centre lies strictly between head-centre and tail-centre in the
ascending sense; **2** = strictly descending; **1** = default (disabled, missing links, or
not strictly between). Only three writers exist for `+0x110` in the whole image
(`0x1801A76B0/76FE/7713`), all constants.

Readers: `sub_1801A60C0` (`0x1801A62CD`, `0x1801A6458`) compares it with a state slot's
group id (`v34 == 1 || v34 == entry[5]`); `sub_1801AA4D0`, `sub_1801A84F0`, `sub_1801A77E0`.

**The `entry+20` tag writer** = `sub_180307170` phase 2 (`0x1803072B3..0x1803073EC`), the
same code that fills `dw6`. Proven ordering rules: same key -> inherit `dw5` and
`dw6 = other.dw6 + 1` (>= 25 re-arms 1); key-1 or key+1 with `dw5 != 1` -> inherit that
`dw5`; result is forced to 1 when 0.

**The `0x1813E46F8` list**: an intrusive `std::set` header (self-linked at
`0x180305C06`, node size 0x18 taken from the free loop at `0x180305C18: lea eax,[r13+18h]`),
count at `qword_1813E4700`, cleared per chart by `sub_180305B30`. `sub_1802F4060` inserts
the offending keys into it (`sub_180081F70`).

**`note+84 == 1`**: not proven. `note+84` is written only by vtable slot 29, from the input
event's `+0x20` dword. Nothing in the image says what value 1 means beyond the scoring rule
"`id > 1 && rank > 1` -> rank 5". **Smallest next evidence:** a live read of the event's
`+0x20` for a single-key press vs a 2-key chord.

## B8. `note+344`, `note+348`, `config+0x2C`

**Status: REFUTED for `note+344/348` as described; proven-bytes for `config+0x2C`.**

* The `+0x158` **count + inline dword array at +0x15C** pattern exists, but in **scene**
  objects (`CMainTestMenuScene` ctor `sub_1800A7620`: `mov [rdi+158h], esi` /
  `mov [rdi+rax*4+15Ch], esi`, with a per-player index `*((int*)v1+70)`), **not** in notes.
  Reading it as "note+344 count / note+348 array" was a displacement-only match.
* In notes, `+0x158` (344) and `+0x15C` (348) are touched only by the long/velocity types
  and **not as a count/array**:
  * velocity: `cmp dword ptr [rsi+158h], 1` (`0x1801AAF2A`) - a palette select;
  * long: `movss xmm0,[rsi+158h]` (`0x1802F529C/52FD/535C/53BF`) - read as a **float**;
    `lea rdi,[rsi+158h]` passed as a pointer (`0x1802F507C/512E`).
  No function in the image zeroes `note+344` on a tick, and no `+348` increment/wrap exists.
* `config+0x2C` **is** proven: `byte_1812D3E77 = *(BYTE*)(sub_180311A00(v7) + 44)` at
  `0x180352680..0x180352687` (`call sub_180311A00` / `mov cs:byte_1812D3E77, cl`), one
  writer only, gate `*(config+37) != 0`, file default `0xFF`.

**Fail-closed:** do not implement a `note+344/348` counter. Treat `note+344` as a
type-specific flag/float chosen per note type.
**Smallest next evidence:** for the long note, the writer of the float at `+0x158` - it is
inside `sub_1802F4220`/`sub_1802F4E80`'s tail regions (`0x1802F47F7`, `0x1802F48E3`),
one function each to read completely.

## B9. PANB level / edge / velocity

**Status: proven-bytes (unchanged from `input-panb.md`; re-verified this pass).**

Judgement consumes **neither the level nor an edge**: the chain
`sub_1802F7C10 -> note vtable 4 -> vtable 13 -> sub_180303E80` never calls
`sub_1803520D0`, never reads `a2+12*n`, and never reads `byte_1812D3E77`. The averaged
nibble (`avg = accum / blockcount`, integer, 0..15) is consumed only by
`sub_1803524A0`/`sub_180352560` -> `sub_180353600` (LED/lamp path). The only per-hit number
in scoring is `note+84` (event `+0x20`), used solely for `id > 1 && rank > 1 -> rank 5`.
`value` is the literal 100 at every call site.

**Safe to rely on:** a port may compute grades without any PANB value.
**Fail-closed:** the PANB key order == lane order is still **[hyp]**.

---

# C. Rendering

## C10. Glissando `element+0x14` and the 104-byte record time fields

**Status: proven-bytes for `+0x14`; decompiler-only for `+0x00/+0x04`.**

Writer of `element+0x14` = **`sub_1801A43E0`, `0x1801A4945`** (the 0x68-record builder;
records live in the vector at `this+0xE8`, stride `0x68`):

```
1801A48F9  eax = src[0]                                  ; r14 = 0x9A0-byte source record
1801A4907  elem+0x00 = r8d
1801A4912  elem+0x04 = edx = (int)((float)src[0x0C] / rate)   ; rate = dword_1813E471C = 1.0f
1801A4935  elem+0x14 = (int)((double)elem+0x00 * 60.0 / 1000.0)
1801A495A  elem+0x18 = (int)((double)elem+0x04 * 60.0 / 1000.0)
1801A4977  elem+0x1C = (int)((double)r9d       * 60.0 / 1000.0)   ; r9d = src[8]/rate
1801A4999  elem+0x30 = src[0x20]    1801A49A8 elem+0x34 = src[0x24]
1801A49B7  elem+0x08 = 0            1801A49C3 elem+0x60 = 0 (byte)
1801A49CF  elem+0x58 = src[0x14]    1801A49DE elem+0x5C = src[0x18]
```

* `element+0x14` is an **integer frame** in the same unit as `dword_18070A610`
  (`1801A6D3A: mov eax,[rbx+14h]; sub eax, dword_18070A610`), derived from a millisecond
  timestamp by `trunc(ms * 60 / 1000)`.
* `element+0x58` / `+0x5C` are the left/right key range (used by the projector call and by
  the `note+272` derivation).
* The previous pack's worry that `element+0` is "an id compared against a word" is
  **resolved**: `+0` is the millisecond start time and `+4` the millisecond end time;
  the word comparison in the link scanner was reading a different field of a different
  record type.
* **Zero-based:** the record stores raw chart ms and converts; whether the chart's ms origin
  is song start is a chart-format question, not an ABI question. Keep it zero-based, same
  origin as the chart file.

## C11. `note+296`, `note+300`, `note+316` (velocity zone)

**Status: partial - proven-bytes for `+316/+300`, not-found for the writer of `+296`.**

Exhaustive displacement scan of all note-type functions: `+0x128` (296), `+0x12C` (300),
`+0x13C` (316) are touched **only** by the velocity type
(`sub_1801AA340` input, `sub_1801AA4D0` render) - no other note type references them.

```
1801AA380  eax = note+296                     ; a frame value
1801AA386  eax -= note+316                    ; minus another frame value
1801AA453  note+316 = eax                     ; "end anchor" = start - span
1801AA45C  byte note+308 = 1                  ; held/active flag
1801AA479  note+300 = 1                       ; state: tail clamp armed
1801AA369  ecx = note+300                     ; read back
1801AA64F  imul eax, note+316, 180             ; render: 180*frames/rate^2 distance
1801AA563  cmp note+300, 0                    ; render branch
1801AB1CD  cmp note+300, 1                    ; second render branch
```

So `note+316` is a **frame count** used exactly like a note's own frame in the distance
formula. **CORRECTED twice - run 2 (PROBE_ANALYSIS.md s9) is final:** the displacement
census now covers the whole image, and **no glissando function references `+0x128`,
`+0x12C` or `+0x13C`**, yet real glissando objects (vtable `0x180679AB8`, proven by probe)
carry live values there - so the writer is a shared/base initialiser, not a per-type field.
Observed: `+296` in {0,1,257}, `+300` in {-1} u [42..57], `+316` in {0} u [600..730].
`sub_1801A6820` never reads them, so they cannot affect gliss geometry - **do not model
them for glissando**. For velocity zones the reading by `sub_1801AA4D0` stands, but the
"velocity-only" and "+300 is a 0/1 flag" claims are **withdrawn**: `note+300` is **not** a
two-state flag - observed values are `-1, 30, 69, 72`, i.e. a key/group-magnitude quantity
[hyp]; `note+296` is **not** a frame value - observed `0/1`, i.e. a flag [hyp]; `note+316`
(`0, 562, 820`) is unrefuted. `note+296` is only *read*
in the input handler, so its writer is upstream (velocity binder `sub_1801A5DB0` or a
helper). **Fail-closed:** do not derive `+296` from `chart+4`.
**Smallest next evidence:** writers of `+0x128` in `sub_1801A5DB0` and `sub_1801AB3A0`/
`sub_1801AB410` (three functions, none of which contain a `+128h]` store - so the writer
must use a different base register; scan for `128h` with a symbol-relative base).

## C12. Ribbon gradient tables and one-tick RGB lag

**Status: proven-bytes for the read pattern; writers not applicable.**

The gradient/colour ramps `0x1812D3278`, `0x1812D3298..0x1812D32B4`, `0x1812D32B8..0x1812D32C4`,
`0x1812D3398/0x1812D33A8/0x1812D33C8/0x1812D33D8`, `0x181366018` are **read-only data**
(`.rdata`/`.data` with static initialisers); the renderers only load them with `movups`
and select between two palettes on `chart+48 == 1` and `note+344 == 1`
(`0x1801AAF2A..0x1801AAF53`). No function writes them at runtime.

**Consequence: a box strip can be evaluated without any one-tick RGB lag** - the colour is a
pure function of (`chart+48`, `note+344`, the projected scale/opacity out-params), all
available in the same draw. `*0.25` alpha is applied when the fade flag `chart+97` is set.
**Fail-closed:** the *contents* of the ramps are data, not ABI; copy them from the image if
colour fidelity matters.

## C13. Trill / tail clamp branches

**Status: not-found (label), proven (branch set).**

There is no separate "trill" note type: the five vtables are normal, glissando, velocity,
beats, long. The `+0x13C`/`+0x12C` pair belongs to the velocity type (C11) and is not a
trill. The long-note cluster that behaves like a tail clamp is
`+0x134` (byte), `+0x138` (byte), `+0x140` (int), `+0x144` (int), `+0x148` (int), with
`cmp dword ptr [rsi+144h], 64h` (100) as an explicit 0..100 clamp
(`sub_1802F4220` at `0x1802F4B76`). **Smallest next evidence:** full read of
`sub_1802F4220` (long-note input, ~1200 bytes of code) - it is the only remaining
unread note handler.

---

# D. AFP UI (afp-core.dll, no symbols)

## D14. Node properties beyond the matrix, and the matrix convention

**Status: proven-bytes for the matrix; not-found for colour/opacity/pivot/clip/visible.**

**Matrix [bytes, SSE]** - 4 x `__m128` **rows**, translation in **row 3**
(row-vector convention, `v' = v * M`):

* `sub_180045A50(dst, a, b)` computes `dst.row_i = Sum_k b[i][k] * a.row_k`
  (`_mm_shuffle_epi32(*a3, 0/85/170/255)` broadcasts `b[i][0..3]`) -> **`dst = b x a`**.
* `sub_180045B30(dst, a, T)` where `T` is a 4-float translation: `row0..row2` copied,
  `row3 = Sum_k T[k] * a.row_k` -> **pre-multiplies a translation**: `dst = T x a`.
* `sub_180045F30(dst, a, b, float f, u32, u32)` = `b x a`, then the same row combination
  with a vector built as `{f, u2, u1, 0}` -> **scale/last-row adjustment applied after**.
* Only `node+192`, `node+196`, `node+200` (read as **floats**, negated) and the 2-bit
  `mode` split over `node+204` bit0 / `node+205` bit1 are proven node properties beyond
  `+128` (`sub_180003080`, mode 0..3).

**Not found:** colour, opacity, pivot-vector, clip rectangle, visible flag. The node is a
raw struct with no RTTI and no strings. **Smallest next evidence:** the pool allocator for
nodes (the function that returns a node and stamps `+312`) plus one draw call; the property
block must be within `+0 .. +191` (the matrix occupies `+128 .. +191`).

## D15. Timeline evaluator, keyframe layout, callback ABI, pool free

**Status: not-found.** Recorded state:

* `sub_1800BF770(play)` is the dispatcher: `if (*(BYTE*)(play+32) & 4) return noop();`
  `v1 = *(int**)(play+336); if (!v1 || *v1 <= 0) return noop();` else enter the walker at
  `0x1800BF7A3`.
* `play+336` -> `{int track_count; ...}`.
* `sub_18010CF60` is **not** the evaluator (it is `afp_set_abc_method_body_data`, an action
  queue).

**Smallest missing range:** `0x1800BF7A3` .. the end of its containing function, plus the
track table at `play+336`. That is **one function read** and it yields the keyframe record
layout and the callback dispatch. The pool-free entry point and the acquire path are in the
function that writes `node+312` (the "stamp"); both are single-read tasks, not analysis
problems.

---

# E. Audio

## E16. Where the sample format is chosen

**Status: partial - the loader is identified, the format tag consumer is not.**

* `libxact!Class_Wavebank::Load` = **`sub_180006180`** (named by its own log string
  `"Class_Wavebank::Load"` at `0x18001BC80`).
  It reads a **52-byte header** into `wb+98`, then seeks to the dword at `wb+110` and reads
  **96 bytes** into `wb+0`; `*(BYTE*)wb & 1` selects the bank version:
  * bit set -> `sub_180003720(&unk_188F5FEC0, {handle,0,64}, wb+416)` (streamed bank) and
    the load polls an async status object `wb+416` slot 7 until it equals 4, with
    `Sleep(100)` between polls;
  * bit clear -> `sub_180003840(handle)` for the size, `avs2_core_55(&unk_180052D30, size, 1)`
    to allocate, then `sub_180003700(&unk_188F5FEC0, data, size, 0, 0, wb+416)`.
* **libxact contains no `WAVEFORMATEX`, no `CreateSourceVoice` and no `SubmitSourceBuffer`**
  (confirmed independently by audio-002). The sample format is therefore decoded by the
  wave-bank parser instance at `unk_188F5FEC0` via `sub_180003700`/`sub_180003720`, not by
  libxact's XAudio2 layer.

**Smallest next evidence:** read `sub_180003700` and `sub_180003720` and the object at
`unk_188F5FEC0` (its vtable gives the PCM/ADPCM decision). If those turn out to forward
into `avs2-core.dll`, the format decision is in **avs2-core**, not in either analysed
module - that module was not provided.

## E17. Command-word protocol, `execute`, and the cue array

**Status: proven-bytes** (this is one of the strongest results of the pass).

`CLibXact` cue table: **1024 entries x 568 bytes starting at `this+8`**; a slot is free when
its command dword at `+4` is 0 (`add_cue`: `v5 = (DWORD*)(this+12); v5 += 142` = +568 bytes,
`if (*v5 == 0) take it`). `this+12` is literally **`cue[0].cmd`**.

Record layout, from `add_cue` (`0x180008F40`) and the binary's own diagnostic format string
in `FinishCommand` (`0x180006B30`):
`"cue[%d] : cmd->%d p1->%d p2->%d p3->%f p4->%f p5->%f p6->%f p7->%d s1->%s s2->%s"`

| off | type | name |
|---|---|---|
| `+0x000` | byte | flag/type |
| `+0x004` | int | **cmd** (0 = free slot) |
| `+0x008` | int | p1 |
| `+0x00C` | int | p2 |
| `+0x010` | float | p3 |
| `+0x014` | float | p4 |
| `+0x018` | float | p5 |
| `+0x01C` | float | p6 |
| `+0x020` | qword | p7 |
| `+0x028` | char[260] | s1 (sound/cue name) |
| `+0x12C` | char[260] | s2 (second name) |
| `+0x230` | - | padding; total 568 |

Other proven CLibXact offsets: lock at `this+581652` (`0x8E014`, used through
`avs2_core_16/17`), an **event** at `this+581644` (`SetEvent`, `avs2_core_25`) and a
**waiter** at `this+581648` (`avs2_core_26(handle, 1, 16, buf, 60000 ms)` - a 60 s timeout
on a 16-byte item), and the completion status dword at `this+568`
(`FinishCommand`: `*(DWORD*)this + 142) = a2`).

`?execute@CLibXact@@AEAAXXZ` (`0x1800092E0`) is the worker body; `?LaunchExecute`
(`0x180007660`) is a thunk that calls it (the thread entry point). `?FinishCommand` sets the
status, signals, then blocks up to 60 s per item and dumps the whole cue table on timeout.

**Nostalgia-side `sub_180304F30` + the 128 x 24-byte cue array at `0x1813E4AD8`:**
not resolved. The array/consumer is reached only through a computed base
(`dword_1813668B0 + 0x7E228`); no data xref exists. From `sub_1802F6800` the entry shape is
`{int id; int pad; u64 handle; u64 extra}` with id set to -1 and the handle cleared after a
fade. **Smallest next evidence:** the instruction that computes `0x7E228` - scan `.text`
for the immediate `7E228h`; that names the gate function.

## E18. Engine / mastering / source vtables, `CreateFX`, remaining slots

**Status: unchanged from `audio-xaudio2.md`, re-verified; nothing new contradicts it.**

* Engine created with `CoCreateInstance` at `0x180001403` (no `XAudio2Create` import).
* Proven engine slots: `+0x20` GetDeviceDetails (0x5F0-byte details, `0x1800013DB`),
  `+0x48` CreateSubmixVoice (`0x180001644/0x18000168E`, 2 ch, 44100),
  `+0x50` CreateMasteringVoice (`0x180001593`).
* Voice slots proven by argument shape: `+0x10` SetEffectChain, `+0x30`
  SetEffectParameters (52-byte block), `+0x40` SetFilterParameters, `+0x80`
  SetOutputMatrix, `+0x90` DestroyVoice -> 19-entry base with **no IUnknown triple**.
  Source-voice slots beyond that are **[hyp]** - do not implement them from the pack.
* `CreateFX(rcx = CLSID, rdx = IUnknown**)`: only two registers are written at all three
  call sites; the safe bridge form is the explicit four-argument call with a null init
  block. This remains the one place where a bridge must match *undefined-register*
  behaviour of the original - implement it as `CreateFX(clsid, &pp, nullptr, 0)`.

**Remaining slot the bridge must implement:** none proven beyond the above. What a bridge
must provide, at minimum, is: the engine object with callable slots 4, 9, 10; mastering and
submix voices with slots 2 (`SetEffectChain`), 6 (`SetEffectParameters`), 8
(`SetFilterParameters`), 16 (`SetOutputMatrix`), 18 (`DestroyVoice`); and `CreateFX` from
`XAPOFX1_5.dll`. That set is exactly what the image calls.

---

# Consolidated corrections to earlier packs

1. `judgment-2f7c10.md` s2 / s8.3 - candidate record offsets and the comparator: replaced
   by A2 above. `sub_1800CAE90` is a stable merge sort, its `mode` argument is dead.
2. `judgment-2f7c10.md` s8.1 and `render-velocity-beats.md` s4.4 - the frame unit is
   **proven 1/60 s** (A5). No live sample needed.
- `render-velocity-beats.md` s4.1 - withdrawn: run 2 proves no glissando function touches
  `+296/+300/+316`, yet gliss objects hold live values there, so those slots are shared
  base storage and are inert for glissando (C11, PROBE_ANALYSIS.md s9). The earlier
  "held-head" reading of `+300` is also withdrawn - it is not a 0/1 flag.
4. `render-gliss.md` s1/s6.2 - `element+0x00/+0x04` are **millisecond** start/end and
   `+0x14/+0x18` are the trunc(ms*60/1000) frame conversions; the writer of `+0x14` is
   `0x1801A4945` (C10). The `+0x50` clipping value is still without a writer.
5. `input-panb.md` - unchanged and re-verified (B9).
6. `afp-ui.md` s1 - matrix convention and multiply order are now **proven** (D14).
7. `audio-xaudio2.md` - unchanged; `Class_Wavebank::Load` = `sub_180006180` and the cue
   table is decoded (E16/E17).
