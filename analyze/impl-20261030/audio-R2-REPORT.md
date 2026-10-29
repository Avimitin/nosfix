# audio R2 REPORT - libxact cue table, command handoff, frame-gate scheduler

Stream: audio. Branch: `native/r2-audio-20261030`. Date: 2026-10-30.
Round 2. Builds on the round-1 bridge (`native/audio-20261030`, merged).
Parent owns review, wiring, merge and deployment.

## 0. Constraint compliance

- Worktree only. No other worktree or the main repository was modified.
- No IDA, IDAPython or idalib. The evidence is `analyze/impl-20261030/` plus the
  exported pseudocode in `analyze/evidence/`.
- `nosfix.ini`, `src/config.cpp`, `src/bootstrap.cpp`, `src/game_hooks.cpp`,
  `src/hooks.cpp` untouched (git status confirms). `native_*`, `input_*`,
  `timed_*`, `motion_hooks.*` untouched.
- No hook, detour, thread or default-on path was added. No config key exists. No
  real audio device is opened: every test target uses `FakeAsioDriver` only, and
  no test calls `AsioAudioHost::Open` with a registered CLSID.
- No merge, rebase, push or deploy. One commit on the stream branch.

## 1. What changed

### 1.1 `include/nosfix/xaudio2_bridge.hpp`, `src/xaudio2_bridge.cpp` - `namespace xact`

The proven CLibXact cue table and the owner -> worker handoff, encoded as a
layout model over *caller* memory. Nothing reaches a game object by itself.

- Constants and `static_assert`s: 1024 records x 568 bytes at `this+8`
  (`kCueTableBytes` = 581632, `kCueTableEnd` = 581640), command word at
  `record+4` (`this+12` is literally `cue[0].cmd`), event `this+581644`,
  waiter `this+581648`, lock `this+581652`, minimum object size 581656, waiter
  item 16 bytes, free command `0`, shutdown command `2`.
- `XactCueRecord` (packed, 568 bytes) with the field list from `add_cue`
  (`0x180008F40`) and `FinishCommand`'s own diagnostic format string:
  `flag`@0, `cmd`@4, `p1`@8, `p2`@0xC, `p3..p6`@0x10..0x1C, `p7`@0x20,
  `s1[260]`@0x28, `s2[260]`@0x12C, padding to 0x238.
- `XactCueTable`: attach/validate (size and 8-byte alignment, fail closed),
  `Record(index)`, acquire/release `Command(index)` (atomic ref on the caller's
  word - the command word is the publication point), `FindFree()` with the
  original's lowest-free-slot scan.
- `XactCommandRing`: bounded 256-entry SPSC ring of cue indices plus
  `Post(record)` and `DrainAtBlockStart(sink, completion)`. This is the
  replacement for the `Sleep(2)` poll at `0x18000942F`: the producer writes the
  record with the command word still free, pushes the index and publishes the
  word; the audio block start applies every pending command once. A command
  posted after a drain is applied by the next block
  (`kHandoffLatencyBlocks == 1`).
  - ring full -> `RingFull`, counted, and the table is *not* touched;
  - all 1024 slots busy -> `TableFull`, counted;
  - command word 2 -> `shutdown_`, counted, never handed to the sink, everything
    queued behind it counted as abandoned (matches the `0x18000A869` shutdown
    case that ends the worker loop), later posts refused and counted;
  - command words other than 0 and 2 are transported verbatim, applied and
    counted (`unknown_command`) - no semantics are invented for them;
  - `SetGeneration(g)` clears every non-free command word, empties the ring and
    counts both, so a device/game reset cannot apply a stale cue;
  - a null sink drains nothing, so a command is never retired unsigned;
  - refused applies are counted and completed with nonzero status.
- Completion is published through `XactCompletionSink` (status 0/1), *not*
  through the event/waiter slots, because the status dword offset is unresolved
  (section 4.1). The event, waiter and lock offsets are recorded and never
  read or written.

### 1.2 `include/nosfix/keysound_dispatch.hpp`, `src/keysound_dispatch.cpp` - `FrameGateSchedule`

A tested model of nostalgia `sub_180304F30` (the frame-scheduled cue array).
No hook, no call into libxact, frame thread only, allocation and lock free.

- `FrameGateRecord` (packed, 24 bytes): `sound_id`@0, `flags`@4, `param`@5 (a3),
  `param2`@6 (a4), `start_frame`@8, `end_frame`@12, `handle`@16 - which is
  exactly `qword_1813E4AE8[3 * slot]`. Capacity 128: `(0x1813E56D8 -
  0x1813E4AD8) / 24 = 0xC00 / 24 = 128`, agreeing with the pseudocode's own
  dedupe bound at `0x1803050DB` (`v23 >= 384` with a 3-quadword stride).
  `audio-frame-gate-proposal.md` quotes 2048 from the same range; that is an
  arithmetic slip and is corrected here.
