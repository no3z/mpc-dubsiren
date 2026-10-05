# Native DSP implementation (2.0.0)

The source-observed behavior and attribution are documented in
[BARZINE_DSP_REFERENCE.md](BARZINE_DSP_REFERENCE.md). The engine runs at 44,100 Hz,
matching the sd88me engine ABI. Version 2.0.0 keeps only the controls that are on
Q-Links plus FIRE, LATCH, SIREN/ZAP and STOP; reverb, echo characters, flange,
spread, tape noise, drift, 8-bit, chop, noise mix, nested LFOs, oscillator/master
EQ, filter modes, FILTER FX, freeze, bend, invert, kill and the FAST/SLOW/octave
modifiers are removed.

Signal path: oscillator → envelope → resonant lowpass → tape echo → fixed 70%
level → compressor → limiter → OUTPUT → soft bound below 0.98.

## Block structure and SIMD

`Siren::render` processes chunks of at most 16 samples aligned to a global
16-sample grid, which is also the control interval. Each chunk runs stage by
stage over small aligned buffers instead of one long per-sample loop:

1. Pitch glide (closed form of the 10 ms one-pole), LFO or ZAP frequency — vector.
2. Oscillator phase accumulation — serial, double precision.
3. Band-limited waveform (PolyBLEP square/saw, PolyBLAMP triangle, polynomial
   sine) or 4-lane xorshift noise — vector, four samples per instruction.
4. Envelope and resonant lowpass — serial (the only per-sample recursion left).
5. Echo — vector reads, writes and panning; serial loop filters.
6. Compressor and limiter — vector detection, log2/exp2 shaping curve and gain
   application; serial detector and gain smoothing.
7. Output gain, bounds, and float→int16 interleaving — vector.

`src/simd.h` maps four-lane operations to NEON intrinsics on ARM (the Force,
`-mfpu=neon-vfpv4`, fused multiply-add), SSE2 on x86 for desktop tests, and
plain loops when `DUB_SIMD_SCALAR` is defined. Vector sine, log2 and exp2 are
range-reduced polynomials (errors below 2e-7, 2e-6 and 1e-7, checked in
`tests/performance_controls.cpp`). Scalar code is built with `-ffp-contract=off`
so results do not depend on where LTO inlines a call. `tools/build.sh test`
renders all presets with the SIMD and the scalar paths and allows at most
2 LSB of 16-bit difference (0 measured); `tools/build.sh arm-test` runs the whole suite on the NEON build
under qemu-user.

## Oscillator, modulation and envelope

PITCH is 10–2400 Hz (logarithmic knob). LFO DEPTH is a percentage of the gliding
pitch: `f = pitch·(1 + depth·LFO)`. 100% sweeps between 0 Hz and twice the pitch,
so PITCH transposes the whole siren at every value. (1.0.x used absolute Hz depth;
below ~100 Hz the default 480 Hz depth swamped the pitch and the knob had little
audible effect.) LFO shapes are triangle, square, saw and sine; the LFO free-runs
through gate changes. Signed frequency is bounded to ±0.4·Fs.

ZAP is an exponential sweep from the pitch to `max(1 Hz, pitch·2^(sweep/12))`
over a fixed 180 ms, then holds; the envelope opens for `max(attack, 180 ms)`.
Attack/release are exponential with time constants `max(1 ms, attack/4.6)` and
`max(3 ms, release/4.6)`. Wave changes crossfade over ~10 ms. FIRE gives a 250 ms
gate (or one complete ZAP); MIDI notes gate while held; LATCH sustains. Gates
change on the 16-sample grid, so a FIRE pulse may end up to 15 samples late.

The 12 dB resonant lowpass (cutoff 200–9000 Hz, resonance 0–20 dB as source Q)
keeps double-precision state and rejects non-finite or runaway output.

Saw and square oscillators are scaled by 0.84: Chromium normalises its band-limited
tables by the Gibbs peak, so the page's flat parts sit at 0.82–0.85 of ±1 (see
[FIDELITY.md](FIDELITY.md)).

## Echo

`src/effects.cpp` is the BARZINE CLEAN echo topology: a preallocated 3-second
mono delay with linear fractional reads, 16-sample control ramps (time changes
bend pitch like tape), and feedback (0–88%) through fixed 120 Hz highpass and
7600 Hz lowpass filters (source Q 0.707 dB). The first repeat is unfiltered.
Wet gain is 0.62·MIX. The feedback path takes 128 samples more per pass, as in Web
Audio (the graph breaks the delay cycle with one render quantum): the first repeat
lands at T, repeat k at k·T+(k−1)·128 samples. PING PONG alternates the wet signal between equal-power
pan positions ±ping with a square LFO at 1/(2·delay). Dry passes at unity.

The shortest delay (2205 samples) is longer than a chunk, so a chunk's reads all
precede its writes and run as contiguous vector loads when the delay time is
steady. Reset is constant time: reads older than the writes since reset return
silence. Once the ring has held only zeros for its full length and the input is
silent, a chunk returns exact silence without touching the buffer.

## Master

The BARZINE page runs its master compressor and output limiter as Web Audio
`DynamicsCompressorNode`s, and their curve is much gentler than the numbers
suggest. Threshold −24 dB / knee 30 dB / ratio 12 (compressor) and −1 dB / 0 dB / 20
(limiter) are the node's parameters, but in Chromium's implementation the knee extends
*above* the threshold, the output carries an automatic makeup gain of
`(1/curve(1.0))^0.6` (+3.66 dB and +0.57 dB for these two) and the release adapts to
the amount of compression. A textbook compressor with the same numbers is ~19 dB
quieter on a loud signal. 2.0.0 ports Chromium's algorithm (BSD-3-Clause, credited in
NOTICE.md): exponential knee solved for slope 1/ratio, detector with a 2.5 ms
release, 32-sample divisions with the adaptive quartic release and attack rate, and
the sine-warped gain. The static curve matches the node to 0.006 dB over −60…0 dBFS
(`tests/performance_controls.cpp` checks seven points measured on Chromium).

Differences from the node: no 6 ms look-ahead (the instrument adds no latency; the page's
two nodes delay its output 12 ms) and the state starts settled on silence. Without
look-ahead the limiter lets peaks through on bass-heavy echo presets (up to about +4 dB
over the ceiling, in ≤1.5 % of samples), so OUTPUT ends in a soft bound instead of a
hard clip: identity up to 0.9, then a smooth knee that approaches 0.98 and never
exceeds it.

## State and real-time safety

Project state is `DFS2 key=value ...`; unknown keys are ignored and missing keys
take defaults, so later versions can add or remove parameters. 1.0.x `DFS1`
chunks are migrated by key; their Hz depth becomes the same sweep expressed as a
percentage (capped at 100%), and their waveform IDs map to the new WAVE order.
Malformed chunks are rejected whole. Audio processing does not allocate, lock or
call external APIs; parameter targets are lock-free atomics read once per render.
