# High-refresh timebase evidence

Status: **IDA analysis only.** No game code or hook behavior was changed in this
pass.

This is the evidence base for removing the 60 Hz assumption. It supersedes
guesses in earlier notes. The full site table is at the end of this document;
raw extracts are in:

- `analyze/evidence/frame_counter_xrefs.csv`
- `analyze/evidence/frame_counter_decomp.txt`
- `analyze/evidence/frame_counter_table.md`
- the raw IDA database remains local under `analyze/work/` (not committed).

## Method

- IDA Pro 9.4, `analyze/work/nostalgia_shared.i64`, the build already pinned
  by `VerifyNostalgiaModule`.
- Enumerated every xref to `dword_18070A610` with `ida_xref` and recorded the
  containing function, access type and instruction.
- Decompiled all 57 containing functions and extracted the context around
  every reference.
- Followed the writers/readers of the related rate and offset globals.
- Dumped the two rate-selection tables and the default judgment-window values.
- Function role labels are descriptive inferences from decompiled behavior,
  not original symbols. Where IDA had no function definition, the label names
  the address instead.

## 1. The counter is one 32-bit timeline field

`dword_18070A610` is the first field of the music timeline object:

- `sub_1802FC810(a1, chart)` initializes `a1+0` to `-200`, copies the chart
  end frame from `chart+168` to `a1+4`, and resets tempo/beat state.
- `sub_1802FC9D0(a1)` is the only increment path:
  - `v3 = *(_DWORD *)a1 + 1; *(_DWORD *)a1 = v3;`
  - It then advances the tempo event index and computes the beat position.
- The value is 32-bit signed. It is reset to `-200` for preroll, then counts
  up through the song.

Exactly three sites call the timeline tick:

| Site | Caller | Form |
|---|---|---|
| `0x1801975ef` | `sub_180197330` | `jmp sub_1802FC9D0` |
| `0x1801ad981` | `sub_1801AD660` | `call sub_1802FC9D0` |
| `0x1802ff329` | `sub_1802FF090` | `call sub_1802FC9D0` |

There is no direct `inc dword_18070A610` and no other direct arithmetic write.
The 14 direct writes are:

- **5 resets** to `0xFFFFFF38` (`-200`):
  `0x1802FC55A`, `0x1802FC629`, `0x1801978EF`, `0x1801ADC47`,
  `0x18030038F`.
- **9 address-of sites** passing or returning `&dword_18070A610`:
  `0x1802FC5FE`, `0x1801975E4`, `0x180197F26`, `0x1801AD97A`,
  `0x1801AE23D`, `0x1801AE7A4`, `0x1802FC960`, `0x1802FF322`,
  `0x1802FFB44`.

This is good news: the write set is centralized. The read set is not.

## 2. The 105-reference breakdown

| Class | Count | Meaning |
|---|---:|---|
| `DELTA` (`sub`) | 56 | `note_time - counter - offsets`, remaining time or angle input |
| `READ` (`mov`) | 24 | combined time, current time, progress input |
| `CMP` | 10 | chart marker, song end, backtrack trigger |
| `ADDR` (`lea`) | 9 | pointer passed to init/tick/accessor |
| `RESET` (`mov -200`) | 5 | timeline reset |
| `ADD` | 1 | combined time passed to a callback |
| **Total** | **105** | 57 functions |

The `DELTA` and `CMP` sites are mostly unit-agnostic **if both operands are
converted together**. They do not each need a bespoke formula. The dangerous
sites are the formula constants, the event-equality checks and the mixed-unit
offsets.

## 3. The â€œ60â€ global is a mutable tick-rate setting, not a literal

The code does not use a hard-coded 60 in most places. It uses:

- `qword_18070B2F8` â€” double tick rate;
- `dword_18070B300` â€” float tick rate;
- `dword_18070B304` â€” integer tick rate;
- `dword_18070B308` â€” rate mode index.

The table at `0x18062FD80` contains two entries:

- `59.977` (index 0)
- `60.0` (index 1)

`sub_180218AA0` case 4 only accepts a value in `[59.0, 60.0]` before writing
all three globals. `sub_18033A470` selects one of the two table entries by
index. In other words, the existing configuration path supports two nominal
60 Hz modes, not high refresh.

At 60 Hz the following identity hides a bug:

