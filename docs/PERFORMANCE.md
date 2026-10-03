# Force CPU optimization — 1.0.1

The primary target is met: **11.3% p99**, compared with the original **19.6%**
(8.3 percentage points, **42.3% lower**). Worst block in the 10-second follow-up
was **12.2%**, versus the original 20.8%. Ordinary playing p99 is about 10.4%.
The 8–10% stretch target is not met by the worst stress workload.

## Measurement method and preserved baseline

All comparisons use the actual Force, 44.1 kHz, 128 frames, thread CPU clock,
`bench plugin.so -s 2 -v 1,4 -p -j`. Control edits are included in each timed block.
The original binary, SHA256
`f2d92642e6a3b50824032c62f0c76ccfa8857153b9727898d0a75de239e2707a`,
original report, refreshed p95 report, and baseline audio remain under
`build/performance/baseline/`. No background DSP threads were observed.

p95 was added without changing the original PRNG, workload, timing boundaries or
stage order. `-r`, built with `-DSIREN_PROFILES -Ibuild`, appends representative
workloads and all twelve presets. Results of those additional stages are not
substituted for the original all-parameter storm. The filter-sweep stage moves
cutoff every block with maximum resonance; other representative stages hold
notes and their selected settings. Freeze runs through entry and sustain;
standalone effects tests also exercise exit and feedback extremes.

A final `-s 10 -v 1,4 -p -j` follow-up confirms **11.3% p99**, 12.2% max,
all-param mean 10.0%, p95 10.8%. The primary comparison stays the original 2-second
workload. Percentages are of a 2902-microsecond block budget, not whole-device
CPU utilization; MPC's own load and scheduling delay are outside this metric.

## Profile first

Host perf is prohibited by kernel policy; perf is unavailable on the Force.
An initial gprof diagnostic exhausted its arc table. Increasing that table
produced implausible function labels/counts; **that report was discarded**.
No DSP was changed before obtaining the statistical profile described below.

`tools/profile_sample.cpp` samples the ARM instruction PC with SIGPROF and
ITIMER_PROF, with fixed preallocated capture storage. DSP sources remain
uninstrumented. GCC `-O2 -g` was used for the baseline source profile across all
12 presets (120 seconds of audio): **18,695 samples**. ARM addr2line resolves
executable PCs; dladdr identifies math/library samples. Sampling includes
helpers inlined into a caller, so helper percentages cannot be assigned exactly
to one DSP module. libc alias names can differ from the source operation.

The later full VST storm profile used current release objects plus the sampler:
**8,055 samples**, with **19.6% in libc**, including numeric formatting/parsing;
strcmp alone accounted for 2.0%. Actual release benchmarks measure the resulting
savings. The sampler and gprof padding are never linked into the plugin.

| Area investigated | Evidence / decision |
| --- | --- |
| Main oscillator / PolyBLEP | wave 1.3%, BLEP .5%, BLAMP .2% direct samples; preserve audible std::sin and anti-aliasing, skip only blend contributions below 1e-8. |
| Trig / nested LFOs | Combined library sine/sincos about 11.2%; slow modulation/panners use an immutable interpolated table; audible sine remains unchanged. |
| Parameter smoothing | 22 effect approaches/sample, clean/finite/bounds helpers prominent; replace control smoothing with 16-sample exponential endpoints and linear interpolation. |
| ZAP | Per-sample pow was present; double-precision multiplicative exponential recurrence with the same endpoints and duration removes it. |
| Tone / EQ filters | Core biquad processing 7.6% direct samples; coefficients/EQ rebuilt unnecessarily. Cache unchanged inputs, fixed EQ trigonometry; update changing coefficients every 32 samples. Preserve double coefficients/state. |
| Crusher | Quantizers ran at zero blend; skip below 1e-8 while retaining blend decay and re-enable behavior. |
| Delay interpolation / wrapping | Ring reads/at about 8.4% direct samples plus helpers; remove modulo, keep fractional interpolation. Fixed reverb reads avoid interpolation and a second slot read. |
| Feedback filters | Keep HP/LP processing and safety guards; cache coefficients when cutoff is unchanged. |
| Saturation | Already a bounded rational curve. Unchanged; no tanh replacement is needed. |
| Characters / wow/flutter | Keep every character and drift/phase evolution; smoother and lookup improvements reduce math without deleting paths. |
| Reverb | Remains continuously connected, including at zero wet amount, preserving latent tails and later wet re-enable; only fixed buffer addressing changed. |
| Stereo processing | Cache panning coefficients; retain existing stereo fold/spread behavior. |
| Parameter lookup / wrapper | Full VST profile exposed libc cost. Numeric port-local bridge removes snprintf, strtof and linear key lookup for ordinary controls. Engine function-table ABI and chunk strings are unchanged. |
| Output conversion / housekeeping | Wrapper FIFO, int16 bridge, momentary countdowns and host refresh retained; no lock/allocations introduced. |

