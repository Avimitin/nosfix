# audio REPORT - XAudio2 2.7 bridge and ASIO output prototype

Stream: audio. Branch: `native/audio-20261030`. Date: 2026-10-30.
Parent review required before any wiring, merge or deployment.

## 0. Constraint compliance

- Only this worktree was modified. No IDA, IDAPython, idalib or decompiler was used; the
  analysis worker's exports and its `answers/audio-001.md`, `answers/audio-002.md` and `evidence/audio-xaudio2.md` (including the audio-002 erratum)
  are the binary evidence.
- No hook is installed, no detour exists in the tree, no config key was added, and nothing
  can enable the bridge at runtime: `nosfix.ini`, `config.*`, `bootstrap.*`,
  `game_hooks.*`, `hooks.*`, `native_*`, `input_*`, `timed_*` and `motion_hooks.*` are
  untouched. The engine is only reachable by calling `Xa2BridgeCreate` explicitly.
- No merge, rebase, push or deploy. One commit on the stream branch.
- **Disclosure:** during test development, one draft test opened the locally registered
  ASIO driver (`{5AF0A297-...}`, TOPPING/TUSBAudio) through `AsioAudioHost::Open` to prove
  that a bogus CLSID fails closed. `Open` succeeded, so `ASIOInit`/`CreateBuffers` ran for a
  few milliseconds; `ASIOStart` was never called, no audio was rendered, and the host was
  destroyed at scope exit (`ASIOStop` not needed, buffers disposed, `ASIOExit`). The test
  now uses an unregistered CLSID (`{DEADBEEF-...}`) and no test opens hardware. This is
  reported rather than hidden; the parent may want to re-run the suite on a machine without
  the driver.
- No real device is opened by the final test suite; only `FakeAsioDriver` is used.

## 1. What changed

### 1.1 `include/nosfix/xaudio2_bridge.hpp`, `src/xaudio2_bridge.cpp` (new)

An XAudio2 2.7-shaped engine surface for the libxact client, built as explicit POD vtables
so the byte offsets are testable, not compiler-dependent. It is split into three tiers
(section 2.3): the surface libxact is *proven* to call, the proven-but-unused layout, and a
speculative source-voice surface that is **prototype-only and not claimed to be
libxact-compatible**.

- Engine slots 0..120: QueryInterface (IID_IUnknown, IID_IXAudio2 2.7), AddRef, Release,
  GetDeviceCount (=1), GetDeviceDetails (2.7 layout, see 2.2), Initialize (idempotent),
  Register/UnregisterForCallbacks, CreateSourceVoice (prototype-only slot, no libxact
  call site), CreateSubmixVoice, CreateMasteringVoice, StartEngine, StopEngine,
  CommitChanges, GetPerformanceData
  (counted only: the 2.7 struct size is not in this tree, so nothing is written),
  SetDebugConfiguration.
- Voice slots 0..144: GetVoiceDetails, SetOutputVoices, SetEffectChain,
  Enable/DisableEffect, GetEffectState, Set/GetEffectParameters, Set/GetFilterParameters,
  Set/GetOutputFilterParameters, Set/GetVolume, Set/GetChannelVolumes,
  Set/GetOutputMatrix, DestroyVoice.
- Source slots 152..224: Start, Stop, SubmitSourceBuffer, FlushSourceBuffers,
  Discontinuity, ExitLoop, GetState, Set/GetFrequencyRatio, SetSourceSampleRate.
  **Prototype-only**: libxact.dll contains no CreateSourceVoice, no SubmitSourceBuffer and
  no WAVEFORMATEX (audio-002), so `kLibxactUsesSourceVoice` is false and nothing here is
  claimed for libxact. The order is the SDK declaration order inferred from the proven
  19-entry base (Start +0x98 ... SetSourceSampleRate +0xE0); only a live vtable dump can
  confirm it. The mixer needs a source voice, so the surface stays - marked, not removed.
- Mastering voice: GetChannelMask at 152.
- Every table ends with an 8-slot rejection tail (engine 128..184, voice 232..288) that
  returns `XAUDIO2_E_INVALID_CALL` and counts the call, so an out-of-layout client call is
  a counted failure instead of a jump into unrelated memory.
- Operation sets: SetVolume / SetFrequencyRatio / SetOutputMatrix / Start / Stop / ExitLoop
  with a nonzero operation set are queued (bounded, 32) and applied by
  `CommitChanges(set)` or `CommitChanges(0xFFFFFFFF)`; `CommitChanges(0)` is refused.
