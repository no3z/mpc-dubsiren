# Existing 12-preset release audit

All 12 full51-value snapshots are finite, in descriptor ranges, distinct by
parameter vector and audible in fresh instances. Generated source snapshots
were compared against the checked-in file. Distinct character/pitch/wave/ZAP,
rate and nested modulation combinations support the intended varied sounds;
subjective usefulness and sound distinctness still need the physical card.
No new presets or factory-value edits were made. Selecting a preset clears
performance holds and recalls all factory values. Laser is a single ZAP burst;
Clash repeats while gated. Zero output after Laser finishes is intentional.

Peak/RMS are normalized per channel, from 8 seconds per preset with LATCH ON,
including attack. These are actual core output measurements, not loudness ratings.
All supplied presets stay below .436 peak in this test; hard bound is .98.

| Preset | Mode / wave | PitchHz | RateHz / DepthHz | CutoffHz / QdB | Echo | Delays / FB% | Reverb% | Level / Out% | Peak / RMS |
| --- | --- | ---: | --- | --- | --- | --- | ---: | --- | --- |
| 1. Air Raid | SIREN / SQUARE | 620 | 0.55 / 480 | 4000 / 2 | CLEAN | 0.417 / 42 | 16 | 70 / 100 | 0.4259 / 0.0542 |
| 2. Laser | ZAP / SAW | 2100 | 0.55 / 480 | 7000 / 4 | CLEAN | 0.417 / 42 | 16 | 70 / 100 | 0.3466 / 0.0131 |
| 3. Fog Horn | SIREN / SQUARE | 110 | 0.25 / 40 | 900 / 1 | SMOKE | 0.420 / 66 | 32 | 70 / 100 | 0.4241 / 0.0527 |
| 4. Police | SIREN / SQUARE | 700 | 2.00 / 350 | 4500 / 2 | CLEAN | 0.417 / 42 | 16 | 70 / 100 | 0.4351 / 0.0546 |
| 5. UFO | SIREN / SINE | 900 | 3.20 / 700 | 6000 / 3 | CLEAN | 0.417 / 42 | 16 | 70 / 100 | 0.3684 / 0.0518 |
| 6. Space Echo | SIREN / TRIANGLE | 420 | 0.70 / 280 | 4000 / 2 | ORBIT | 0.625 / 70 | 60 | 70 / 100 | 0.3153 / 0.0427 |
| 7. Smoke | SIREN / SQUARE | 340 | 6.50 / 180 | 3200 / 2 | SMOKE | 0.420 / 66 | 32 | 70 / 100 | 0.4312 / 0.0532 |
| 8. Clash | ZAP / SQUARE | 1400 | 0.55 / 480 | 4000 / 2 | CLASH | 0.417 / 80 | 12 | 70 / 100 | 0.4257 / 0.0550 |
| 9. Heavy Dub | SIREN / SQUARE | 180 | 0.35 / 100 | 1200 / 2 | DESK | 0.180 / 78 | 8 | 70 / 100 | 0.4252 / 0.0571 |
| 10. Deep Orbit | SIREN / SINE | 280 | 0.45 / 160 | 4000 / 2 | ORBIT | 0.625 / 70 | 44 | 70 / 100 | 0.3651 / 0.0435 |
| 11. Feedback Madness | SIREN / SQUARE | 620 | 0.55 / 480 | 4000 / 2 | CLASH | 0.090 / 88 | 12 | 45 / 70 | 0.3906 / 0.0365 |
| 12. Sci-Fi Alarm | SIREN / SAW | 1100 | 12.00 / 600 | 4000 / 2 | ORBIT | 0.625 / 70 | 44 | 70 / 100 | 0.3393 / 0.0413 |

Feedback Madness uses the maximum practical 88% feedback with lower source/output
45/70%, not an unsafe unbounded feedback coefficient. Freeze is a separate
performance hold and is not stored in factory patches. Full custom user state
can save Freeze, Latch and the other holds. Host/device topology and monitoring
volume still affect perceived loudness; start the physical card quietly.
