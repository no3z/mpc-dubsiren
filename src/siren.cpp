#include "siren.h"
#include "dsp_core.h"
#include "effects.h"
#include "params.h"
#include <atomic>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace dub {
static_assert(std::atomic<float>::is_always_lock_free,"Realtime controls require lock-free float atomics");
static_assert(std::atomic<uint32_t>::is_always_lock_free,"Realtime gates require lock-free word atomics");
namespace {
float initial(int i) noexcept{return PHYSICAL_DEFAULTS[i];}
float validate(int i,float v) noexcept{return PARAMS[i].nopts ? float(int(bound(v,0,float(PARAMS[i].nopts-1))+.5f)) : bound(v,PARAMS[i].min,PARAMS[i].max);}
constexpr float ZapTime=.18f;   // fixed ZAP sweep time (BARZINE default)
constexpr float Level=.7f;      // siren level into the master compressor
constexpr float Glide=.002265f; // ~10 ms per-sample pitch and wave smoothing
enum Wave {Sine,Triangle,Saw,Square,Noise};
// Echo controls have their own ramps in Effects; do not smooth them twice.
constexpr int smoothControls[]={P_pitch,P_rate,P_depth,P_zap_sweep,P_attack,P_release,P_cutoff,P_resonance,P_output};
constexpr int discreteControls[]={P_wave,P_lfo_wave,P_latch,P_mode};
// glide[j] = (1-Glide)^(j+1): closed form of the one-pole for sample j of a chunk.
alignas(16) const std::array<float,Chunk> glide=[]{
    std::array<float,Chunk> g{};double q=1;
    for(int j=0;j<Chunk;++j){q*=1.0-Glide;g[j]=float(q);}
    return g;
}();
const std::array<double,Chunk> glideExact=[]{
    std::array<double,Chunk> g{};double q=1;
    for(int j=0;j<Chunk;++j){q*=1.0-Glide;g[j]=q;}
    return g;
}();
}
struct Siren::Impl {
    std::atomic<float> p[P_Count];
    std::atomic<uint32_t> notes[4];
    std::atomic<uint32_t> noteTriggers{0},fireTriggers{0},stopTriggers{0},stopFire{0},stopNote{0};
    float c[P_Count],t[P_Count];
    uint32_t noteSeen=0,fireSeen=0,stopSeen=0,tick=0;
    alignas(16) uint32_t rng[4]={0xeca12345u,0x9e3779b9u,0x7f4a7c15u,0x2545f491u};
    double phase=0,lfo=0,basePitch=620;
    float env=0,waveBlend[5]={};
    float attackCoeff=.01f,releaseCoeff=.01f,cachedAttack=-1,cachedRelease=-1;
    bool lastGate=false,zapActive=false;int lastMode=0,pulse=0;
    int selectedWave=Square,settledWave=Square,lfoShape=0;
    float pitchTarget=620,outputGain=1;
    // ZAP: exponential f0 -> f0*2^(sweep/12) over ZapTime, then hold.
    double zapAge=0,zapFrequency=620;float zapEnd=155,zapDecay=ZapTime;
    double zapSteps[Chunk+1]={};
    Lowpass tone;Effects effects;EchoConfig fx;
    Compressor comp{-24,30,12,.25f},limiter{-1,0,20,.08f};

