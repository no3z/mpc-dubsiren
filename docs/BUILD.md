# Build and verification

Run from the repository root:

```sh
./tools/build.sh host
./tools/build.sh test
./tools/build.sh arm
# Or all three, stopping on the first failure:
./tools/build.sh all
```

Host artifacts are `build/host/dub_force_siren.so`. Sanitized artifacts and tests
are in `build/test/`. Hardware artifact is `build/arm/dub_force_siren.so`.
The script regenerates `build/params.h` from the stable ordered descriptor before
compiling. C wrapper compiles with GNU C11; native sources use C++17. Every source
is compiled separately; the link uses g++ and `-Wl,--no-undefined`.

Host prerequisites: Python 3, GCC/G++, standard C/C++ library. The parameter-only
generation uses vendored Python tooling without downloading dependencies. Skin
generation/preview additionally needs Pillow and system DejaVu Sans fonts. Run
`tools/skin.sh` for the project's original procedural artwork/native JSON generator;
see `docs/UI_REDESIGN.md`. It emits all three design previews and validates native
asset geometry/remapping. The stock numeric overlay is referenced from the device,
not copied into the repository.

ARM uses the existing local Docker image `mnm-armhf-builder-glibc231:latest`,
which provides Debian GCC 10.2.1 `arm-linux-gnueabihf-gcc/g++` and glibc 2.31.
Override only via `DUB_ARM_IMAGE` if an equally compatible toolchain is available.
It runs a cross compiler on the host architecture: no ARM emulation is necessary.
Project directory mounts at `/work`, and the container uses the host UID/GID.

ARM flags: `-O3 -flto -fno-math-errno -fPIC -fvisibility=hidden -mcpu=cortex-a17 -mfpu=neon-vfpv4
-mfloat-abi=hard`; C11 wrapper/C++17 sources; `-lm`. All artifacts link with no
undefined symbols. ARM artifact is stripped and inspected with cross `readelf`.
The script rejects any required GLIBC version newer than 2.32 and prints global
exports. Intended export is only `VSTPluginMain`. Cortex-A17 is this project's
Force target; do not claim the binary supports unrelated CPU generations without
testing their instruction compatibility.

## Vendored integration changes

Upstream reference is pinned in `VENDORED.md`; source reference checkout was not
modified. Wrapper changes in the project copy preserve existing source comments:

- Reject nonfinite normalized control values, clamp enum readback, guard invalid
  indices, NULL host/event/chunk pointers and invalid event counts/sizes.
- Keep MIDI timestamps at upstream block resolution; no unproven sample-accurate
  event behavior is advertised.
- C11 lock-free atomics protect parameter shadow positions, popup states,
  deferred display flags and trigger countdowns shared across host/audio threads.
  One bounded compare/exchange countdown attempt per parameter avoids losing a
  concurrent newly fired trigger; contention defers that countdown to the next
  block rather than retrying on the audio thread.
- `popup.h` signatures use the same atomic arrays. Build-time assertions require
  lock-free integer, byte and float atomics for realtime operation.
- Chunk getter forces bounded string termination. State serialization remains
  per-instance textual `DFS1` schema, with all parsed fields validated before
  restore. Hidden popup open flags stay outside saved DSP state.
- The fixed-rate engine accepts only 44100 Hz; other sample rates return failure.
  Host may ignore that dispatcher return, so **this plugin requires a 44.1 kHz
  host**. Positive arbitrary host block sizes are serviced through the upstream
  128-frame synth FIFO. Invalid/empty output buffers are ignored safely.
- MPC-specific upstream display buffer sizes are preserved for this native host:
  name/product/vendor 32 bytes, display 24, label 8. Generic desktop hosts with
  smaller name/display storage require a separately bounded desktop integration.
- Display precision preserves meaningful short times/rates: seconds use three
  decimals, Hz ranges at most 32 wide use two, larger Hz and percent ranges use
  whole numbers. Attack 0.010 s and nested LFO 0.17 Hz have explicit regressions;
  every parameter is checked against the 24-byte display-buffer bound.

## Tests and practical limits

`./tools/build.sh test` compiles with AddressSanitizer + UndefinedBehaviorSanitizer,
frame pointers, and leak detection. Integration checks real MIDI-to-audio (failure
on silence), fresh-instance silence, instance separation, ABI identity, rejected
nonfinite/invalid controls, supported sample rate, chunk round trip and malformed
chunk preservation, arbitrary host blocks, legacy additive processing, all five
finite/bounded native waveforms, latch/release, 440 Hz crossing count, render-block
continuity, random extreme control edits, and concurrent setter/render stress.

Allocation hooks intercept C malloc/calloc/realloc/free and ordinary C++ new/delete
for linked project code. Audio calls are marked using a thread-local flag; counters
must stay zero. This is a meaningful regression check for project allocations,
not proof that every allocation inside a separately linked system library is
intercepted. Rendering source review remains necessary. Constructor allocations
are expected and are not marked as realtime calls.

The separate effects test runs under the same sanitizers. It verifies fresh
silence, first-echo timing, first-repeat routing independently of feedback gain,
constant-time reset/tail discard, stereo panning/spread, all five characters,
modulated-delay wrap/rapid edits, freeze, inversion, extreme controls and NaN guards.

Verified on 2026-10-03: 29 integration assertions passed, plus `effects_test PASSED`,
under ASan/UBSan with leak detection. Measured sine frequency was 440 crossings
in one second. Intercepted callback allocations/frees were both zero. STOP
regressions cover pending FIRE, pending MIDI, and successful FIRE after STOP.
The final ARM ELF is 32-bit ARM hard-float, requires at most GLIBC_2.29, and exports
only `VSTPluginMain`. These results refer to the final core/effects integration;
they do not substitute for subsequent hardware/CPU/UI checks.

Sanitizer success is not a CPU benchmark or an audible browser-equivalence claim.
The local build is fixed 44.1 kHz; hardware loader checks, CPU timing, native skin
touch/Q-Links and listening are separate deployment verification tasks.

Release optimization uses strict floating-point behavior (no fast-math), LTO, and
port-local numeric parameter access via `DUB_TYPED_PARAMETERS`. This avoids
per-edit snprintf/strtof/linear key lookup while preserving the sd88me function
table, sole public VST export, parameter indices and textual saved chunks.
See [PERFORMANCE.md](PERFORMANCE.md) for measured results and approximations.

Release-candidate clean-build commands, exact toolchain and matching 1.0.1
checksums are in [REPRODUCIBILITY.md](REPRODUCIBILITY.md). State, UI and long-run
hardening results are in [RELEASE_CANDIDATE.md](RELEASE_CANDIDATE.md).
