# 2.0.0 — NEON block engine and simplified controls

Installed and measured on the Force on 2026-10-05 (device section first). The
desktop and qemu-user tables further down are the pre-install estimates; qemu does
not reproduce Cortex-A17 timing, so only their direction carries over.

All figures are the vendored benchmark (`bench <plugin.so> -s N -v 1,4 -p`):
thread CPU per 128-frame block as a percentage of the 2902 µs budget, including
the VST wrapper. 1.0.2 is the release binary from commit 978b356.

## On the Force (Cortex-A17, 44.1 kHz, 128 frames)

`bench -s 10 -v 1,4 -p -j`, 1.0.2 and 2.0.0 run alternately on the same device
with MPC running and idle (two rounds each; both rounds gave identical means).
Raw output: `resources/force-installed-2.0.0-10s.txt`.

| Stage | 1.0.2 mean / p99 | 2.0.0 mean / p99 |
| --- | ---: | ---: |
| idle | 3.0 / 3.4 | 0.7 / 1.0 |
| 1 voice | 5.4 / 5.9 | 1.1 / 1.4 |
| 4 voices | 5.4 / 5.9 | 1.1 / 1.4 |
| all-parameter sweep | 6.0 / 8.0 | 1.3 / 1.9 |
| release tail | 5.4 / 7.2 | 0.8 / 1.4 |

Worst block 2.2% (1.0.2: 8.6%). One voice is about 4.9× cheaper. This is the
plugin's own thread CPU per block, not the Force UI meter or a busy-project
deadline test; the `-r` stages were not run.

## Desktop x86-64 (SSE2 path)

| Stage | 1.0.2 mean / p99 | 2.0.0 mean / p99 |
| --- | ---: | ---: |
| idle | 1.0 / 1.2 | 0.3 / 0.5 |
| 1 voice | 1.6 / 2.2 | 0.3 / 0.6 |
| 4 voices | 1.6 / 2.1 | 0.3 / 0.4 |
| all-parameter sweep | 2.2 / 3.8 | 0.4 / 0.7 |
| release tail | 1.5 / 2.3 | 0.2 / 0.3 |

## ARMv7 NEON build under qemu-user (two runs each, means)

| Stage | 1.0.2 | 2.0.0 scalar path | 2.0.0 NEON |
| --- | ---: | ---: | ---: |
| idle | 49.5 | — | 12.3 |
| 1 voice | 62.7 | 27.9 | 17.4 |
| all-parameter sweep | 65.1 | 24.7 | 17.3 |
| release tail | 61.6 | 18.5 | 12.1 |

"Scalar path" is the same 2.0.0 engine built with `-DDUB_SIMD_SCALAR`; the gap to
the NEON column is what the intrinsics add on top of removing effects and
restructuring into blocks. On the device, 1.0.2 measured 5.3% mean for one voice
(`docs/PERFORMANCE_1.0.2.md`) and 5.4% again in the comparison above; qemu
overstated absolute cost (62.7% for 1.0.2) but the ratio (3.6× in qemu, 4.9× on the
device) points the same way.

## Where the savings come from

- Removed per-sample work: eight reverb combs and four allpasses, spread and
  flange taps, tape noise and drift, three oscillator and six master EQ biquads,
  the always-on 19.8 kHz post filter, 8-bit, chop, kill and nested LFOs.
- Compressor and limiter: `log10`/`pow` per sample became four-lane polynomial
  log2/exp2; a chunk below the knee skips them entirely.
- Oscillator: one band-limited waveform evaluated four samples at a time; the
  LFO sine uses a polynomial instead of double `sin`.
- Echo: contiguous vector reads and writes, fixed loop-filter coefficients, and
  exact silence once the ring is empty.
- Wrapper output: float→int16 rounding and interleaving with NEON narrowing stores.

## Verification

`tools/build.sh test` (ASan/UBSan) and `tools/build.sh arm-test` (NEON, qemu)
pass the integration, performance-control and echo suites; `tools/rc_audit.sh 120`
passes state, level and stress checks. The SIMD and scalar builds render all
twelve presets to identical 16-bit audio. Chunked and single-sample echo
processing match exactly.

Device commands (copy `bench` built for ARM and the plugin to `/tmp`):

```sh
/tmp/bench /tmp/dub_force_siren.so -s 2 -v 1,4 -p -j
/tmp/bench /tmp/dub_force_siren.so -s 10 -v 1,4 -p -j -r   # -r needs -DSIREN_PROFILES
```

## After the fidelity fixes (master chain, echo feedback tap, soft bound)

The master now follows Chromium's compressor and limiter ([FIDELITY.md](FIDELITY.md)), and the
echo reads a second tap for the feedback path. This costs CPU. Measured on the Force on
2026-10-05 with `bench -s 10 -v 1,4 -p -j`, the previous 2.0.0 binary (from its backup) and the
new one alternated, two rounds each with identical means (raw output:
`resources/force-installed-2.0.0-fidelity-10s.txt`; the new build's line is
`resources/force-bench-2.0.0.json`):

| Stage | previous 2.0.0 mean / p99 | with fidelity fixes mean / p99 |
| --- | ---: | ---: |
| idle | 0.7 / 1.0 | 0.7 / 1.1 |
| 1 voice | 1.1 / 1.4 | 1.6 / 2.0 |
| 4 voices | 1.1 / 1.4 | 1.6 / 2.0 |
| all-parameter sweep | 1.3 / 1.9 | 1.7 / 2.6 |
| release tail | 0.8 / 1.4 | 1.1 / 2.0 |

Worst block 2.9 % (previous 2.3 %); 1.0.2 was 5.4 % mean for one voice. Output is much louder
than before (benchmark peak 0.96 instead of 0.43), as intended. Under qemu the same change
reads 15.4 % → 26–28 % per voice and on x86 0.3 % → 0.8 %: qemu overstates the cost by about
8×. As before this is plugin thread CPU per block, not the Force UI meter or a busy project.