- Effect chains (up to 4 descriptors) are copied and counted; **no DSP runs** and the XAPO
  objects are never invoked. SetEffectParameters records that a block was set but does not
  copy it (the block size is not known here; copying 32 bytes would over-read the client
  stack). GetEffectParameters returns zeros.
- Formats accepted (prototype boundary, **not** libxact-derived): PCM 16/24/32 and IEEE
  float32, plain or `WAVE_FORMAT_EXTENSIBLE`; block_align must match; rate 8 kHz..384 kHz;
  1..8 channels. ADPCM (tag 2), XMA, WMA, 8-bit PCM and unknown sub-formats are refused and
  counted. audio-002 withdrew the earlier claim that `0x180003143` was a CreateSourceVoice
  call site (it is SetFilterParameters), so libxact gives no evidence for or against these
  formats; the format is chosen in the cue/xsb path inside avs2-core.
- Mastering voice: only one, only device 0, only flags 0, and only the configured channel
  count and rate (no remix, no resample). CreateSourceVoice with an implausible
  `MaxFrequencyRatio` uses the documented default 2.0, because no libxact call site pins
  that argument either.
- Filters: `SetFilterParameters` is a **proven libxact path** (audio-002: three parallel
  12-byte `{Type, Frequency, OneOverQ}` records at CLibXact +0x58/+0x64/+0x70 handed to the
  voices at +0x15970/+0x15978/+0x15980 with OperationSet 0). They are recorded and counted
  but **no filter DSP runs**, so a filtered cue is audible unfiltered here. This is a known
  fidelity gap on a proven path, not a speculative one.

### 1.2 `include/nosfix/audio_mixer.hpp`, `src/audio_mixer.cpp` (new)

Bounded, allocation-free mixer on the device render timeline.

- 64 voices, 16 buses, 8 channels, block <= 2048 frames; every buffer is a member array;
  `Prepare` allocates nothing.
- Owner thread -> audio thread through one 256-entry POD command ring; a full ring is
  refused and counted. Audio thread state (queue, cursors, gains) is never written by the
  owner after a voice is published.
- Per voice: source-format decode (PCM 16/24/32, float32), linear interpolation driven by
  an exact 32.32 step (`ratio * source_rate / device_rate`), `PlayBegin/PlayLength`,
  `LoopBegin/LoopLength/LoopCount` (255 = infinite), `XAUDIO2_END_OF_STREAM`, a 4-deep
  buffer queue, per-channel gain from volume/pan, `SamplesPlayed` in source frames.
- Callbacks: processing pass start/end, stream end, buffer start/end, loop end, voice error
  — all POD function pointers, called from the render thread.
- `StopAllVoices()` for a device generation change; stats for underruns, starved voices,
  clipped frames, command/buffer overflow and rejected calls.

### 1.3 `include/nosfix/asio_audio_host.hpp`, `src/asio_audio_host.cpp` (new)

Binds `AsioOutput` + `AudioTimeline` + `KeysoundQueue` + the bridge engine + the mixer.

- `Open` validates that the ASIO options and the bridge config agree (rate, explicit
  channel count, buffer, mixer block size, keysound capacity, initial sequence), creates
  the engine, prepares the mixer, opens the *injected or explicitly named* driver, then
  re-checks the driver's real buffer size, rate, channel count and reported latency.
  Any mismatch closes and fails: no default device, pair, rate or buffer.
- One render callback: reject a block without a valid driver sample position (counted as
  silence), detect a device generation change (stop every mixer voice), dispatch due
  keysounds with the timeline mapper, then render the mixer for the same block. No
  allocation, no lock, no logging.
- `BindCueVoice(cue_id, Xa2Voice*)`: the sink can only start a bound source voice; an
  unbound cue is refused and counted (`sink_rejected`), never played on an arbitrary voice.
- `PushKeysound` fills the sequence from a process-wide counter; `SetKeysoundMapper`
  (setup only) allows a host-owned clock.

### 1.4 `tests/xaudio2_bridge_tests.cpp` (new, 16 test groups)

Vtable-offset client (no C++ method calls, like a compiled XAudio2 consumer): slot
distinctness and rejection tail, device details at byte 1030, mastering voice shape,
source-format gate, submit/render/end-of-stream with callbacks, start-before-submit,
loops + ExitLoop, operation sets (deferred volume and deferred start), multiple voices and
matrices (uniform, diagonal pan, cross-feed refused, 2x6 refused), submix routing and bus
gain, rate conversion and SamplesPlayed, effects recorded not applied, channel volumes and
argument rejection, engine callbacks, allocation freedom (0 allocations over 8 render
blocks), invalid configuration.

