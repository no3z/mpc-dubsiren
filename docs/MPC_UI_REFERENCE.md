# Native MPC/Force skin reference

Research date 2026-10-03. Read-only source: sd88me/mpc-vst-plugins at commit
`faae55acfbf3ccac88d15df8ab3c45f74002e91c`.
Facts below come from `tools/shadow_skin.py`, `poc/menuprobe_skin.py`,
`tools/gen_vst.py`, `tools/release.py`, and `docs/NOTES.md`.
No proprietary stock skin assets were copied. Examples describe small factual
fragments of the data format using original names.

Device-format correction established during the redesign: actual Bassline stock
slider PNG 114x1016 with bounds 114x8 and numFrames127 contains 127 rectangular
frames; a stock knob is 76x10922 with bounds 76x86 and numFrames127. Thus native
`numFrames` is the frame count and frames need not be square. The original
upstream square-padding/128-images-with127-metadata assumptions below are source
findings, not the final project's format. See `UI_REDESIGN.md` for the corrected
generator and read-only device evidence.

## Capabilities and primary-page recommendation

The usable plugin canvas is **1280 x 628**. Original Shadow coordinates map
by subtracting 86 from y; independently generated native skins can use zero-based
canvas coordinates directly. Absolute placement permits more than 16 visible
touch controls. Upstream records a 44-control Tone tab; 16 is the Q-Link mapping
limit per subpage, not a visible-control limit.

Keep one main `tabs` entry and one 16-parameter Q-Link map. All other controls
remain touchable. Adding Q-Link subpages creates native dot/arrow navigation even
when every page references identical screen content. On Force, the ordinary two
banks cover the existing 16 Q-Link positions without adding touchscreen subpages.

Proven types include filmstrip `Knob` (also used as a slider), `Button`, `Label`,
`Image`, and `Focus`. Native custom graph/XY/envelope rendering has not been
identified. Native Meter was tried upstream and broke the whole page; avoid it.
Display text can be dynamic; plugin does not paint custom widgets. Live fonts
verified upstream are Titillium Web and Roboto. Static original labels can be baked
into PNGs with another appropriately licensed font.

## Files and top-level JSON

Install layout:

```text
<vendor> - VST - <plugin>/
  <plugin>.so
  plugin-meta.xml
  version.xml
  Plugin Skins/
    TUI.json
    Q-Links.json
    Q-Links - 8by1.json
    original_background.png
    original control PNGs / filmstrips
```

`TUI.json` root has `pageData`:

```json
{
  "pageData": {
    "version": 1,
    "info": {"version": 1, "type": "CompleteDescription"},
    "componentDefinitions": {
      "version": 2,
      "importFiles": [],
      "localComponentDefinitions": []
    },
    "tabs": [{
      "version": 3, "tabName": "SIREN", "fnKeyIndex": 0,
      "fnKeySubIndex": 0, "componentName": "SirenPage",
      "qlinkBoundsData": ["0 0 0 0"],
      "initialSize": "0 0 1280 628", "scale": 1.0
    }]
  }
}
```

This illustrates the envelope, not a complete page: definitions must include
`SirenPage` and all placed custom types. `importFiles` may reference absolute
device resources such as `/usr/share/Akai/Content/Synths/Generic/Generic Knob Overlay.json`
and `Generic Menu Overlay.json`. Do not import menu overlay unless needed: VST2
native menus open empty. Absolute paths are used because imports otherwise resolve
relative to the skin folder. Original local controls need no copied stock assets.

Each local definition is `{"key":"CustomType","value":{...}}`. Its value contains:
`version:4`, `actions:[]`, `backgroundData` version 1 with `focussed` and
`unfocussed` entries (`version:1, colour:"0", image:""`),
`ignoreMousePresses:false`, `disableCoarseDataWheel:false`, `repeats:1`,
`hideQLinkBounds:true`, and `componentsData:[...]`.
The page itself is such a local definition, whose components are background and
placed controls. Components listed later are drawn above earlier ones; upstream
explicitly adds popup panels last so they cover underlying controls and take touches.

## Components, parameter binding and bounds

Placed custom component example:

```json
{
  "version": 2,
  "componentData": {
    "version": 1, "name": "Pitch", "type": "SirenFader",
    "data": {"version": 1, "handleName": "Data"}
  },
  "handle remapping": {
    "version": 1,
    "map": [{"key": "Data", "value": "Parameter 2"}]
  },
  "bounds": {
    "version": 2, "acceptsHWFocus": "Yes",
    "showWhenDataModelInvalid": "Hide", "whenVisible": "Always",
    "boundsType": "Absolute", "bounds": "30 95 130 170",
    "additionalInvalidatingHandles": []
  }
}
```

