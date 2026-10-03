#!/usr/bin/env python3
"""Original, deterministic hardware-panel artwork and native MPC skin.

Three design passes share the exact parameter wiring. Preview images compose the
same PNGs and bounds as the native JSON; live labels use parameter defaults.
"""
import json
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

# x/y/w/h are native, zero-based canvas bounds, not Shadow y+86 coordinates.
ZONES = [(16,232,"SIREN","01"),(256,282,"MODULATION","02"),
         (546,202,"TONE","03"),(756,300,"ECHO","04"),(1064,200,"MASTER","05")]
FADERS = [
    ("pitch","PITCH",28,344,208,112,1),
    ("attack","ATTACK",28,470,100,62,0),("release","RELEASE",136,470,100,62,0),
    ("level","LEVEL",28,544,100,62,0),("noise","NOISE",136,544,100,62,0),
    ("rate","LFO RATE",268,106,125,148,1),("depth","LFO DEPTH",401,106,125,148,1),
    ("lfo2_rate","LFO2 RATE",268,330,125,72,0),("lfo2_amount","RATE MOD",401,330,125,72,0),
    ("lfo3_rate","LFO3 RATE",268,414,125,72,0),("lfo3_amount","DEPTH MOD",401,414,125,72,0),
    ("zap_sweep","SWEEP",268,518,82,86,0),
    ("zap_time","TIME",356,518,82,86,0),("repeat","REPEAT",444,518,82,86,0),
    ("cutoff","CUTOFF",558,106,83,160,1),("resonance","RESO",649,106,87,160,1),
    ("chop_rate","CHOP",558,340,83,110,0),("chop_amount","DEPTH",649,340,87,110,0),
    ("delay_time","DELAY",768,106,132,156,1),("feedback","FEEDBACK",908,106,136,156,1),
    ("delay_hp","LO CUT",768,394,132,72,0),("delay_lp","HI CUT",912,394,132,72,0),
    ("delay_mix","ECHO MIX",768,478,132,68,0),("ping","PING PONG",912,478,132,68,0),
    ("reverb","REVERB",1076,106,176,154,1),("output","OUTPUT",1076,276,176,154,1),
]
SELECTORS = [
    ("wave",["SQUARE","SAW","TRI","SINE","NOISE"],28,198,208,40,3),
    ("mode",["SIREN","ZAP"],28,294,208,38,2),
    ("lfo_wave",["TRI","SQR","SAW","SINE"],268,280,258,38,4),
    ("filter_type",["LP","BP","HP"],558,286,178,40,3),
    ("character",["CLEAN","DESK","SMOKE","ORBIT","CLASH"],768,290,276,40,3),
]
BUTTONS = [
    ("fire","FIRE",28,112,140,64,"fire"),("latch","LATCH",176,112,60,64,"toggle"),
    ("crush","8 BIT",558,482,178,44,"toggle"),
    ("filter_hold","FILTER FX",558,536,178,44,"toggle"),
    ("freeze","FREEZE",768,558,86,44,"toggle"),
    ("bend","BEND",862,558,86,44,"toggle"),("invert","INVERT",956,558,88,44,"toggle"),
    ("kill","KILL",1076,464,176,48,"toggle"),("stop","STOP",1076,526,176,62,"stop"),
]
PRIMARY = ["pitch","rate","depth","zap_sweep","cutoff","resonance","delay_time","feedback"]
SECONDARY = ["attack","release","lfo2_amount","lfo3_amount","delay_mix","ping","reverb","output"]
UTILITY = ["osc_low","osc_mid","osc_high","master_low","master_mid","master_high",
           "fast","slow","oct_up","oct_down"]
POPUPS = ["preset","wave","lfo_wave","mode","filter_type","character"]

def font(size, bold=False):
    return ImageFont.truetype(str(BOLD if bold else FONT),size)

def center(draw,box,label,size=15,fill=CREAM,bold=False):
    x,y,w,h=box
    f=font(size,bold)
    while draw.textbbox((0,0),label,font=f)[2]>w-6 and size>10:
        size-=1;f=font(size,bold)
    draw.text((x+w/2,y+h/2),label,font=f,fill="#"+fill,anchor="mm")

