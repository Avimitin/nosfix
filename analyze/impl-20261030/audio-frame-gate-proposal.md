# Frame-gate hook proposal (DISABLED - do not install)

Stream: audio. Date: 2026-10-30. Status: proposal only. Nothing in this document is
compiled into a hook, no detour exists in the tree, and nothing here is enabled by default.
The parent owns review, wiring and deployment.

Two upstream delays keep keysounds off the device timeline. Neither can be removed by
replacing the output device, and this stream does not attempt to remove them. What follows
is what a later, explicit patch would have to do, which scheduling guarantee replaces each
gate, and which evidence is still missing.

## 1. nostalgia `0x180304F30` - frame-scheduled cue array

### What the code does (evidence)

`analyze/evidence/nos202502_0x180304f30.c` (exported pseudocode):

- A cue is keyed by `a1` (sound id). If a record with that id exists, the old sound is
  faded out (`CLibXact::FadeOutSound`), the record is cleared (`+0 = -1`, `+4..+6 = 0`,
  `+8 = -1`, handle slot = 0).
- `v14 = a6 + HIDWORD(qword_181366908)` - the scheduling frame is the current frame counter
  plus `a6`.
- `v27` (a 16-byte record template) gets `+8 = current + v14` and `+12 = a5 + current + v14`
  when `v14 > 0`; the record is written into the next free 24-byte slot and its handle is
  stored in `qword_1813E4AE8[3 * slot]`.
- If `v14 <= 0`, `CLibXact::SetSoundWithParam` is called for this frame immediately.
- A dedupe scan compares `+5` (the `a3` byte), `+8 != -1` and the handle against `a2`
  before appending.

Constants:

- array base `0x1813E4AD8`, next-slot dword `0x1813E56D8`, handle array `0x1813E4AE8`
  (one qword per 24-byte record, stride 3 qwords).
- frame counter high dword `0x181366908`.
- address range / 24: `(0x1813E56D8 - 0x1813E4AD8) / 24 = 2048` records. The stream brief
  says 128 entries and the pseudocode's dedupe loop is bounded by `v23 >= 384` (= 128
  records at 24 bytes); audio-001 reports 469 live entries. **The capacity is not settled**
  (2048 by address range, 128 by the dedupe bound, 469 observed). Any hook must read the
  bound from the code it patches, not from this document.

### Why it costs latency

The sound is not started when the game decides to play it; it is started by whatever
per-frame consumer walks the array and sees `record+8 <= current frame`. The quantisation
is therefore one game frame (the unit of `HIDWORD(qword_181366908)`, believed 60 Hz but not
proven - audio-001 question B2 is still open), plus the delay the caller asked for in `a6`.

### Replacement guarantee

`KeysoundQueue` + `AudioMixer` already provide the exact guarantee the gate is emulating:

- the producer stamps `capture_qpc` (the QPC of the input/game event) or an explicit
  `due_qpc` derived from the same frame counter, **at the moment the game decides**;
- `KeysoundDispatch` maps that QPC to a render sample with `AudioTimeline` and hands the
  sink `offset_in_block ∈ [0, block_frames)`;
- `AudioMixer::StartVoice(handle, offset_in_block)` starts the voice on exactly that frame
  of the device timeline, and a late event is reported (`late_frames`) instead of being
  moved to the next frame boundary.

So the replacement is: **start at the mapped device sample, not at the next game frame.**
A hook would therefore (a) capture the QPC and the id/parameter bytes at `0x180304F30`
entry, (b) publish them to the command consumer as a `KeysoundRequest` with `due_qpc`
converted from `a6` only if `a6` is proven to be a frame count, and (c) leave the game's
array untouched so vanilla behaviour remains a valid rollback.

### Preconditions before any such patch may be written

1. audio-001 B2: the per-frame consumer's RVA and the retirement write. Without it the
   array cannot be read race-free and a hook cannot know when a record is freed.
2. The unit of the frame counter and of `a6` (frames vs milliseconds). A wrong unit is a
   wrong due time, which is worse than the gate.
3. `0x1813E4AD8` capacity (2048 vs 128 vs 469) and whether a record may be reused while
   the sound is still fading out.
4. Proof that `0x180004870` may be called from the producer thread (audio-001 B1). Until
   then a replacement must route through the existing worker, not call it directly.