```
rate * rate == rate * 60      only when rate == 60
```

There are **22 occurrences of `dword_18070B300 * dword_18070B300` in 12
functions**. They are used as:

```
180 * delta_frames / (rate * rate)
```

The general correct denominator is `rate * 60`, not `rate * rate`. At 120 Hz
the current expression would halve the angle/beat result. This is a direct
60 Hz assumption even though the source text looks parameterized.

`sub_1802FC9D0` uses the same product for beat position:

```
beat = (counter - last_tempo_frame) / ((rate * rate) / BPM) + beat_base
```

For frames per second `r`, frames per beat are `r * 60 / BPM`; the code uses
`r * r / BPM`. Again correct only at 60.

## 4. Chart loader converts milliseconds to frames

`sub_1801A43E0` contains **14 conversions** of the form:

```
(int)(chart_ms * qword_18070B2F8 / 1000.0)
```

and uses `1000.0` at every one. These populate:

- chart length (`chart+168`);
- note start/end fields (`+20`, `+24`, `+28`);
- long-note and event arrays;
- lane/velocity data;
- chart offset fields.

`sub_1802FC810` copies `chart+168` to timeline `+4`, which is
`dword_18070A614` (end frame). `sub_1802FDB30` compares `counter >= end`.

For a microsecond timeline these conversions become:

```
chart_us = chart_ms * 1000
```

There must be no runtime `rate` multiplication in the chart path.

## 5. Two offset fields participate in almost every note comparison

Almost every note eligibility expression is:

```
note_time - dword_18070A610
          - HIDWORD(qword_181366914)   // 0x181366918
          - HIDWORD(qword_181366908)   // 0x18136690C
```

These offsets are not optional. The xref counts are:

- `0x18136690C`: **80 references**;
- `0x181366918`: **59 references**.

What is known:

- `qword_181366908` is initialized to 0; its high dword has no native writer
  besides that initialization. It is currently always 0 in the native path.
- `qword_181366914` is initialized to 2; its high dword is written by
  `sub_180277000`, `sub_1803049D0` and `sub_180305B30`. The value comes from
  chart/option data (chart record `+72` in `sub_180305B30`), and is used as an
  offset in frame units.
- `sub_1803049D0(int)` is a direct setter for `0x181366918`; its callers are
  virtual/indirect, so the unit at the call site still needs runtime evidence.

If the counter becomes microseconds but these offsets remain frames, every
note comparison will be wrong by orders of magnitude. They must be converted
in the same change.

## 6. Judgment windows are fixed frame counts

`sub_1800CB9A0` initializes:

| Global | Frames | At 60 Hz |
|---|---:|---:|
| `dword_1813668E0` | 9 | +150 ms |
| `dword_1813668E8` | 7 | +116.7 ms |
| `dword_1813668F0` | 4 | +66.7 ms |
| `dword_1813668F8` | 2 | +33.3 ms |
| `dword_1813668FC` | -2 | -33.3 ms |
| `dword_1813668F4` | -4 | -66.7 ms |
| `dword_1813668EC` | -6 | -100 ms |
| `dword_1813668E4` | -11 | -183.3 ms |

Only one write was found for each default. They are compared directly against
`note_time - counter - offsets`. At 120 Hz, leaving them as frame counts would
halve every window. They become microsecond boundaries in the same load-time
conversion as the notes.

## 7. Tempo events use exact frame equality

`sub_1802FC9D0`:

```
v3 = counter + 1;
counter = v3;
if (tempo_index < tempo_count) {
    event_frame = tempo_array[tempo_index].frame;
    if (v3 == event_frame) {
        bpm = tempo_array[tempo_index].bpm;
        ...
        tempo_index++;
    }
}
```

At 60 Hz, `+1` hits every integer frame. At a high-refresh or late update,
`v3` can jump over an event and the tempo change is silently skipped. This must
become a range check / while loop:

```
while (tempo_index < tempo_count && counter_us >= event_us) { ... }
```

That is a correctness fix independent of input judgment.

## 8. Formula families and replacement rules

