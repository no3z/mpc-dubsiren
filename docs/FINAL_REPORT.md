# Delivery report — Dub Force Siren 1.0.1

This report records the original installed-package measurements. Published ZIPs
now have refreshed source credits; current download checksums are in
[REPRODUCIBILITY.md](REPRODUCIBILITY.md). The plugin binary is unchanged.

CPU profiling, optimization, UI redesign, rebuilding and redeployment are
complete. Existing functionality, all 51 public parameter IDs, six hidden UI IDs,
saved chunks and sd88me engine function-table ABI are retained.

| Force original benchmark | Baseline | Installed 1.0.1 |
| --- | ---: | ---: |
| Worst p99 | 19.6% | 11.0% |
| Worst block | 20.8% | 11.8% |
| Verdict | WARN | PASS |

Final values come from the installed-path 10-second run of the same original
stage/control workload. The original 2-second run also passes: 11.2% p99.
The primary <=12% target is met; the worst workload 8–10% stretch target remains
unmet. p99 reduction from the baseline is 43.9%. Playing normally measures about
10.2% p99. These are thread CPU fractions of the 44100 Hz/128-frame block budget,
not complete MPC project load. Every representative workload and all 12 presets
were benchmarked. [PERFORMANCE.md](PERFORMANCE.md) contains baseline and every
optimization's mean/p95/p99/max measurements, profile evidence and sonic limits.

Measured optimizations: branch-based ring wrap and fixed reverb reads; unchanged
coefficient/panner caching; zero-gain EQ bypass only with zero state; control
smoothing via 16-sample interpolated exponential endpoints; modulation/panner
sine lookup; cheap phase wrap; below-knee compressor shortcut; tiny inactive
wave/crush contribution skipping; 32-sample core coefficient updates; fixed EQ
trig caching; ZAP exponential recurrence; strict O3/LTO; numeric control bridge
removing formatting/parsing/key lookup. The float-filter trial was rejected.
Double filters, audible sine, PolyBLEP/BLAMP, saturation, all characters and
continuous echo/reverb state are preserved.

The approximations can shift transition response and echo phase. Host A/B
per-preset RMS stays within 0.05 dB, with nonzero sample residuals in modulated
patches. That does not establish subjective sonic equivalence. The dense sine
lookup check measured max absolute error 1.3019e-7. No unsafe fast-math is enabled.

The UI was rendered and inspected through three iterations:
[V1](../resources/ui-v1.png), [V2](../resources/ui-v2.png),
[V3 main](../resources/ui-v3.png), [utility](../resources/utility-v3.png).
The single main panel follows SIREN -> MODULATION -> TONE -> ECHO -> MASTER,
with charcoal/cream/amber artwork, stronger FIRE, adjacent LATCH and larger
primary faders/values. Both Q-Link banks keep that panel visible. Primary mapping:
Pitch, Rate, Depth, Zap, Cutoff, Resonance, Delay, Feedback. Native rectangular
filmstrip format was confirmed against actual Force stock schema; original
assets were generated locally. 55 main touch regions and 106 referenced assets pass
bounds, overlap, frame-format and reference validation. See [UI_REDESIGN.md](UI_REDESIGN.md).

Validation: 29 ASan/UBSan integration assertions plus effects tests pass, with leak
detection, randomized/concurrent controls, feedback/freeze extremes, wrap,
MIDI/STOP, state and block continuity checks. Callback allocation/free counts
remain zero. Host and real ARM builds pass; ARM ELF32 hard-float requires at most
GLIBC 2.29 and exports only VSTPluginMain. Binary 99,896 bytes; ZIP 308,424 bytes.

Installed directory:
`/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/`.
It contains `dub_force_siren.so`, `Plugin Skins/TUI.json`, both Q-Link JSONs,
original PNGs, presets, content metadata and retained notices.
All 116 files match [installed hashes](../resources/installed-1.0.1.sha256).
Settings remain `/media/az01-internal/Settings/MPC/MPC.settings`: one target
registration, total 29; other 28 registrations unchanged. `acvs` restart passes and
MPC logs discovery. The installed library loads/renders in the device VST harness.

Rollback files were preserved before installation:

- `/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.0-before-1.0.1/`
- `/media/AKAI_SSD/Dub-Force-Siren-backups/MPC.settings-before-1.0.1`
- `/media/az01-internal/Settings/MPC/MPC.settings.bak-dub_force_siren-20261003-012001`

Artifact: `dist/Dub-Force-Siren-1.0.1-mpc-armv7.zip`.
Binary SHA256: `1c92678caafd8d4a9c50758f2ecf53d805d03579847df62e3dda277ebc190beb`.
ZIP SHA256: `99e886f8f24c24e77007ea021aec7caf0d617187362e131eeee07675343d5a9a`.

Changed implementation files: `src/siren.cpp`, `src/dsp_core.h`, `src/effects.cpp`,
`src/engine.cpp`, new `src/fast_trig.h`, vendored `wrapper/vst2_wrap.c` and
`tools/bench.c`. Changed project tooling/config: `tools/build.sh`, `tools/package.sh`,
`tools/skin.sh`, new `tools/skin_design.py`, `tools/profile.sh`,
`tools/profile_sample.cpp`, `layout.conf`, `vst.json`. UI previews, validation and
installed hash resources are included. README/TODO/build/DSP/architecture/install,
performance/UI/device/final reports were updated; the baseline device report was
preserved as BASELINE_DEVICE_TEST.md.

Physical Force screenshot, editor touch/Q-Link/pad checks, real output listening,
project save/reload and busy-project testing remain pending. Direct framebuffer
capture returned black. At the final remote check MPC had discovered the plugin
but had not selected/mapped it; device harness loading is separately verified.
See [DEVICE_TEST.md](DEVICE_TEST.md) for evidence and the short physical checklist.