### 1.5 `tests/asio_backend_tests.cpp` (new, 15 test groups)

Fake driver only: config mismatch matrix, explicit-device open, missing latency / failing
start / unregistered CLSID, render reaching the driver buffers, sample-accurate keysound
offset with a deterministic mapper (silent frames 0..36, sample at 37), late keysound at
offset 0, unbound cue refused, stale generation dropped before the sink, generation change
stopping voices, buffer-size change via Restart, buffer beyond the mixer block refused,
pause and invalid-position blocks, the real timeline mapper (offset within 35..39 after the
bounded correction), sequence filling, bindings.

### 1.6 `tests/keysound_dispatch_tests.cpp` (extended)

Added a mixer-integration section: dispatch into a real `AudioMixer` voice at the exact
offset (silence before frame 37, sample at 37), once-only re-dispatch, generation mismatch
dropped before the sink, and a dead mixer handle refused by the sink and counted once.

### 1.7 `CMakeLists.txt`

Added `src/asio_audio_host.cpp`, `src/audio_mixer.cpp`, `src/xaudio2_bridge.cpp` to the
`nosfix` target and two test targets + CTest entries; `src/audio_mixer.cpp` also joined the
keysound test target. No other build change.

## 2. Evidence used

### 2.1 Request / answer ids

- `coord/requests/audio-001.md` -> `coord/answers/audio-001.md` (partial: engine and
  base-voice offsets verified from bytes; source slots and the source format unproven) plus
  `coord/evidence/audio-xaudio2.md`.
- `coord/requests/audio-002.md` -> `coord/answers/audio-002.md` (answered, verified from
  bytes). It **corrects audio-001**: `0x180003143` is `SetFilterParameters` on a voice
  (base index 8, `+0x40`), not `CreateSourceVoice`; `qword_188F75800` is libxact's own
  sound-control singleton, not an IXAudio2 object; libxact.dll has no `CreateSourceVoice`,
  no `SubmitSourceBuffer` and no `WAVEFORMATEX`. audio-001's source-slot list is withdrawn.
  The errata are recorded in `coord/evidence/audio-xaudio2.md` (section 5.2 and ERRATUM).

### 2.2 Exact vtable evidence

Byte-proven (call-site RVAs and register setup), after applying the audio-002 erratum:

| Object | Offset | Slot | Call site |
|---|---|---|---|
| engine | +0x20 | GetDeviceDetails(0, details) | `0x1800013DB` |
| engine | +0x48 | CreateSubmixVoice(ppVoice, 2, 44100, ..., 5, 0) | `0x180001644`, `0x18000168E` |
| engine | +0x50 | CreateMasteringVoice(ppVoice, 2 or 6, 44100, 0, 0) | `0x180001593` |
| voice | +0x10 | SetEffectChain(chain{count, {pEffect, initialState}}) | `0x1800015CD` |
| voice | +0x30 | SetEffectParameters(0, block, 52, 0) | `0x1800016xx` |
| voice | +0x40 | SetFilterParameters({Type,Frequency,OneOverQ}, 0) | three sites in `sub_180002F40`, `0x180003143` |
| voice | +0x80 | SetOutputMatrix(dest, 2, 2, matrix, 0) | `0x1800012E0` |
| voice | +0x90 | DestroyVoice (index 18) | derived from the 19-entry base |

