# Parameters

Indices are persistent VST project-state identifiers. Never reorder existing keys.

| Index | Key | Range / options | Default | Units |
|---:|---|---|---|---|
| 0 | `fire` | 0 .. 1 | 0 |  |
| 1 | `latch` | OFF, ON | 0 |  |
| 2 | `preset` | Air Raid, Laser, Fog Horn, Police, UFO, Space Echo, Smoke, Clash, Heavy Dub, Deep Orbit, Feedback Madness, Sci-Fi Alarm | 0 |  |
| 3 | `mode` | SIREN, ZAP | 0 |  |
| 4 | `wave` | SINE, TRIANGLE, SAW, SQUARE, NOISE | 3 |  |
| 5 | `pitch` | 10 .. 2400 | 620 | Hz |
| 6 | `level` | 0 .. 100 | 70 | % |
| 7 | `noise` | 0 .. 100 | 0 | % |
| 8 | `lfo_wave` | TRIANGLE, SQUARE, SAW, SINE | 0 |  |
| 9 | `rate` | 0.05 .. 24 | 0.55 | Hz |
| 10 | `depth` | 0 .. 1400 | 480 | Hz |
| 11 | `attack` | 0.005 .. 1 | 0.01 | s |
| 12 | `release` | 0.02 .. 3 | 0.05 | s |
| 13 | `lfo2_rate` | 0.03 .. 8 | 0.17 | Hz |
| 14 | `lfo2_amount` | 0 .. 100 | 0 | % |
| 15 | `lfo3_rate` | 0.03 .. 8 | 0.11 | Hz |
| 16 | `lfo3_amount` | 0 .. 100 | 0 | % |
| 17 | `chop_rate` | 0.25 .. 32 | 4 | Hz |
| 18 | `chop_amount` | 0 .. 100 | 0 | % |
| 19 | `zap_sweep` | -48 .. 48 | -24 | st |
| 20 | `zap_time` | 0.04 .. 1.2 | 0.18 | s |
| 21 | `repeat` | 0 .. 16 | 0 | Hz |
| 22 | `filter_type` | LP, BP, HP | 0 |  |
| 23 | `cutoff` | 200 .. 9000 | 4000 | Hz |
| 24 | `resonance` | 0 .. 20 | 2 | dB |
| 25 | `delay_time` | 0.05 .. 3 | 0.4166666666666667 | s |
| 26 | `feedback` | 0 .. 88 | 42 | % |
| 27 | `delay_mix` | 0 .. 100 | 100 | % |
| 28 | `delay_hp` | 40 .. 1200 | 120 | Hz |
| 29 | `delay_lp` | 800 .. 12000 | 7600 | Hz |
| 30 | `ping` | 0 .. 100 | 0 | % |
| 31 | `character` | CLEAN, DESK, SMOKE, ORBIT, CLASH | 0 |  |
| 32 | `reverb` | 0 .. 100 | 16 | % |
| 33 | `output` | 0 .. 100 | 100 | % |
| 34 | `crush` | OFF, ON | 0 |  |
| 35 | `invert` | OFF, ON | 0 |  |
| 36 | `freeze` | OFF, ON | 0 |  |
| 37 | `bend` | OFF, ON | 0 |  |
| 38 | `fast` | OFF, ON | 0 |  |
| 39 | `slow` | OFF, ON | 0 |  |
| 40 | `oct_up` | OFF, ON | 0 |  |
| 41 | `oct_down` | OFF, ON | 0 |  |
| 42 | `kill` | OFF, ON | 0 |  |
| 43 | `filter_hold` | OFF, ON | 0 |  |
| 44 | `stop` | 0 .. 1 | 0 |  |
| 45 | `osc_low` | -24 .. 12 | 0 | dB |
| 46 | `osc_mid` | -24 .. 12 | 0 | dB |
| 47 | `osc_high` | -24 .. 12 | 0 | dB |
| 48 | `master_low` | -24 .. 12 | 0 | dB |
| 49 | `master_mid` | -24 .. 12 | 0 | dB |
| 50 | `master_high` | -24 .. 12 | 0 | dB |

Pitch uses logarithmic 10–2400 Hz mapping; other continuous controls are linear. Wave names follow SINE, TRIANGLE, SAW, SQUARE, NOISE externally while saved engine waveform IDs stay unchanged. FIRE is a 250 ms tap trigger (or complete one-shot ZAP); MIDI notes provide press/release and LATCH sustains. Modifier buttons are toggles because native touch release is unconfirmed. STOP clears performance holds, gates and FX tails. Presets are complete native snapshots; character selection recalls its source effect settings.
