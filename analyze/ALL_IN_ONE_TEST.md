# All-in-one test plan - 2026-10-30

> Historical test plan. The `smooth_motion`, `smooth_ui`, `clock_motion`,
> `timeline_probe`, `high_refresh_timeline` and `native_shadow_feed` switches
> have been removed. The microsecond timeline is mandatory and host prediction
> is not installed. Use `analyze/HIGH_REFRESH_FIRST.md` and the current
> `nosfix.ini` for the current build.

Build under test: `out/all-in-one-20261030/`
DLL SHA-256: `D271D6A26338FCB54E39001CECAFD6C6C3A8CED224464EEA4DC8E707010DE72E`

## Configuration

`nosfix.ini` enables every diagnostic/integration path in one DLL:

```ini
enabled=true
precise_audio_clock=true
skip_monitor_check=true
capture_input=true
clock_motion=true
native_shadow=true
native_shadow_feed=true
diagnostic_probe=true
target_hz=auto
smooth_motion=true
smooth_ui=true
log_level=info
```

The original judge, scoring, keysounds, rendering ownership and audio output
remain authoritative.

## Automated baseline

`scripts/build.ps1 -RunTests` passes **50/50** suites. The suite covers:

- scheduler, presentation, render-tail and hook transaction/rollback
- QPC cue-clock patch, song clock, gameplay clock and shadow clock
- normal judgment, native match/scoring, bucket table and shadow feed
- staging-bucket input, PANB pressure and note-type states
- timed normal/geometry/rendering and timed UI timeline/adapter
- XAudio2 bridge, mixer, ASIO host, keysound dispatch and fake-driver tests
- plugin load, missing-module rejection, real-image hook install/call patch

The live DLL is the same binary as the tested `build/nosfix.dll`.

## Live checklist

1. **Startup**
   - `diagnostic PROBE enabled`
   - `raw input observation prepared`
   - `shadow song clock publication enabled`
   - `SHADOW live feed configured (normal notes, diagnostic only)`
   - `experimental normal-note projection samples the song clock each presentation`
   - no `[ERROR]` lines

2. **Five-second counters**
   - legacy ticks near 60, renders near the selected display rate
   - `5s shadow clock`: `mapped` grows, `unmapped` small after the cue starts,
     `backwards=0`, `regressed=0`, `generation` stable within a song
   - `5s SHADOW live`: frames roughly 60/s, notes and outcomes nonzero, errors 0,
     `last=ok`
   - `5s note motion` / `5s UI motion`: expected prediction/timed counters
   - no queue overflow, read error or malformed packet

3. **Note coverage**
   - normal notes and one two-key chord
   - one long note, one trill, one glissando with a held connector key
   - one velocity-zone note if the chart contains one
   - pause/retry/seek once; clock generation changes and the feed recovers

4. **Shadow comparison**
   - `SHADOW outcome` ranks follow the visible judgments for the same notes
   - `SHADOW effect` score/combo deltas agree with the game's visible score
   - record the first note where they diverge, with its `note=`, `lane=`,
     `rank=`, `at=` and `input=` values

5. **Stability**
   - 5-10 minute run without crash, feed error, input overflow or unbounded log
   - no unexpected `backwards`/`regressed` counter growth

## Interpretation

- `SHADOW live feed waiting for the judgment-window sample` that never becomes
  `configured` means the window block was still zero; the feed could not start.
- `last=...` other than `ok` identifies the exact feed refusal.
- `PROBE` lines are capped; `SHADOW` outcome lines are capped at 2000 per run.
- A crash before any `SHADOW` line should be reported with the startup log.

## Known fail-closed items

- `note+84 > 1` (same-key jack) is reported unproven and not applied.
- Velocity `note+296` writer and `note+300` key meaning are not modelled.
- `chart+0x50` under a held glissando connector is not yet sampled.
- AFP property/callback ABI remains unimplemented.