Raw source/profile evidence: `baseline/samples.txt`, `baseline/statistical-profile.txt`,
`wrapper-before-samples.txt`, `wrapper-before-profile.txt` under `build/performance/`.
`tools/profile.sh` reproduces the statistical diagnostic build.

## Measurements after each meaningful change

Each table uses the original five-stage 2-second workload. All columns are CPU
percent. Small variations below roughly .2 points should not be overinterpreted.

### Refreshed baseline

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 13.5 | 14.1 | 14.7 | 15.4 |
| 1 voices | 15.2 | 15.8 | 16.1 | 16.5 |
| 4 voices | 15.3 | 15.9 | 16.4 | 17.7 |
| all-param sweep | 17.4 | 18.8 | 19.3 | 20.7 |
| release tail | 14.8 | 15.7 | 16.2 | 16.7 |

### 1. Coefficient caches, unity EQ bypass with state checks, branch wrapping and fixed reverb reads

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 11.3 | 11.8 | 12.2 | 13.1 |
| 1 voices | 12.9 | 13.5 | 13.7 | 14.1 |
| 4 voices | 13.0 | 13.6 | 13.9 | 14.4 |
| all-param sweep | 16.0 | 17.5 | 17.7 | 19.8 |
| release tail | 12.6 | 13.2 | 13.5 | 13.9 |

### 2. Cached panners, precise control settling, cheap phase wrap, below-knee compressor shortcut

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 9.5 | 9.9 | 10.2 | 10.9 |
| 1 voices | 11.0 | 11.4 | 11.8 | 12.6 |
| 4 voices | 11.0 | 11.5 | 11.9 | 12.8 |
| all-param sweep | 15.2 | 16.3 | 17.0 | 19.4 |
| release tail | 10.2 | 11.5 | 11.9 | 12.3 |

### 3. Interpolated control smoothing, modulation sine lookup and inaudible blend skipping

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 8.8 | 9.3 | 9.6 | 10.4 |
| 1 voices | 9.8 | 10.2 | 10.5 | 10.9 |
| 4 voices | 9.8 | 10.2 | 10.5 | 11.3 |
| all-param sweep | 14.2 | 15.3 | 15.7 | 17.8 |
| release tail | 9.4 | 10.3 | 10.7 | 11.3 |

### 4. 32-sample coefficient updates, cached EQ trigonometry/exp mapping and ZAP recurrence

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 8.9 | 9.2 | 9.5 | 10.8 |
| 1 voices | 9.6 | 10.0 | 10.3 | 11.1 |
| 4 voices | 9.7 | 10.1 | 10.4 | 10.7 |
| all-param sweep | 13.5 | 14.3 | 14.8 | 17.3 |
| release tail | 9.3 | 10.0 | 10.3 | 10.6 |

### 5. O3 + LTO + fno-math-errno; no fast-math

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 8.6 | 9.0 | 9.3 | 10.8 |
| 1 voices | 9.5 | 9.9 | 10.3 | 10.8 |
| 4 voices | 9.4 | 9.9 | 10.2 | 10.8 |
| all-param sweep | 13.2 | 14.1 | 14.6 | 16.3 |
| release tail | 9.0 | 9.7 | 10.1 | 10.5 |

### 6. Float biquad trial — rejected; no useful p99 saving, double precision retained

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 8.7 | 9.1 | 9.5 | 10.8 |
| 1 voices | 9.5 | 9.9 | 10.2 | 10.6 |
| 4 voices | 9.5 | 9.9 | 10.2 | 11.3 |
| all-param sweep | 13.1 | 13.9 | 14.6 | 15.6 |
| release tail | 9.1 | 9.8 | 10.1 | 11.0 |

### 7. Numeric parameter bridge; double biquads retained

| Stage | Mean | p95 | p99 | Max |
| --- | ---: | ---: | ---: | ---: |
| idle | 8.6 | 9.1 | 9.4 | 10.9 |
| 1 voices | 9.5 | 9.9 | 10.3 | 10.8 |
| 4 voices | 9.5 | 9.9 | 10.2 | 10.7 |
| all-param sweep | 10.1 | 10.8 | 11.3 | 12.0 |
| release tail | 9.0 | 9.7 | 10.0 | 10.4 |

## Representative workloads and worst factory preset

Baseline and improved runs use the same extra-stage setup, including cutoff
sweep, and the same preceding original workload. Timings include control edits.