| Current form | Sites | Microsecond replacement |
|---|---:|---|
| `note - counter - offA - offB` | 56 `sub` | same arithmetic, all operands `_us` |
| `counter >=/</== event` | 10 `cmp` | `counter_us >= event_us`; use `while` for crossings |
| `counter` moved into combined time | 24 `mov` | `counter_us`, `offA_us`, `offB_us` |
| `offA + offB + counter` | 1 `add` | same, all `_us` |
| reset `-200` | 5 | `-3,333,333 us` (truncated toward zero from `-3,333,333.333 us`) |
| `&counter` | 9 | `&song_time_us`; no 60-unit pointer |
| `180 * delta / (rate * rate)` | 22 | `180 * delta_us / 60,000,000` |
| `delta / ((rate * rate) / BPM)` | 1 | `delta_us * BPM / 60,000,000` |
| `counter * 1000 / rate` | 1 | `counter_us / 1000` |
| `counter / end` progress ratio | 2 | unchanged if both are `_us` |

The last row matters: `sub_1801BD160` and the section-progress path in
`sub_1801BC520` are ratios. Converting both numerator and denominator to
microseconds leaves them correct without a special formula.

Rounding must be chosen once and used everywhere. The table uses truncation
toward zero for the `-200` frame preroll (`-3,333,333 us`); the alternative is
`-3,333,334 us` if the preroll must not be shortened. Chart milliseconds to
microseconds is exact (`ms * 1000`). No runtime conversion is allowed.

## 9. Additional time fields that must move with the counter

These are outside the 105 direct xrefs but are part of the same timeline:

- `dword_18070A740` (frame adjustment): 6 references. `sub_1801C3FE0`
  converts it with `* 1000 / rate`; the tick path decrements it. It is in frame
  units today.
- `dword_18070A750` (clock correction state): 4 references. It gates the
  main-loop milliseconds calculation.
- `dword_18070A614` (end frame): 5 references; copied from chart `+168`.
- `qword_18070B2F8` / `dword_18070B300`: rate selection, not time. They should
  remain only for frame pacing/output configuration, never for time conversion.
- `HIDWORD(qword_181366908)` / `HIDWORD(qword_181366914)`: offsets described
  above.

The host side currently assumes frames as well:

- `src/timed_rendering.cpp:385` reads `0x70A610` and compares `<= 0`;
- `src/timed_rendering.cpp` computes `frame_adjust * 1000 / rate`;
- `src/motion_hooks.cpp:298` reads `0x70A610` as `music_frame`;
- `src/diagnostics.cpp:195` logs it as `frame=`;
- `FrameScheduler::kLegacySimulationHz = 60` still schedules the native tail at
  60.

These must be changed in the same branch. If the native counter becomes
microseconds while these readers still treat it as frames, the host diagnostic
and prediction paths will be wrong even if the game is correct.

## 10. Open questions that need runtime evidence

1. **Offset units:** `HIDWORD(qword_181366914)` is set from chart/option data,
   but the setter callers are indirect. Need a live value paired with the chart
   and audio position to confirm it is frames and not milliseconds.
2. **Tick frequency:** confirm exactly one `sub_1802FC9D0` call per native
   update. If any path calls it more than once, an elapsed write needs a
   single-writer guard.
3. **Counter width:** 32-bit signed microseconds spans about Â±35.8 minutes.
   Songs fit; service/menu sessions may not. The reset paths must be shown to
   run before that range matters.
4. **Judgment-window mutation:** only the default writes were found. Confirm
   there is no external/virtual writer that changes the windows at runtime.
5. **59.977 mode:** chart frames are currently rate-dependent. The
   microsecond conversion should make both 59.977 and 60.0 modes use the same
   song time; validate note boundaries against the current 60.0 behavior.
6. **`qword_181366908` high dword:** no native writer outside initialization.
   Confirm no external module writes it before treating it as permanently 0.

## 11. Next step

Add a **read-only** diagnostic around the timeline tick that logs, once per
native update:

- QPC delta and tick-call count;
- `dword_18070A610`, `dword_18070A614`;
- both offset high dwords;
- `dword_18070A740`, `dword_18070A750`;
- the current rate globals and mode.

That resolves the open questions without introducing a second timing path.
Only then should the chart loader, tick and formula families be patched in one
branch.

## Appendix: all 105 direct references

