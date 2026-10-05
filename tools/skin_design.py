#!/usr/bin/env python3
"""Single-page native MPC skin whose screen mirrors the two Q-Link banks.

The top row is bank 1 (SIREN), the bottom row bank 2 (FX); each column is one
knob, numbered 1-16, and knobs that act together share a frame. FIRE, LATCH,
SIREN/ZAP and STOP sit in the header. The preview composes the same PNGs and
bounds as the native JSON and shows parameter defaults.
"""
import json
import math
import pathlib
import shutil
import sys
from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parents[1]
TOOLS = ROOT / "vendor/mpc-vst-plugins/tools"
sys.path.insert(0, str(TOOLS))
import gen_vst
import params as parameter_source
import shadow_skin

W, H = 1280, 628
CREAM = "F1E8D6"
DIM = "B7B0A1"
AMBER = "E6AE55"
RED = "CC633D"
BG = "161716"
FACE = "20221F"
LINE = "444740"
FONT = pathlib.Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
BOLD = FONT.with_name("DejaVuSans-Bold.ttf")

# Knob order is parameter order 0..15 (tools/define_parameters.py).
BANKS = [
    ("SIREN", [("OSC", ["pitch", "wave"]), ("LFO", ["rate", "depth", "lfo_wave"]),
               ("ZAP", ["zap_sweep"]), ("ENVELOPE", ["attack", "release"])]),
    ("FX", [("FILTER", ["cutoff", "resonance"]), ("ECHO", ["delay_time", "feedback", "delay_mix", "ping"]),
            ("MASTER", ["preset", "output"])]),
]
QLINKS = [key for _, groups in BANKS for _, keys in groups for key in keys]
LABELS = {"pitch": "PITCH", "wave": "WAVE", "rate": "RATE", "depth": "DEPTH", "lfo_wave": "SHAPE",
          "zap_sweep": "SWEEP", "attack": "ATTACK", "release": "RELEASE", "cutoff": "CUTOFF",
          "resonance": "RESO", "delay_time": "TIME", "feedback": "FEEDBACK", "delay_mix": "MIX",
          "ping": "PING PONG", "preset": "PATCH", "output": "OUTPUT"}
CHOICES = {"wave": ["SINE", "TRI", "SAW", "SQUARE", "NOISE"], "lfo_wave": ["TRI", "SQUARE", "SAW", "SINE"]}
POPUPS = ["preset"]
X0, GAP, COL = 16, 8, 149
ROWS = [86, 358]
ROW_H = 264
TITLE_Y, CONTROL_Y, CONTROL_H, VALUE_Y, VALUE_H = 28, 54, 174, 232, 28
UNIT_W = {"Hz": 24, "dB": 22, "st": 20, "s": 14, "%": 16}


def column(k):
    return X0 + k * (COL + GAP)


def font(size, bold=False):
    return ImageFont.truetype(str(BOLD if bold else FONT), size)


def center(draw, box, label, size=15, fill=CREAM, bold=False):
    x, y, w, h = box
    f = font(size, bold)
    while draw.textbbox((0, 0), label, font=f)[2] > w - 6 and size > 10:
        size -= 1
        f = font(size, bold)
    draw.text((x + w / 2, y + h / 2), label, font=f, fill="#" + fill, anchor="mm")


def bounds(x, y, w, h, focus="No", condition=None):
    return dict(version=2, acceptsHWFocus=focus, showWhenDataModelInvalid="Show",
                whenVisible="Always", boundsType="Absolute", bounds=f"{x} {y} {w} {h}",
                additionalInvalidatingHandles=[] if condition is None else [condition])


def component(kind, data, box, name="", mapping=None, focus="No", condition=None):
    return {"version": 2, "componentData": {"version": 1, "name": name, "type": kind, "data": data},
            "handle remapping": {"version": 1, "map": mapping or []},
            "bounds": bounds(*box, focus, condition=condition)}


def action(event, handler, additional=""):
    return {"version": 2, "onAction": event, "handler": handler,
            "handleName": "" if handler == "Show Overlay" else "Data",
            "additionalData": additional, "handle remapping": {"version": 1, "map": []}}


def local(key, children, actions=None):
    clear = {"version": 1, "colour": "0", "image": ""}
    return {"key": key, "value": {"version": 4, "actions": actions or [],
            "backgroundData": {"version": 1, "focussed": clear, "unfocussed": clear},
            "ignoreMousePresses": False, "disableCoarseDataWheel": False, "repeats": 1,
            "hideQLinkBounds": True, "componentsData": children}}