| Workload | Baseline mean / p95 / p99 / max | Improved mean / p95 / p99 / max |
| --- | --- | --- |
| basic dry | 15.3 / 15.9 / 16.2 / 16.7 | 9.6 / 10.3 / 10.8 / 11.1 |
| filter sweep | 15.9 / 16.5 / 17.0 / 17.6 | 10.2 / 10.7 / 11.0 / 11.9 |
| heavy echo | 15.3 / 15.9 / 16.2 / 16.8 | 9.6 / 10.4 / 10.7 / 12.1 |
| freeze | 15.4 / 16.0 / 16.5 / 17.2 | 9.9 / 10.5 / 10.9 / 11.3 |
| reverb full | 15.4 / 16.1 / 16.3 / 16.9 | 9.5 / 10.0 / 10.2 / 10.6 |
| all modulation | 15.5 / 16.1 / 16.3 / 16.8 | 9.8 / 10.2 / 10.5 / 10.7 |
| preset 00 | 15.4 / 16.0 / 16.3 / 16.7 | 9.5 / 10.3 / 10.7 / 11.5 |
| preset 01 | 15.4 / 15.9 / 16.4 / 16.7 | 9.4 / 9.8 / 10.1 / 10.9 |
| preset 02 | 15.5 / 16.1 / 16.4 / 16.8 | 9.5 / 10.0 / 10.3 / 10.6 |
| preset 03 | 15.3 / 15.9 / 16.3 / 16.8 | 9.5 / 10.0 / 10.3 / 10.6 |
| preset 04 | 15.7 / 16.3 / 16.7 / 17.4 | 10.1 / 10.6 / 10.9 / 11.2 |
| preset 05 | 15.5 / 16.0 / 16.5 / 17.2 | 9.6 / 10.3 / 10.7 / 11.2 |
| preset 06 | 15.5 / 16.1 / 16.4 / 17.4 | 9.6 / 10.1 / 10.4 / 10.8 |
| preset 07 | 15.4 / 16.0 / 16.3 / 17.0 | 9.5 / 10.0 / 10.3 / 10.7 |
| preset 08 | 15.3 / 15.8 / 16.1 / 16.6 | 9.5 / 9.9 / 10.3 / 10.5 |
| preset 09 | 15.4 / 16.0 / 16.6 / 17.4 | 9.9 / 10.5 / 10.8 / 11.3 |
| preset 10 | 15.3 / 15.9 / 16.3 / 16.6 | 9.6 / 10.4 / 11.0 / 11.3 |
| preset 11 | 15.5 / 16.1 / 16.4 / 16.8 | 9.5 / 10.2 / 10.4 / 10.7 |

Worst factory-preset p99 in the improved run is Feedback Madness (preset 10),
11.0%; all-modulation p99 10.5%. The full-storm worst remains the acceptance test.

## Sonic changes and realtime limits

No modules or presets were removed. Oscillator PolyBLEP/BLAMP, audible sine,
rational saturation, interpolation, stereo topology, freeze behavior and
continuously connected reverb remain. Coefficient/state precision stays double;
a float-state trial had no useful saving and was reverted. Compiler flags do
not enable unsafe fast-math or remove NaN/Inf guards.

Approximations are explicit: modulation/panner lookup has under 1.5e-7 absolute
interpolation error for float unit-cycle input; conversion of a double LFO phase
to float adds its normal phase rounding. Controls use 16-sample linear segments
between exponential endpoints (at most 0.34 ms extra target latency), and changing
core filter coefficients update every 32 rather than 16 samples. ZAP uses a
double recurrence instead of per-sample float pow. Control endpoints now settle
at the intended value instead of stalling one or several float ULPs short.
Tiny crusher/wave contributions below 1e-8 are skipped, without freezing blend
state. Numeric setters retain full float precision rather than six-digit %g.
These changes can shift echo phase and transition response; this is not a claim
of bit-identical audio or completed subjective listening.

The 36-second/12-preset host A/B files remain available in the baseline folder
and `build/demo.wav`. Per-preset RMS differences stayed within 0.05 dB in the
measured render; sample residuals are larger in modulated/time-changing patches
and must not be interpreted as an inaudibility proof. Overall peaks/RMS and
per-preset residuals are recorded in `audio-comparison.txt`. Physical listening
and UI/touch/Q-Link behavior remain separate manual checks.

ASan/UBSan/leak, MIDI, finite waveforms, 440-Hz pitch, block continuity, state
round-trip/invalid state, allocation hooks, concurrent setter/render, randomized
controls, extreme feedback, freeze/invert, ring wrap and STOP tests pass. Audio
allocations/frees remain zero. ARM loader, installed checksums, service restart
and discovery are recorded in the final deployment report.

## Verification of the installed final artifact

After the final EQ unity-state correction (retaining pole decay when state is
nonzero), the installed-path 2-second original storm measured **11.2% p99**,
11.4% max; the full set of representative stages also stayed within target.
The installed-path 10-second follow-up measured **11.0% p99**, **11.8% max**,
storm mean 9.9%, p95 10.6%. Relative to 19.6%, final p99 reduction is **43.9%**.
The small difference from the pre-install 11.3% is within run variation.
Every installed payload file matched its local package hash (116 files).

A dense host check across 1,000,001 float phases, including the wrap boundary,
measured sine table error 1.3019e-7; evidence is `build/performance/math-check.txt`.