Four independent base indices (2, 6, 8, 16) now anchor the 19-entry, no-IUnknown voice
table, so `DestroyVoice = +0x90` is fixed: a wrong value would call SetOutputMatrix with
three arguments. The bridge uses 144 and that table. Engine StartEngine/StopEngine/
CommitChanges (88/96/104) and engine +0x40 CreateSourceVoice have **no call site** in
libxact, so their slot numbers remain inferred from the 2.7 engine order (consistent with
Spice's declaration in `reference/spice2x/src/spice2x/hooks/audio/xact.cpp`).

`XAUDIO2_DEVICE_DETAILS`: libxact allocates the details local and reads the channel count as
a `uint16` at **byte 1030** of it (exported pseudocode: `char v118[1030]` followed by
`unsigned __int16 v119`). That fixes DeviceID `WCHAR[256]` @0, DisplayName `WCHAR[256]`
@512, Role @1024, `WAVEFORMATEXTENSIBLE` @1028 - the order Spice declares. `sizeof` is
1068; the bridge `static_assert`s every offset. audio-001 describes the local as "0x5F0
bytes", which is the frame displacement (`[rbp+20h]`), not a size; the 1030-byte figure
comes from the variable layout and is the one the channel read depends on.

`CreateFX`: audio-001 proves the import is `XAPOFX1_5.dll!CreateFX`, that all three call
sites (`0x180001438/0x180001474/0x180001532`) initialise only `rcx` (REFCLSID) and `rdx`
(IUnknown**), and that `r8`/`r9` are whatever the previous call left. The bridge never calls
`CreateFX`; the descriptors handed to `SetEffectChain` are recorded. A future replacement
engine must call `CreateFX(clsid, &ppEffect, nullptr, 0)` explicitly.

### 2.3 Surface tiers: proven libxact surface vs speculative XAudio2 surface

The bridge and its tests are organised around this split (header comment "Surface tiers",
`kLibxactUsesSourceVoice`, `TestLibxactProvenSurface`, `TestSpeculativeSourceSurfaceIsMarked`).

| Tier | Slots | Evidence | Claim |
|---|---|---|---|
| A. Proven and exercised by libxact | engine +0x20, +0x48, +0x50; voice +0x10, +0x30, +0x40, +0x80, +0x90 | byte-verified call sites (audio-001/002) | the bridge must keep these exact shapes; a break is a libxact break |
| B. Proven layout, no libxact call site | the other base voice slots 0..144, engine 0..120 | same 19-entry table, argued by declaration order | implemented, tested, not claimed for libxact |
| C. Speculative, no libxact call site | source voice +0x98..+0xE0; engine +0x40 CreateSourceVoice | none - libxact has no CreateSourceVoice/SubmitSourceBuffer/WAVEFORMATEX (audio-002) | prototype-only surface for this stream's host; `kLibxactUsesSourceVoice == false`; every call stays fail-closed through the rejection tail |

Tier C exists because the mixer and the ASIO host need a source voice and its tests are the
only way to prove the mixer. It is deliberately inert for libxact: nothing routes libxact to
it, and the header says so.

### 2.4 Local header facts

`.tools/sdk/.../um/xaudio2.h` in this tree is 2.9 (19 base voice methods, no
GetDeviceCount/GetDeviceDetails/Initialize on the engine), which is why the engine and
voice tables had to be split.

### 2.5 Frame gate

`analyze/evidence/nos202502_0x180304f30.c` and `verified_xact_0x1800092e0.c`, summarised
in `analyze/impl-20261030/audio-frame-gate-proposal.md`.

## 3. Tests

Command (equivalent to `scripts/build.ps1 -RunTests`; see section 6 for why the script's
`vcvars64`/RC path needs the wrapper):

```
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -RunTests
```

Result with the environment wrapper: `100% tests passed, 0 tests failed out of 41`
(39 pre-existing + `xaudio2_bridge` + `asio_backend`), Release `/O2 /DNDEBUG`, `/W4
/permissive-`, no compiler warnings. The keysound test target also compiles the mixer.

This branch is isolated and still carries the 39-test baseline plus this stream's two
targets. The parent's merged tree (`main`, `2271ba8`) carries 46 tests (the other streams'
five). To check the correction against that tree without touching this branch's history,
`main` was exported with `git archive` into a scratch directory inside this worktree, this
commit's patch was applied with `git apply`, and the result was configured and built the
same way: **`100% tests passed, 0 tests failed out of 46`** (5.6 s). The only compiler
warnings in that build are pre-existing C4324 "structure was padded due to alignment
specifier" notices from other streams' headers (`song_clock_publication.hpp`,
`raw_input_capture.hpp`); this stream's files compile warning-free. The scratch copy was
deleted afterwards.

Allocation freedom is measured (not asserted by inspection) with the global `operator new`
probe in `tests/test_support.hpp`: 0 allocations across 8 render blocks.

## 4. Remaining blockers

1. **Source-voice order is settled as far as bytes allow, and it does not matter for
   libxact.** audio-002 proves base indices 2/6/8/16, withdraws audio-001's alternative
   list and confirms the SDK order at `+0x98`..`+0xE0` as a [hyp] from the proven 19-entry
   base. Since libxact has no source-voice call site, the request is closed for libxact and
   only a live `XAUDIO2SourceVoice` vtable dump (or the day libxact starts creating one)
   could raise it again. The bridge keeps the SDK order, fails closed through the rejection
   tail, and marks the surface with `kLibxactUsesSourceVoice == false`.