def value_label(box, size, handle="Data"):
    return component("Label", {"version": 1, "textStyle": {"version": 1,
        "font": {"version": 1, "name": "Titillium Web", "style": "SemiBold", "height": float(size)},
        "colour": "ff" + CREAM, "justification": "horizontallyCentred verticallyCentred", "case": "Original"},
        "type": "Value", "handleName": handle}, box, "Value")


def physical_default(p):
    d = p.get("default", p.get("min", 0))
    if p.get("options"):
        return p["options"].index(d) if isinstance(d, str) else int(d)
    return float(d)


def normalized_default(p):
    lo, hi, v = p.get("min", 0), p.get("max", 1), physical_default(p)
    if p.get("scale") == "log":
        return math.log(v / lo) / math.log(hi / lo)
    return (v - lo) / (hi - lo) if hi > lo else 0


def display(p):
    """Mirror of the wrapper's effGetParamDisplay formatting."""
    v = physical_default(p)
    if p.get("options"):
        return p["options"][v]
    span = p.get("max", 1) - p.get("min", 0)
    decimals = 0 if span > 20 else 1
    if p.get("unit") == "s":
        decimals = 3
    elif p.get("unit") == "Hz":
        decimals = 2 if span <= 32 else 0
    if p.get("scale") == "log" and v < 100:
        decimals = 1
    return f"{v:.{decimals}f}"


