# Native DSP implementation

The source-observed behavior and attribution are documented in [BARZINE_DSP_REFERENCE.md](BARZINE_DSP_REFERENCE.md). Native implementation runs at 44,100Hz, matching the sd88me engine ABI.

## Echo and reverb

`src/effects.cpp` implements a preallocated 5.2-second mono feedback delay, short stereo spread tap, parallel dry flange, tape noise, randomized smooth drift, feedback filter modulation, output panning, gate-driven ducking, phase inversion and stereo algorithmic reverb. Echo character order is CLEAN, DESK, SMOKE, ORBIT, CLASH. Character-specific internal constants follow BARZINE; core parameter selection sets character time/feedback/reverb/HP/LP/ping defaults.

The first delayed repeat remains unfiltered. Subsequent repeats pass through HP, LP, rational saturation and feedback gain before re-entering the primary delay. Both feedback biquads convert the source Q=.707dB property to linear Q. Saturation evaluates `x/(1+1.8*drive*abs(x))` with source endpoint clamping outside ±1. The native implementation evaluates this directly, without the browser's 257-point lookup interpolation or 2× oversampling; this is an approximation in feedback distortion spectrum.

Feedback is limited to .88 normally and .955–.96 during freeze. Freeze suppresses input to .0001; bend multiplies base delay by1.72 up to5.2 seconds. Configurations and moving parameters are bounded and checked for finiteness. Internal delay writes are limited to±16, filter states reset if their output is invalid or exceeds±32, and combined effects output has a±32 guard. These are stability guards; the downstream compressor/limiter handles listening/output levels. Filters sit inside the echo loop, while the final output limiter sits outside it.

Delay-time, wet, reverb, modulation and panning targets use exponential parameter smoothing. Main delay changes intentionally produce tape-style pitch bends rather than an unrelated crossfade between static taps. Fractional delays interpolate linearly. Delay panning uses the source square LFO at1/(2*delay time); it does not use stereo cross-feedback. The spread signal is panned before mixing into the final wet stereo panner. The always-connected spread output makes the browser wet mixing input stereo, so primary mono wet upmixes equally into both channels before final stereo panning. Native panning follows the Web Audio equal-power mono and stereo formulas.

Random wander uses32 smoothstep segments in an eight-second repeating curve. The constructor creates its points with a deterministic xorshift seed; BARZINE generates random points per context. Tape noise uses a continuously running PRNG rather than the browser's two-second noise buffer. The source's periodic noise realization is therefore not reproduced sample-for-sample.

Stereo reverb is an explicit CPU-conscious approximation to BARZINE's normalized randomized1.8-second convolution. Four damped combs and two allpasses per channel provide distinct left/right reflections and an approximate1.8-second RT60. It receives dry plus raw primary delay output, preserving source send topology. No browser impulse response, heavy convolution or image/audio asset is bundled.

All rings and validity stamps allocate in `Effects` construction. Processing does not allocate, free, lock, log or call external APIs. `reset()` increments ring generations and clears scalar states in constant time, avoiding large buffer clears in a STOP callback. Denormal-scale values explicitly become zero. A fixed sample rate and deterministic seeded randomness are native implementation choices.

`tests/effects_test.cpp` verifies digital silence, unfiltered first repeat, reset/tail invalidation, stereo modulation, twelve seconds of rapidly changed extreme configurations, invalid input/control guards and zero callback/reset heap allocations. It passes normal optimized compilation and AddressSanitizer/UndefinedBehaviorSanitizer. Listening comparisons and device CPU measurements remain separate verification tasks.

## Native oscillator, modulation, envelope and output

`src/siren.cpp` owns per-instance phase, gate, ZAP and parameter state.
`src/dsp_core.h` implements PolyBLEP square/saw and PolyBLAMP triangle; sine
uses the standard sinusoid. Signed oscillator frequency is bounded to ±0.4×Fs;
phase wraps in both directions. Noise is a deterministic per-instance PRNG,
rather than the browser's random two-second repeating buffer. Wave selection
crossfades over 10 ms. LFO triangle/square/saw/sine and nested sine phases run
continuously through gate releases. STOP resets phases, states and FX tails.

Pitch is linear Hz FM: base + depth×LFO. LFO2 multiplies nominal rate by
`1 + 0.9×amount×sin`; LFO3 multiplies nominal depth by `1+amount×sin`.
FAST/SLOW multiply rate and ZAP repetition by4/.25. Octave and bend modifiers
match source pitch multipliers. MIDI notes gate the siren without replacing
its pitch control. Held-note bitsets keep it open while any note remains down.

Attack/release use exponential source time constants max(1ms,attack/4.6),
max(3ms,release/4.6). ZAP is an exponential f0→max(30,f0×2^(sweep/12)) ramp,
followed by release at max(attack,zapTime). Repetitions use native sample timing.
Touch FIRE produces a bounded250ms siren pulse or complete ZAP; MIDI pads supply
press/release and LATCH sustains. True native touch-hold was not established.
Modifier buttons toggle and STOP clears them. Source XY gestures are omitted.

Continuous controls update at a16-sample control interval with approximately
20 ms smoothing. Chop uses source square gain from1-amount to1, with an additional
0.8 ms native anti-click smoothing. Filter/EQ coefficients update every 32 samples in 1.0.1, with unchanged-input caching.
LP/HP Q properties convert source dB to linear Q; the additional BP mode uses
this same resonance scale. Oscillator/master EQ use shelves120/6000Hz and a
1000Hz Q=.8 peak. The post filter includes the source FILTER FX mapping.

Independent soft-knee stereo compression approximates source threshold−24dB,
knee30dB, ratio12, attack3ms/release250ms; final limiting uses−1dB, ratio20,
attack3ms/release80ms, plus a final ±.98 hard bound. Browser lookahead/resampling
and transfer implementation are not reproduced. Optional8bit uses the source
255-interval quantizer with an8 ms crossfade and exact digital-silence guard.
This quantizer is dormant in the current Deck browser UI; native8bit is an
exposed extension of that source code. Output and source levels are linear.

The 51 public parameters each reach DSP, preset/state management or a trigger.
Popup-open parameters appended by the skin generator belong only to the wrapper.
Factory presets are complete native snapshots; browser partial recall and its
character reapplication behavior are intentionally not used. Selecting a native
character does recall its documented time/feedback/HP/LP/reverb/panning defaults.
Synchronized character defaults use72BPM; host tempo sync is not implemented.

Version 1.0.1 uses 16-sample interpolated exponential effect-control endpoints,
a high-resolution modulation/panning sine lookup and double ZAP recurrence.
Audible sine, filter state precision and saturation are preserved. See
[PERFORMANCE.md](PERFORMANCE.md) for measurements, approximation bounds and A/B limits.
