# Parameters

Project state is saved by key (`DFS2 key=value ...`); 1.0.x `DFS1` positional chunks are migrated on load.
Indices 0–15 are the Q-Links in knob order. VST automation recorded with 1.0.x indices does not carry over.

| Index | Key | Range / options | Default | Units | Q-Link |
|---:|---|---|---|---|---|
| 0 | `pitch` | 10 .. 2400 | 620 | Hz | bank 1, knob 1 |
| 1 | `wave` | SINE, TRIANGLE, SAW, SQUARE, NOISE | 3 |  | bank 1, knob 2 |
| 2 | `rate` | 0.05 .. 24 | 0.55 | Hz | bank 1, knob 3 |
| 3 | `depth` | 0 .. 100 | 77.4 | % | bank 1, knob 4 |
| 4 | `lfo_wave` | TRIANGLE, SQUARE, SAW, SINE | 0 |  | bank 1, knob 5 |
| 5 | `zap_sweep` | -48 .. 48 | -24 | st | bank 1, knob 6 |
| 6 | `attack` | 0.005 .. 1 | 0.01 | s | bank 1, knob 7 |
| 7 | `release` | 0.02 .. 3 | 0.05 | s | bank 1, knob 8 |
| 8 | `cutoff` | 200 .. 9000 | 4000 | Hz | bank 2, knob 1 |
| 9 | `resonance` | 0 .. 20 | 2 | dB | bank 2, knob 2 |
| 10 | `delay_time` | 0.05 .. 3 | 0.4166666666666667 | s | bank 2, knob 3 |
| 11 | `feedback` | 0 .. 88 | 42 | % | bank 2, knob 4 |
| 12 | `delay_mix` | 0 .. 100 | 100 | % | bank 2, knob 5 |
| 13 | `ping` | 0 .. 100 | 0 | % | bank 2, knob 6 |
| 14 | `preset` | Air Raid, Laser, Fog Horn, Police, UFO, Space Echo, Smoke, Clash, Heavy Dub, Deep Orbit, Feedback Madness, Sci-Fi Alarm | 0 |  | bank 2, knob 7 |
| 15 | `output` | 0 .. 100 | 100 | % | bank 2, knob 8 |
| 16 | `fire` | 0 .. 1 | 0 |  | touch |
| 17 | `latch` | OFF, ON | 0 |  | touch |
| 18 | `mode` | SIREN, ZAP | 0 |  | touch |
| 19 | `stop` | 0 .. 1 | 0 |  | touch |

Pitch uses logarithmic 10–2400 Hz mapping; other continuous controls are linear. FIRE is a 250 ms tap trigger (or one complete ZAP); MIDI notes provide press/release and LATCH sustains. LFO DEPTH is a percentage of the current pitch: 100% sweeps between 0 Hz and twice the pitch. STOP clears LATCH, gates and echo tails. Presets are complete snapshots of every sound parameter. ZAP time is fixed at 180 ms.
