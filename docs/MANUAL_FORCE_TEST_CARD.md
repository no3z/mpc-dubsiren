# Force 1.0.1 RC — 10–15 minute test card

Use a new, otherwise empty 44.1 kHz project and quiet stereo monitoring. Record
Force firmware: __________  Date: __________. Leave the installed 1.0.1 in place.
For each numbered test tick **PASS**, **FAIL**, or **NOT SURE**. Approximate slider
values are fine; for the save test, write down the actual displayed values.

**Dry setup:** select Air Raid, press STOP, choose SIREN / SINE, DEPTH 0,
NOISE 0, LP / CUTOFF 9000 / RESO 0, ECHO MIX 0, REVERB 0.
Keep LEVEL 70 and OUTPUT 70. Reuse this setup when directed below.

| # / time | Do this | Expect this | Result |
| --- | --- | --- | --- |
| **1 / 30s — Load** | Find **Dub Force Siren** in the instrument browser; instantiate and open its editor. Open/close PATCH. | Editor and preset list appear; no crash or stuck loading. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **2 / 30s — Trigger** | Apply dry setup. Tap FIRE; toggle LATCH on/off; then LATCH on and STOP. | FIRE gives a short sound; LATCH sustains; off releases; STOP silences and clears performance switches. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **3 / 45s — Pitch/waves/level** | LATCH on. Drag PITCH 200→1000; select SQUARE, SAW, TRI, SINE, NOISE. Restore SINE; drag NOISE 0→70→0, LEVEL 70→0→70, OUTPUT 70→0→70. | Pitch rises; wave timbres differ; NOISE adds hiss; either gain at zero silences. Touch moves only the intended control. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **4 / 45s — Envelope/touch** | Echo/reverb remain off. Change ATTACK .01→.8 and retrigger with LATCH off/on; RELEASE .05→1 and switch LATCH off. Return .01/.05. Test slider endpoints and read their values. | Slow attack fades in; long release fades out. Small tracks respond under a finger; no neighbor moves or unreadable/clipped value. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **5 / 45s — LFOs** | LATCH on; DEPTH 480; RATE .5→4. Try all four LFO shapes. Set both nested rates around .3 and both RATE MOD / DEPTH MOD to 100, then return amounts to 0. | Speed, excursion and contour change; nested controls vary the movement over time. All six modulation sliders and four shape buttons respond correctly. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **6 / 45s — ZAP** | STOP; choose ZAP, REPEAT 0, SWEEP −24, TIME .18; FIRE. Change SWEEP +12 and TIME .6; FIRE. Set REPEAT 6 and LATCH on; then STOP. | First burst falls, second rises and lasts longer; repeat produces a train of bursts. Each control changes its own value. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **7 / 60s — Primary Q-Links** | Use the Force Q-Link bank/column selector and LCD names to reach **Pitch, Rate, Depth, Zap Sweep, Cutoff, Resonance, Delay Time, Feedback**. Turn each both ways; use ZAP to hear Sweep and SIREN for Rate/Depth. | Eight distinct mappings; clockwise increases the named value, anticlockwise decreases it; editor stays on the main panel. Small turns allow useful adjustment without large jumps. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **8 / 45s — Tone/chop/crush** | Dry setup, SAW, LATCH on. Try LP/BP/HP; CUTOFF 500→4000; RESO 0→10. Toggle FILTER FX. Set CHOP 4 and its DEPTH 100; vary CHOP to 8; toggle 8 BIT. Restore chop depth 0, 8 BIT/FILTER FX off. | Filter types/cutoff/resonance change tone; FILTER FX changes processing; chop makes rhythmic gaps and speeds up; 8 BIT adds grit. No uncontrolled level. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **9 / 45s — Echo/filters** | STOP; select CLEAN first, then SIREN, DELAY .1, FEEDBACK 20, ECHO MIX 100, REVERB 0. FIRE. Repeat at DELAY .6 / FEEDBACK 80. Try LO CUT 40→1000 and HI CUT 12000→800. | Longer spacing and longer tails; LO CUT thins later repeats, HI CUT dulls them. Listen beyond the first repeat. MIX 0 removes echo; MIX 100 brings it back. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **10 / 30s — Stereo/modifiers** | With LATCH on and echo audible, drag PING PONG 0→100; toggle INVERT, then BEND on/off. | Higher Ping moves wet sound across stereo. Invert changes wet polarity; a subtle difference is acceptable. Bend lowers live pitch and lengthens delay. Buttons follow taps. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **11 / 45s — Characters** | STOP, select CLEAN / DESK / SMOKE / ORBIT / CLASH in turn, and FIRE after each. | Five selections respond and produce different echo behavior. Selecting a character also recalls its delay/feedback/filter/ping/reverb defaults; this is expected. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **12 / 45s — Feedback/freeze/kill** | Select Feedback Madness; LATCH on for 3s, off; toggle FREEZE on for 5s, then off. Toggle KILL on/off during a new sound. Press STOP. | Bounded ringing/held tail without runaway; Freeze need not sustain forever. Kill mutes; off restores an active sound. STOP ends the tail and clears holds. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **13 / 45s — Reverb/Utility** | Dry setup; REVERB 0→80, FIRE and listen to tail. Open Utility; on a latched SAW, sweep each of six EQ sliders −12→+6→0. Toggle FAST, SLOW, OCT UP, OCT DOWN separately; return off. | Reverb adds a tail; each EQ changes tone; Fast/Slow changes modulation speed; octave switches raise/lower pitch. Return to main without losing values. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **14 / 60s — All 12 presets** | Browse Air Raid, Laser, Fog Horn, Police, UFO, Space Echo, Smoke, Clash, Heavy Dub, Deep Orbit, Feedback Madness, Sci-Fi Alarm. FIRE each; briefly LATCH Clash. STOP between patches. | Each recalls different settings and produces sound; no invalid values or excessive jumps. Laser ends after one burst; Clash repeats while gated. Popup selects the touched row. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **15 / 2min — Save/reload** | Select SMOKE character **first**. Set SIREN / SAW, PITCH ≈333, HP / CUTOFF ≈1700 / RESO ≈5; RATE ≈3 / DEPTH ≈250; LFO2 rate ≈.27 / amount ≈73; LFO3 rate ≈.19 / amount ≈61; DELAY ≈.33 / FB ≈61 / REVERB ≈37 / OUTPUT ≈70. Leave LATCH, FREEZE, BEND, KILL off. Record displayed values and other controls in photos/notes. Save project as **DubSirenRCTest**; load a blank project to unload it, then reopen the saved project. | Wave, character, filter mode, nested LFOs, delay/reverb and **all recorded values** return exactly. Quiet until FIRE; old audio tails are discarded. PATCH may still show the last factory name after edits. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **16 / 30s — Rapid use** | LATCH on; rapidly move Pitch, Rate, Depth, Cutoff, Delay and Feedback, switch waves/filters, tap FIRE repeatedly. Press STOP; FIRE again. | No crash, hang, persistent audio corruption or stuck gate; STOP clears sound and the next FIRE works. | ☐ PASS ☐ FAIL ☐ NOT SURE |
| **17 / 45s — CPU** | In the empty project select **Feedback Madness**, observe Force CPU at rest, then LATCH on for 30s while moving Delay/Feedback. Record idle ___% / peak ___%; STOP. | No sustained overload, audible dropouts or crash. Host CPU includes other work; the earlier 11% benchmark is not a required Force meter reading. | ☐ PASS ☐ FAIL ☐ NOT SURE |

**Failure / NOT SURE report:** test number; Force firmware; preset or saved project;
actual displayed settings; exact gesture/Q-Link LCD name and selected bank;
expected vs actual behavior; reproducibility; idle/peak CPU if relevant. Attach
a screen photo or short audio/video clip when useful. For touch failures name
both the intended control and the neighboring control that moved. For reload
failures list each value before and after, and keep the saved test project.

Failed test # ____  Settings/steps: __________________________________________

Expected / actual / repeatable?: ____________________________________________

Physical validation status: **PENDING** until this card is completed on the device.
