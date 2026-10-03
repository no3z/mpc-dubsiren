# RC UI and Q-Link wiring audit

Generated from `params.json`, `src/param_ids.h`, `src/siren.cpp`, the actual
native `TUI.json`, both Q-Link JSONs and original panel definitions. No parameter,
layout or DSP change is required. Every public parameter is bound. Repeated IDs
within an enum group are intentional; each option has a unique buttonId and the
correct group size. There are no duplicate local definition keys or duplicate
active IDs inside a Q-Link bank. Hidden popup ID 51 is transient; IDs 52–56 remain
compatibility flags, with no sound role.

Main Q-Link order is slots 13, 9, 5, 1, 14, 10, 6, 2: Pitch, Rate, Depth, Zap Sweep,
Cutoff, Resonance, Delay Time, Feedback. Slots 15, 11, 7, 3, 16, 12, 8, 4 form the second
set: Attack, Release, LFO2 amount, LFO3 amount, Echo Mix, Ping, Reverb, Output.
These are native 16-slot addresses, not a claim that Force has 16 physical knobs.
The Force selects subsets/banks; exact hardware selection must be tested.
Utility maps six EQ bands and Fast/Slow/octave up/down. Unused slots are -1.
Turning upward/rightward increases physical values; fader invert=false. Native
Q-Link names/ranges come from the descriptor, whereas section captions are
shortened artwork labels. Continuous normalized values are not quantized to the
32/48 artwork frames; enum values snap and support the wrapper's option nudge.
Zap Sweep is semitones, Delay is seconds, Resonance is source Q in dB, and Echo
LO CUT/HI CUT mean feedback highpass/lowpass. Primary ranges/defaults are below.

