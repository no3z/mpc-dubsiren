# 1.0.2 — CPU, low Pitch and performance Q-Links

Installed and verified on Force on 2026-10-03. The original two-second storm
measures **7.8% p99** versus a fresh 11.6%; the ten-second follow-up measures
**8.3%** versus a fresh 11.5% (historical installed baseline 11.0%). Reduction is
32.8% in the short comparison and 27.8% in the long comparison. The <=8% target
is reached in the short run, **but not consistently in the longer follow-up**.
The 6% stretch target was not pursued through oscillator/reverb quality reductions.

The actual device test uses 44.1 kHz, 128 frames and thread CPU time, expressed
as a percentage of the 2902 microsecond block budget. This is not the Force UI
CPU meter or a full busy-project deadline test. No background DSP threads.
Commands and original PRNG/timed setter+render boundaries remain unchanged:

```sh
/tmp/bench plugin.so -s 2 -v 1,4 -p -j
/tmp/bench plugin.so -s 10 -v 1,4 -p -j -r
```

`-r` only appends the established dry/modulation/filter/echo/freeze/reverb/preset
stages. New Wave ordering and logarithmic Pitch change the physical values
corresponding to the same random normalized edits in the storm; they have not
been silently substituted into a different workload. The representative stages
recall the same physical factory settings in both versions. The wrapper still
advertises 57 parameters, 51 public and six compatibility/UI parameters.

## Same original five-stage benchmark

All columns are percent of the block CPU budget. Raw reports are checked in at
`resources/force-baseline-1.0.1-{2,10}s.txt` and
`resources/force-installed-1.0.2-{2,10}s.txt`.

| Stage (2 s) | 1.0.1 mean / p95 / p99 / max | Installed 1.0.2 mean / p95 / p99 / max |
| --- | ---: | ---: |
| idle | 8.7 / 9.5 / 9.9 / 10.9 | 3.0 / 3.2 / 3.5 / 5.1 |
| 1 voices | 9.5 / 10.3 / 10.6 / 11.5 | 5.3 / 5.6 / 5.6 / 6.3 |
| 4 voices | 9.5 / 10.2 / 10.6 / 11.0 | 5.3 / 5.6 / 5.6 / 5.8 |
| all-param sweep | 10.1 / 11.0 / 11.6 / 12.1 | 5.9 / 7.7 / 7.8 / 9.2 |
| release tail | 9.1 / 9.9 / 10.5 / 11.0 | 5.6 / 6.1 / 6.2 / 6.4 |

| Stage (10 s) | 1.0.1 mean / p95 / p99 / max | Installed 1.0.2 mean / p95 / p99 / max |
| --- | ---: | ---: |
| all-param sweep | 10.1 / 11.0 / 11.5 / 12.5 | 6.1 / 8.1 / 8.3 / 8.5 |
| basic dry | 9.5 / 10.3 / 10.7 / 11.9 | 5.9 / 6.3 / 7.0 / 7.5 |
| all modulation | 9.9 / 10.6 / 11.0 / 11.8 | 6.4 / 6.8 / 6.9 / 7.5 |
| filter sweep | 10.4 / 11.2 / 11.7 / 12.1 | 6.3 / 6.7 / 6.8 / 7.0 |
| heavy echo | 9.6 / 10.4 / 10.9 / 11.9 | 5.6 / 6.0 / 6.6 / 7.1 |
| freeze | 10.0 / 10.8 / 11.2 / 11.9 | 6.6 / 7.0 / 7.2 / 7.8 |
| reverb full | 9.6 / 10.3 / 10.7 / 12.0 | 6.0 / 6.4 / 6.8 / 7.3 |

Freeze exercises the existing 95.5–96% feedback transition; Heavy Echo uses
Feedback Madness at 88% feedback. All twelve factory stages are included in the
raw reports and `resources/performance-1.0.2.json`. The heaviest installed preset
p99 was **8.3% (Deep Orbit)**; whole-run worst block **9.8%**. The final installed
storm itself was 6.1% mean / 8.1% p95 / 8.3% p99 / 8.5% max. Startup/cache variation
is visible in some preset stages; differences of about .2 points are not treated
as reliable improvements. The benchmark's generic PASS threshold is looser than
the requested 8%; PASS does not mean that target was achieved consistently.

## Profile and changes retained

Profiling preceded edits. The release-object ARM storm sampler captured 24,341
PC samples on 1.0.1. Biquad processing (~12%), finite/denormal helpers, per-sample
bounds/control setup and ring metadata were prominent. A second profile after
initial optimizations captured 14,438 samples: biquads 15.7%, ring push/read,
modulation lookup and slew advancement remained important. Inline helper labels
are sampling locations, not separate function calls or exclusive module totals.
Perf is unavailable; `tools/profile_sample.cpp` is diagnostic only.