def bounds(x,y,w,h,focus="No",show="Show",condition=None):
    return dict(version=2,acceptsHWFocus=focus,showWhenDataModelInvalid=show,
                whenVisible="Always",boundsType="Absolute",bounds=f"{x} {y} {w} {h}",
                additionalInvalidatingHandles=[] if condition is None else [condition])

def component(kind,data,box,name="",mapping=None,focus="No",condition=None):
    return {"version":2,"componentData":{"version":1,"name":name,"type":kind,"data":data},
            "handle remapping":{"version":1,"map":mapping or []},
            "bounds":bounds(*box,focus,condition=condition)}

def action(event,handler,additional=""):
    return {"version":2,"onAction":event,"handler":handler,"handleName":"" if handler=="Show Overlay" else "Data",
            "additionalData":additional,"handle remapping":{"version":1,"map":[]}}

def local(key,children,actions=None):
    clear={"version":1,"colour":"0","image":""}
    return {"key":key,"value":{"version":4,"actions":actions or [],
            "backgroundData":{"version":1,"focussed":clear,"unfocussed":clear},
            "ignoreMousePresses":False,"disableCoarseDataWheel":False,"repeats":1,
            "hideQLinkBounds":True,"componentsData":children}}

def value_label(box,size,handle="Data",colour=CREAM):
    return component("Label",{"version":1,"textStyle":{"version":1,
        "font":{"version":1,"name":"Titillium Web","style":"SemiBold","height":float(size)},
        "colour":"ff"+colour,"justification":"horizontallyCentred verticallyCentred","case":"Original"},
        "type":"Value","handleName":handle},box,"Value")

def physical_default(p):
    d=p.get("default",p.get("min",0))
    if p.get("options"):
        return p["options"].index(d) if isinstance(d,str) else int(d)
    return float(d)

def display(p):
    v=physical_default(p)
    if p.get("options"):return p["options"][v]
    span=p.get("max",1)-p.get("min",0)
    decimals=3 if p.get("unit")=="s" else (2 if p.get("unit")=="Hz" and span<=32 else (0 if span>20 or p.get("unit")=="Hz" else 1))
    return f"{v:.{decimals}f}"