| Page / visible control | ID / key | DSP destination | Physical range | Default | Native display | Q-Link address |
| --- | --- | --- | --- | --- | --- | --- |
| SIREN / PITCH | 5 / pitch | c[P_pitch] → basePitch / zapStart | 60…2400 Hz | 620 Hz | 0 decimals | Main Q-Link 13 |
| SIREN / ATTACK | 11 / attack | attackCoeff / ZAP decay | 0.005…1 s | 0.01 s | 3 decimals | Main Q-Link 15 |
| SIREN / RELEASE | 12 / release | releaseCoeff → env | 0.02…3 s | 0.05 s | 3 decimals | Main Q-Link 11 |
| SIREN / LEVEL | 6 / level | c[P_level] → effect-output gain | 0…100 % | 70 % | 0 decimals | touch only |
| SIREN / NOISE | 7 / noise | noiseGain → oscillator white-noise mix | 0…100 % | 0 % | 0 decimals | touch only |
| SIREN / LFO RATE | 9 / rate | mainRate → s.lfo phase | 0.05…24 Hz | 0.55 Hz | 2 decimals | Main Q-Link 9 |
| SIREN / LFO DEPTH | 10 / depth | mainDepth → frequency FM | 0…1400 Hz | 480 Hz | 0 decimals | Main Q-Link 5 |
| SIREN / LFO2 RATE | 13 / lfo2_rate | s.lfo2 phase | 0.03…8 Hz | 0.17 Hz | 2 decimals | touch only |
| SIREN / RATE MOD | 14 / lfo2_amount | mainRate nested modulation | 0…100 % | 0 % | 0 decimals | Main Q-Link 7 |
| SIREN / LFO3 RATE | 15 / lfo3_rate | s.lfo3 phase | 0.03…8 Hz | 0.11 Hz | 2 decimals | touch only |
| SIREN / DEPTH MOD | 16 / lfo3_amount | mainDepth nested modulation | 0…100 % | 0 % | 0 decimals | Main Q-Link 3 |
| SIREN / SWEEP | 19 / zap_sweep | zapEnd / zapMultiplier | -48…48 st | -24 st | 0 decimals | Main Q-Link 1 |
| SIREN / TIME | 20 / zap_time | zapLength / zapDecay | 0.04…1.2 s | 0.18 s | 3 decimals | touch only |
| SIREN / REPEAT | 21 / repeat | repeatClock / startZap | 0…16 Hz | 0 Hz | 2 decimals | touch only |
| SIREN / CUTOFF | 23 / cutoff | tone coefficients / FILTER FX cutoff | 200…9000 Hz | 4000 Hz | 0 decimals | Main Q-Link 14 |
| SIREN / RESO | 24 / resonance | tone coefficients / FILTER FX Q | 0…20 dB | 2 dB | 1 decimal | Main Q-Link 10 |
| SIREN / CHOP | 17 / chop_rate | s.chop phase | 0.25…32 Hz | 4 Hz | 2 decimals | touch only |
| SIREN / DEPTH | 18 / chop_amount | chopGain / chopSmoothed | 0…100 % | 0 % | 0 decimals | touch only |
| SIREN / DELAY | 25 / delay_time | EchoConfig.time → Effects.time | 0.05…3 s | 0.416667 s | 3 decimals | Main Q-Link 6 |
| SIREN / FEEDBACK | 26 / feedback | EchoConfig.feedback → Effects.feedback | 0…88 % | 42 % | 0 decimals | Main Q-Link 2 |
| SIREN / LO CUT | 28 / delay_hp | EchoConfig.hp → feedback hpFilter | 40…1200 Hz | 120 Hz | 0 decimals | touch only |
| SIREN / HI CUT | 29 / delay_lp | EchoConfig.lp → feedback lpFilter | 800…12000 Hz | 7600 Hz | 0 decimals | touch only |
| SIREN / ECHO MIX | 27 / delay_mix | EchoConfig.mix → wet / spread gains | 0…100 % | 100 % | 0 decimals | Main Q-Link 16 |
| SIREN / PING PONG | 30 / ping | EchoConfig.ping → stereo pan depth | 0…100 % | 0 % | 0 decimals | Main Q-Link 12 |
| SIREN / REVERB | 32 / reverb | EchoConfig.reverb → reverbGain | 0…100 % | 16 % | 0 decimals | Main Q-Link 8 |
| SIREN / OUTPUT | 33 / output | c[P_output] → final output gain | 0…100 % | 100 % | 0 decimals | Main Q-Link 4 |
| SIREN / SQUARE | 4 / wave | c[P_wave] → waveBlend / wave() | SQUARE / SAW / TRIANGLE / SINE / NOISE | SQUARE | option text | touch only |
| SIREN / SAW | 4 / wave | c[P_wave] → waveBlend / wave() | SQUARE / SAW / TRIANGLE / SINE / NOISE | SQUARE | option text | touch only |
| SIREN / TRI | 4 / wave | c[P_wave] → waveBlend / wave() | SQUARE / SAW / TRIANGLE / SINE / NOISE | SQUARE | option text | touch only |
| SIREN / SINE | 4 / wave | c[P_wave] → waveBlend / wave() | SQUARE / SAW / TRIANGLE / SINE / NOISE | SQUARE | option text | touch only |
| SIREN / NOISE | 4 / wave | c[P_wave] → waveBlend / wave() | SQUARE / SAW / TRIANGLE / SINE / NOISE | SQUARE | option text | touch only |
| SIREN / SIREN | 3 / mode | c[P_mode] → siren/ZAP gate and frequency | SIREN / ZAP | SIREN | option text | touch only |
| SIREN / ZAP | 3 / mode | c[P_mode] → siren/ZAP gate and frequency | SIREN / ZAP | SIREN | option text | touch only |
| SIREN / TRI | 8 / lfo_wave | lfoWave(s.lfo, c[P_lfo_wave]) | TRIANGLE / SQUARE / SAW / SINE | TRIANGLE | option text | touch only |
| SIREN / SQR | 8 / lfo_wave | lfoWave(s.lfo, c[P_lfo_wave]) | TRIANGLE / SQUARE / SAW / SINE | TRIANGLE | option text | touch only |
| SIREN / SAW | 8 / lfo_wave | lfoWave(s.lfo, c[P_lfo_wave]) | TRIANGLE / SQUARE / SAW / SINE | TRIANGLE | option text | touch only |
| SIREN / SINE | 8 / lfo_wave | lfoWave(s.lfo, c[P_lfo_wave]) | TRIANGLE / SQUARE / SAW / SINE | TRIANGLE | option text | touch only |
| SIREN / LP | 22 / filter_type | tone.tone(type,cutoff,resonance) | LP / BP / HP | LP | option text | touch only |
| SIREN / BP | 22 / filter_type | tone.tone(type,cutoff,resonance) | LP / BP / HP | LP | option text | touch only |
| SIREN / HP | 22 / filter_type | tone.tone(type,cutoff,resonance) | LP / BP / HP | LP | option text | touch only |
| SIREN / CLEAN | 31 / character | Impl::character defaults / EchoConfig.character → Characters | CLEAN / DESK / SMOKE / ORBIT / CLASH | CLEAN | option text | touch only |
| SIREN / DESK | 31 / character | Impl::character defaults / EchoConfig.character → Characters | CLEAN / DESK / SMOKE / ORBIT / CLASH | CLEAN | option text | touch only |
| SIREN / SMOKE | 31 / character | Impl::character defaults / EchoConfig.character → Characters | CLEAN / DESK / SMOKE / ORBIT / CLASH | CLEAN | option text | touch only |
| SIREN / ORBIT | 31 / character | Impl::character defaults / EchoConfig.character → Characters | CLEAN / DESK / SMOKE / ORBIT / CLASH | CLEAN | option text | touch only |
| SIREN / CLASH | 31 / character | Impl::character defaults / EchoConfig.character → Characters | CLEAN / DESK / SMOKE / ORBIT / CLASH | CLEAN | option text | touch only |
| SIREN / FIRE | 0 / fire | fireTriggers → pulse / startZap / gate | 0…1  | 0  | 1 decimal | touch only |
| SIREN / LATCH | 1 / latch | c[P_latch] → gate | OFF / ON | OFF | option text | touch only |
| SIREN / 8 BIT | 34 / crush | bitMix → 255-step quantizer | OFF / ON | OFF | option text | touch only |
| SIREN / FILTER FX | 43 / filter_hold | postL/postR FILTER FX coefficients | OFF / ON | OFF | option text | touch only |
| SIREN / FREEZE | 36 / freeze | EchoConfig.freeze → feedback/send transition | OFF / ON | OFF | option text | touch only |
| SIREN / BEND | 37 / bend | pitch half / EchoConfig.bend → delay multiplier | OFF / ON | OFF | option text | touch only |
| SIREN / INVERT | 35 / invert | EchoConfig.invert → wet polarity | OFF / ON | OFF | option text | touch only |
| SIREN / KILL | 42 / kill | killGain → output mute slew | OFF / ON | OFF | option text | touch only |
| SIREN / STOP | 44 / stop | stopTriggers → reset / clear holds and notes | 0…1  | 0  | 1 decimal | touch only |
| SIREN / PATCH | 51 / preset__open | UI popup, transient | Closed / Open | Closed | preset name | none |
| SIREN / AIR RAID (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / LASER (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / FOG HORN (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / POLICE (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / UFO (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / SPACE ECHO (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / SMOKE (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / CLASH (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / HEAVY DUB (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / DEEP ORBIT (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / FEEDBACK MADNESS (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| SIREN / SCI-FI ALARM (PATCH overlay) | 2 / preset | Impl::preset full parameter snapshot | Air Raid / Laser / Fog Horn / Police / UFO / Space Echo / Smoke / Clash / Heavy Dub / Deep Orbit / Feedback Madness / Sci-Fi Alarm | Air Raid | option text | touch only |
| UTILITY / OSC LOW | 45 / osc_low | oscEq[0] coefficients/process | -24…12 dB | 0 dB | 0 decimals | Utility Q-Link 13 |
| UTILITY / OSC MID | 46 / osc_mid | oscEq[1] coefficients/process | -24…12 dB | 0 dB | 0 decimals | Utility Q-Link 9 |
| UTILITY / OSC HIGH | 47 / osc_high | oscEq[2] coefficients/process | -24…12 dB | 0 dB | 0 decimals | Utility Q-Link 5 |
| UTILITY / MASTER LOW | 48 / master_low | masterEqL/R[0] coefficients/process | -24…12 dB | 0 dB | 0 decimals | Utility Q-Link 1 |
| UTILITY / MASTER MID | 49 / master_mid | masterEqL/R[1] coefficients/process | -24…12 dB | 0 dB | 0 decimals | Utility Q-Link 14 |
| UTILITY / MASTER HIGH | 50 / master_high | masterEqL/R[2] coefficients/process | -24…12 dB | 0 dB | 0 decimals | Utility Q-Link 10 |
| UTILITY / FAST | 38 / fast | speed ×4 / ZAP repeat | OFF / ON | OFF | option text | Utility Q-Link 6 |
| UTILITY / SLOW | 39 / slow | speed ×.25 / ZAP repeat | OFF / ON | OFF | option text | Utility Q-Link 2 |
| UTILITY / OCT UP | 40 / oct_up | pitch ×2 | OFF / ON | OFF | option text | Utility Q-Link 15 |
| UTILITY / OCT DOWN | 41 / oct_down | pitch ×.5 | OFF / ON | OFF | option text | Utility Q-Link 11 |

## Touch practicality

Final main and utility previews were inspected again. FIRE is 140×64, LATCH 60×64,
with 8 px horizontal clearance. Primary fader **fields** are large, but those field
bounds are not all draggable: the actual Knob child bounds are separately recorded
in `resources/rc-touch-targets.json`. Secondary attack/release/level/noise tracks
are 92×20; nested/echo auxiliary tracks are 117/124×29–33. Wave/shape buttons are
mostly 57–66×38–40 with 6 px gaps; several selectors and small auxiliary tracks need
real finger validation. No overlapping siblings, reversed faders or missing
bindings were found. No further redesign was performed: physical feel, native
hit propagation and accidental activation cannot be established from a preview.

Primary pitch drag is 200×58, rate/depth 117×94, delay 124×102, feedback 128×102.
Tone cutoff/resonance 75/79×106. Primary fonts 16 px, large values 24 px (narrow tone
values 17 px); secondary fonts 13 px / values 17 px and units 11 px. Clipped labels were not
observed in the rendered defaults. Large/high digit counts at extremes remain
part of the physical card. Drag travel is approximate drawn track length minus
19 px horizontally or21 px vertically; precise value resolution/gesture sensitivity
is native host behavior, not inferred from artwork. Double-click/Enter requests
the verified Generic Knob Overlay for numeric editing.

Control reachability is established by source reads and wrapper/parameter tests.
Some controls require the relevant context: Zap/Repeat in ZAP mode, nested amounts
above zero, Chop amount above zero, feedback HP/LP heard on subsequent repeats,
EQ on Utility, and performance toggles active. Invert flips wet polarity rather
than swapping channels. KILL mutes output; STOP clears holds, MIDI and effect state.
PATCH name remains the last selected factory preset after custom edits; full
custom values, not merely the preset name, are serialized.