- Ring validity is tracked by distance/filled count; per-slot generation tags
  are removed. Reset remains O(1). Fixed reverb taps use their write head and
  one fill check per read/write pair, preserving all delay lengths.
- Bounds, character selection and 22 effect slew endpoints share a 16-sample
  boundary. Ramps still advance every sample. Raw effect targets/scaling are
  prepared once per render block; core invariants once per 16 samples.
- Stereo post/EQ coefficients are computed once and used with independent
  double histories. Filter state cleanup below 1e-25 runs every 32 samples;
  sample output guards remain and reject NaN/Inf. Finite-state checks removed
  only where bounded coefficients/float input/bounded output prove them redundant.
- Zero nested amounts skip their sine lookup, while phases keep advancing.
  Settled Wave evaluates one oscillator; active blends share repeated BLEP
  calculations. Inactive effect modulation skips unused lookup/tap evaluation,
  while phase and buffer history continue.
- Zero-history reverb with zero input returns exact silence until first excited.
  Once excited, all eight combs/four allpasses continue even at zero wet gain,
  preserving latent tails. A regression test enables wet after a muted impulse.
- Core controls snap when float smoothing stalls, avoiding tiny nonzero EQ/LFO
  values keeping an inactive path alive indefinitely. Defaults are contiguous;
  bounded positive enum rounding avoids ARM libm calls.

Eight/16/32-sample effect-control trials produced storm p99 8.4/8.5/8.5%
respectively; 32 gave no reliable benefit, so 16 is retained. Explicit NEON
ramp trials gave 8.4% versus 8.3% scalar, within variation; the NEON change was
removed. Existing -O3/LTO/Cortex-A17 hard-float flags remain. No global fast-math,
new oscillator approximation, reduced reverb network or heavyweight crossfade.
Audible sine, PolyBLEP/BLAMP, main/nested LFO rates, fractional delay, saturation
and filter precision are retained.

A 36-second twelve-preset A/B against the pre-edit render had RMS level changes
below .003 dB and unchanged 16-bit peaks except one LSB for Fog Horn. Residuals
range from -136.8 to -43.7 dBFS; small timing/control changes are present, so this
is not a bit-identical or subjective listening claim. The first ring-only change
was bit-identical across that render. Full measurements are in the JSON report.

## Pitch path and compatibility

Base Pitch changed **60–2400 → 10–2400 Hz**, logarithmic:
`exp(log(10) + normalized * log(240))`, calculated at parameter API rate.
The skin preview follows the same scale. Below 100 Hz the native display shows
one decimal; above it shows whole Hz. Standard `effString2Parameter` supports
physical numeric editing and rejects invalid text/NaN, with bounded finite
numeric input. Tests cover 20/30/40/60/80/100/150 Hz and small low-end nudges.

The DSP positive base and ZAP endpoint floors changed **30 → 1 Hz**. A base of
10 Hz plus octave-down and bend reaches 2.5 Hz; downward ZAP can reach 1 Hz.
Frequency FM remains signed, including zero crossings and reverse phase, bounded
at +/-17,640 Hz. Feedback HP retains its separate safe 40 Hz minimum.
Measured six-second dry sine tests:

| Requested Hz | Measured Hz |
| ---: | ---: |
| 10 | 10.0009 |
| 20 | 20.0018 |
| 30 | 30.0027 |
| 50 | 50.0045 |
| 100 | 99.8424 |
| 620 | 619.8896 |
| 2400 | 2399.8843 |

Zero-crossing count resolution limits these measurements; every case is within
.5 Hz. The low ZAP test counted six amplitude-qualified crossings over two
seconds, demonstrating departure from the old 30 Hz floor. Eight low-frequency
max-FM/filter/echo/crush/ZAP cases ran 240 seconds: finite, peak .780392,
mean left -.000201. Deep/sub-audio material still needs listening on hardware.

IDs, DFS1 version and legacy physical waveform IDs remain unchanged. **37 actual
1.0.1 chunk fixtures (1887 targets) restore exactly**, including all twelve
factories and 25 custom Wave/Pitch combinations. Direct comparison with the old
shared library also passes. Project chunks remain physical, so old saved Hz and
waveform restore. New default Release is represented as .0500000007 rather than
.049999997 seconds (3.7 ns rounding); musical factory settings are unchanged.