| # | Site | Function | Op | Class | Current | Replacement rule |
|---:|---|---|---|---|---|---|
| 1 | `0x1801bc39f` | `unnamed`<br>chart marker advance (unnamed function 0x1801BC370) | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 2 | `0x1802fc55a` | `unnamed`<br>timeline reset A (unnamed 0x1802FC510) | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 3 | `0x1802fc5fe` | `unnamed`<br>timeline reset B returns counter pointer (unnamed 0x1802FC5B0) | `lea` | ADDR | `lea     rax, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 4 | `0x1802fc629` | `unnamed`<br>timeline reset B (unnamed 0x1802FC5B0) | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 5 | `0x180304dbb` | `unnamed`<br>remaining-time helper (unnamed 0x180304D90) | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 6 | `0x1801963a3` | `sub_180196390`<br>chart event dispatch / combined time | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 7 | `0x1801963f8` | `sub_180196390`<br>chart event dispatch / combined time | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 8 | `0x18019711d` | `sub_180197080`<br>sound cue scheduling | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 9 | `0x180197217` | `sub_1801971B0`<br>sound cue scheduling / angle | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 10 | `0x1801972c1` | `sub_1801972A0`<br>backtrack trigger at chart end | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 11 | `0x18019735e` | `sub_180197330`<br>gameplay update / cue + timeline tick | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 12 | `0x18019744d` | `sub_180197330`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 13 | `0x180197527` | `sub_180197330`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 14 | `0x1801975e4` | `sub_180197330`<br>gameplay update / cue + timeline tick | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 15 | `0x1801978ef` | `sub_180197770`<br>timeline reset | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 16 | `0x180197f26` | `sub_180197C30`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 17 | `0x1801a58cc` | `sub_1801A58C0`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 18 | `0x1801a59d1` | `sub_1801A5990`<br>note judge window | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 19 | `0x1801a5a07` | `sub_1801A5990`<br>note judge window | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 20 | `0x1801a5e7c` | `sub_1801A5DB0`<br>note animation delta (unreachable block) | `sub` | DELTA | `sub     r10d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 21 | `0x1801a60e0` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     r14d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 22 | `0x1801a61e1` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     r14d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 23 | `0x1801a6522` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 24 | `0x1801a6624` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     r14d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 25 | `0x1801a68e1` | `sub_1801A6820`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 26 | `0x1801a691b` | `sub_1801A6820`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 27 | `0x1801a6d42` | `sub_1801A6820`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 28 | `0x1801a707e` | `sub_1801A6820`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 29 | `0x1801a74f8` | `sub_1801A7450`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 30 | `0x1801a7b70` | `sub_1801A7AF0`<br>note judge / long note | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 31 | `0x1801a7d01` | `sub_1801A7AF0`<br>note judge / long note | `sub` | DELTA | `sub     ebx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 32 | `0x1801a7d8a` | `sub_1801A7AF0`<br>note judge / long note | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 33 | `0x1801a849f` | `sub_1801A8490`<br>note judge predicate | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 34 | `0x1801a85ca` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 35 | `0x1801a8699` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     edx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 36 | `0x1801a8bde` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 37 | `0x1801a90b8` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 38 | `0x1801a9b81` | `sub_1801A9AB0`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 39 | `0x1801aa38c` | `sub_1801AA340`<br>note state transition | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 40 | `0x1801aa43e` | `sub_1801AA340`<br>note state transition | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 41 | `0x1801aa545` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 42 | `0x1801aa597` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 43 | `0x1801aa9aa` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 44 | `0x1801aadc8` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 45 | `0x1801aaf5b` | `sub_1801AA4D0`<br>note judge / animation | `mov` | READ | `mov     ecx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 46 | `0x1801ab508` | `sub_1801AB410`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 47 | `0x1801ac116` | `sub_1801ABFA0`<br>combined time / chart end | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 48 | `0x1801ac3dd` | `sub_1801ABFA0`<br>combined time / chart end | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 49 | `0x1801ad44d` | `sub_1801AD3B0`<br>sound cue scheduling | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 50 | `0x1801ad547` | `sub_1801AD4E0`<br>sound cue scheduling / angle | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 51 | `0x1801ad5f1` | `sub_1801AD5D0`<br>backtrack trigger at chart end | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 52 | `0x1801ad68e` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 53 | `0x1801ad76d` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 54 | `0x1801ad847` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 55 | `0x1801ad97a` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 56 | `0x1801adc47` | `sub_1801ADB00`<br>timeline reset | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 57 | `0x1801ae23d` | `sub_1801ADFB0`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 58 | `0x1801ae7a4` | `sub_1801AE480`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 59 | `0x1801b8bf6` | `sub_1801B8660`<br>remaining-time readout | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 60 | `0x1801bc2ee` | `sub_1801BC240`<br>chart marker advance | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 61 | `0x1801bc45e` | `sub_1801BC3F0`<br>chart marker advance | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 62 | `0x1801bc5a7` | `sub_1801BC520`<br>chart section progress / marker | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 63 | `0x1801bc647` | `sub_1801BC520`<br>chart section progress / marker | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 64 | `0x1801bd176` | `sub_1801BD160`<br>progress bar ratio | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 65 | `0x1801c4787` | `sub_1801C3FE0`<br>music position in ms / audio sync | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 66 | `0x18021ad30` | `sub_18021ACF0`<br>combined time scene gate | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 67 | `0x1802f42b5` | `sub_1802F4220`<br>note judge window | `sub` | DELTA | `sub     ebp, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 68 | `0x1802f451e` | `sub_1802F4220`<br>note judge window | `sub` | DELTA | `sub     ebx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 69 | `0x1802f45ae` | `sub_1802F4220`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 70 | `0x1802f4e2f` | `sub_1802F4E20`<br>note judge predicate | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 71 | `0x1802f4f58` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 72 | `0x1802f500a` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     edx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 73 | `0x1802f54ef` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 74 | `0x1802f598c` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 75 | `0x1802f625e` | `sub_1802F6190`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 76 | `0x1802f6822` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 77 | `0x1802f6902` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 78 | `0x1802f6ace` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 79 | `0x1802f6c6c` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 80 | `0x1802f6ec7` | `sub_1802F6DF0`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 81 | `0x1802f6f1a` | `sub_1802F6DF0`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 82 | `0x1802f7456` | `sub_1802F73B0`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 83 | `0x1802f7903` | `sub_1802F7900`<br>note judge predicate | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 84 | `0x1802f8886` | `sub_1802F7C10`<br>judge / scoring combined time | `mov` | READ | `mov     r15d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 85 | `0x1802f959a` | `sub_1802F7C10`<br>judge / scoring combined time | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 86 | `0x1802f9a11` | `sub_1802F7C10`<br>judge / scoring combined time | `mov` | READ | `mov     r12d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 87 | `0x1802fc960` | `sub_1802FC960`<br>timeline counter accessor | `lea` | ADDR | `lea     rax, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 88 | `0x1802fdb9e` | `sub_1802FDB30`<br>gameplay controller / end check | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 89 | `0x1802fdcc3` | `sub_1802FDB30`<br>gameplay controller / end check | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 90 | `0x1802fe00e` | `sub_1802FDB30`<br>gameplay controller / end check | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 91 | `0x1802fec3d` | `sub_1802FEBA0`<br>sound cue scheduling | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 92 | `0x1802fed37` | `sub_1802FECD0`<br>sound cue scheduling / angle | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 93 | `0x1802fee0e` | `sub_1802FEDC0`<br>backtrack trigger at chart end | `mov` | READ | `mov     r15d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 94 | `0x1802ff13d` | `sub_1802FF090`<br>gameplay update / timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 95 | `0x1802ff207` | `sub_1802FF090`<br>gameplay update / timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 96 | `0x1802ff2d9` | `sub_1802FF090`<br>gameplay update / timeline tick | `add` | ADD | `add     ecx, cs:dword_18070A610` | `song_time_us + offset_us` |
| 97 | `0x1802ff2f5` | `sub_1802FF090`<br>gameplay update / timeline tick | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 98 | `0x1802ff322` | `sub_1802FF090`<br>gameplay update / timeline tick | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 99 | `0x1802ff32e` | `sub_1802FF090`<br>gameplay update / timeline tick | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 100 | `0x1802ffb44` | `sub_1802FF8D0`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 101 | `0x18030038f` | `sub_1803001F0`<br>timeline reset | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 102 | `0x180304bf0` | `sub_180304BF0`<br>timeline counter accessor | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 103 | `0x1803074d0` | `sub_1803074D0`<br>angle helper predicate | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 104 | `0x180307540` | `sub_180307540`<br>angle helper | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 105 | `0x180307580` | `sub_180307580`<br>combined time helper | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
