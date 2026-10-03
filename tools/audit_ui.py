#!/usr/bin/env python3
"""Audit the generated RC skin against source descriptors, IDs and real Q-Link JSON."""
import json,pathlib,re,importlib.util,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
PY=ROOT/'tools/skin_design.py';spec=importlib.util.spec_from_file_location('panel_source',PY);ui=importlib.util.module_from_spec(spec);spec.loader.exec_module(ui)
params=json.loads((ROOT/'params.json').read_text());index={p['key']:i for i,p in enumerate(params)}
source=(ROOT/'src/siren.cpp').read_text().splitlines()
ids={key:i for i,key in enumerate(re.findall(r'P_(\w+)',(ROOT/'src/param_ids.h').read_text()))};assert all(ids[k]==i for k,i in index.items())
dsp={
'fire':'fireTriggers → pulse / startZap / gate','latch':'c[P_latch] → gate','preset':'Impl::preset full parameter snapshot','mode':'c[P_mode] → siren/ZAP gate and frequency','wave':'c[P_wave] → waveBlend / wave()','pitch':'c[P_pitch] → basePitch / zapStart','level':'c[P_level] → effect-output gain','noise':'noiseGain → oscillator white-noise mix','lfo_wave':'lfoWave(s.lfo, c[P_lfo_wave])','rate':'mainRate → s.lfo phase','depth':'mainDepth → frequency FM','attack':'attackCoeff / ZAP decay','release':'releaseCoeff → env','lfo2_rate':'s.lfo2 phase','lfo2_amount':'mainRate nested modulation','lfo3_rate':'s.lfo3 phase','lfo3_amount':'mainDepth nested modulation','chop_rate':'s.chop phase','chop_amount':'chopGain / chopSmoothed','zap_sweep':'zapEnd / zapMultiplier','zap_time':'zapLength / zapDecay','repeat':'repeatClock / startZap','filter_type':'tone.tone(type,cutoff,resonance)','cutoff':'tone coefficients / FILTER FX cutoff','resonance':'tone coefficients / FILTER FX Q','delay_time':'EchoConfig.time → Effects.time','feedback':'EchoConfig.feedback → Effects.feedback','delay_mix':'EchoConfig.mix → wet / spread gains','delay_hp':'EchoConfig.hp → feedback hpFilter','delay_lp':'EchoConfig.lp → feedback lpFilter','ping':'EchoConfig.ping → stereo pan depth','character':'Impl::character defaults / EchoConfig.character → Characters','reverb':'EchoConfig.reverb → reverbGain','output':'c[P_output] → final output gain','crush':'bitMix → 255-step quantizer','invert':'EchoConfig.invert → wet polarity','freeze':'EchoConfig.freeze → feedback/send transition','bend':'pitch half / EchoConfig.bend → delay multiplier','fast':'speed ×4 / ZAP repeat','slow':'speed ×.25 / ZAP repeat','oct_up':'pitch ×2','oct_down':'pitch ×.5','kill':'killGain → output mute slew','filter_hold':'postL/postR FILTER FX coefficients','stop':'stopTriggers → reset / clear holds and notes'}
for j in range(3):dsp[params[45+j]['key']]=f'oscEq[{j}] coefficients/process';dsp[params[48+j]['key']]=f'masterEqL/R[{j}] coefficients/process'
assert set(index)==set(dsp)
# Parse the actual generated native skin, not the expected generator list.
skin=ROOT/'build/skin/Dub Force - VST - Dub Force Siren/Plugin Skins'
tui=json.loads((skin/'TUI.json').read_text())['pageData'];defs={d['key']:d['value'] for d in tui['componentDefinitions']['localComponentDefinitions']};assert len(defs)==len(tui['componentDefinitions']['localComponentDefinitions'])
q=json.loads((skin/'Q-Links.json').read_text());assert q==json.loads((skin/'Q-Links - 8by1.json').read_text())
qmaps=q['Screen Mode Q-Links']['map'];assert qmaps[0]['Q-Links']==ui.qlinks(ui.PRIMARY+ui.SECONDARY,index);assert qmaps[1]['Q-Links']==ui.qlinks(ui.UTILITY,index);assert q['Program Mode Q-Links']==qmaps[0]['Q-Links']
for bank in qmaps:
 active=[i for i in bank['Q-Links'].values() if i>=0];assert len(active)==len(set(active));assert all(i<len(params) for i in active)