`Parameter N` is the **zero-based VST index**, independent of display name.
Space and capitalization in `handle remapping` are exact. Additional handles
can map `Text` or other names to another parameter; a child Label then uses
`handleName:"Text"`. A child native component uses the same envelope with empty
remapping and relative bounds inside the parent. Typical child bounds use
`acceptsHWFocus:"No"`, `showWhenDataModelInvalid:"Show"`.

`Focus` native data: `version:1`, `backgroundColour:"00000000"`,
`outlineColour:"00000000"`, `backgroundInset:2.0`, `outlineThickness:0.0`.
Its bounds can use `whenVisible:"WhenFocussed"`; transparent focus avoids large
highlight boxes on dense panels. A parent can suppress Q-Link outlines using
`hideQLinkBounds:true`. Avoid giant padded touch rectangles overlapping neighbors.

## Sliders/faders and knobs

Actual upstream sliders are **`Knob` filmstrips**, not an unverified new native
Slider schema. This provides real touchscreen horizontal/vertical drag control.
Native child data for a horizontal fader:

```json
{
  "version": 5, "knobType": "FilmStrip",
  "filmStrip": "pitch_fader.png", "numFrames": 64,
  "invert": false, "dragOrientation": "Horizontal", "handleName": "Data"
}
```

Use `type:"Knob"` in its componentData. Vertical faders and rotary knobs use
`dragOrientation:"Vertical"`. `numFrames` is the native frame count, confirmed
from actual Force stock assets during the redesign. A 64-frame strip sets 64.
Frames stack vertically and match the native child width/height. A 100x42 fader
with 64 frames is a 100x2688 strip. The upstream generator instead produces
128 images with numFrames127 and square padding; that earlier source convention
is not the corrected final generator's format.
Avoid image heights exceeding 16384 (upstream reports rendering defects).

Widget actions are objects with `version:2`, `onAction`, `handler`, `handleName`,
`additionalData`, and empty version-1 `handle remapping`. Upstream slider/knob
actions are Mouse Down -> Q-Link (`handleName:"Data"`), Double Click -> Show Overlay
(`additionalData:"knob overlay", handleName:""`), and Enter Pressed -> Show Overlay.
Q-Link action selects/focuses the parameter; the native Knob handles drag movement.

## Buttons, trigger limitation and options

Native Button child data:

```json
{
  "version": 2, "onImage": "fire_on.png", "offImage": "fire_off.png",
  "buttonId": 1, "numButtonsInGroup": 1,
  "handleName": "Data", "gestureBehaviour": "Instant"
}
```

Upstream uses this schema for both toggles and trigger buttons. A local wrapper
around it has Mouse Down -> Q-Link and Enter Pressed -> Toggle Switch actions.
List rows/popup fields with overlaid labels instead use Mouse Down -> Toggle Switch
because the label intercepts touches. Button images must be nonempty: empty images
can show an unwanted generic Button caption on device.

**No `Mouse Up` action or a verified native momentary gestureBehaviour value was
found anywhere in the inspected repository.** Do not invent `Momentary` or `Hold`.
The generic wrapper implements triggers by scheduling a return-to-zero host value
from audio processing, not by proving touch-up gate events. For FIRE, use a clearly
documented tap gate/toggle or bounded one-shot retrigger with visible LATCH and
support true press/release through MIDI. A claim of press-and-hold touchscreen FIRE
requires observing actual device setters or a separately verified skin schema.

Options use one Button per option, all mapped to the same VST parameter:
`buttonId:0..N-1`, `numButtonsInGroup:N`, `gestureBehaviour:"Instant"`, independent
on/off original images. Only first item needs `acceptsHWFocus:"Yes"`; others can
use No. Enum normalized value is option index/(N-1). Layout may be horizontal,
vertical or wrapped rows; no native dropdown is necessary. Wave and echo character
segments are suitable for the single-page workflow.

## Live labels and images

Native Label data example:

```json
{
  "version": 1,
  "textStyle": {
    "version": 1,
    "font": {"version": 1, "name": "Titillium Web", "style": "SemiBold", "height": 18.0},
    "colour": "ffbde7df", "justification": "horizontallyCentred verticallyCentred",
    "case": "Original"
  },
  "type": "Value", "handleName": "Data"
}
```