**Old normalized Pitch/Wave automation changes meaning.** The log range and new
external wave order cannot preserve those older normalized curves at the same
IDs. Inspect/remap those curves when upgrading an automated project. Actual MPC
project reload is still a manual check; chunk harnesses do not prove host behavior.

## Native Q-Link mapping and preset safety

| Primary order | Secondary order |
| --- | --- |
| Pitch | Resonance |
| Wave | Preset |
| LFO Rate | Attack |
| LFO Depth | Release |
| Zap Sweep | Echo Mix |
| Cutoff | Ping |
| Delay Time | Reverb |
| Feedback | Output |

Native primary slots: 13,9,5,1,14,10,6,2. Secondary: 15,11,7,3,16,12,8,4.
Utility keeps six EQ bands and Fast/Slow/octave switches. These are host mapping
addresses; Force's four physical knob/bank selection still needs manual checking.
Both Q-Link files and all 77 touch controls/51 public IDs pass the generated audit.

Wave steps **SINE → TRIANGLE → SAW → SQUARE → NOISE**, reads back exact discrete
positions and reports those names through `effGetParamDisplay`. Bidirectional
small nudges and 1000 arbitrary inputs pass. The existing short exponential blend
keeps oscillator phase continuous during active switching; it reduces clicks
without promising an artifact-free transition in every extreme case.

Preset uses the **existing native P_preset enum/snapshot infrastructure**, shared
with the touchscreen factory selector. The wrapper's VST program menu is still
one placeholder program; `effSetProgram` is not supported. The secondary Q-Link
selects the actual twelve internal factory snapshots by name; it does not browse
external files or arbitrary user programs. There is no host workaround.

Rapid 2400-block preset+Wave changes under held MIDI and maximum feedback were
finite, peak **.366760**, zero output-bound frames, with saved-state round trips.
Setter/render allocation and free counters stay zero; source paths do no
filesystem IO, locks or unbounded work. Existing gain/filter/effect smoothing
and output protection remain. Concurrent preset controls/render tests pass.
Selecting a preset clears LATCH and other performance holds, as before; use a
held MIDI pad or retrigger after selection. The patch name is the last selected
factory name even after custom edits; all custom physical values are saved.

## Build, installation and verification

VST version **1002**, native skin **1.0.2.0**; sole exported symbol VSTPluginMain,
ARMv7 Cortex-A17 hard-float, highest required GLIBC_2.29. Both workspace and an
independent source copy build identical ARM/ZIP files in the recorded toolchain.
Package CPU metadata now records the actual installed 10-second benchmark;
only root package metadata changed after that benchmark. The portable payload
stays identical to all **116 verified installed hashes**.

- All integration/effects/low-Pitch/enum/legacy tests pass ASan/UBSan/leak checks.
- Production random stress: 1800 seconds of simulated audio, 620,156 blocks,
  79,379,968 frames, 50.931 seconds wall, RSS +64 KiB, 152 state checkpoints pass.
  Peak .98; 1093 bound frames (0.00138%). Means L -.000056 / R -.000049.
- Sanitizer stress: 600 seconds of audio, 206,718 blocks, 109.493 seconds wall,
  no sanitizer/leak failure. These are offline runs, not a realtime hardware soak.
- The complete 116-file installed 1.0.1 and settings were backed up outside
  Synths. Both backup versions pass read-only rollback checks; a complete
  1.0.2→1.0.1 fixture rollback restores every hash and preserves other settings.
- The existing installer restarted acvs. Actual installed-path library opens,
  plays and passes the original/representative workloads. All 558 current plugin
  registrations remain, including 557 unrelated entries; all unrelated settings
  match the installer backup semantically. acvs is active, MPC PID 13376;
  current startup reports `Found, link: Dub Force Siren` with no related error.
  MPC has not yet mapped the new library in a track; harness loading is separate.

Physical touch, hardware Q-Link LCD/bank feel, listening and real project reload
remain pending on [the updated manual card](MANUAL_FORCE_TEST_CARD.md).
Use [plugin-only rollback](ROLLBACK.md) to restore the verified 1.0.1 if wanted.

```sh
./tools/build.sh all
./tools/package.sh
python3 tools/audit_ui.py
./tools/rc_audit.sh 600
```

Final artifacts:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `build/arm/dub_force_siren.so` | 99904 | `fb7e3b4abcaff4e475561bc0abdc9a13a4a3c1ded4b3dd9c2111d95e8369aeb5` |
| `dist/Dub-Force-Siren-1.0.2-mpc-armv7.zip` | 307194 | `8eaadb037b146a70144abb74b1da1bbc11f89159bd7d8d561ee34d4b68c652a6` |