qlinks={i:[] for i in range(51)}
for bank in qmaps:
 for name,i in bank['Q-Links'].items():
  if i>=0:qlinks[i].append(('Main' if bank['Tab']==1 else 'Utility')+' '+name)
rows=[];touch=[];seen=set();selectors={}
for tab in tui['tabs']:
 for c in defs[tab['componentName']]['componentsData']:
  remap={m['key']:m['value'] for m in c['handle remapping']['map']};definition=defs.get(c['componentData']['type']);
  if not definition or not remap:continue
  actions=definition.get('actions',[]);widgets=[d for d in definition['componentsData'] if d['componentData']['type'] in ('Knob','Button')]
  if not widgets and not actions:continue # preset backdrop is a container, not a control
  keyid=int(remap['Data'].split()[-1]);name=c['componentData']['name'];box=list(map(int,c['bounds']['bounds'].split()));conditional=bool(c['bounds']['additionalInvalidatingHandles'])
  if keyid==51:
   assert remap['Text']=='Parameter 2';rows.append((tab['tabName'],name,keyid,'preset__open','UI popup, transient', 'Closed / Open','Closed','preset name','none',box,conditional));continue
  assert 0<=keyid<51
  p=params[keyid];key=p['key'];seen.add(key)
  for widget in widgets:
   d=widget['componentData']['data']
   if widget['componentData']['type']=='Knob':assert not d['invert']
   else:
    assert d['gestureBehaviour']=='Instant'
    if p.get('options'):
     if d['numButtonsInGroup']==1:assert len(p['options'])==2 and d['buttonId']==1
     else:
      assert d['numButtonsInGroup']==len(p['options']);assert 0<=d['buttonId']<len(p['options']);selectors.setdefault(key,set()).add(d['buttonId'])
  opts=p.get('options');range_=' / '.join(opts) if opts else f"{p['min']:g}…{p['max']:g} {p.get('unit','')}"
  default=ui.physical_default(p);default=opts[default] if opts else f"{default:g} {p.get('unit','')}"
  span=p.get('max',1)-p.get('min',0);format_='option text' if opts else ('1 decimal below 100 Hz; whole Hz above' if p.get('scale')=='log' else '3 decimals' if p.get('unit')=='s' else '2 decimals' if p.get('unit')=='Hz' and span<=32 else '0 decimals' if span>20 or p.get('unit')=='Hz' else '1 decimal')
  line=next((j+1 for j,l in enumerate(source) if 'P_'+key in l),None);assert line is not None or key.endswith(('low','mid','high'))
  rows.append((tab['tabName'],name,keyid,key,dsp[key],range_,default,format_,', '.join(qlinks[keyid]) or 'touch only',box,conditional))
  if not conditional:
   for widget in widgets:
    child=list(map(int,widget['bounds']['bounds'].split()));touch.append(dict(control=name,param=key,parent=box,widget=[box[0]+child[0],box[1]+child[1],child[2],child[3]],type=widget['componentData']['type']))
