# Reproducing release candidate 1.0.1

Historical 1.0.1 record. Current 1.0.2 results and compatibility limits are in
[PERFORMANCE_1.0.2.md](PERFORMANCE_1.0.2.md).
For the old build commands below, use source commit `bb02086`; current main builds 1.0.2.

Two separate source copies without `build/` or `dist/` reproduced the same ARM
binary and ZIP. First build ran `./tools/build.sh all`; second ran
`./tools/build.sh arm`. Both packaged with `./tools/package.sh`, which generates
the native skin. Run from a fresh source copy:

```sh
./tools/build.sh all
./tools/package.sh
sha256sum build/arm/dub_force_siren.so dist/Dub-Force-Siren-1.0.1-mpc-armv7.zip
```

| Artifact | Version / bytes | SHA-256 |
| --- | --- | --- |
| `build/arm/dub_force_siren.so` | VST 1001 / 99,896 | `1c92678caafd8d4a9c50758f2ecf53d805d03579847df62e3dda277ebc190beb` |
| `dist/Dub-Force-Siren-1.0.1-mpc-armv7.zip` | 1.0.1 / 308,109 | `6492b12687b006bd1a15830c368e472d3d2ae0abf5e205bb2cb16bb297a6f263` |

ARM ELF is 32-bit ARMv7 hard-float, Cortex-A17, highest required GLIBC_2.29;
only public export is `VSTPluginMain`. Native metadata remains 1.0.1.0. No
version change was necessary. The binary matches the installed baseline; the
published ZIP includes refreshed source credits.

Exact environment used:

- Docker image `mnm-armhf-builder-glibc231:latest`, image ID
  `sha256:d04f352258d8b0de7c41a8b5ea6b12f37bd3d3ba19840f20092e3b8c6da102c1`;
  Debian cross GCC 10.2.1 / glibc 2.31.
- Python 3.10.13, Pillow 12.1.1.
- DejaVuSans.ttf SHA-256
  `ae7b7855e115a5966d8b1b3f80f254ccc117ec86f9965e202ee2940453837280`.
- DejaVuSans-Bold.ttf SHA-256
  `5c1247acef7f2b8522a31742c76d6adcb5569bacc0be7ceaa4dc39dd252ce895`.

Reproducibility is demonstrated within this environment. The Dockerfile's
package repositories are not frozen; building it later with different compiler,
font or Pillow versions is not guaranteed to produce identical bytes.

**Packaging issue fixed:** the packager used optional ignored
`build/force-bench.json`; a clean source copy omitted installed CPU results,
changing ZIP metadata. It now reads the checked-in verified
`resources/force-bench-1.0.1.json`. Before the source-credit refresh, the clean ZIP
matched the installed baseline package byte for byte. The published ZIP was then
reproduced from the independent clean build with the refreshed credits; its
current checksum is in the table above. DSP, parameters, skin and binary are
unchanged. The original installed ZIP hash was
`99e886f8f24c24e77007ea021aec7caf0d617187362e131eeee07675343d5a9a`;
`resources/installed-1.0.1.sha256` records the untouched device payload.

Additional hardening commands (desktop simulated audio, not hardware time):

```sh
./tools/rc_audit.sh 600   # existing suite + 10 minutes of ASan/UBSan stress
python3 tools/audit_ui.py
# Release harness, using fresh host objects:
./tools/build.sh host
g++ -O3 -std=c++17 -Isrc -Itests -Ibuild -Ivendor/mpc-vst-plugins/wrapper \
  tests/rc_audit.cpp build/host/{wrapper,siren,engine,effects}.o -lm \
  -o build/rc-audit/rc_audit_release
build/rc-audit/rc_audit_release 1800
```

See [BUILD.md](BUILD.md) for compiler flags and dependency requirements.