- `Schedule(request, current_frame)` reproduces the call order:
  1. gate flag clear -> `Refused` (the original returns immediately);
  2. `due = a6 + current` as a signed 32-bit add that may wrap;
  3. a record with the same id is cleared first (`replaced_same_id`), matching
     the fade-out + clear sequence at `0x180304FE6..0x180305016`;
  4. `due > 0` -> stored with `start_frame = current + a6` and
     `end_frame = a5 + start` when `a5 != -1` -> `Deferred`;
  5. `due <= 0` -> played immediately, after the dedupe scan (param 1..0x58, a
     *stored pending* record with the same handle) -> `Immediate*`, stored with
     `start_frame = -1` as the "already started" marker;
  6. a full array drops a deferred request (`TableFull`) and loses only the
     marker of an immediate one (`ImmediateUnrecorded`).
- `RetireDue(current_frame, out, capacity)` is the per-frame consumer model:
  index order, `start_frame != -1 && start_frame <= current`, then the same
  clear sequence. A too-small out array leaves the rest stored and counts them.
- `SetGeneration(g)` drops every stored cue without retiring it, counted.

### 1.3 `include/nosfix/asio_audio_host.hpp`, `src/asio_audio_host.cpp`

- `XactCommandHookup` + `SetXactCommandHookup()`: an optional ring, sink and
  completion callback. `Render()` drains it at the very start, before the
  position check, before keysound dispatch and before the mixer, so a silent or
  paused block cannot stall the command queue and a command posted during block
  N is applied at the start of block N+1. No allocation, no lock, no logging.
- `AsioHostStats::xact_commands_applied`; `Close()` clears the hookup so a
  destroyed ring is never read.
- Default state is a null ring: the host behaves exactly as in round 1.

### 1.4 Tests (assertions added inside existing targets, no new target)

- `tests/xaudio2_bridge_tests.cpp`: `TestXactCueTableLayout`,
  `TestXactCommandHandoff`, `TestXactCommandRingOverflow`,
  `TestXactCommandGenerationAndShutdown` - every constant and every `offsetof`,
  `this+12 == cue[0].cmd` on raw bytes, short/misaligned/detached refusals,
  slot scan, one-block handoff, once-only drain, completion statuses, unknown
  command counting, ring overflow with the table untouched, table-full, free
  command refusal, generation clearing (and its no-op), shutdown refusal.
- `tests/keysound_dispatch_tests.cpp`: `TestFrameGateLayout`,
  `TestFrameGateDeferred`, `TestFrameGateImmediate`,
  `TestFrameGateOverflowAndGeneration` - record offsets, disabled gate,
  `current + a6` storage, `a5` end frame, `a6 == 0` still deferring, retire at
  the exact frame and never twice, immediate marker never retiring, dedupe hit
  and its param/handle bounds, same-id replacement, table full (deferred and
  immediate), generation drop, `-1`/null-handle refusal, 32-bit wrap,
  too-small out array.
- `tests/asio_backend_tests.cpp`: `TestXactCueHandoffThroughTheHost` - post
  applies nothing until a block, drain at block start through the fake driver,
  a position-less block still drains, clearing the hookup stops the drain and
  leaves the command queued.
- Source-voice surface: unchanged and still prototype-only
  (`kLibxactUsesSourceVoice == false`, asserted at `xaudio2_bridge_tests.cpp:913`
  and in `TestLibxactProvenSurface`). Source format stays fail-closed; nothing in
  this round touches it.

## 2. Evidence used

| Fact | Source |
|---|---|
| cue table 1024 x 568 at `this+8`, cmd at `+4`, `this+12` = `cue[0].cmd` | `ABI_COMPLETION.md` E17 (proven-bytes), `add_cue` `0x180008F40` |
| record field list and 568-byte size | `FinishCommand` `0x180006B30` format string, E17 table |
| lock `this+581652` (`0x8E014`), event `this+581644` (`0x8E00C`), waiter `this+581648` (`0x8E010`, `avs2_core_26(handle, 1, 16, buf, 60000)`) | E17 |
| `Sleep(2)` at `0x18000942F` when cmd is 0; shutdown case `0x18000A869` | E17, `verified_xact_0x1800092e0.c`, `audio-frame-gate-proposal.md` s2 |
| 128 x 24-byte array `0x1813E4AD8`, end `0x1813E56D8`, handles `0x1813E4AE8`, gate `word_1813E472C`, frame counter `0x181366908` | `analyze/evidence/nos202502_0x180304f30.c`, E17 |
| clear sequence, dedupe bound `0x1803050DB`, replacement fade | same pseudocode |
| source format decided outside libxact (avs2-core cue/xsb path) | E16, audio-002 errata in round-1 report |

## 3. Build and tests

