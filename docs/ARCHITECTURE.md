# Architecture

Consolidated research review: 2026-10-03. The native siren is based on BARZINE
Siren Deck and uses sd88me's MPC integration. Source references: `VENDORED.md`.

`params.json` is the ordered VST descriptor; `src/param_ids.h` is its generated
native index mapping. The vendored generator creates `build/params.h`, metadata,
parameter metadata. `tools/skin_design.py` generates the original native TUI
and Q-Link maps. No web runtime is used on the Force.

`src/engine.cpp` implements sd88me's engine table (create/destroy/MIDI/string
parameters/state/render). State belongs to each instance. Parameters use lock-free
atomics; DSP snapshots targets at block boundaries and smooths continuous controls.
MIDI gates and trigger counters cross control/audio boundaries without locks.
The native DSP is float internally; the reference wrapper converts interleaved
int16 stereo to host floats, using 44.1 kHz and 128-frame render blocks.

The smallest sine instrument must build and produce MIDI audio before adding
siren DSP. Then integrate waves/modulation/ZAP/envelopes, chop/crush/filter/EQ,
source-topology echo characters, and lightweight stereo reverb. Delay and reverb
memory allocate only during instance construction. Runtime coefficients update
at a bounded control rate; no allocation, files, network or locks in rendering.

Signal: waves/noise -> envelope -> LP/BP/HP -> oscillator EQ -> chop -> dry.
Parallel sends feed a mono feedback delay and stereo algorithmic reverb. Echo HP,
LP and rational saturation precede feedback gain inside the loop. Primary echo
and delayed spread are panned at wet output; polarity inversion affects those
wet paths only. Character flanging is fed from dry. Reverb receives dry plus raw
delay. Source level -> post filter/compressor -> optional 8bit -> kill -> master
EQ -> limiter -> output. Safety clipping protects loop state separately.

Main TUI shows the complete sound controls and prominent FIRE/LATCH/STOP.
Only secondary EQ/modifier configuration may use a utility tab. Q-Link banks
reuse the same main panel. Native press/release gestures are not source-confirmed:
FIRE uses a bounded tap, MIDI sustains from pad press/release, LATCH toggles sustain.
Browser XY, backing riddim, recording and looper remain explicit exclusions of
this native siren engine rather than disconnected placeholder features.

Build: host GCC sanitizer tests; ARM hard-float GCC using a glibc 2.31 toolchain,
no undefined symbols. Release includes portable skin/binary/presets/provenance
and the upstream checksummed installer. Actual discovered target folder is
`/media/AKAI_SSD/Synths` on Force `192.168.2.31`, root SSH, `acvs` service;
settings registration requires its documented application stop/start and backup.

For 1.0.1, optional port-local numeric setter/getter functions bypass string
conversion for ordinary VST controls. The engine table and textual DFS1 state
remain compatible. No new public export or parameter index is introduced.