class Panel:
    def __init__(self,version,plist,output):
        self.version=version;self.ps={p["key"]:p for p in plist};self.index={p["key"]:i for i,p in enumerate(plist)}
        self.output=output;output.mkdir(parents=True,exist_ok=True)
        self.defs=[];self.kids=[];self.regions=[];self.assets=set()
        self.image=Image.new("RGB",(W,H),"#"+BG);self.dr=ImageDraw.Draw(self.image)
        self.background()

    def background(self):
        for y in range(H):
            c=23+int(5*(1-y/H)) if self.version==3 else 23
            self.dr.line((0,y,W,y),fill=(c,c+1,c))
        center(self.dr,(22,8,216,34),"DUB FORCE",24,AMBER,True)
        center(self.dr,(22,42,216,14),"PERFORMANCE SIREN",11,DIM)
        center(self.dr,(734,14,512,30),"OSC  >  MOD  >  TONE  >  ECHO  >  MASTER",14,DIM)
        for x,w,label,number in ZONES:
            self.dr.rounded_rectangle((x,66,x+w,616),radius=7,fill="#"+FACE,outline="#"+LINE,width=1)
            self.dr.rounded_rectangle((x+1,67,x+w-1,94),radius=6,fill="#2B2D27")
            center(self.dr,(x+12,68,w-44,26),label,18 if self.version>1 else 15,CREAM,True)
            center(self.dr,(x+w-32,68,24,26),number,11,AMBER)
            if self.version==3:
                for sx in (x+7,x+w-7):
                    self.dr.ellipse((sx-2,605,sx+2,609),fill="#5B5D52")
                    self.dr.line((sx-1,607,sx+1,607),fill="#252623")
        center(self.dr,(28,180,208,14),"TAP FIRE  /  PADS HOLD",11,DIM)
        center(self.dr,(768,268,276,16),"ECHO CHARACTER",13,DIM)
        center(self.dr,(268,260,258,14),"LFO SHAPE",12,DIM)
        center(self.dr,(268,498,258,16),"ZAP CONTOUR",12,DIM)
        center(self.dr,(1076,438,176,18),"PERFORMANCE CUTS",12,DIM)

    def save(self,name,img):
        img.save(self.output/name,optimize=True);self.assets.add(name)

    def placed(self,key,name,param,box,extra=None,condition=None):
        mapping=[{"key":"Data","value":f"Parameter {self.index[param]}"}]
        for handle,target in (extra or {}).items():mapping.append({"key":handle,"value":f"Parameter {self.index[target]}"})
        self.kids.append(component(key,{"version":1,"handleName":"Data"},box,name,mapping,"Yes",condition))
        if condition is None:self.regions.append((param,box))

    def fader(self,key,label,x,y,w,h,primary):
        p=self.ps[key];title_h=22 if h>=90 else 18
        value_h=28 if primary and self.version>1 else 21
        track_h=max(20,h-title_h-value_h-4);track_w=w-8
        vertical=h>=90 and w<h*1.35
        center(self.dr,(x,y,w,title_h),label,16 if primary and self.version>1 else 13,CREAM,True)
        unit=p.get("unit","")
        unit_w={"Hz":24,"dB":22,"st":20,"s":14,"%":16}.get(unit,0)
        value_box=(x+4,y+h-value_h,w-12-unit_w if unit else w-8,value_h)
        value_size=24 if primary and self.version>1 else 17
        if w<=95:value_size=min(value_size,17)
        self.dr.rounded_rectangle((x+2,y+h-value_h,x+w-2,y+h),radius=3,fill="#151713",outline="#353C2F")
        if unit:center(self.dr,(x+w-unit_w-4,y+h-value_h,unit_w,value_h),unit,11,DIM)
        # Inspected Force stock Bassline: filmstrip dimensions are rectangular
        # frame width x (frame height * numFrames); numFrames is the frame COUNT.
        frames=48 if primary else 32
        strip=Image.new("RGBA",(track_w,track_h*frames),(0,0,0,0))
        for i in range(frames):
            f=Image.new("RGBA",(track_w,track_h),(0,0,0,0));d=ImageDraw.Draw(f);n=i/(frames-1)
            if vertical:
                cx=track_w/2;top,bottom=10,track_h-11;yy=bottom-(bottom-top)*n
                d.rounded_rectangle((cx-5,top,cx+5,bottom),radius=4,fill="#0F110F",outline="#54574A")
                d.line((cx,bottom,cx,yy),fill="#"+AMBER,width=4)
                if key=="feedback":d.line((cx,top,cx,top+(bottom-top)*.2),fill="#"+RED,width=4)
                for tick in range(6):
                    ty=top+(bottom-top)*tick/5;d.line((cx-17,ty,cx-10,ty),fill="#65665B")
                capw=54 if primary else min(42,track_w-12)
                d.rounded_rectangle((cx-capw/2,yy-9,cx+capw/2,yy+9),radius=3,fill="#"+(CREAM if self.version>1 else DIM),outline="#807A6D")
                d.line((cx-capw/2+5,yy,cx+capw/2-5,yy),fill="#7C6747",width=2)
            else:
                cy=track_h/2;left,right=9,track_w-10;xx=left+(right-left)*n
                d.rounded_rectangle((left,cy-4,right,cy+4),radius=3,fill="#0F110F",outline="#54574A")
                d.line((left,cy,xx,cy),fill="#"+AMBER,width=3)
                capw,caph=(32,24) if primary and self.version>1 else (18,16)
                d.rounded_rectangle((xx-capw/2,cy-caph/2,xx+capw/2,cy+caph/2),radius=3,fill="#"+CREAM,outline="#807A6D")
            strip.paste(f,(0,i*track_h))
        filename=f"fader_{track_w}x{track_h}_{'v' if vertical else 'h'}_{int(bool(primary))}_{int(key=='feedback')}.png"
        self.save(filename,strip)
        localkey=f"Fader_{key}"
        children=[component("Knob",{"version":5,"knobType":"FilmStrip","filmStrip":filename,
            "numFrames":frames,"invert":False,"dragOrientation":"Vertical" if vertical else "Horizontal",
            "handleName":"Data"},(4,title_h,track_w,track_h),"Fader"),
            value_label((value_box[0]-x,value_box[1]-y,value_box[2],value_box[3]),value_size)]
        self.defs.append(local(localkey,children,[action("Mouse Down","Q-Link"),
            action("Double Click","Show Overlay","knob overlay"),
            action("Enter Pressed","Show Overlay","knob overlay")]))
        self.placed(localkey,label,key,(x,y,w,h))
        lo,hi=p.get("min",0),p.get("max",1)
        n=(physical_default(p)-lo)/(hi-lo) if hi>lo else 0
        frame_index=round(n*(frames-1))
        frame=strip.crop((0,frame_index*track_h,track_w,(frame_index+1)*track_h))
        self.image.paste(frame,(x+4,y+title_h),frame)
        center(self.dr,value_box,display(p),value_size,CREAM,True)

    def button(self,key,label,x,y,w,h,style="toggle",option=None,total=1,condition=None):
        name=f"button_{key}_{option if option is not None else 'toggle'}"
        images=[]
        for on in (0,1):
            im=Image.new("RGB",(w,h),"#"+FACE);d=ImageDraw.Draw(im)
            fill=AMBER if on else (RED if style=="fire" else "292C26")
            text=BG if on else CREAM
            edge=AMBER if on or style in ("fire","stop") else LINE
            d.rounded_rectangle((1,1,w-2,h-2),radius=5,fill="#"+fill,outline="#"+edge,width=2 if style=="fire" else 1)
            if self.version==3:d.line((6,3,w-7,3),fill="#"+("F4C779" if on else "625B4F"))
            fs=26 if style=="fire" and self.version>1 else (18 if h>=48 and self.version>1 else 13)
            center(d,(3,0,w-6,h),label,fs,text,True)
            fn=name+("_on.png" if on else "_off.png");self.save(fn,im);images.append(im)
        data={"version":2,"onImage":name+"_on.png","offImage":name+"_off.png",
            "buttonId":option if option is not None else 1,"numButtonsInGroup":total,
            "handleName":"Data","gestureBehaviour":"Instant"}
        self.defs.append(local(name,[component("Button",data,(0,0,w,h),label)],
                               [action("Mouse Down","Q-Link"),action("Enter Pressed","Toggle Switch")]))
        self.placed(name,label,key,(x,y,w,h),condition=condition)
        v=physical_default(self.ps[key]);on=(v==option) if option is not None else v>.5
        if condition is None:self.image.paste(images[int(on)],(x,y))

    def selector(self,key,labels,x,y,w,h,cols):
        gap=6;slot=(w-gap*(cols-1))//cols
        for i,label in enumerate(labels):
            self.button(key,label,x+(i%cols)*(slot+gap),y+(i//cols)*(h+6),slot,h,option=i,total=len(labels))

    def preset(self):
        x,y,w,h=282,12,426,42
        self.dr.rounded_rectangle((x,y,x+w,y+h),radius=5,fill="#262921",outline="#74664A")
        center(self.dr,(x+8,y,80,h),"PATCH",13,AMBER,True)
        center(self.dr,(x+w-30,y,22,h),"v",16,AMBER)
        self.defs.append(local("PresetField",[value_label((94,0,w-128,h),19,"Text")],
                               [action("Mouse Down","Toggle Switch"),action("Enter Pressed","Toggle Switch")]))
        self.placed("PresetField","PATCH","preset__open",(x,y,w,h),{"Text":"preset"})
        center(self.dr,(x+94,y,w-128,h),display(self.ps["preset"]),19,CREAM,True)
        condition=f"IndexedEnabling/1/2/Parameter {self.index['preset__open']}"
        pbox=(282,74,746,228)
        panel=Image.new("RGB",pbox[2:],"#1B1D18");d=ImageDraw.Draw(panel)
        d.rounded_rectangle((0,0,745,227),radius=7,outline="#"+AMBER,width=2)
        center(d,(12,8,720,26),"CHOOSE PATCH  /  TAP A SOUND TO CLOSE",16,AMBER,True)
        self.save("preset_panel.png",panel)
        self.defs.append(local("PresetPanel",[component("Image",{"version":2,"imageType":"Regular","colour":"0","image":"preset_panel.png"},(0,0,746,228))]))
        self.placed("PresetPanel","PATCHES","preset__open",pbox,condition=condition)
        for i,label in enumerate(self.ps["preset"]["options"]):
            self.button("preset",label.upper(),294+(i%3)*240,116+(i//3)*44,228,38,option=i,total=12,condition=condition)

    def finish(self,page_name="SirenPage"):
        # Background must precede the controls. Artwork includes static names/units.
        fn="panel_background.png";self.save(fn,self.background_only)
        children=[component("Image",{"version":2,"imageType":"Regular","colour":"0","image":fn},(0,0,W,H),"Panel")]+self.kids
        self.defs.append(local(page_name,children))
        return self.defs,self.image

def qlinks(keys,index):
    order=[13,9,5,1,14,10,6,2,15,11,7,3,16,12,8,4]
    result={f"Q-Link {i}":-1 for i in range(1,17)}
    for slot,key in enumerate(keys):result[f"Q-Link {order[slot]}"]=index[key]
    return result

def generate(version,plist,folder,plugin_version):
    skin=folder/"Plugin Skins"
    p=Panel(version,plist,skin)
    # Save static labels/units into the background while the preview additionally
    # paints default frames and values. Record each component layer separately.
    p.background_only=p.image.copy()
    for args in FADERS:p.fader(*args)
    for args in SELECTORS:p.selector(*args)
    for args in BUTTONS:p.button(*args)
    p.preset()
    # Repaint static panel + labels/readout/unit badges independently of values.
    static=Panel(version,plist,skin)
    for key,label,x,y,w,h,primary in FADERS:
        title_h=22 if h>=90 else 18;value_h=28 if primary and version>1 else 21
        center(static.dr,(x,y,w,title_h),label,16 if primary and version>1 else 13,CREAM,True)
        static.dr.rounded_rectangle((x+2,y+h-value_h,x+w-2,y+h),radius=3,fill="#151713",outline="#353C2F")
        unit=p.ps[key].get("unit","")
        unit_w={"Hz":24,"dB":22,"st":20,"s":14,"%":16}.get(unit,0)
        if unit:center(static.dr,(x+w-unit_w-4,y+h-value_h,unit_w,value_h),unit,11,DIM)
    static.dr.rounded_rectangle((282,12,708,54),radius=5,fill="#262921",outline="#74664A")
    center(static.dr,(290,12,80,42),"PATCH",13,AMBER,True);center(static.dr,(678,12,22,42),"v",16,AMBER)
    p.background_only=static.image
    defs,preview=p.finish()
    # Auxiliary page retains existing EQ and four performance toggles only.
    u=Panel(version,plist,skin)
    u.image=Image.new("RGB",(W,H),"#"+BG);u.dr=ImageDraw.Draw(u.image)
    center(u.dr,(16,12,1248,40),"DUB FORCE  /  UTILITY",25,AMBER,True)
    for j,(key,label) in enumerate([(k,k.replace('_',' ').upper()) for k in UTILITY[:6]]):
        u.fader(key,label,32+(j%3)*416,94+(j//3)*210,376,168,1)
    for j,key in enumerate(UTILITY[6:]):u.button(key,key.replace('_',' ').upper(),48+j*312,532,272,58)
    utility_static=Image.new("RGB",(W,H),"#"+BG);d=ImageDraw.Draw(utility_static)
    center(d,(16,12,1248,40),"DUB FORCE  /  UTILITY",25,AMBER,True)
    for j,key in enumerate(UTILITY[:6]):
        x,y=32+(j%3)*416,94+(j//3)*210
        center(d,(x,y,376,22),key.replace('_',' ').upper(),16,CREAM,True)
        d.rounded_rectangle((x+2,y+140,x+374,y+168),radius=3,fill="#151713")
        center(d,(x+346,y+140,28,28),"dB",11,DIM)
    u.background_only=utility_static
    u.save("utility_background.png",utility_static)
    children=[component("Image",{"version":2,"imageType":"Regular","colour":"0","image":"utility_background.png"},(0,0,W,H))]+u.kids
    defs+=u.defs+[local("UtilityPage",children)]
    tabs=[{"version":3,"tabName":label,"fnKeyIndex":i,"fnKeySubIndex":0,"qlinkBoundsData":["0 0 0 0"],
           "componentName":key,"initialSize":"0 0 1280 628","scale":1.0}
          for i,(label,key) in enumerate([("SIREN","SirenPage"),("UTILITY","UtilityPage")])]
    tui={"pageData":{"version":1,"componentDefinitions":{"version":2,
        "importFiles":["/usr/share/Akai/Content/Synths/Generic/Generic Knob Overlay.json"],"localComponentDefinitions":defs},
                     "info":{"version":1,"type":"CompleteDescription"},"tabs":tabs}}
    coremap=qlinks(PRIMARY+SECONDARY,p.index)
    maps=[{"Tab":i+1,"SubTab":1,"Bank Direction":"Column","Q-Links":mapping}
          for i,mapping in enumerate([coremap,qlinks(UTILITY,p.index)])]
    qmap={"version":4,"info":{"version":1,"type":"CompleteDescription"},
          "Screen Mode Q-Links":{"version":4,"map":maps},"Program Mode Q-Links":coremap}
    for name,data in [("TUI.json",tui),("Q-Links.json",qmap),("Q-Links - 8by1.json",qmap)]:
        (skin/name).write_text(json.dumps(data,indent=2)+"\n")
    (folder/"version.xml").write_text(f'<?xml version="1.0" encoding="utf-8"?>\n<plugincontent version="1.0"><identifier>Dub Force.vst.dubforcesiren</identifier><version>{plugin_version}</version></plugincontent>\n')
    return p,u,preview

def validate(folder,plist,panel,utility):
    skin=folder/"Plugin Skins";tui=json.loads((skin/"TUI.json").read_text())
    defs={d["key"]:d["value"] for d in tui["pageData"]["componentDefinitions"]["localComponentDefinitions"]}
    errors=[];refs=set();assigned=set()
    for key,d in defs.items():
        if key.startswith("Fader_"):
            rects=[list(map(int,c["bounds"]["bounds"].split())) for c in d["componentsData"]]
            a,b=rects[0],rects[1]
            if max(a[0],b[0])<min(a[0]+a[2],b[0]+b[2]) and max(a[1],b[1])<min(a[1]+a[3],b[1]+b[3]):
                errors.append("fader and value overlap "+key)
        for c in d["componentsData"]:
            data=c["componentData"]["data"]
            for attr in ("image","filmStrip","onImage","offImage"):
                if data.get(attr):refs.add(data[attr])
            if data.get("filmStrip"):
                asset=Image.open(skin/data["filmStrip"])
                _,_,cw,ch=map(int,c["bounds"]["bounds"].split())
                if asset.width!=cw or asset.height!=ch*data["numFrames"]:
                    errors.append("filmstrip geometry mismatch "+key)
            for m in c["handle remapping"]["map"]:
                if m["value"].startswith("Parameter "):
                    target=int(m["value"].split()[-1]);assigned.add(target)
                    if not 0<=target<len(plist):errors.append("bad parameter "+key)
    for i,p in enumerate(plist):
        if "popup_of" not in p and i not in assigned:errors.append("unbound public parameter "+p["key"])
    for name in refs:
        if not (skin/name).is_file():errors.append("missing asset "+name)
        elif Image.open(skin/name).height>16384:errors.append("oversized filmstrip "+name)
    for page in (panel,utility):
        for name,(x,y,w,h) in page.regions:
            if min(x,y,w,h)<0 or x+w>W or y+h>H:errors.append("out of bounds "+name)
        for i,(a,ra) in enumerate(page.regions):
            for b,rb in page.regions[i+1:]:
                ax,ay,aw,ah=ra;bx,by,bw,bh=rb
                if max(ax,bx)<min(ax+aw,bx+bw) and max(ay,by)<min(ay+ah,by+bh):errors.append("overlap "+a+" / "+b)
    assert not errors,"\n".join(errors)
    return {"assets_checked":len(refs),"main_touch_regions":len(panel.regions),"utility_touch_regions":len(utility.regions),
            "parameters":len(plist),"public_parameters_bound":sum("popup_of" not in p for p in plist),
            "primary_qlinks":PRIMARY,"bounds_and_overlap_errors":errors,
            "minimum_primary_touch_size":{key:[w,h] for key,_,_,_,w,h,primary in FADERS if primary}}

def main():
    cfg=gen_vst.load(str(ROOT/"vst.json"));plist,_=parameter_source.load(str(ROOT/"params.json"))
    popup=shadow_skin.popup_params(str(ROOT/"layout.conf"),plist)
    assert [p["key"] for p in popup]==[k+"__open" for k in POPUPS],"Popup IDs must stay stable"
    plist=plist+popup
    build=ROOT/"build";build.mkdir(exist_ok=True)
    gen_vst.gen_params(cfg,plist,str(build/"params.h"))
    (build/"pluginlist-entry.xml").write_text(gen_vst.entry(cfg)+"\n")
    resources=ROOT/"resources";resources.mkdir(exist_ok=True)
    reports=[]
    for version in (1,2,3):
        folder=build/f"ui-v{version}"/"Dub Force - VST - Dub Force Siren"
        if folder.exists():shutil.rmtree(folder)
        v=int(cfg.get("version",1000));plugin_version=f"{v//1000}.{v%1000//100}.{v%100}.0"
        p,u,preview=generate(version,plist,folder,plugin_version)
        report=validate(folder,plist,p,u);report["iteration"]=version;reports.append(report)
        preview.save(resources/f"ui-v{version}.png",optimize=True)
        u.image.save(resources/f"utility-v{version}.png",optimize=True)
        print(f"UI V{version}: {report['main_touch_regions']} main touch regions; {report['assets_checked']} assets checked; bounds/overlaps OK")
    final=build/"skin"/"Dub Force - VST - Dub Force Siren"
    if final.exists():shutil.rmtree(final)
    shutil.copytree(folder,final)
    shutil.copy2(resources/"ui-v3.png",resources/"preview.png")
    shutil.copy2(resources/"ui-v3.png",build/"siren-page-0.png")
    shutil.copy2(resources/"utility-v3.png",build/"siren-page-1.png")
    # Compose the real conditional preset components from native JSON as a
    # separate inspection artifact, not an invented web/mockup overlay.
    skin=final/"Plugin Skins"
    tui=json.loads((skin/"TUI.json").read_text())["pageData"]
    defs={d["key"]:d["value"] for d in tui["componentDefinitions"]["localComponentDefinitions"]}
    opened=Image.open(resources/"ui-v3.png").convert("RGBA")
    for c in defs["SirenPage"]["componentsData"]:
        if not c["bounds"]["additionalInvalidatingHandles"]:continue
        x,y,_,_=map(int,c["bounds"]["bounds"].split())
        for child in defs[c["componentData"]["type"]]["componentsData"]:
            data=child["componentData"]["data"];sx,sy,_,_=map(int,child["bounds"]["bounds"].split())
            fn=data.get("image") or (data.get("onImage") if data.get("buttonId")==0 else data.get("offImage"))
            if fn:opened.alpha_composite(Image.open(skin/fn).convert("RGBA"),(x+sx,y+sy))
    opened.convert("RGB").save(resources/"ui-v3-presets.png",optimize=True)
    (resources/"ui-validation.json").write_text(json.dumps(reports,indent=2)+"\n")

if __name__=="__main__":main()
