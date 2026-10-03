# BARZINE Dub Siren UI reference

This is a source-grounded reference for the browser instrument at [barzine.news/siren](https://barzine.news/siren/) and its alternate hardware-style surface at [barzine.news/siren-deck](https://barzine.news/siren-deck/). Inspected 2026-10-03. It records visible behavior and proportions for an original MPC-facing recreation; it does not include BARZINE artwork or copied page assets.

## Surface and hierarchy

The classic page is a dark, flat gunmetal instrument with square-edged panels and controls. Its visual hierarchy is: preset selection; SIREN/ZAP mode; oscillator/source; LFO or mode-specific ZAP controls; filter and scope/master; echo chamber; BPM looper and backing riddim. Expandable EQ and modulation panels hide secondary controls behind `EQ`, `MOD`, and `TONE` buttons. On narrow screens these panels stack and scroll above a persistent performance dock. On wide screens settings use three compact columns and the dock remains visible along the bottom.

The bottom dock has a narrow stop/play rail, a wide touch pad, and a bank of performance buttons. The pad is the primary performance surface: hold to sound, drag horizontally for pitch and vertically for LFO rate. It is a broad rectangle rather than a small button; its height is `clamp(100px, 16vh, 144px)` on mobile and at least 132px on desktop. On desktop, the performance button bank occupies about 420px beside the pad and uses two rows. The page calls the main action a pad rather than a separate FIRE key.

The companion Deck UI is a compact hardware-inspired surface with a large round pitch/speed control, physical-looking knobs, a short row of high-priority controls, preset previous/next, SIREN/ZAP selectors, and a link to the full controls. Its full-controls panel presents the same settings as the classic page. The deck has a direct **8BIT DECK** switch that opens the alternate 8-bit surface; this is navigation to that separate instrument surface, not an in-place quantizer toggle. The classic controls page does not expose that switch.

## Visible controls and ranges

The table reflects the classic page's HTML slider bounds, steps, and labels. Values shown in the page are presentation defaults; DSP initialization can differ (see the note below).

| Group | Control | Range / options | Shown default |
| --- | --- | --- | --- |
| Presets | Preset | AIR RAID, PEW PEW, RAPID, LASER, WOBBLE, FOGHORN, STEAM, UFO | AIR RAID |
| Mode | Voice mode | SIREN (sustain + LFO), ZAP (sweep + repeat) | SIREN |
| Oscillator | Pitch | 60–2400 Hz, 1 Hz | 620 Hz |
|  | Siren level | 0–100%, 1% | 45% on classic page; 70% on deck surface |
|  | Random noise | 0–100%, 1% | 0 / OFF |
|  | Oscillator waveform | SQR, SAW, TRI, SIN, NOIZ | SQR |
| Oscillator EQ | Low / mid / high | −24 to +12 dB, 1 dB | 0 dB each |
| LFO | Rate | 0.05–24 Hz, 0.05 Hz | 0.55 Hz |
|  | Depth | 0–1400 Hz, 5 Hz | 480 Hz |
|  | Waveform | TRI, SQR, SAW, SIN | TRI |
| Nested motion | Attack / release | 5 ms–1 s / 20 ms–3 s | 10 ms / 50 ms |
|  | LFO 2 speed / amount to speed | 0.03–8 Hz / 0–100% | 0.17 Hz / OFF |
|  | LFO 3 speed / amount to depth | 0.03–8 Hz / 0–100% | 0.11 Hz / OFF |
|  | Chop speed / amount | 0.25–32 Hz / 0–100% | 4 Hz / OFF |
| ZAP | Sweep | −48 to +48 semitones | −24 st |
|  | Speed | 0.04–1.2 s | 0.18 s |
|  | Repeat | 0–16, 0.5 | 0 / OFF |
| Filter | Cutoff / resonance | 200–9000 Hz / 0–20 | 4 kHz / 2 |
| Output | Master | 0–100% | 100% |
| Echo | Style | DESK, SMOKE, ORBIT, CLASH, CLEAN | CLEAN |
|  | Delay / feedback / reverb | 0.05–3 s / 0–88% / 0–100% | 0.42 s / page says 60% / page says 25% |
|  | Ping-pong | 0–100% | OFF |
| Delay tone | Wet mix / HP / LP | 0–100% / 40–1200 Hz / 800–12000 Hz | 100% / 120 Hz / 7.6 kHz |
|  | Wet phase | INVERT toggle | Off |
| Looper | BPM / loop length | 45–180 BPM / 1, 2, 4, 8 beats | 72 BPM / 4 beats |
|  | Capture / playback | REC LOOP, PLAY, CLEAR | Empty |
| Backing riddim | Pattern / volume | OFF, SK, OD, ST, RS, DG / 0–100% | OFF / 60% |
|  | BGM EQ low / mid / high | −24 to +12 dB | 0 dB each |
| Master EQ | Low / mid / high | −24 to +12 dB | 0 dB each |

The performance dock actions are: **LATCH** (toggle), **OCT UP** and **OCT DOWN** (hold, ±1 octave), **FAST** and **SLOW** (hold, ×4 / ×¼), **BEND** (hold for pitch and echo dive), **FREEZE** (hold echo), **FILTER** (hold low filter effect), **KILL** (hold mute), and **REC** (export recording). There is also a stop-all button and a separate hold-to-play-at-center button.

## Fire, latch, and XY behavior

- Pressing the pad starts the currently selected sound. Releasing it triggers the release envelope; SIREN mode sustains while held, while ZAP mode fires a sweep and can repeat according to its repeat setting. Thus the most faithful `FIRE` control is momentary, with the selected mode determining whether it sustains or triggers a one-shot.
- LATCH is a separate on/off toggle. Turning it on starts playback if idle and keeps playback alive after pad release; turning it off releases an active sound. A stop-all action clears latch and releases performance holds.
- The pad's X range is a symmetric two-octave multiplier around its center (`0.5×` at the left edge through `2×` at the right). Its Y range scales the LFO rate from `4×` at the top through `¼×` at the bottom. Center therefore means the parameter values shown by the regular controls.
- OCT and BEND change pitch while held. FAST/SLOW scale modulation speed; in ZAP mode they also affect repeat cadence. Freeze, filter, and kill are hold gestures that restore their underlying setting on release, rather than persistent toggles.
- Keyboard Space/Enter acts as a momentary trigger, matching the pad's press/release semantics.

## Recommendations for an MPC reproduction

Keep the main sound-shaping controls together on the first page to honor the requested one-screen performance workflow: mode, oscillator waveform/pitch/level/noise, LFO 1 rate/depth/wave, LFO 2 and 3 speeds and modulation amounts, attack/release and chop, ZAP sweep/speed/repeat, filter cutoff/resonance, echo style/time/feedback/reverb/ping-pong, and delay HP/LP/mix/invert. Use compact labeled groups and show the active mode's controls prominently; keep the other mode's controls visible but visually secondary so switching modes does not move the rest of the page. This is a dense page, so prioritize short labels, readable value readouts, and touch targets that remain usable at the MPC's fixed 1280×628 size.

Reserve auxiliary pages or popovers for the three-band oscillator/master/BGM EQs and other supporting controls. The looper and backing-riddim section can occupy a separate performance page if space requires it, while keeping all core siren/ZAP and echo tone controls on the main page.

Use a large, clearly momentary **FIRE** trigger and a visibly separate **LATCH** toggle. Preserve the source relationship in which release affects FIRE only, while LATCH holds the voice after release. Keep octave, rate, bend, freeze, filter, and kill as momentary actions if the engine supports them; label hold actions explicitly. If XY drag cannot be reproduced on the target surface, expose pitch and rate as ordinary controls and keep their center/default behavior aligned with the browser pad.

Make preset recall explicit and user-facing. The browser offers eight named starting sounds, while MPC VST2 instruments do not have a separate per-plugin preset-save UI; a plugin-owned preset selector/browser is the appropriate way to switch sounds. Keep preset names short, and retain a master/output level control near the performance controls because the original warns that high feedback can build up.

For a visual homage, use the documented shape language—flat dark background, gunmetal section panels, square black buttons, compact uppercase labels, bright active states, and a restrained red/gold/green accent stripe—while drawing original artwork and using project-owned text and icons. The source site uses responsive browser controls and is a behavioral/layout reference, not an MPC skin asset source.

## Source notes

- Classic instrument: <https://barzine.news/siren/>. Its inline HTML/CSS/JS supplies control labels, slider bounds, responsive layout, pad mappings, and hold/latch behavior.
- Alternate hardware panel: <https://barzine.news/siren-deck/>. It exposes the compact deck, links to full controls, and has a direct **8BIT DECK** navigation control to the distinct 8-bit surface.
- DSP caveat: the browser page's initial labels and the runtime engine state are not identical for every value. For example, the source code starts the classic `vol` at 0.45 and deck `vol` at 0.70; the initialized delay is tempo-synced (about 0.4167 s at 72 BPM), with runtime feedback 0.42 and reverb 0.16 even though the static labels show 60% and 25%. Use the DSP implementation's state as the source for sound behavior and this document for visible UI bounds and presentation.
