# Force 2.0.0 — 10 minute test card

Use a new, otherwise empty 44.1 kHz project and quiet stereo monitoring. Record
Force firmware: __________  Date: __________.
For each numbered test tick **PASS**, **FAIL**, or **NOT SURE**. Approximate values
are fine; for the save test, write down the actual displayed values.

The screen mirrors the Q-Links: the top row is bank 1 (knobs 1–8: PITCH, WAVE,
RATE, DEPTH, SHAPE, SWEEP, ATTACK, RELEASE), the bottom row bank 2 (knobs 9–16:
CUTOFF, RESO, TIME, FEEDBACK, MIX, PING PONG, PATCH, OUTPUT).

**Dry setup:** select Air Raid, press STOP, choose SIREN, WAVE SINE, DEPTH 0,
CUTOFF 9000, RESO 0, MIX 0, OUTPUT 70.

| # / time | Do this | Expect this | Result |
| --- | --- | --- | --- |
| **1 / 30s — Load** | Find **Dub Force Siren** in the instrument browser; instantiate and open its editor. Open/close PATCH. | One page with two rows of eight controls; preset list opens and closes; no crash. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **2 / 30s — Trigger** | Dry setup. Tap FIRE; toggle LATCH on/off; then LATCH on and STOP. | FIRE gives a short sound; LATCH sustains; off releases; STOP silences and clears LATCH. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **3 / 60s — Q-Link banks** | Select bank 1 and turn knobs 1–8, then bank 2 and turn knobs 1–8. | Each knob moves the control in its column of the matching row; clockwise increases; WAVE/SHAPE/PATCH step through named options. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **4 / 60s — Low pitch** | Air Raid (DEPTH 77%), LATCH on. Turn PITCH slowly from 620 down to 10 Hz and back. | The whole siren sweep moves down with the knob over the full range; below ~40 Hz it becomes a rumble/click train rather than a tone. No dead zone. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **5 / 45s — Waves** | Dry setup, LATCH on. Select SINE, TRI, SAW, SQUARE, NOISE. | Timbres differ; switching is smooth while held. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **6 / 45s — LFO** | LATCH on; DEPTH 50; RATE .5→4; try TRI, SQUARE, SAW, SINE shapes; DEPTH 0→100. | Speed, contour and excursion change; SQUARE gives a two-tone; 100% dives to 0 Hz and up to twice the pitch. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **7 / 45s — ZAP / envelope** | STOP; choose ZAP; SWEEP −24, FIRE; SWEEP +12, FIRE. SIREN again: ATTACK .01→.8 and RELEASE .05→1 with LATCH off/on; return .01/.05. | First zap falls, second rises; slow attack fades in, long release fades out. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **8 / 45s — Filter** | Dry setup, SAW, LATCH on. CUTOFF 500→4000; RESO 0→10. | Brightness and resonance change; no uncontrolled level. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **9 / 60s — Echo** | STOP; MIX 100; TIME .1 / FEEDBACK 20, FIRE; TIME .6 / FEEDBACK 80, FIRE; PING PONG 0→100 with LATCH on; MIX 0. | Longer spacing and tails; later repeats get thinner/darker; ping pong alternates left/right; MIX 0 removes echo. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **10 / 60s — Presets** | Browse all twelve patches with the PATCH list and with knob 15; FIRE each. Select Feedback Madness, LATCH on 3 s, off, STOP. | Each recalls different settings; Laser/Clash zap once per FIRE; feedback rings without runaway; STOP ends the tail. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **11 / 2min — Save/reload** | Set SAW, PITCH ≈333, CUTOFF ≈1700, RESO ≈5, RATE ≈3, DEPTH ≈40, SHAPE SQUARE, TIME ≈.33, FEEDBACK ≈61, PING ≈50, OUTPUT ≈70. Save as **DubSiren2Test**, load a blank project, reopen it. Also open a project saved with 1.0.2. | All values return exactly; quiet until FIRE. The 1.0.2 project keeps its pitch, wave, LFO, filter and echo time/feedback; removed effects (reverb, characters, EQ…) are gone. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **12 / 45s — Rapid use / CPU** | LATCH on; rapidly move all knobs, tap FIRE repeatedly, then STOP and FIRE. Note Force CPU idle ___% / latched ___%. | No crash, stuck gate or dropouts; STOP clears sound and the next FIRE works. | ☐ PASS ☐ FAIL ☐ NOT SURE |

**Failure / NOT SURE report:** test number; Force firmware; preset or saved project;
displayed settings; Q-Link bank and knob; expected vs actual behavior;
reproducibility. Attach a screen photo or short clip when useful.

Failed test # ____  Settings/steps: __________________________________________

Expected / actual / repeatable?: ____________________________________________

Physical validation status: **PENDING** until this card is completed on the device.

Automation recorded with 1.0.x uses the old parameter indices and does not carry
over to 2.0.0; project state (the chunk) is migrated by key.