    Impl(){
        for(int i=0;i<P_Count;++i){c[i]=t[i]=initial(i);p[i].store(c[i]);}
        for(auto& n:notes)n.store(0);
        selectedWave=settledWave=int(c[P_wave]);waveBlend[selectedWave]=1;
        basePitch=pitchTarget=c[P_pitch];
        coefficients();
    }
    void control() noexcept{
        pitchTarget=bound(c[P_pitch],1,8000);
        outputGain=c[P_output]*.01f;lfoShape=int(c[P_lfo_wave]);
        if(selectedWave!=int(c[P_wave])){selectedWave=int(c[P_wave]);settledWave=-1;}
    }
    void coefficients() noexcept{
        tone.scrub();
        if(c[P_attack]!=cachedAttack){cachedAttack=c[P_attack];attackCoeff=1-std::exp(-1/(Fs*std::max(.001f,c[P_attack]/4.6f)));}
        if(c[P_release]!=cachedRelease){cachedRelease=c[P_release];releaseCoeff=1-std::exp(-1/(Fs*std::max(.003f,c[P_release]/4.6f)));}
        tone.set(c[P_cutoff],c[P_resonance]);
    }
    bool noteGate() const noexcept{for(const auto& n:notes)if(n.load(std::memory_order_relaxed))return true;return false;}
    void startZap(float pitch) noexcept{
        zapActive=true;zapAge=0;zapFrequency=pitch;
        zapEnd=std::max(1.f,pitch*std::pow(2.f,c[P_zap_sweep]/12));zapDecay=std::max(c[P_attack],ZapTime);
        const double m=std::exp(std::log(double(zapEnd)/pitch)/(double(ZapTime)*Fs));
        double q=1;for(int j=0;j<=Chunk;++j){zapSteps[j]=q;q*=m;}
    }
    void reset() noexcept{
        phase=lfo=0;env=0;pulse=0;zapActive=lastGate=false;tick=0;
        effects.reset();tone.reset();comp.reset();limiter.reset();
    }
    void preset(int index) noexcept{
        // Complete snapshots: a preset change also releases LATCH.
        for(int i=0;i<P_Count;++i)if(i!=P_preset && i!=P_fire && i!=P_stop)p[i].store(initial(i),std::memory_order_relaxed);
        const auto set=[this](int i,float v){p[i].store(validate(i,v),std::memory_order_relaxed);};
        switch(index){
        case 1:set(P_mode,1);set(P_wave,Saw);set(P_pitch,2100);set(P_zap_sweep,-40);set(P_cutoff,7000);set(P_resonance,4);break;
        case 2:set(P_pitch,110);set(P_rate,.25);set(P_depth,36.4);set(P_cutoff,900);set(P_resonance,1);set(P_delay_time,.42);set(P_feedback,66);set(P_ping,10);break;
        case 3:set(P_pitch,700);set(P_rate,2);set(P_depth,50);set(P_lfo_wave,1);set(P_cutoff,4500);break;
        case 4:set(P_wave,Sine);set(P_pitch,900);set(P_rate,3.2);set(P_depth,77.8);set(P_lfo_wave,3);set(P_cutoff,6000);set(P_resonance,3);break;
        case 5:set(P_wave,Triangle);set(P_pitch,420);set(P_rate,.7);set(P_depth,66.7);set(P_delay_time,.625);set(P_feedback,70);set(P_ping,90);break;
        case 6:set(P_pitch,340);set(P_rate,6.5);set(P_depth,52.9);set(P_lfo_wave,1);set(P_cutoff,3200);set(P_delay_time,.42);set(P_feedback,66);set(P_ping,10);break;
        case 7:set(P_mode,1);set(P_pitch,1400);set(P_zap_sweep,-26);set(P_feedback,80);set(P_ping,12);break;
        case 8:set(P_pitch,180);set(P_rate,.35);set(P_depth,55.6);set(P_cutoff,1200);set(P_delay_time,.18);set(P_feedback,78);break;
        case 9:set(P_wave,Sine);set(P_pitch,280);set(P_rate,.45);set(P_depth,57.1);set(P_delay_time,.625);set(P_feedback,70);set(P_ping,90);break;
        case 10:set(P_feedback,88);set(P_delay_time,.09);set(P_delay_mix,95);set(P_ping,12);set(P_output,70);break;
        case 11:set(P_wave,Saw);set(P_pitch,1100);set(P_rate,12);set(P_depth,54.5);set(P_delay_time,.625);set(P_feedback,70);set(P_ping,90);break;
        default:break;
        }
    }
    // One chunk of n <= 16 samples inside a single control interval.
    void chunk(float* left,float* right,int n,bool retrigger,bool midiHeld) noexcept{
        using namespace simd;
        const int vectors=(n+3)/4,padded=4*vectors;
        const int mode=int(c[P_mode]);
        // Gates change on the control grid; a FIRE pulse ends at most 15 samples late.
        const bool gate=midiHeld || c[P_latch]>.5f || pulse>0;
        pulse=std::max(0,pulse-n);
        if(mode && (retrigger || (gate && !lastGate) || (gate && mode!=lastMode)))startZap(pitchTarget);
        if(!gate && lastGate)zapActive=false;
        lastGate=gate;lastMode=mode;

        alignas(16) float freq[Chunk],phases[Chunk],steps[Chunk],osc[Chunk],dry[Chunk],l[Chunk],r[Chunk];
        // Pitch glide, then siren LFO FM or the ZAP sweep.
        // The glide state is double so chunk boundaries do not round it.
        const double offset=basePitch-pitchTarget;
        for(int v=0;v<vectors;++v)store(freq+4*v,madd(splat(pitchTarget),splat(float(offset)),load(glide.data()+4*v)));
        basePitch=std::fabs(offset)<1e-4 ? pitchTarget : pitchTarget+offset*glideExact[n-1];
        const double lfoStep=double(c[P_rate])/Fs;
        const bool zapping=mode && zapActive;
        if(!mode){
            // Depth is a fraction of the gliding pitch: f = pitch*(1 + depth*lfo).
            // Lane phases come from the double phase, so any block split is identical.
            alignas(16) float at[Chunk];
            for(int j=0;j<padded;++j)at[j]=float(frac(lfo+j*lfoStep));
            const vf depth=splat(c[P_depth]*.01f);
            for(int v=0;v<vectors;++v){
                const vf f=load(freq+4*v);
                store(freq+4*v,madd(f,mul(f,depth),simd::lfo(lfoShape,load(at+4*v))));
            }
        }else if(zapping){
            for(int j=0;j<padded;++j)freq[j]=zapAge+j/double(Fs)>=ZapTime ? zapEnd : float(zapFrequency*zapSteps[j]);
            zapFrequency*=zapSteps[n];
        }
        lfo=frac(lfo+n*lfoStep);
        // Oscillator phase stays double precision across chunks.
        for(int j=0;j<n;++j){
            const float f=bound(freq[j],-Fs*.4f,Fs*.4f);
            phases[j]=float(phase);steps[j]=std::fabs(f)*(1/Fs);
            phase=frac(phase+double(f)/Fs);
        }
        for(int j=n;j<padded;++j)phases[j]=steps[j]=0;
        const double age=zapAge;
        if(zapping)zapAge+=n/double(Fs);
        const bool sounding=env!=0 || (mode ? zapping && age+1/double(Fs)<zapDecay : gate);
        // Waveform: one band-limited shape once settled, a 10 ms blend after changes.
        if(sounding){
            if(settledWave>=0)render(settledWave,osc,phases,steps,vectors);
            else{
                alignas(16) float shape[Chunk];
                std::fill_n(osc,padded,0.f);
                for(int w=0;w<5;++w){
                    const float to=w==selectedWave ? 1.f : 0.f,from=waveBlend[w];
                    if(std::max(from,to+(from-to)*glide[n-1])<=1e-8f)continue;
                    render(w,shape,phases,steps,vectors);
                    for(int v=0;v<vectors;++v){
                        const vf blend=madd(splat(to),splat(from-to),load(glide.data()+4*v));
                        store(osc+4*v,madd(load(osc+4*v),blend,load(shape+4*v)));
                    }
                }
            }
        }
        if(settledWave<0){
            bool settled=true;
            for(int w=0;w<5;++w){
                const float to=w==selectedWave ? 1.f : 0.f,next=to+(waveBlend[w]-to)*glide[n-1];
                if(std::fabs(next-to)<1e-7f)waveBlend[w]=to;else{waveBlend[w]=next;settled=false;}
            }
            if(settled)settledWave=selectedWave;
        }
        // Envelope and resonant lowpass are the serial part.
        if(!sounding && tone.silent())std::fill_n(dry,padded,0.f);
        else{
            for(int j=0;j<n;++j){
                const bool on=mode ? zapping && age+(j+1)/double(Fs)<zapDecay : gate;
                env+=(on ? attackCoeff : releaseCoeff)*((on ? 1.f : 0.f)-env);
                if(env<1e-9f && !on)env=0;
                dry[j]=tone.process(sounding ? osc[j]*env : 0.f);
            }
            for(int j=n;j<padded;++j)dry[j]=0;
            for(int v=0;v<vectors;++v)store(dry+4*v,guard(load(dry+4*v),32.f));
        }
        effects.process(dry,l,r,n,fx);
        for(int v=0;v<vectors;++v){store(l+4*v,mul(load(l+4*v),splat(Level)));store(r+4*v,mul(load(r+4*v),splat(Level)));}
        comp.process(l,r,n);limiter.process(l,r,n);
        const vf gain=splat(outputGain);
        for(int v=0;v<vectors;++v){
            store(l+4*v,clamp(softBound(mul(guard(load(l+4*v),32.f),gain)),-.98f,.98f));
            store(r+4*v,clamp(softBound(mul(guard(load(r+4*v),32.f),gain)),-.98f,.98f));
        }
        std::memcpy(left,l,n*sizeof(float));std::memcpy(right,r,n*sizeof(float));
    }
    void render(int wave,float* out,const float* phases,const float* steps,int vectors) noexcept{
        using namespace simd;
        if(wave==Noise){
            vu x=uload(rng);
            for(int v=0;v<vectors;++v){
                x=uxor(x,ushl<13>(x));x=uxor(x,ushr<17>(x));x=uxor(x,ushl<5>(x));
                store(out+4*v,sub(mul(to_float(ushr<8>(x)),splat(1.f/8388608.f)),splat(1.f)));
            }
            ustore(rng,x);return;
        }
        for(int v=0;v<vectors;++v)store(out+4*v,oscillator(wave,load(phases+4*v),load(steps+4*v)));
    }
};
Siren::Siren():impl(new Impl){}
Siren::~Siren(){delete impl;}
float Siren::get(int i)const noexcept{return i>=0 && i<P_Count ? impl->p[i].load(std::memory_order_relaxed) : 0;}
void Siren::set(int i,float v)noexcept{
    if(i<0 || i>=P_Count || !std::isfinite(v))return;
    auto& s=*impl;v=validate(i,v);
    if(i==P_fire){if(v>.5f)s.fireTriggers.fetch_add(1,std::memory_order_relaxed);return;}
    if(i==P_stop){if(v>.5f){s.stopFire.store(s.fireTriggers.load(std::memory_order_relaxed),std::memory_order_relaxed);s.stopNote.store(s.noteTriggers.load(std::memory_order_relaxed),std::memory_order_relaxed);s.p[P_latch].store(0,std::memory_order_relaxed);for(auto& n:s.notes)n.store(0,std::memory_order_relaxed);s.stopTriggers.fetch_add(1,std::memory_order_release);}return;}
    s.p[i].store(v,std::memory_order_relaxed);
    if(i==P_preset)s.preset(int(v));
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
    if(fireEvent)s.pulse=int(Fs*(s.t[P_mode]>.5f ? std::max(.25f,std::max(ZapTime,s.t[P_attack])) : .25f));
    const bool midiHeld=s.noteGate();
    auto& fx=s.fx;fx.time=s.t[P_delay_time];fx.feedback=s.t[P_feedback]*.01f;fx.mix=s.t[P_delay_mix]*.01f;fx.ping=s.t[P_ping]*.01f;
    for(int done=0;done<frames;){
        if(s.tick%Chunk==0){
            for(auto i:discreteControls)s.c[i]=s.t[i];
            for(auto i:smoothControls){
                const float next=s.c[i]+.0179784f*(s.t[i]-s.c[i]);
                s.c[i]=(next==s.c[i] || std::fabs(next-s.t[i])<1e-8f) ? s.t[i] : next;
            }
            s.control();s.coefficients();
        }
        const int n=std::min(frames-done,int(Chunk-s.tick%Chunk));
        s.chunk(left+done,right+done,n,retrigger,midiHeld);
        retrigger=false;done+=n;s.tick+=unsigned(n);
    }
}
}
