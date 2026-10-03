# Dub Force panel redesign

The main instrument stays on one 1280 x 628 native page. Its left-to-right zones
are SIREN, MODULATION, TONE, ECHO, MASTER. The charcoal chassis, warm cream text,
amber selection lamps and orange FIRE key are original procedural artwork;
no BARZINE or Akai logos, images or stock skin files are distributed.

## Three inspected iterations

- **V1 functional organization:** [preview](../resources/ui-v1.png). Grouped the
  complete signal flow into five columns and made waveform/filter/character
  choices directly visible. Inspection found the first stacked ZAP time/repeat
  sliders colliding with their values; replaced that arrangement with three
  separate SWEEP/TIME/REPEAT fields. Shortened RESONANCE to RESO and the chop
  labels to keep them readable without clipping.
- **V2 hierarchy:** [preview](../resources/ui-v2.png). Enlarged FIRE to a 26-pixel
  caption, primary value readouts to 24 pixels and the pitch fader thumb to a
  32 x 24 cap. LATCH remains beside FIRE. Primary modulation, delay and feedback
  occupy taller faders than secondary controls. Narrow tone fields use smaller
  bounded value fonts rather than clipped large digits.
- **V3 polish:** [final main preview](../resources/ui-v3.png),
  [utility preview](../resources/utility-v3.png),
  [open preset chooser](../resources/ui-v3-presets.png). Added restrained chassis
  gradient, button bevels, consistent warm colors and section fasteners. Shared
  identical fader assets across fields with the same geometry. High feedback's
  upper fifth is marked orange; this is a visual hot-zone indication, not a
  change to its 0..88 percent range.

All three main previews and the final utility preview were opened and visually
inspected, along with the open twelve-preset chooser. The preview uses the exact
original PNGs and native component bounds,
with values generated from factory defaults. These numbers are **offline preview
values**: actual native Label components bind to the DSP and update dynamically.
Factory-default previews do not simulate live parameter changes, sound, or touch.

## Main controls and Q-Links

SIREN: FIRE, adjacent LATCH, all five waves, SIREN/ZAP mode, Pitch, Attack, Release,
Level, Noise. MODULATION: Rate, Depth, four LFO shapes, both nested LFO rates and
amounts, Zap sweep/time/repeat. TONE: Cutoff, Resonance, LP/BP/HP, Chop rate/depth,
8 Bit and Filter FX. ECHO: Delay, Feedback, five actual character modes, feedback
HP/LP, mix, ping-pong, Freeze, Bend and Invert. MASTER: Reverb, Output, Kill and
Stop. PATCH opens the existing twelve-preset chooser at the top. No new DSP modes
were added; the actual source has five echo characters.

The main page has 55 independently bound touch regions, including individual
selector buttons. FIRE is 140 x 64, LATCH 60 x 64. Primary pitch field is 208 x 112;
LFO rate/depth are each 125 x 148; delay and feedback are 132/136 x 156. Smaller
secondary fields remain compact; their real feel still needs a physical finger
test. Bounds are measured in native canvas pixels, not millimeters.

There is exactly one SIREN tab entry and one UTILITY tab entry, each with subindex
zero. Main bank 1 is Pitch, Rate, Depth, Zap, Cutoff, Resonance, Delay, Feedback.
Main bank 2 is Attack, Release, LFO2 amount, LFO3 amount, Echo mix, Ping, Reverb,
Output. Both banks show the same complete main panel; no main subpage navigation.
Track/program Q-Links use that same 16-parameter mapping. UTILITY retains six EQ
bands plus Fast, Slow and octave up/down.
Double-click/Enter numeric overlays from the previous UI are retained through
the device's Generic Knob Overlay, whose existence was checked read-only.

Public parameter IDs 0..50 and all six shipped hidden popup IDs 51..56 remain
unchanged. The unused selector-open flags are retained to preserve compatibility;
the preset chooser uses its existing hidden flag and the existing auto-close
wrapper behavior. `layout.conf` records grouping and stable popup/Q-Link order;
`tools/skin_design.py` owns the final native widget geometry and original art.

## Native frame-format verification

Read-only inspection of the actual Force stock Bassline skin established:

| Stock component | Native bounds | Stock PNG dimensions | numFrames |
| --- | --- | --- | --- |
| slider FilmStrip Knob | 114 x 8 | 114 x 1016 | 127 |
| blue knob FilmStrip Knob | 76 x 86 | 76 x 10922 | 127 |

Both assets contain exactly `numFrames` rectangular frames at the declared child
width/height. This corrects the vendored generator's square-padding and
`128 artwork frames / numFrames 127` assumptions. Our final strips use rectangular
frames matching their native bounds, and `numFrames` is the frame **count**.
No unverified square-to-rectangle scaling is needed. Only structural facts were
recorded; stock artwork stayed in transient `/tmp` inspection files.

The independent generator uses 48 visual frames for primary faders and 32 for
secondary faders, without quantizing actual DSP parameter values. These are
cosmetic frame counts; the native control still supplies normalized continuous
values. Identical strips share filenames. Static graphics do not run on the audio
thread. Live fonts use MPC's Titillium Web; static captions are rasterized with
DejaVu Sans. No font binaries are included in the plugin payload.
The final 106 original PNGs total 285,918 compressed bytes; their combined decoded
RGBA size is about 34.5 MiB. Shared geometric filmstrips avoid duplicating identical
artwork for different parameters.

## Generation and validation

Run `./tools/skin.sh`. It writes native skin resources under
`build/skin/Dub Force - VST - Dub Force Siren/`, refreshes `build/params.h` and the
plugin-list entry, and creates all three iteration previews under `resources/`.
`version.xml` derives its version from `vst.json` (1001 becomes 1.0.1.0).

[Machine-readable validation](../resources/ui-validation.json) reports 106
referenced original assets checked per iteration, 55 main touch regions and
10 utility regions. Validation checks:

- Every referenced PNG exists and image height stays at or below 16384.
- Every filmstrip has the exact native frame width and height times frame count.
- Parameter remappings stay inside the unchanged 57-parameter table.
- All 51 public parameters have a bound control on main or utility.
- All ordinary hit rectangles stay inside the 1280 x 628 canvas and do not overlap.
- Each fader's drag component and live value rectangle do not overlap.
- Preset overlay uses deliberate conditional layering above the main panel.

The attempted `/dev/fb0` device screenshot was entirely black, so an actual
rendered Force screenshot is unavailable through that capture method. Native
resource/schema verification and offline visual inspection are separate from
on-device rendering. Required physical checks: insert the plugin, verify both
pages, drag every primary fader, select every wave/character, open/select/close
PATCH, test FIRE/LATCH/STOP, and turn both Q-Link banks while the main screen
remains unchanged.