`Value` uses VST effGetParamDisplay; `Name` uses effGetParamName. Full static labels
can be baked into original background/controls, avoiding standard VST name/display
buffer limits. Upstream wrapper uses MPC-specific 32-byte name and 24-byte display
copies; an independent standard VST2 wrapper should keep standard 8-byte buffers
safe or explicitly document a confirmed host-specific extension.

`audioMasterUpdateDisplay` makes MPC reread dynamic Label text/name (upstream device
findings); normalized numeric changes also drive Value updates. A continuously
changing DSP readout needs explicit refresh management; simply changing engine
state is insufficient. The generic wrapper handles setter-driven notifications.

Image child data is `version:2, imageType:"Regular", colour:"0",
image:"original_background.png"`, with its own absolute bounds. PNG filenames
resolve in Plugin Skins. Color strings are hexadecimal ARGB (`ff` alpha plus RGB);
`0` is used for transparent component backgrounds.

Conditional display uses a bounds `additionalInvalidatingHandles` string like
`IndexedEnabling/2/6/Parameter 34` (option index 2 of six for parameter 34).
Upstream uses `showWhenDataModelInvalid:"Show"` with it. Popups need a separate
hidden open parameter and wrapper state; optional for this project's dense segments.

## Q-Link JSON and bank order

`Q-Links.json` and `Q-Links - 8by1.json` have identical upstream content. Root:
`version:4`, `info:{version:1,type:"CompleteDescription"}`, `Screen Mode Q-Links`
object `{version:4,map:[...]}`, and `Program Mode Q-Links` containing a direct map.
Each Screen map entry is:

```json
{
  "Tab": 1, "SubTab": 1, "Bank Direction": "Column",
  "Q-Links": {
    "Q-Link 1": 3, "Q-Link 2": 7, "Q-Link 3": -1, "Q-Link 4": -1,
    "Q-Link 5": 2, "Q-Link 6": 6, "Q-Link 7": -1, "Q-Link 8": -1,
    "Q-Link 9": 1, "Q-Link 10": 5, "Q-Link 11": -1, "Q-Link 12": -1,
    "Q-Link 13": 0, "Q-Link 14": 4, "Q-Link 15": -1, "Q-Link 16": -1
  }
}
```

Map values are VST indices or -1 for unused. Tab/SubTab are **one-based**, while
TUI fnKeyIndex/fnKeySubIndex are zero-based. Upstream Force mapping physical knob
slots 1..8 is `[13,9,5,1,14,10,6,2]`; second bank is `[15,11,7,3,16,12,8,4]`.
The example maps the first eight parameter indices in physical bank order.
Program Mode Q-Links repeats the direct inner map to make track/program mode useful.
Recommended first bank: Pitch, LFO Rate, LFO Depth, Zap, Cutoff, Resonance, Delay Time,
Feedback. Second bank can provide feedback HP/LP, wet, chop, attack/release, reverb,
output while retaining one visible main page.

## Metadata

Original version descriptor example:

```xml
<?xml version="1.0" encoding="utf-8"?>
<plugincontent version="1.0">
  <identifier>dubforce.vst.siren</identifier>
  <version>1.0.0.0</version>
</plugincontent>
```

`plugin-meta.xml` contains a single PLUGIN element. Example data fields (choose
project identity once and keep UID stable):

```xml
<PLUGIN name="Dub Force Siren" descriptiveName="Dub Force Siren" format="VST"
 category="Synth" manufacturer="Dub Force" version="1.0"
 file="%payload-path%/Dub Force - VST - Dub Force Siren/dub_force_siren.so"
 uid="44465331" isInstrument="1" fileTime="0" infoUpdateTime="0"
 numInputs="0" numOutputs="2" isShell="0"/>
```

This example UID is hexadecimal `DFS1` and must agree with AEffect.uniqueID and
the registered plugin entry. Installer substitutes `%payload-path%` with the
confirmed Synths directory. Folder naming must match manufacturer/plugin identity
for native skin lookup. The actual registration in MPC.settings uses an absolute
file path, not the placeholder. Escape XML attributes for chosen names/paths.

## Confidence and remaining validation

Schemas are confirmed from upstream generators and supported by upstream device
notes; our original generated skin still needs on-device verification. Touch
hold/release FIRE behavior is unconfirmed. Native slider drag sensitivity and
minimum useful hit area are not established by static source. Single-page density,
value-label legibility, all options, two knob banks, and project restore need a
physical Force smoke test; a desktop composite preview verifies artwork/layout only.

Source link: [pinned skin generator](https://github.com/sd88me/mpc-vst-plugins/blob/faae55acfbf3ccac88d15df8ab3c45f74002e91c/tools/shadow_skin.py).
