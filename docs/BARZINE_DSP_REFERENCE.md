# BARZINE Siren Deck DSP reference

Research date: 2026-10-03. Primary source is the publicly delivered [DECK page](https://barzine.news/siren-deck/); the [classic page](https://barzine.news/siren/) was checked for differences. This records source observations, not an audio equivalence measurement.

## Source reference

Normal HTTPS downloads were saved outside the repository as `/tmp/dub-force-research/deck.html` and `index.html`. SHA256 of DECK: `f43e43a2ac55159be34672aebbfb550b7c890d5e0d7a07204a3fda1c65adf9cb`; classic: `0890adfdddd4fed62013e435ece95dab6830e51991dd5d199ddb21fb8866a07f`. Line references below refer to these snapshots, not the web tool's extracted text. Source can change later.

The main engine is an inline JavaScript IIFE in DECK HTML, approximately lines 1401–3290. A second inline script, lines 3293 onward, proxies front-panel controls to underlying inputs. No external DSP bundle, AudioWorklet, or source map is used by this page. Loaded `metrics.js` and `lab-screenshot-mode.js` are auxiliary, not the identified DSP.

Evidence labels: **CONFIRMED FROM SOURCE** means directly read engine/configuration; **INFERRED FROM BEHAVIOR/API** means graph mathematics or Web Audio semantics; **OUR OWN IMPLEMENTATION CHOICE** means a proposed native equivalent. Confidence is high for explicit constants/routing, lower for undocumented browser internals. Native choices are recommendations until implementation documents record actual behavior.

## Runtime defaults and parameter ranges

**CONFIRMED FROM SOURCE**, DECK `P`, lines 1465–1480; input elements lines 1163–1366; final initialization line 3284. Native UI mappings should use actual runtime defaults. Initial HTML displays feedback 60%, reverb 25%, time .42s, but final `applyDelayStyle(clean)` sets feedback .42, reverb .16, delay 60/72 × .5 = .4166667s. DECK source volume is .70; classic is .45.

All input sliders map linearly in the displayed units; percentage bindings divide by 100. Deck proxy `ratioOf`/`nudge`, lines 3334 onward, also uses linear parameter ranges. Musical pitch dragging on the XY pad is a separate exponential multiplier.

| Parameter | UI range / step | Actual startup | Native representation |
|---|---|---|---|
| Mode | Siren / Zap | Siren | enum |
| Source wave | square, sawtooth, triangle, sine, noise | square | enum |
| Pitch | 60–2400 / 1 Hz | 620 Hz | Hz |
| Siren level | 0–100 / 1% | 70% DECK | linear gain .70 |
| Noise mix | 0–100 / 1% | 0 | normalized amount |
| Master | 0–100 / 1% | 100% | linear gain 1 |
| Osc/master/BGM EQ, each band | −24–12 / 1 dB | 0 dB | dB |
| LFO1 wave | triangle, square, sawtooth, sine | triangle | enum |
| LFO1 rate | .05–24 / .05 Hz | .55 Hz | Hz |
| LFO1 depth | 0–1400 / 5 Hz | 480 Hz | additive FM Hz |
| Attack | .005–1 / .005 s | .010s | seconds |
| Release | .02–3 / .01 s | .050s | seconds |
| LFO2 rate | .03–8 / .01 Hz | .17 Hz | Hz |
| LFO2 amount | 0–100 / 1% | 0 | normalized |
| LFO3 rate | .03–8 / .01 Hz | .11 Hz | Hz |
| LFO3 amount | 0–100 / 1% | 0 | normalized |
| Chop rate | .25–32 / .25 Hz | 4 Hz | Hz |
| Chop amount | 0–100 / 1% | 0 | normalized |
| Zap sweep | −48–48 / 1 semitone | −24 st | semitones |
| Zap duration | .04–1.2 / .01 s | .18s | seconds |
| Zap repeat | 0–16 / .5 per second | 0 | Hz, zero means single shot |
| Tone cutoff | 200–9000 / 10 Hz | 4000 Hz | Hz |
| Resonance | 0–20 / .5 | 2 | Web Audio LP Q in dB |
| Delay time | .05–3 / .01 s | .4166667s | seconds |
| Feedback | 0–88 / 1% | 42% | gain .42 |
| Reverb | 0–100 / 1% | 16% | gain .16 |
| Ping pong | 0–100 / 1% | 0 | wet panner depth |
| Delay mix | 0–100 / 1% | 100% | wet-only scale, dry stays 1 |
| Feedback HP | 40–1200 / 10 Hz | 120 Hz | Hz |
| Feedback LP | 800–12000 / 50 Hz | 7600 Hz | Hz |
| Wet polarity | normal / invert | normal | wet sign |
| 8-bit engine state | dormant on/off code; no Deck toggle | off | native switch is an extension |
| BPM | 60–180 / 1 DECK; classic 45–180 | 72 | BPM |
| BGM level | 0–100 / 1% | 60% | gain .60 |
| Loop length | 1, 2, 4, 8 beats | 4 | integer |

## Signal routing

**CONFIRMED FROM SOURCE**, DECK `buildGraph`, lines 1568–1820; connections 1755–1802. Confidence high. Let `s` denote post-envelope/filter/EQ/chop mono signal and `d` the primary delay output.

```text
oscillator + selected noise + additional noise
  -> envelope -> tone LP -> oscillator EQ low/mid/high -> chop -> s
s -> dry -------------------------------------------------------> source master
s -> delay send -> primary delay -> d -> wet gain/pan ------------> source master
                                     -> spread delay/gain/pan --> wet pan input
                                     -> HP -> LP -> drive -> FB -> primary delay input
s -> short flange delay/gain -----------------------------------> source master
s + d -> stereo convolution reverb -> reverb gain ---------------> source master
source master -> global performance LP -> compressor
                -> optional 8bit branch -> kill -> master EQ -> limiter -> output gain
```

Tape noise enters delay send, bypassing note envelope. Dry signal and reverb receive no delay-mix attenuation. Spread tap is panned first, mixed with ordinary wet signal, then both pass through the final wet panner. The feedback branch is taken from primary delay output before wet gain/pan: progressively filtered repeats are essential. The first delayed repeat is not processed by the feedback HP/LP/drive; later repeats are. There is one primary mono delay for mono siren input; source ping-pong is an output panner, not crossed stereo feedback.

BGM dry bus enters global LP independently of source master. A separate BGM aux EQ path feeds delay send. Loop playback enters global LP at gain .9. Recording output taps final masterOut; looper capture taps killGain before master EQ/limiter/output gain (`connectLoopTap`, 2467 onward). Native monophonic synth need not implement BGM/record/looper unless separately chosen; do not claim these are reproduced if omitted.

## Oscillator, phase, pitch, and noise

**CONFIRMED FROM SOURCE**, `buildGraph` 1645–1662, `currentPitch` 1910–1916, `makeNoiseBuffer` 1534–1541, `setWave` 3158 onward. Web Audio `OscillatorNode` supplies square/sawtooth/triangle/sine. No custom anti-aliasing implementation appears. Oscillator and all LFOs start once when graph is constructed, continue through note release, and are not restarted/reset by noteOn or zap. STOP closes context; next play reconstructs graph and restarts phases/noise buffers.

XY pad normalized coordinates x,y in [0,1]: pitch multiplier `2^(2x−1)` (.5–2), rate multiplier `4^(1−2y)` (.25–4). Base pitch is `clamp(pitch × padPitch × octaveUp × octaveDown × bend,30,8000)`, where up=2, down=.5, bend=.5 when held. Clamp applies to base pitch, not the subsequent additive LFO. FM can yield zero/negative instantaneous oscillator frequency. Web Audio defines signed oscillator frequency; native safety policy must explicitly record any clamping departure.

Noise is uniform white noise in [−1,1], a randomly generated two-second mono buffer repeating indefinitely. NOIZ selects noise at gain1 and mutes oscillator. For other waves, extra noise gain is `.24 × clamp(noiseMix,0,1)^1.7`. Same noise source also drives tape-noise send, preserving correlated noise branches. White noise sample sequence is nondeterministic per graph construction.

**OUR OWN IMPLEMENTATION CHOICE:** phase accumulator with PolyBLEP square/saw and a bandlimited triangle; keep phases free-running. Native anti-aliasing is an approximation to browser waveform generation. A realtime PRNG or preallocated 2s noise loop is a practical equivalent; document which is used. Browser harmonic limits/implementation and exact output phase at first user interaction are not established by BARZINE code. Confidence high on phase lifecycle/noise mapping, medium on spectral equivalence.

## Nested LFO and chop

**CONFIRMED FROM SOURCE**, graph connections 1755–1758, helpers 1920–1951. Let `r = rate × fast × slow × padRate`, where fast=4 and slow=.25 if held. LFO2 and LFO3 are sine waves; main LFO has selected waveform `W`, phase continuously accumulated.

Mathematical equivalent: `r1(t)=r × [1+.9 × amount2 × sin(phase2)]`; `depth(t)=depth × [1+amount3 × sin(phase3)]`; `frequency(t)=basePitch(t)+depth(t) × W(phase1)` in Siren. In Zap, main LFO gain and LFO3 gain become zero. Thus rate modulation is additive in Hz with depth proportional to base rate, and pitch modulation is additive Hz rather than semitones. At full amount2 main LFO rate varies .1r–1.9r; at full amount3 depth varies 0–2depth. Simultaneous fast+slow cancel to multiplier1.

Chop is a square oscillator into chop gain: `gain(t)=1−amount/2 + (amount/2) × square(phaseChop)`, giving alternating gain1 and gain(1−amount). Chop occurs after oscillator EQ and before all dry/FX sends. Amount/base changes smooth with tau .015s; chop rate tau .02s. Edge waveform smoothing comes from Web Audio square oscillator bandlimiting, not an explicit envelope.

**OUR OWN IMPLEMENTATION CHOICE:** sample-accurate sine nested modulators, free-running phase, smoothed parameter targets. Confidence high on equations, lower on reproducing browser bandlimited LFO edges exactly.

## Envelope, trigger, latch, Zap

**CONFIRMED FROM SOURCE**, `fireZap` 2122–2133, `startZapLoop` 2134–2140, `noteOn` 2141–2156, `noteOff` 2157–2170, latch handler 2262–2268. Attack approaches gain1 exponentially with `tauA=max(.001,attack/4.6)`; release approaches zero with `tauR=max(.003,release/4.6)`. Displayed attack/release approximate 99% settling times, not exponential time constants. Envelope schedules cancel future values on retrigger; current gain is retained, not reset to zero. Siren noteOn moves base pitch with tau .01s and starts attack; noteOff starts release while FX tails continue.

Zap triggers oscillator base `f0=currentPitch()`, then exponentially sweeps to `f1=max(30,f0 × 2^(sweep/12))` over duration T: `f(t)=f0 × (f1/f0)^(t/T)` for 0≤t≤T. No upper clamp on f1 in source. Envelope release begins at `max(attack,T)` after trigger. Each repeated zap reschedules sweep/envelope from current envelope gain. Pitch sweep itself resets immediately to f0 for each shot; oscillator phase stays continuous.

Repeat base `repeat × padRate`. If fast and slow have equal state, use base. Otherwise, with repeat>0 use `min(base × fastSlowMultiplier,64)`; with repeat=0 use `min((fast?8:2) × padRate,64)`. `setInterval(1000/rate)` controls source repetition and has browser timing jitter. First shot fires immediately on noteOn. Rate changes restart timer without an immediate extra shot.

Latch ON immediately fires if not already playing; pad release is ignored while latched. Latch OFF releases current note. Changing mode releases playing voice and resumes only when latch is enabled. Presets likewise preserve latch. STOP clears holds/gate/latch and closes graph, cutting all tails. Blur releases unlatched gate and held performance modifiers.

**OUR OWN IMPLEMENTATION CHOICE:** native sample counter for repeat timing avoids browser jitter; signed safe frequency limit at Nyquist and sample-rate guards must be documented. Confidence high on scheduling/equations, no listening comparison yet.

## Filters and EQ: important Q-unit distinction

**CONFIRMED FROM SOURCE**, `buildGraph` 1664–1675, 1713–1715, 1725–1753; cutoff/resonance helpers 1961–1975. Source tone is second-order Web Audio lowpass; no source BP/HP selector exists. Main cutoff/resonance changes use tau .02s. Feedback has highpass then lowpass, each Q property .707. Global LP normally cutoff20000 and Q property .7; filter hold cutoff `clamp(.12 × toneCutoff,140,420)` and Q property `max(10,reso)`.

**CONFIRMED FROM WEB AUDIO SPEC**, [BiquadFilterNode attributes and coefficients](https://www.w3.org/TR/webaudio/#BiquadFilterNode): LP/HP Q properties are dB, so an ordinary RBJ implementation must use `Qlinear=10^(Qproperty/20)`, not the property directly. Therefore reso2 gives Qlinear1.2589, reso20 gives10, feedback .707 gives1.0848, and global .7 gives1.0839. BP/peaking Q is linear; shelves ignore Q. Native BP/HP tone modes would be project extensions, not source findings.

RBJ equivalent: w=2πf/fs, alpha=sin(w)/(2×10^(Qproperty/20)); denominator coefficients `(1+alpha,−2cos(w),1−alpha)`. LP numerator `((1−cos(w))/2,1−cos(w),(1−cos(w))/2)`; HP numerator `((1+cos(w))/2,−(1+cos(w)),(1+cos(w))/2)`. Normalize by a0. Shelves use slope1.

Osc/master EQ: low shelf120Hz, peaking1000Hz Q=.8 linear, high shelf6000Hz. BGM EQ: shelf140Hz, peaking850Hz Q=.8, high shelf4200Hz; both BGM dry/aux have these settings. All gains −24..+12dB, tau .02s. Native stable biquads with coefficient/control smoothing are suitable. Confidence high; clamp cutoff relative to native sample rate as an explicit safety addition.

## Dub delay, saturation, modulation, and stereo

**CONFIRMED FROM SOURCE**, helpers 1952–1960/1976–2039, `applyDelayPan` 2040–2053, `applyDelayCharacter` 2054–2075. Primary DelayNode capacity5.2s; UI max3s. Bend changes delay to `min(5.2,1.72×time)` with tau .12s engaged/.18s released; base pitch bend tau .13/.20s. Normal time edits tau .05s. Fractional-delay interpolation is browser internals; native interpolated circular buffer is the equivalent.

Feedback path `delay → HP → LP → waveshaper → fbGain → delay`. Saturation transfer on [−1,1] is `x/(1+1.8×drive×abs(x))`; 257-point curve and 2× oversampling. Drive0 bypasses waveshaper. This is symmetric attenuation saturation, not asymmetric clipping. Web Audio transfer inputs beyond [−1,1] use endpoint values. A native exact formula without finite table interpolation/oversampling is a documented approximation.

Freeze send gain=.0001 and feedback=`min(.96,max(fb,.955))`; no exact unity feedback. Freeze-on send tau .015s, FB tau .04s; release FB tau .05s and send restoration delayed .04s then tau .02s. Clash cuts send to .0001 whenever voice gate playing=false, including release tail; other styles send1. Tape noise bypasses gate but still encounters delay send/freeze/clash.

Delay time = base + `modDepth × sin(phaseDelay)` + `wander × drift(t)`. Same sine modulates feedback LP frequency by `filterSweep × sin(phaseDelay)`. Drift is an 8-second repeating random curve:32 segments, uniform endpoints in [−1,1], final point=first, interpolant `u²(3−2u)`. Each segment lasts .25s. Flange is a parallel delayed tap of dry s: `flangeTime + flangeDepth × sin(phaseFlange)`; no flange feedback.

Ping-pong: square panning oscillator frequency `clamp(1/(2×max(.05,currentDelay)),.05,10)`; pan base is0 if ping amount>.0001, otherwise style.panBase. Pan amplitude equals normalized ping amount (style.panDepth only initializes that amount). Oscillator free-runs and rate changes tau .08s; panning is not synchronized to first note or first delay repeat. Native equal-power panning is correct for mono input; stereo spread input requires Web Audio stereo-panner handling for exact equivalence. Fallback browser panner uses linear left/right gains, a capability-dependent difference.

Wet gain=`style.wet × delayMix × (playing ? 1−duck : 1) × polarity`; spread gain analogous using style.spread. Tau .018s while playing/.14s released. Reverb gain=`rev × (playing ? 1−.65×duck : 1)` with tau .025/.18s. Duck is gate-driven, not an audio envelope follower. Polarity inversion affects primary wet/spread only; not feedback/flange/reverb/dry. Delay mix similarly excludes flange/reverb. Confidence high on graph/equations, medium on browser interpolation/stereo details.

## Echo character constants

**CONFIRMED FROM SOURCE**, DECK `DELAY_STYLES` lines 1481–1522. All times/depths are seconds; filterSweep Hz; rates Hz; pan in [−1,1]. Character selection also resets time/fb/rev/ping/HP/LP; leaves delayMix and polarity unchanged. A preset re-applies selected character, so its embedded historical delay values are overridden. Manual time edit disables BPM sync until character is selected again. No SPACE character exists in this source; Space Echo can be an original native preset.

| Constant | DESK | SMOKE | ORBIT | CLASH | CLEAN |
|---|---:|---:|---:|---:|---:|
| Fixed time / beat division | .18s | .42s | .75 beats | .5 beats | .5 beats |
| fb | .54 | .66 | .70 | .80 | .42 |
| rev | .08 | .32 | .44 | .12 | .16 |
| wet | .62 | .70 | .72 | .88 | .62 |
| HP | 180 | 70 | 220 | 260 | 120 |
| LP | 2600 | 1750 | 5200 | 3400 | 7600 |
| drive | .28 | .48 | .10 | .30 | 0 |
| delay sine depth | 0 | .007 | .0015 | 0 | 0 |
| delay sine rate | .20 | .34 | .17 | .20 | .20 |
| ping initial depth | 0 | .10 | .90 | .12 | 0 |
| pan base | 0 | 0 | 0 | 0 | −.28 |
| spread gain | 0 | .07 | .12 | 0 | .28 |
| spread time | .008 | .018 | .013 | .008 | .012 |
| spread pan | .60 | .55 | .70 | .60 | .62 |
| flange gain | 0 | .035 | .16 | 0 | 0 |
| flange base time | .004 | .005 | .004 | .004 | .004 |
| flange depth | 0 | .0012 | .003 | 0 | 0 |
| flange rate | .15 | .11 | .19 | .15 | .15 |
| tape noise | 0 | .0015 | 0 | 0 | 0 |
| duck | 0 | 0 | 0 | 0 | .58 |
| random wander | 0 | .004 | 0 | 0 | 0 |
| LP filter sweep | 0 | 300 | 1800 | 350 | 0 |

Sync time=`clamp((60/BPM)×division,.05,3)`; at72BPM Orbit .625s and Clean/Clash .4166667s. `applyDelayStyle`, 2076–2105: delay tau .05s, FB tau .03s, character frequencies/depths usually .04s. Native equivalent is this table driving actual filtering/saturation/delay modulation/spread/flange/duck/send behavior, not just renamed generic presets. Confidence high.

## Reverb, crush, output protection

**CONFIRMED FROM SOURCE**, `makeImpulse` 1558–1566 and `buildGraph`1710. Stereo ConvolverNode impulse is independently randomized each graph construction, 1.8s length, sample envelope `uniform(−1,1) × (1−n/N)^3.2`. Convolver normalize property is not changed from default true. Both dry s and pre-feedback-filter primary delay output feed convolver. No explicit predelay or algorithmic allpass/comb network exists. **OUR OWN IMPLEMENTATION CHOICE:** lightweight stereo comb/allpass or FDN approximation, preserve both sends, ~1.8s decay character and gain mapping. Not sample-exact to convolution and not fixed browser IR.

DECK `makeBitCurve`1994–2001: 8-bit signed transfer `q(x)=2×round(255×(clamp(x,−1,1)+1)/2)/255−1`, stored as65536-point table. No sample hold, no downsampling, oversample none. Dry/wet branch switch tau .008s after first compressor, before kill/master EQ/limiter. **Dormant source feature:** `P.bit8=false`; `setBit8` exists but has no caller or event listener in this snapshot. The page has no `id=bit8` element, and the helper would dereference a missing element if called. The visible `8BIT DECK` control is a link to the distinct `/siren-8bit/` instrument, not an in-place switch. Therefore these quantizer equations are confirmed code, but active public Deck bitcrushing is not established. Curve interpolation makes exact output slightly differ from ideal quantizer and zero can interpolate between adjacent nonzero levels; native direct quantizer should special-case digital silence or reproduce table interpolation. Classic has no bitcrush branch. Exposing native crush controls is a project extension implementing the dormant algorithm.

First compressor is a default Web Audio DynamicsCompressorNode: source changes no settings. API defaults threshold−24dB, knee30dB, ratio12, attack.003s, release.25s. Final limiter explicitly threshold−1dB, knee0, ratio20, attack.003s, release.08s, followed by linear masterOut. Kill ramps to0 over6ms and recovers with tau .018s. Limiter is outside feedback loop; it does not guarantee stable internal echo state. Native feedback safety/finite guards and limiting must protect internal state separately while preserving sane settings. Exact browser compressor transfer/lookahead is not established by BARZINE code. Confidence high on settings, medium on native equivalence.

## Source presets and interactions

**CONFIRMED FROM SOURCE**, `PRESETS`1524–1533; `applyPreset`3210–3249. Each preset updates only its declared core parameters; attack/release/noise/EQ/nested/chop/crush/level remain as previously set. Selected echo character is re-applied, overriding preset delay/fb/rev fields. Factory preset design may intentionally save complete native state, which is a project choice.

| Preset | Mode/wave | Pitch | Main rate/depth/wave or zap sweep/time/repeat | Cutoff | Reso |
|---|---|---:|---|---:|---:|
| Air Raid | Siren/square | 620 | .55Hz /480Hz /triangle | 4000 | 2 |
| Wobble | Siren/square | 340 | 6.5 /180 /square | 3200 | 4 |
| Fog Horn | Siren/square | 110 | .25 /40 /triangle | 900 | 1 |
| UFO | Siren/sine | 900 | 3.2 /700 /sine | 6000 | 3 |
| Steam | Siren/noise | 620 | 1.2 /0 /triangle | 2600 | 8 |
| Pew Pew | Zap/square | 1400 | −26st /.16s /7Hz | 5200 | 3 |
| Rapid | Zap/square | 900 | −18 /.07 /13 | 4200 | 5 |
| Laser | Zap/sawtooth | 2100 | −40 /.5 /0 | 7000 | 4 |

## Native equivalence limits and verification requirements

Source inspection establishes algorithms, not perceived matching. No browser/Force audio was captured in this research. Remaining uncertainties include browser oscillator bandlimiting, signed/extreme frequency behavior, fractional delay interpolation, 2× waveshaper resampling, exact compressor/lookahead, convolver normalization/random realization, and stereo panning of spread input. Native algorithmic reverb and PolyBLEP are explicitly approximations. Additional LP/BP/HP tone modes, adjustable crush, SPACE preset, host MIDI/tempo integration, safety guards and sample-accurate repeats are native extensions/choices.

Tests should verify additive-Hz pitch FM, full nested bounds, free-running phase through gate changes, attack/release exponential constants, zap end pitch/duration, continuous delay-time changes, feedback filters inside loop with unfiltered first repeat, freeze send/FB mapping, gate-driven Clean ducking, Clash send gating, output-pan ping-pong, wet-only inversion, actual startup defaults, silence with crush, and finite internal/output state at extremes. Document any deliberate departure in `docs/DSP.md` and do not describe omitted BGM/looper/record as implemented.