class Skin:
    def __init__(self, plist, skin):
        self.ps = {p["key"]: p for p in plist}
        self.index = {p["key"]: i for i, p in enumerate(plist)}
        self.out = skin
        skin.mkdir(parents=True, exist_ok=True)
        self.defs, self.kids, self.regions, self.assets = [], [], [], set()
        self.static = Image.new("RGB", (W, H), "#" + BG)
        self.draw = ImageDraw.Draw(self.static)
        self.defaults = []      # (image, position) default-state artwork for the preview
        self.texts = []         # (box, text, size) default value text for the preview
        self.open_layers = []   # (image, position) for the open PATCH list preview

    def save(self, name, image):
        image.save(self.out / name, optimize=True)
        self.assets.add(name)

    def placed(self, key, name, param, box, extra=None, condition=None):
        mapping = [{"key": "Data", "value": f"Parameter {self.index[param]}"}]
        for handle, target in (extra or {}).items():
            mapping.append({"key": handle, "value": f"Parameter {self.index[target]}"})
        self.kids.append(component(key, {"version": 1, "handleName": "Data"}, box, name, mapping, "Yes", condition))
        if condition is None:
            self.regions.append((param, box))

    def background(self):
        d = self.draw
        for y in range(H):
            c = 23 + int(5 * (1 - y / H))
            d.line((0, y, W, y), fill=(c, c + 1, c))
        center(d, (16, 12, 220, 32), "DUB FORCE", 24, AMBER, True)
        center(d, (16, 46, 220, 22), "SIREN  2.0", 13, DIM)
        number = 0
        for row, (bank, groups) in zip(ROWS, BANKS):
            k = 0
            for title, keys in groups:
                x, w = column(k) - 2, len(keys) * COL + (len(keys) - 1) * GAP + 4
                d.rounded_rectangle((x, row, x + w, row + ROW_H), radius=7, fill="#" + FACE, outline="#" + LINE)
                d.rounded_rectangle((x + 1, row + 1, x + w - 1, row + 24), radius=6, fill="#2B2D27")
                center(d, (x + 8, row + 1, w - 16, 24), title, 15, CREAM, True)
                if k == 0:
                    center(d, (x + 4, row + 1, 70, 24), f"BANK {ROWS.index(row) + 1}", 10, AMBER, True)
                for key in keys:
                    number += 1
                    cx = column(k)
                    center(d, (cx, row + TITLE_Y, 30, 22), str(number), 13, AMBER, True)
                    center(d, (cx + 26, row + TITLE_Y, COL - 52, 22), LABELS[key], 15, CREAM, True)
                    k += 1

    def fader(self, key, x, row):
        p = self.ps[key]
        tw, th, frames = COL - 8, CONTROL_H, 48
        unit = p.get("unit", "")
        uw = UNIT_W.get(unit, 0)
        d = self.draw
        d.rounded_rectangle((x + 2, row + VALUE_Y, x + COL - 2, row + VALUE_Y + VALUE_H), radius=3,
                            fill="#151713", outline="#353C2F")
        if unit:
            center(d, (x + COL - uw - 4, row + VALUE_Y, uw, VALUE_H), unit, 11, DIM)
        strip = Image.new("RGBA", (tw, th * frames), (0, 0, 0, 0))
        for i in range(frames):
            f = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
            g = ImageDraw.Draw(f)
            n = i / (frames - 1)
            cx, top, bottom = tw / 2, 10, th - 11
            yy = bottom - (bottom - top) * n
            g.rounded_rectangle((cx - 5, top, cx + 5, bottom), radius=4, fill="#0F110F", outline="#54574A")
            g.line((cx, bottom, cx, yy), fill="#" + AMBER, width=4)
            if key == "feedback":
                g.line((cx, top, cx, top + (bottom - top) * .2), fill="#" + RED, width=4)
            for tick in range(6):
                ty = top + (bottom - top) * tick / 5
                g.line((cx - 30, ty, cx - 22, ty), fill="#65665B")
                g.line((cx + 22, ty, cx + 30, ty), fill="#65665B")
            g.rounded_rectangle((cx - 27, yy - 9, cx + 27, yy + 9), radius=3, fill="#" + CREAM, outline="#807A6D")
            g.line((cx - 22, yy, cx + 22, yy), fill="#7C6747", width=2)
            strip.paste(f, (0, i * th))
        filename = f"fader_{tw}x{th}_{int(key == 'feedback')}.png"
        self.save(filename, strip)
        box = (x, row + TITLE_Y, COL, VALUE_Y + VALUE_H - TITLE_Y)
        value = (2, VALUE_Y - TITLE_Y, COL - 6 - uw if unit else COL - 4, VALUE_H)
        children = [component("Knob", {"version": 5, "knobType": "FilmStrip", "filmStrip": filename,
            "numFrames": frames, "invert": False, "dragOrientation": "Vertical", "handleName": "Data"},
            (4, CONTROL_Y - TITLE_Y, tw, th), "Fader"), value_label(value, 22)]
        self.defs.append(local(f"Fader_{key}", children, [action("Mouse Down", "Q-Link"),
            action("Double Click", "Show Overlay", "knob overlay"),
            action("Enter Pressed", "Show Overlay", "knob overlay")]))
        self.placed(f"Fader_{key}", LABELS[key], key, box)
        frame = round(normalized_default(p) * (frames - 1))
        self.defaults.append((strip.crop((0, frame * th, tw, (frame + 1) * th)), (x + 4, row + CONTROL_Y)))
        self.texts.append(((x + 2 + value[0], row + VALUE_Y, value[2], VALUE_H), display(p), 22))

    def button(self, key, label, box, style="toggle", option=None, total=1, condition=None, size=None):
        x, y, w, h = box
        name = f"button_{key}_{option if option is not None else 'toggle'}"
        images = []
        for on in (0, 1):
            im = Image.new("RGB", (w, h), "#" + FACE)
            g = ImageDraw.Draw(im)
            fill = AMBER if on else (RED if style == "fire" else "292C26")
            edge = AMBER if on or style in ("fire", "stop") else LINE
            g.rounded_rectangle((1, 1, w - 2, h - 2), radius=5, fill="#" + fill, outline="#" + edge,
                                width=2 if style == "fire" else 1)
            g.line((6, 3, w - 7, 3), fill="#" + ("F4C779" if on else "625B4F"))
            center(g, (3, 0, w - 6, h), label, size or (26 if style == "fire" else 18 if h >= 48 else 14),
                   BG if on else CREAM, True)
            self.save(name + ("_on.png" if on else "_off.png"), im)
            images.append(im)
        data = {"version": 2, "onImage": name + "_on.png", "offImage": name + "_off.png",
                "buttonId": option if option is not None else 1, "numButtonsInGroup": total,
                "handleName": "Data", "gestureBehaviour": "Instant"}
        self.defs.append(local(name, [component("Button", data, (0, 0, w, h), label)],
                               [action("Mouse Down", "Q-Link"), action("Enter Pressed", "Toggle Switch")]))
        self.placed(name, label, key, box, condition=condition)
        v = physical_default(self.ps[key])
        on = (v == option) if option is not None else v > .5
        (self.open_layers if condition else self.defaults).append((images[int(on)], (x, y)))

    def selector(self, key, x, row):
        labels = CHOICES[key]
        gap = 5 if len(labels) > 4 else 6
        h = (ROW_H - 4 - CONTROL_Y - gap * (len(labels) - 1)) // len(labels)
        for i, label in enumerate(labels):
            self.button(key, label, (x + 2, row + CONTROL_Y + i * (h + gap), COL - 4, h),
                        option=i, total=len(labels), size=15)

    def preset(self, x, row):
        box = (x + 2, row + CONTROL_Y, COL - 4, ROW_H - 4 - CONTROL_Y)
        bx, by, bw, bh = box
        d = self.draw
        d.rounded_rectangle((bx, by, bx + bw, by + bh), radius=5, fill="#262921", outline="#74664A")
        center(d, (bx, by + 8, bw, 20), "TAP TO CHOOSE", 10, DIM)
        center(d, (bx, by + bh - 34, bw, 26), "v", 18, AMBER)
        label = (4, 40, bw - 8, 80)
        self.defs.append(local("PresetField", [value_label(label, 17, "Text")],
                               [action("Mouse Down", "Toggle Switch"), action("Enter Pressed", "Toggle Switch")]))
        self.placed("PresetField", "PATCH", "preset__open", box, {"Text": "preset"})
        self.texts.append(((bx + label[0], by + label[1], label[2], label[3]), display(self.ps["preset"]), 17))
        condition = f"IndexedEnabling/1/2/Parameter {self.index['preset__open']}"
        pbox = (176, 92, 928, 252)
        panel = Image.new("RGB", pbox[2:], "#1B1D18")
        g = ImageDraw.Draw(panel)
        g.rounded_rectangle((0, 0, pbox[2] - 1, pbox[3] - 1), radius=7, outline="#" + AMBER, width=2)
        center(g, (12, 6, pbox[2] - 24, 26), "CHOOSE PATCH  /  TAP A SOUND TO CLOSE", 16, AMBER, True)
        self.save("preset_panel.png", panel)
        self.defs.append(local("PresetPanel", [component("Image", {"version": 2, "imageType": "Regular",
            "colour": "0", "image": "preset_panel.png"}, (0, 0, *pbox[2:]))]))
        self.placed("PresetPanel", "PATCHES", "preset__open", pbox, condition=condition)
        self.open_layers.append((panel, pbox[:2]))
        for i, label in enumerate(self.ps["preset"]["options"]):
            self.button("preset", label.upper(), (195 + (i % 3) * 300, 128 + (i // 3) * 54, 290, 46),
                        option=i, total=12, condition=condition, size=16)

    def build(self):
        self.background()
        k = 0
        for row, (_, groups) in zip(ROWS, BANKS):
            for _, keys in groups:
                for key in keys:
                    x = column(k % 8)
                    if key in CHOICES:
                        self.selector(key, x, row)
                    elif key == "preset":
                        self.preset(x, row)
                    else:
                        self.fader(key, x, row)
                    k += 1
        self.button("fire", "FIRE", (248, 8, 300, 68), "fire")
        self.button("latch", "LATCH", (560, 8, 150, 68))
        for i, label in enumerate(self.ps["mode"]["options"]):
            self.button("mode", label, (722 + i * 136, 8, 130, 68), option=i, total=2)
        self.button("stop", "STOP", (1000, 8, 264, 68), "stop")
        self.save("panel_background.png", self.static)
        children = [component("Image", {"version": 2, "imageType": "Regular", "colour": "0",
                                        "image": "panel_background.png"}, (0, 0, W, H), "Panel")] + self.kids
        self.defs.append(local("SirenPage", children))

    def previews(self):
        closed = self.static.convert("RGBA")
        for image, position in self.defaults:
            closed.alpha_composite(image.convert("RGBA"), position)
        d = ImageDraw.Draw(closed)
        for box, text, size in self.texts:
            center(d, box, text, size, CREAM, True)
        opened = closed.copy()
        for image, position in self.open_layers:
            opened.alpha_composite(image.convert("RGBA"), position)
        return closed.convert("RGB"), opened.convert("RGB")


def qlinks(keys, index):
    # Physical knob slots 1..8 then the second bank (docs/MPC_UI_REFERENCE.md).
    order = [13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3, 16, 12, 8, 4]
    result = {f"Q-Link {i}": -1 for i in range(1, 17)}
    for slot, key in enumerate(keys):
        result[f"Q-Link {order[slot]}"] = index[key]
    return result


def generate(plist, folder, plugin_version):
    skin = Skin(plist, folder / "Plugin Skins")
    skin.build()
    tabs = [{"version": 3, "tabName": "SIREN", "fnKeyIndex": 0, "fnKeySubIndex": 0, "qlinkBoundsData": ["0 0 0 0"],
             "componentName": "SirenPage", "initialSize": f"0 0 {W} {H}", "scale": 1.0}]
    tui = {"pageData": {"version": 1, "componentDefinitions": {"version": 2,
        "importFiles": ["/usr/share/Akai/Content/Synths/Generic/Generic Knob Overlay.json"],
        "localComponentDefinitions": skin.defs},
        "info": {"version": 1, "type": "CompleteDescription"}, "tabs": tabs}}
    coremap = qlinks(QLINKS, skin.index)
    qmap = {"version": 4, "info": {"version": 1, "type": "CompleteDescription"},
            "Screen Mode Q-Links": {"version": 4, "map": [{"Tab": 1, "SubTab": 1, "Bank Direction": "Column",
                                                         "Q-Links": coremap}]},
            "Program Mode Q-Links": coremap}
    for name, data in [("TUI.json", tui), ("Q-Links.json", qmap), ("Q-Links - 8by1.json", qmap)]:
        (skin.out / name).write_text(json.dumps(data, indent=2) + "\n")
    (folder / "version.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n<plugincontent version="1.0"><identifier>Dub Force.vst.dubforcesiren'
        f'</identifier><version>{plugin_version}</version></plugincontent>\n')
    return skin


def validate(folder, plist, skin):
    out = folder / "Plugin Skins"
    tui = json.loads((out / "TUI.json").read_text())
    defs = {d["key"]: d["value"] for d in tui["pageData"]["componentDefinitions"]["localComponentDefinitions"]}
    errors, refs, assigned = [], set(), set()
    if QLINKS != [p["key"] for p in plist[:16]]:
        errors.append("Q-Link order must equal parameter indices 0..15")
    if len(set(QLINKS)) != 16:
        errors.append("duplicate Q-Link")
    for key, d in defs.items():
        for c in d["componentsData"]:
            data = c["componentData"]["data"]
            for attr in ("image", "filmStrip", "onImage", "offImage"):
                if data.get(attr):
                    refs.add(data[attr])
            if data.get("filmStrip"):
                asset = Image.open(out / data["filmStrip"])
                _, _, cw, ch = map(int, c["bounds"]["bounds"].split())
                if asset.width != cw or asset.height != ch * data["numFrames"]:
                    errors.append("filmstrip geometry mismatch " + key)
            for m in c["handle remapping"]["map"]:
                if m["value"].startswith("Parameter "):
                    target = int(m["value"].split()[-1])
                    assigned.add(target)
                    if not 0 <= target < len(plist):
                        errors.append("bad parameter " + key)
    for i, p in enumerate(plist):
        if i not in assigned:
            errors.append("unbound parameter " + p["key"])
    for name in refs:
        if not (out / name).is_file():
            errors.append("missing asset " + name)
        elif Image.open(out / name).height > 16384:
            errors.append("oversized filmstrip " + name)
    for name, (x, y, w, h) in skin.regions:
        if min(x, y, w, h) < 0 or x + w > W or y + h > H:
            errors.append("out of bounds " + name)
    for i, (a, ra) in enumerate(skin.regions):
        for b, rb in skin.regions[i + 1:]:
            ax, ay, aw, ah = ra
            bx, by, bw, bh = rb
            if max(ax, bx) < min(ax + aw, bx + bw) and max(ay, by) < min(ay + ah, by + bh):
                errors.append("overlap " + a + " / " + b)
    assert not errors, "\n".join(errors)
    return {"assets_checked": len(refs), "touch_regions": len(skin.regions), "parameters": len(plist),
            "qlinks": {f"bank {i // 8 + 1} knob {i % 8 + 1}": key for i, key in enumerate(QLINKS)},
            "groups": {bank: {title: keys for title, keys in groups} for bank, groups in BANKS},
            "errors": errors}


def main():
    cfg = gen_vst.load(str(ROOT / "vst.json"))
    plist, _ = parameter_source.load(str(ROOT / "params.json"))
    popup = shadow_skin.popup_params(str(ROOT / "layout.conf"), plist)
    assert [p["key"] for p in popup] == [k + "__open" for k in POPUPS], "Popup IDs must stay stable"
    plist = plist + popup
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    gen_vst.gen_params(cfg, plist, str(build / "params.h"))
    (build / "pluginlist-entry.xml").write_text(gen_vst.entry(cfg) + "\n")
    folder = build / "skin" / "Dub Force - VST - Dub Force Siren"
    if folder.exists():
        shutil.rmtree(folder)
    v = int(cfg.get("version", 1000))
    skin = generate(plist, folder, f"{v // 1000}.{v % 1000 // 100}.{v % 100}.0")
    report = validate(folder, plist, skin)
    closed, opened = skin.previews()
    resources = ROOT / "resources"
    closed.save(resources / "preview.png", optimize=True)
    opened.save(resources / "preview-presets.png", optimize=True)
    (resources / "ui-validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"UI: {report['touch_regions']} touch regions, {report['assets_checked']} assets, "
          f"16 Q-Links in {sum(len(g) for _, g in BANKS)} groups; bounds/overlaps OK")


if __name__ == "__main__":
    main()
