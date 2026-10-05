#!/usr/bin/env python3
"""Canonical parameter descriptor. Regenerates params.json, src/param_ids.h and docs/PARAMETERS.md.

Indices 0..15 are the sixteen Q-Links in knob order: bank 1 (SIREN) then bank 2 (FX),
grouped by function. The four touch-only performance buttons follow. Project state is
saved by key (DFS2), so the index order is free to follow the knobs.
"""
import json
from pathlib import Path
P=[]
def num(key,name,lo,hi,default,unit='',**kw): P.append(dict(key=key,name=name,min=lo,max=hi,default=default,unit=unit,**kw))
def opt(key,name,options,default=0,**kw): P.append(dict(key=key,name=name,options=options.split(','),default=default,**kw))
# Bank 1 / SIREN: OSC, LFO, ZAP, ENVELOPE
num('pitch','PITCH',10,2400,620,'Hz',scale='log')
opt('wave','WAVE','SINE,TRIANGLE,SAW,SQUARE,NOISE',default=3)
num('rate','LFO RATE',.05,24,.55,'Hz')
# Depth is relative to pitch so PITCH transposes the whole sweep (480 Hz at 620 Hz).
num('depth','LFO DEPTH',0,100,round(480/620*100,1),'%')
opt('lfo_wave','LFO SHAPE','TRIANGLE,SQUARE,SAW,SINE')
num('zap_sweep','ZAP SWEEP',-48,48,-24,'st')
num('attack','ATTACK',.005,1,.01,'s')
num('release','RELEASE',.02,3,.05,'s')
# Bank 2 / FX: FILTER, ECHO, MASTER
num('cutoff','CUTOFF',200,9000,4000,'Hz')
num('resonance','RESONANCE',0,20,2,'dB')
num('delay_time','DELAY TIME',.05,3,60/72*.5,'s')
num('feedback','FEEDBACK',0,88,42,'%')
num('delay_mix','ECHO MIX',0,100,100,'%')
num('ping','PING PONG',0,100,0,'%')
opt('preset','PRESET','Air Raid,Laser,Fog Horn,Police,UFO,Space Echo,Smoke,Clash,Heavy Dub,Deep Orbit,Feedback Madness,Sci-Fi Alarm')
num('output','OUTPUT',0,100,100,'%')
# Touch-only performance buttons
num('fire','FIRE',0,1,0,momentary=True,hold_ms=250)
opt('latch','LATCH','OFF,ON')
opt('mode','MODE','SIREN,ZAP')
num('stop','STOP',0,1,0,momentary=True)
assert len(P)==20
Path('params.json').write_text(json.dumps(P,indent=2)+'\n')
Path('src/param_ids.h').write_text('#pragma once\nnamespace dub { enum Param {\n'+''.join('    P_'+p['key']+',\n' for p in P)+'    P_Count\n}; }\n')
rows=['# Parameters','','Project state is saved by key (`DFS2 key=value ...`); 1.0.x `DFS1` positional chunks are migrated on load.','Indices 0–15 are the Q-Links in knob order. VST automation recorded with 1.0.x indices does not carry over.','','| Index | Key | Range / options | Default | Units | Q-Link |','|---:|---|---|---|---|---|']
for i,p in enumerate(P): rows.append(f"| {i} | `{p['key']}` | {', '.join(p['options']) if 'options' in p else str(p['min'])+' .. '+str(p['max'])} | {p['default']} | {p.get('unit','')} | {f'bank {i//8+1}, knob {i%8+1}' if i<16 else 'touch'} |")
rows+=['','Pitch uses logarithmic 10–2400 Hz mapping; other continuous controls are linear. FIRE is a 250 ms tap trigger (or one complete ZAP); MIDI notes provide press/release and LATCH sustains. LFO DEPTH is a percentage of the current pitch: 100% sweeps between 0 Hz and twice the pitch. STOP clears LATCH, gates and echo tails. Presets are complete snapshots of every sound parameter. ZAP time is fixed at 180 ms.']
Path('docs/PARAMETERS.md').write_text('\n'.join(rows)+'\n')