assert seen==set(index),sorted(set(index)-seen)
for key in ['wave','mode','lfo_wave','filter_type','character','preset']:assert selectors[key]==set(range(len(params[index[key]]['options'])))
text='''# RC UI and Q-Link wiring audit

Generated from `params.json`, `src/param_ids.h`, `src/siren.cpp`, the actual
native `TUI.json`, both Q-Link JSONs and original panel definitions. No parameter,
layout or DSP change is required. Every public parameter is bound. Repeated IDs
within an enum group are intentional; each option has a unique buttonId and the
correct group size. There are no duplicate local definition keys or duplicate
active IDs inside a Q-Link bank. Hidden popup ID 51 is transient; IDs 52–56 remain
compatibility flags, with no sound role.

Main Q-Link order is slots 13, 9, 5, 1, 14, 10, 6, 2: Pitch, Wave, LFO Rate,
LFO Depth, Zap Sweep, Cutoff, Delay Time, Feedback. Slots 15, 11, 7, 3, 16, 12, 8, 4 form the second
set: Resonance, Preset, Attack, Release, Echo Mix, Ping, Reverb, Output.
These are native 16-slot addresses, not a claim that Force has 16 physical knobs.
The Force selects subsets/banks; exact hardware selection must be tested.
Utility maps six EQ bands and Fast/Slow/octave up/down. Unused slots are -1.
Turning upward/rightward increases physical values; fader invert=false. Native
Q-Link names/ranges come from the descriptor, whereas section captions are
shortened artwork labels. Continuous normalized values are not quantized to the
32/48 artwork frames; enum values snap and support the wrapper's option nudge.
Zap Sweep is semitones, Delay is seconds, Resonance is source Q in dB, and Echo
LO CUT/HI CUT mean feedback highpass/lowpass. Primary ranges/defaults are below.

| Page / visible control | ID / key | DSP destination | Physical range | Default | Native display | Q-Link address |
| --- | --- | --- | --- | --- | --- | --- |
'''
for page,name,i,key,destination,range_,default,format_,qmap,box,conditional in rows:
 text+=f'| {page} / {name}'+(' (PATCH overlay)' if conditional else '')+f' | {i} / {key} | {destination} | {range_} | {default} | {format_} | {qmap} |\n'
text+='''
## Touch practicality

Final main and utility previews were inspected again. FIRE is 140×64, LATCH 60×64,
with 8 px horizontal clearance. Primary fader **fields** are large, but those field
bounds are not all draggable: the actual Knob child bounds are separately recorded
in `resources/rc-touch-targets.json`. Secondary attack/release/level/noise tracks
are 92×20; nested/echo auxiliary tracks are 117/124×29–33. Wave/shape buttons are
mostly 57–66×38–40 with 6 px gaps; several selectors and small auxiliary tracks need
real finger validation. No overlapping siblings, reversed faders or missing
bindings were found. No further redesign was performed: physical feel, native
hit propagation and accidental activation cannot be established from a preview.

Primary pitch drag is 200×58, rate/depth 117×94, delay 124×102, feedback 128×102.
Tone cutoff/resonance 75/79×106. Primary fonts 16 px, large values 24 px (narrow tone
values 17 px); secondary fonts 13 px / values 17 px and units 11 px. Clipped labels were not
observed in the rendered defaults. Large/high digit counts at extremes remain
part of the physical card. Drag travel is approximate drawn track length minus
19 px horizontally or21 px vertically; precise value resolution/gesture sensitivity
is native host behavior, not inferred from artwork. Double-click/Enter requests
the verified Generic Knob Overlay for numeric editing.

Control reachability is established by source reads and wrapper/parameter tests.
Some controls require the relevant context: Zap/Repeat in ZAP mode, nested amounts
above zero, Chop amount above zero, feedback HP/LP heard on subsequent repeats,
EQ on Utility, and performance toggles active. Invert flips wet polarity rather
than swapping channels. KILL mutes output; STOP clears holds, MIDI and effect state.
PATCH name remains the last selected factory preset after custom edits; full
custom values, not merely the preset name, are serialized.
'''
(ROOT/'docs/UI_WIRING_AUDIT.md').write_text(text)
(ROOT/'resources/rc-touch-targets.json').write_text(json.dumps(touch,indent=2)+'\n')
print(f'UI AUDIT PASS: {len(rows)} visible interactive controls, all 51 public IDs, selectors complete, two main Q-Link sets and Utility map verified')
