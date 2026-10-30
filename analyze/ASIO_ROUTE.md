# Native ASIO route - experimental

Updated 2026-10-30. Off by default and not hardware-validated.

## What it replaces

libxact's `Class_XAudioControl::Init` (`0x1800012E0`) creates its XAudio2 engine
through `sub_180003730`: `CoCreateInstance` then the engine's `Initialize`
slot (+40). The experimental route hooks that helper at `libxact+0x3730` and
returns the engine created by `AsioAudioHost` (`Xa2BridgeCreate`) instead. The
original helper remains the fallback.

The bridge already implements the libxact-proven engine/mastering surface:

- engine `+0x20 GetDeviceDetails`, `+0x48 CreateSubmixVoice`,
  `+0x50 CreateMasteringVoice`, `+0x58 StartEngine`, `+0x60 StopEngine`
- voice `+0x10 SetEffectChain`, `+0x30 SetEffectParameters`,
  `+0x40 SetFilterParameters`, `+0x80 SetOutputMatrix`, `+0x90 DestroyVoice`

`AsioAudioHost` owns the ASIO output, the mixer, the timeline and the keysound
queue. The first `Initialize` call from libxact initializes the bridge engine;
later calls return the same engine.

## Configuration

```ini
asio_output=true
; one exact identity; CLSID wins when both are set
asio_driver_clsid={...}
asio_driver_name=Focusrite USB ASIO
asio_output_channels=0,1
asio_sample_rate=44100
asio_buffer_frames=128
```

The route is fail-closed. It refuses and leaves the original engine in place if:

- the libxact helper bytes do not match `40 53 56 57 48 83 EC 40 ...`
- the driver name is missing or ambiguous
- `AsioAudioHost::Open` cannot open the exact CLSID/rate/buffer/channels
- the driver does not report output latency
- MinHook cannot create or enable the helper hook

A failed route is logged as `ASIO route inactive: ...` and the game keeps its
original audio. A successful route logs `ASIO route active: ...`.

## Not proven

- Whether libxact's subsequent XACT engine creation accepts the bridge engine.
- Whether avs2-core submits PCM or compressed cue data; compressed input is
  refused by the bridge.
- Whether the Focusrite/Steam/TOPPING identity on the game host is the ASIO
  driver that should be opened.
- Real callback jitter, underruns, output latency and long-song drift.
- Removal of the `nostalgia 0x304F30` frame gate and libxact `0x92E0` sleep.
  The route only replaces the engine; it does not yet move key-sound dispatch
  off the frame path.