Canonical command: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
scripts\build.ps1 -RunTests`.

Result: the same configure/build NMake commands as `scripts/build.ps1`, Release
`/O2 /DNDEBUG /W4 /permissive- /EHsc /utf-8`, **no warnings** from this stream's
files, **47/47 ctest entries pass** (the pre-existing 47; this round adds
assertions inside three of them rather than new test targets).

The script itself could not be used here: it writes its batch to
`%TEMP%\nosfix-build-env.cmd`, which every worktree on this host shares, and a
concurrent worker in `impl-20261030b\render-ui` overwrote it. The observed run
executed the render-ui paths and exited 3 (`The system cannot find the path
specified` / `nosfix build failed with exit code 3`) even though my build had
reached 100%. Workaround, no script edit: the identical commands in a private
batch under `build\_env\build.cmd` (gitignored), with `TMP`/`TEMP` pointed at
`build\_tmp`, then `ctest --test-dir build`. The forward-slash
`-DCMAKE_RC_COMPILER`/`-DCMAKE_MT` values already present in the script are used
unchanged. Disclosure: that one script run may have started a configure/build in
the render-ui worktree before returning; nothing in this worktree was built by
it.

## 4. Remaining blockers (exact)

1. **The stored start frame may add the frame counter twice.** `0x180305064`
   stores `dword_1813E56D8 + v14`, but `dword_1813E56D8` is the *end address* of
   the array, so one operand of that store is mis-rendered and the second term
   is unknown. This model uses the brief's rule (`start = current + a6`,
   `end = a5 + start`). Settling it needs the instruction bytes at
   `0x180305057..0x180305064`, which are not in this worktree. A wrong answer
   shifts every start by `current` frames - worse than the gate - so nothing is
   live-wired.
2. **The per-frame consumer has no proven RVA (round-1 B2).** `RetireDue()` is
   a model of the documented behaviour, not a call site. Until the consumer and
   its retirement write are named, a replacement cannot know when a record is
   freed or reused while a fade is still running.
3. **What the immediate-path dedupe does to the found record is unresolved.**
   `sub_1802BCAD0` is rendered with the array base as its argument, so the model
   counts the hit (`dedupe_stopped`) and leaves the record in place. A wrong
   action would double-start a sound.
4. **`FinishCommand`'s status dword offset is unusable as given.** The pack
   renders the store as `this+568`, which would land inside `cue[0]` at 0x230;
   the only consistent 4-byte hole is 581640, between the table end and the
   event at 581644. `kStatusWordProven == false`; no code writes or reads a
   status word, and completion goes through a callback the parent can bind to
   the event/waiter slots once their widths are proven. The widths of the
   `0x8E00C`/`0x8E010` fields (4-byte loads, cannot both be HANDLE-sized in 8
   bytes) are likewise unproven, which is why the ring never touches them.
5. **Command-word values other than 0 (free) and 2 (shutdown) are unknown.**
   They are transported, applied and counted, never interpreted.
6. **audio-001 B1 is still open:** whether `0x180004870` may be called from a
   producer thread other than the worker. The model never calls libxact; it only
   publishes records and ready items.
7. **Live wiring is parent-owned and untested on hardware.** Routing libxact to
   `Xa2BridgeCreate`, installing the ring on the real render callback
   (`SetXactCommandHookup`) and mapping a cue id to a source voice are all
   unwired here. The one-block latency is a property of the model, not a
   measurement.
8. **Source format is still undecided** (audio-002: no `WAVEFORMATEX`,
   `CreateSourceVoice` or `SubmitSourceBuffer` in libxact; the format lives in
   avs2-core's xsb path). The source-voice surface stays prototype-only and
   fail-closed - unchanged by this round.

## 5. Merge notes

- Additive only: 9 files, +1634 lines, 0 deletions. No new source file, no new
  test target, **no `CMakeLists.txt` change** at all.
- The only shared-file surface added is `AsioHostStats` (one field),
  `AsioAudioHost::SetXactCommandHookup`/`XactCommands` (opt-in, null by default)
  and the new `nosfix::audio::xact` namespace.
- `XactCueTable::Command/SetCommand` use `std::atomic_ref` on the caller's
  command word; `Attach` refuses a buffer that is too small or not 8-byte
  aligned, so the word is always 4-byte aligned.
- `XactCommandRing` is single-producer/single-consumer and holds indices only;
  `DrainAtBlockStart` copies the 568-byte record into a stack event, so no
  pointer into the game object escapes the drain.
- `FrameGateSchedule` is frame-thread-only (plain counters, no atomics); do not
  call it from the audio thread.
- Working tree files are CRLF; git normalises them to LF on commit
  (`core.autocrlf=true`), so the diff is insert-only.

## 6. Files

```
include/nosfix/xaudio2_bridge.hpp    src/xaudio2_bridge.cpp     (xact cue table + command ring)
include/nosfix/keysound_dispatch.hpp src/keysound_dispatch.cpp   (FrameGateSchedule)
include/nosfix/asio_audio_host.hpp   src/asio_audio_host.cpp     (optional handoff drain)
tests/xaudio2_bridge_tests.cpp       tests/keysound_dispatch_tests.cpp  tests/asio_backend_tests.cpp
analyze/impl-20261030/audio-R2-REPORT.md
```