2. **Audio format is decided outside libxact.** audio-002 proves there is no
   `CreateSourceVoice` and no `WAVEFORMATEX` in the module; submission happens inside
   avs2-core on the cue/xsb path (`avs2_core_79/82/86`, `"%s.xsb"`). The PCM/float gate is
   therefore this bridge's prototype boundary, not a libxact constraint. Before live wiring
   the cue path's actual sample format (PCM vs ADPCM inside the xsb) must be established, or
   keysounds will be refused and counted - never mis-played.
3. **Worker protocol (audio-001 B1) and frame-gate consumer (B2)** are still unproven, so
   the frame-gate proposal in section 5 stays disabled.
4. **No latency measurement.** No real device was opened by this stream; every number here
   is a sample count, not a measurement. Driver buffer range, 44.1 kHz acceptance, output
   latency and callback jitter still have to be measured on the target host with the real
   ASIO driver.
5. **Reverb / XAPO / 5.1**: effect chains and filters are recorded but never applied, and
   only stereo matrices are representable (a cross-feed or 2x6 matrix is refused and
   counted). Submix EQ/filter behaviour is absent by design.
6. `GetPerformanceData` writes nothing (the 2.7 struct size is not in this tree) and
   `SetSourceSampleRate` is refused: both are counted.

## 5. Frame-gate proposal

`analyze/impl-20261030/audio-frame-gate-proposal.md`, disabled and unbuilt:

- `nostalgia 0x180304F30`: the 24-byte record layout, the `current + a6` scheduling rule and
  the `SetSoundWithParam` deferral are documented; capacity is 2048 by address range but
  128 by the dedupe bound and 469 by audio-001 - **unsettled, do not encode it**. The
  replacement guarantee is the queue/mixer sample-accurate start from a QPC stamped at the
  decision point, not at the next game frame.
- `libxact 0x1800092E0`: `Sleep(2)` at `0x18000942F` when the command dword at `this+12`
  is zero; `0x18000A869` is the shutdown case of the same loop. The replacement is the
  bounded command ring drained once per audio block (one block of latency, overflow
  counted), so removing the sleep does not need a hook on that function.

## 6. Merge notes

- New files only; the only shared file touched is `CMakeLists.txt` (three sources, two test
  targets, one extra source for the keysound test).
- `scripts/build.ps1 -RunTests` fails in this sandbox at CMake configure time because
  `-DCMAKE_RC_COMPILER=<windows path with backslashes>` is written unescaped into
  `CMakeRCCompiler.cmake` ("Invalid character escape `\S`"). The script was **not**
  modified (it is not in this stream's ownership). Suggested one-line fix for the parent:
  build `$rc` and `$mt` with forward slashes, e.g.
  `$rc = (Join-Path $sdkCpp "bin\$sdkVersion\x64\rc.exe").Replace('\','/')` (same for
  `$mt`), or drop both `-D` options when the SDK `bin` directory is on `PATH`. The wrapper
  used here calls the same `vcvars64.bat`, appends the repo-local SDK include/lib
  directories and passes forward-slash RC/MT paths; build and tests were run through it.
- The RC/MT paths are the only sandbox-specific part: `vcvars64.bat` itself works in this
  worktree, and the compiler identification succeeded with MSVC 19.44.
- `AsioAudioHost` owns a ~1 MiB engine object and allocates it on the control thread, never
  in the callback. `AudioMixer` must be heap-allocated if ever used standalone.
- Ownership boundary for wiring: libxact's engine must be routed to `Xa2BridgeCreate`, the
  device to `AsioAudioHost::Open` with an explicit CLSID, and each cue to a source voice
  created through the bridge and bound with `BindCueVoice`. None of that is done here, and
  the parent owns it.
- The bridge never destroys the engine on `Release` by default (`destroy_on_release =
  false`) so a client that forgets to release cannot leave a dangling pointer; the host
  calls `Xa2BridgeDestroy` explicitly.

## 7. Files

```
include/nosfix/xaudio2_bridge.hpp      src/xaudio2_bridge.cpp
include/nosfix/audio_mixer.hpp         src/audio_mixer.cpp
include/nosfix/asio_audio_host.hpp     src/asio_audio_host.cpp
tests/xaudio2_bridge_tests.cpp         tests/asio_backend_tests.cpp
tests/keysound_dispatch_tests.cpp      (extended)
CMakeLists.txt                         (sources + two test targets)
analyze/impl-20261030/audio-REPORT.md
analyze/impl-20261030/audio-frame-gate-proposal.md
coord/requests/audio-001.md, audio-002.md;  coord/status/audio.md
```
