# Dub Force Siren

Native ARM VST2 dub siren for Akai Force/MPC OS. The engine ports BARZINE Siren
Deck's siren and echo into C++, connected to sd88me's MPC wrapper, parameter
system, native touchscreen skin, Q-Links and portable installer.
Attribution and preserved notices are in [NOTICE.md](NOTICE.md) and [VENDORED.md](VENDORED.md).

Version 2.0.0 is a simplified, NEON-optimized rewrite: one page, sixteen Q-Links
grouped by function, and only the controls on those knobs plus FIRE, LATCH,
SIREN/ZAP and STOP. See [installation instructions](docs/INSTALL_FORCE.md) before
installing. The [1.0.2](dist/Dub-Force-Siren-1.0.2-mpc-armv7.zip),
[1.0.1](dist/Dub-Force-Siren-1.0.1-mpc-armv7.zip) and
[1.0.0](dist/Dub-Force-Siren-1.0.0-mpc-armv7.zip) ZIPs are retained for rollback.

![Generated native panel preview](resources/preview.png)

| Bank | Knobs |
| --- | --- |
| 1 · SIREN | 1 Pitch, 2 Wave (OSC) · 3 Rate, 4 Depth, 5 Shape (LFO) · 6 Sweep (ZAP) · 7 Attack, 8 Release (ENVELOPE) |
| 2 · FX | 9 Cutoff, 10 Resonance (FILTER) · 11 Time, 12 Feedback, 13 Mix, 14 Ping Pong (ECHO) · 15 Patch, 16 Output (MASTER) |

The top row of the screen is bank 1 and the bottom row bank 2, so each control
sits above the knob that turns it. Five oscillator waves, four LFO shapes, an
exponential ZAP, a resonant lowpass, a tape echo with filtered feedback and
ping-pong, and a compressor/limiter. LFO Depth is a percentage of Pitch, so Pitch
transposes the whole siren down to 10 Hz. Twelve factory presets.

```sh
./tools/build.sh host       # desktop Linux .so
./tools/build.sh test       # ASan/UBSan suites + SIMD/scalar render equivalence
./tools/build.sh arm        # 32-bit ARM hard-float NEON hardware .so
./tools/build.sh arm-test   # the test suites on the NEON build under qemu-user
./tools/skin.sh             # native skin, Q-Links, preview; then tools/audit_ui.py
./tools/package.sh          # checksummed portable ARM install ZIP
```

Host requirements: GCC/G++, Python 3, Pillow; Docker for cross compilation and
qemu-user binfmt for `arm-test`. The glibc 2.31 cross image is selected by
default; `docker build -t dub-force-arm -f tools/Dockerfile.arm .` and
`DUB_ARM_IMAGE=dub-force-arm` build the same toolchain elsewhere.

Requires a 44.1 kHz native host. MIDI timing follows the 128-frame wrapper; it is
not sample-accurate. Touch FIRE is a 250 ms siren tap or one full ZAP; MIDI pads
provide press/release and LATCH sustains.

**2.0.0 compatibility.** Projects saved with 1.0.x reopen with their state
migrated by key (pitch, wave, LFO, ZAP sweep, envelope, filter, echo time,
feedback, mix, ping, output); removed effects are dropped. Automation recorded
against 1.0.x parameter indices does not carry over.

See [DSP](docs/DSP.md), [parameters](docs/PARAMETERS.md),
[UI/Q-Link wiring](docs/UI_WIRING_AUDIT.md), [2.0.0 performance](docs/PERFORMANCE_2.0.0.md),
[installation](docs/INSTALL_FORCE.md), [rollback](docs/ROLLBACK.md),
[Force test card](docs/MANUAL_FORCE_TEST_CARD.md) and [TODO](TODO.md).
Reports for 1.0.x ([performance](docs/PERFORMANCE_1.0.2.md),
[RC audit](docs/RELEASE_CANDIDATE.md), [UI iterations](docs/UI_REDESIGN.md))
describe the earlier engine and panel.
