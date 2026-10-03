#include "siren.h"
#include "dsp_core.h"
#include "effects.h"
#include "fast_trig.h"
#include "params.h"
#include <atomic>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace dub {
static_assert(std::atomic<float>::is_always_lock_free,"Realtime controls require lock-free float atomics");
static_assert(std::atomic<uint32_t>::is_always_lock_free,"Realtime gates require lock-free word atomics");
namespace {
float initial(int i) noexcept{return PARAMS[i].nopts ? std::round(PARAMS[i].def*(PARAMS[i].nopts-1)) : PARAMS[i].min+PARAMS[i].def*(PARAMS[i].max-PARAMS[i].min);}
float validate(int i,float v) noexcept{return PARAMS[i].nopts ? std::round(bound(v,0,float(PARAMS[i].nopts-1))) : bound(v,PARAMS[i].min,PARAMS[i].max);}
constexpr int holds[]={P_latch,P_freeze,P_bend,P_fast,P_slow,P_oct_up,P_oct_down,P_kill,P_filter_hold};
}
struct Siren::Impl {
    std::atomic<float> p[P_Count];
    std::atomic<uint32_t> notes[4];
    std::atomic<uint32_t> noteTriggers{0},fireTriggers{0},stopTriggers{0},stopFire{0},stopNote{0};
    float c[P_Count],t[P_Count];
    uint32_t noteSeen=0,fireSeen=0,stopSeen=0,rng=0xeca12345;
    double phase=0,lfo=0,lfo2=0,lfo3=0,chop=0;
    float env=0,basePitch=620,waveBlend[5]={1,0,0,0,0},killGain=1,bitMix=0;
    float attackCoeff=.01,releaseCoeff=.01,noiseGain=0;
    float cachedAttack=-1,cachedRelease=-1,cachedNoise=-1;
    bool lastGate=false,zapActive=false;int lastMode=0,pulse=0;uint32_t tick=0;
    double zapAge=0,repeatClock=0,zapFrequency=620,zapMultiplier=1;float zapStart=620,zapEnd=155,zapLength=.18,zapDecay=.18;
    Biquad tone,oscEq[3],postL,postR,masterEqL[3],masterEqR[3];
    Effects effects;
    float chopSmoothed=1;
    Compressor comp,limiter;
    Impl(){for(int i=0;i<P_Count;++i){c[i]=t[i]=initial(i);p[i].store(c[i]);}for(auto& n:notes)n.store(0);}
    float noise() noexcept{rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng>>8)*(1.f/8388608.f)-1;}
    bool noteGate() const noexcept{for(const auto& n:notes)if(n.load(std::memory_order_relaxed))return true;return false;}
    void startZap(float pitch) noexcept{
        zapActive=true;zapAge=0;zapStart=pitch;zapEnd=std::max(30.f,pitch*std::pow(2.f,c[P_zap_sweep]/12));
        zapLength=c[P_zap_time];zapDecay=std::max(c[P_attack],zapLength);
        zapFrequency=zapStart;zapMultiplier=std::exp(std::log(double(zapEnd)/zapStart)/(double(zapLength)*SampleRate));
    }
    void reset() noexcept{
        phase=lfo=lfo2=lfo3=chop=0;env=0;pulse=0;zapActive=lastGate=false;repeatClock=0;
        tick=0;chopSmoothed=1;effects.reset();tone.reset();postL.reset();postR.reset();for(auto& q:oscEq)q.reset();for(auto& q:masterEqL)q.reset();for(auto& q:masterEqR)q.reset();
        comp.gain=limiter.gain=1;
    }
    void coefficients() noexcept{
        if(c[P_attack]!=cachedAttack){cachedAttack=c[P_attack];attackCoeff=1-std::exp(-1/(SampleRate*std::max(.001f,c[P_attack]/4.6f)));}
        if(c[P_release]!=cachedRelease){cachedRelease=c[P_release];releaseCoeff=1-std::exp(-1/(SampleRate*std::max(.003f,c[P_release]/4.6f)));}
        tone.tone(int(c[P_filter_type]),c[P_cutoff],c[P_resonance]);
        postL.tone(0,c[P_filter_hold]>.5f ? bound(c[P_cutoff]*.12f,140,420) : 20000,c[P_filter_hold]>.5f ? std::max(10.f,c[P_resonance]) : .7f);
        postR.tone(0,c[P_filter_hold]>.5f ? bound(c[P_cutoff]*.12f,140,420) : 20000,c[P_filter_hold]>.5f ? std::max(10.f,c[P_resonance]) : .7f);
        const float hz[3]={120,1000,6000};for(int j=0;j<3;++j){oscEq[j].eq(j,hz[j],c[P_osc_low+j]);masterEqL[j].eq(j,hz[j],c[P_master_low+j]);masterEqR[j].eq(j,hz[j],c[P_master_low+j]);}
        if(c[P_noise]!=cachedNoise){cachedNoise=c[P_noise];noiseGain=std::pow(c[P_noise]*.01f,1.7f)*.24f;}
    }
    void character(int index) noexcept{
        const float values[5][6]={
            {60.f/72*.5f,42,16,120,7600,0},
            {.18f,54,8,180,2600,0},
            {.42f,66,32,70,1750,10},
            {60.f/72*.75f,70,44,220,5200,90},
            {60.f/72*.5f,80,12,260,3400,12}
        };
        const int ids[6]={P_delay_time,P_feedback,P_reverb,P_delay_hp,P_delay_lp,P_ping};
        for(int j=0;j<6;++j)p[ids[j]].store(values[index][j],std::memory_order_relaxed);
    }
    void preset(int index) noexcept{
        // Full native snapshots: performance holds do not become saved factory patch state.
        for(int i=0;i<P_Count;++i)if(i!=P_preset && i!=P_fire && i!=P_stop)p[i].store(initial(i),std::memory_order_relaxed);
        const auto set=[this](int i,float v){p[i].store(validate(i,v),std::memory_order_relaxed);};
        switch(index){
        case 1:set(P_mode,1);set(P_wave,1);set(P_pitch,2100);set(P_zap_sweep,-40);set(P_zap_time,.5);set(P_cutoff,7000);set(P_resonance,4);break;
        case 2:set(P_pitch,110);set(P_rate,.25);set(P_depth,40);set(P_cutoff,900);set(P_resonance,1);set(P_character,2);character(2);break;
        case 3:set(P_pitch,700);set(P_rate,2);set(P_depth,350);set(P_lfo_wave,1);set(P_cutoff,4500);break;
        case 4:set(P_wave,3);set(P_pitch,900);set(P_rate,3.2);set(P_depth,700);set(P_lfo_wave,3);set(P_cutoff,6000);set(P_resonance,3);break;
        case 5:set(P_wave,2);set(P_pitch,420);set(P_rate,.7);set(P_depth,280);set(P_character,3);character(3);set(P_reverb,60);break;
        case 6:set(P_character,2);character(2);set(P_pitch,340);set(P_rate,6.5);set(P_depth,180);set(P_lfo_wave,1);set(P_cutoff,3200);break;
        case 7:set(P_character,4);character(4);set(P_mode,1);set(P_pitch,1400);set(P_zap_sweep,-26);set(P_zap_time,.16);set(P_repeat,7);break;
        case 8:set(P_pitch,180);set(P_rate,.35);set(P_depth,100);set(P_cutoff,1200);set(P_character,1);character(1);set(P_feedback,78);break;
        case 9:set(P_wave,3);set(P_pitch,280);set(P_rate,.45);set(P_depth,160);set(P_lfo2_amount,80);set(P_lfo3_amount,65);set(P_character,3);character(3);break;
        case 10:set(P_pitch,620);set(P_character,4);character(4);set(P_feedback,88);set(P_delay_time,.09);set(P_delay_mix,95);set(P_level,45);set(P_output,70);break;
        case 11:set(P_wave,1);set(P_pitch,1100);set(P_rate,12);set(P_depth,600);set(P_lfo2_amount,90);set(P_lfo3_amount,80);set(P_chop_amount,55);set(P_character,3);character(3);break;
        default:break;
        }
    }
};
Siren::Siren():impl(new Impl){}
Siren::~Siren(){delete impl;}
float Siren::get(int i)const noexcept{return i>=0 && i<P_Count ? impl->p[i].load(std::memory_order_relaxed) : 0;}
void Siren::set(int i,float v)noexcept{
    if(i<0 || i>=P_Count || !std::isfinite(v))return;
    auto& s=*impl;v=validate(i,v);
    if(i==P_fire){if(v>.5f)s.fireTriggers.fetch_add(1,std::memory_order_relaxed);return;}
    if(i==P_stop){if(v>.5f){s.stopFire.store(s.fireTriggers.load(std::memory_order_relaxed),std::memory_order_relaxed);s.stopNote.store(s.noteTriggers.load(std::memory_order_relaxed),std::memory_order_relaxed);for(auto h:holds)s.p[h].store(0,std::memory_order_relaxed);for(auto& n:s.notes)n.store(0,std::memory_order_relaxed);s.stopTriggers.fetch_add(1,std::memory_order_release);}return;}
    s.p[i].store(v,std::memory_order_relaxed);
    if(i==P_preset)s.preset(int(v));else if(i==P_character)s.character(int(v));
}
void Siren::restore(const float* values)noexcept{for(int i=0;i<P_Count;++i)if(std::isfinite(values[i]))impl->p[i].store(i==P_fire || i==P_stop ? 0 : validate(i,values[i]),std::memory_order_relaxed);}
void Siren::midi(const unsigned char* msg,int len)noexcept{
    if(!msg || len<3 || msg[1]>127 || msg[2]>127)return;
    auto& s=*impl;const unsigned status=msg[0]&0xf0,note=msg[1];
    if(status==0x90 && msg[2]){s.notes[note/32].fetch_or(uint32_t(1)<<(note%32),std::memory_order_relaxed);s.noteTriggers.fetch_add(1,std::memory_order_relaxed);}
    else if(status==0x80 || status==0x90)s.notes[note/32].fetch_and(~(uint32_t(1)<<(note%32)),std::memory_order_relaxed);
    else if(status==0xb0 && (msg[1]==120 || msg[1]==123))set(P_stop,1);
}
void Siren::render(float* left,float* right,int frames)noexcept{
    if(!left || !right || frames<=0)return;
    auto& s=*impl;
    for(int i=0;i<P_Count;++i)s.t[i]=s.p[i].load(std::memory_order_relaxed);
    const uint32_t stop=s.stopTriggers.load(std::memory_order_acquire);if(stop!=s.stopSeen){s.stopSeen=stop;s.reset();s.fireSeen=s.stopFire.load(std::memory_order_relaxed);s.noteSeen=s.stopNote.load(std::memory_order_relaxed);for(int i=0;i<P_Count;++i)s.t[i]=s.p[i].load(std::memory_order_relaxed);}
    const uint32_t fire=s.fireTriggers.load(std::memory_order_relaxed),notes=s.noteTriggers.load(std::memory_order_relaxed);
    const bool fireEvent=fire!=s.fireSeen;
    bool retrigger=fireEvent || notes!=s.noteSeen;s.fireSeen=fire;s.noteSeen=notes;
    if(fireEvent)s.pulse=int(SampleRate*(s.t[P_mode]>.5f ? std::max(.25f,std::max(s.t[P_zap_time],s.t[P_attack])) : .25f));
    const bool midiHeld=s.noteGate();
    for(int k=0;k<frames;++k){
        if(s.tick++%16==0){
            for(int i=0;i<P_Count;++i){if(PARAMS[i].nopts || i==P_fire || i==P_stop)s.c[i]=s.t[i];else s.c[i]+=.0179784f*(s.t[i]-s.c[i]);}
            if((s.tick&31u)==1)s.coefficients();
        }
        auto& c=s.c;const int mode=int(c[P_mode]);const bool gate=midiHeld || c[P_latch]>.5f || s.pulse>0;
        float pitch=c[P_pitch]*(c[P_oct_up]>.5f ? 2.f : 1.f)*(c[P_oct_down]>.5f ? .5f : 1.f)*(c[P_bend]>.5f ? .5f : 1.f);
        pitch=bound(pitch,30,8000);s.basePitch+=.002265f*(pitch-s.basePitch);
        if(retrigger || (gate && !s.lastGate) || (gate && mode!=s.lastMode)){if(mode)s.startZap(pitch);s.repeatClock=0;}
        if(!gate && s.lastGate){s.repeatClock=0;s.zapActive=false;}
        if(s.pulse>0)--s.pulse;
        const float speed=(c[P_fast]>.5f ? 4.f : 1.f)*(c[P_slow]>.5f ? .25f : 1.f);
        float repeat=c[P_repeat]*speed;if(c[P_repeat]<=0 && (c[P_fast]>.5f)!=(c[P_slow]>.5f))repeat=c[P_fast]>.5f ? 8.f : 2.f;
        if(mode && gate && repeat>0){s.repeatClock+=repeat/SampleRate;if(s.repeatClock>=1){s.repeatClock-=1;s.startZap(pitch);}}
        const float mainRate=c[P_rate]*speed*(1+.9f*c[P_lfo2_amount]*.01f*modulationSine(float(s.lfo2)));
        const float mainDepth=c[P_depth]*(1+c[P_lfo3_amount]*.01f*modulationSine(float(s.lfo3)));
        float frequency=s.basePitch;
        if(mode && s.zapActive){frequency=s.zapAge>=s.zapLength ? s.zapEnd : float(s.zapFrequency);s.zapFrequency*=s.zapMultiplier;s.zapAge+=1/SampleRate;}
        else if(!mode)frequency+=mainDepth*lfoWave(s.lfo,int(c[P_lfo_wave]));
        frequency=bound(frequency,-SampleRate*.4f,SampleRate*.4f);
        const bool envOn=mode ? s.zapActive && s.zapAge<s.zapDecay : gate;
        s.env+=(envOn ? s.attackCoeff : s.releaseCoeff)*((envOn ? 1.f : 0.f)-s.env);
        if(s.env<1e-9f && !envOn)s.env=0;
        const float white=s.noise(),dt=std::fabs(frequency)/SampleRate;
        float input=0;for(int w=0;w<5;++w){s.waveBlend[w]+=.002265f*((int(c[P_wave])==w ? 1.f : 0.f)-s.waveBlend[w]);if(s.waveBlend[w]>1e-8f)input+=s.waveBlend[w]*(w==4 ? white : wave(s.phase,dt,w));}
        input+=white*s.noiseGain*(1-s.waveBlend[4]);input*=s.env;
        input=s.tone.process(input);for(auto& q:s.oscEq)input=q.process(input);
        const float chopGain=1-c[P_chop_amount]*.005f+c[P_chop_amount]*.005f*(s.chop<.5 ? 1.f : -1.f);
        s.chopSmoothed+=.027951f*(chopGain-s.chopSmoothed);
        const float dry=safe(input*s.chopSmoothed);
        EchoConfig fx;fx.time=s.t[P_delay_time];fx.feedback=s.t[P_feedback]*.01f;fx.mix=s.t[P_delay_mix]*.01f;fx.hp=s.t[P_delay_hp];fx.lp=s.t[P_delay_lp];fx.ping=s.t[P_ping]*.01f;fx.reverb=s.t[P_reverb]*.01f;fx.character=int(c[P_character]);fx.invert=c[P_invert]>.5f;fx.freeze=c[P_freeze]>.5f;fx.bend=c[P_bend]>.5f;fx.playing=gate;
        float l=0,r=0;s.effects.process(dry,fx,l,r);l*=c[P_level]*.01f;r*=c[P_level]*.01f;
        l=s.postL.process(l);r=s.postR.process(r);s.comp.process(l,r,-24,30,12,.25f);
        s.bitMix+=.00283046f*((c[P_crush]>.5f ? 1.f : 0.f)-s.bitMix);
        const auto crush=[](float v){return std::fabs(v)<1e-8f ? 0.f : 2*std::floor(255*(bound(v,-1,1)+1)*.5f+.5f)/255-1;};
        if(s.bitMix>1e-8f){l+=(crush(l)-l)*s.bitMix;r+=(crush(r)-r)*s.bitMix;}
        s.killGain+=(c[P_kill]>.5f ? .003772f : .001259f)*((c[P_kill]>.5f ? 0.f : 1.f)-s.killGain);l*=s.killGain;r*=s.killGain;
        for(int j=0;j<3;++j){l=s.masterEqL[j].process(l);r=s.masterEqR[j].process(r);}
        s.limiter.process(l,r,-1,0,20,.08f);left[k]=bound(safe(l)*c[P_output]*.01f,-.98f,.98f);right[k]=bound(safe(r)*c[P_output]*.01f,-.98f,.98f);
        s.phase=frac(s.phase+frequency/SampleRate);s.lfo=frac(s.lfo+mainRate/SampleRate);s.lfo2=frac(s.lfo2+c[P_lfo2_rate]/SampleRate);s.lfo3=frac(s.lfo3+c[P_lfo3_rate]/SampleRate);s.chop=frac(s.chop+c[P_chop_rate]/SampleRate);
        s.lastGate=gate;s.lastMode=mode;retrigger=false;
    }
}
}