### Rollback / fail-closed rules

- Disabled by default; a single config gate, default off.
- If the array base, counter address or capacity does not match the expected bytes, refuse
  to patch and log once.
- The hook must never allocate, log or block inside the game frame; it publishes a POD
  request and returns.
- If the queue is full, the event is dropped and counted (`rejected_overflow`), never
  retried inline.
- No `SetSoundWithParam` call from the audio thread; the audio thread only starts a mixer
  voice.

## 2. libxact `0x1800092E0` - `Sleep(2)` worker poll

### What the code does (evidence)

`analyze/evidence/verified_xact_0x1800092e0.c`:

- `CLibXact::execute` copies the command block, takes a lock (`avs2_core_16/17`), reads the
  command dword at `this+12`, releases the lock, and **`Sleep(2)` at `0x18000942F` when the
  command dword is 0**, then switches on the command.
- `case 2` (`0x18000A869`) tears the engine down (`sub_1800027D0`), finishes or deletes the
  cue, `CoUninitialize`, and returns: it is the shutdown case of the same loop, not a
  separate reset function (confirms the parent's note).
- **Corrected by audio-002:** the indirect calls in this loop (`[rax+0x40]`, `[rax+0xA8]`,
  `[r8+0xB0]`, `[rax+0x88]`) are on `qword_188F75800`, libxact's own sound-control
  singleton, not on an IXAudio2 voice. `[+0x88]` receives the string `"COIN"`, `[+0xA8]` a
  0x104-byte name buffer, `[+0xB0]` a 16-bit id plus a float. There is no
  `CreateSourceVoice` and no `SubmitSourceBuffer` anywhere in libxact; audio submission is
  done inside avs2-core (`avs2_core_79/82/86`, the `"%s.xsb"` cue path). The worker is
  therefore a *command and cue-state* loop, not a voice poll - which makes removing its
  `Sleep(2)` a scheduling change on libxact's own object, not on XAudio2.

### Why it costs latency

A command that arrives just after the poll is not seen for up to 2 ms, plus the frame
period of whatever game code produced it. In the same worker thread it also serialises
voice creation with command handling.

### Replacement guarantee

The bridge replaces the *poll* with a **bounded command queue that the audio thread drains
once per block**:

- every engine/voice call enqueues a POD command (`AudioMixer`'s SPSC ring, 256 entries)
  and returns immediately; no call waits for the audio thread;
- the render callback applies all pending commands at the start of the block, so a command
  posted during block *N* is audible in block *N+1* - a bounded one-block latency instead
  of an unbounded 0..2 ms sleep;
- a full ring is refused and counted (`command_overflow`), never silently dropped;
- voice creation still happens on the owner thread (the mixer only allocates a slot from a
  bitmask and writes fields before publishing the command), so no lock is taken in the
  callback.

Removing the `Sleep(2)` therefore does **not** require a new hook on `0x1800092E0`: it
requires the caller of `SetSoundWithParam` to stop depending on the worker's poll loop for
latency, which is the same change as section 1. A direct hook would only be justified if a
measurement shows the worker itself is the bottleneck after the frame gate is gone.

### Preconditions before any such patch may be written

1. audio-001 B1: the command word at `this+12`, its writers, and whether
   `0x180004870` is worker-thread-only.
2. Where avs2-core takes over: the handoff from `CLibXact::execute` into
   `avs2_core_79/82/86`, and whether that path is thread-safe from a producer other than
   the worker.
3. A measurement of the worker's queue depth and service time under load.

### Rollback / fail-closed rules

- No patch that removes `Sleep(2)` may be enabled without the command word's protocol. A
  wrong handshake could drop commands silently.
- Any replacement must preserve the shutdown case (`0x18000A869`) exactly, including
  `CoUninitialize`.

## 3. What this stream did instead

- `src/audio_mixer.cpp`: bounded command ring, sample-accurate starts, loops, callbacks,
  allocation-free render.
- `src/xaudio2_bridge.cpp`: the engine/voice surface libxact calls, with the proven slots
  from audio-001 and rejection stubs for everything else.
- `src/asio_audio_host.cpp`: one render callback that dispatches keysounds and renders the
  mixer on the same `AsioRenderContext` timeline.
- No hook is installed, no config key exists, and no code in this tree can enable one.
