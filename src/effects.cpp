// Native port of BARZINE Siren Deck signal topology and parameter mappings.
// Based on BARZINE Siren Deck; source credits are in NOTICE.md.
// Fractional delays, guards and algorithmic reverb are original native code.
#include "effects.h"
#include "fast_trig.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dub {
namespace {
constexpr float Fs=44100.f, Pi=3.14159265358979323846f;
#ifndef DUB_FX_CONTROL_INTERVAL
#define DUB_FX_CONTROL_INTERVAL 16
#endif
constexpr unsigned ControlInterval=DUB_FX_CONTROL_INTERVAL;
static_assert(ControlInterval==8 || ControlInterval==16 || ControlInterval==32);
// Internal finite-by-construction paths retain their amplitude bounds without
// repeating the nonfinite test performed at the external/signal boundaries.
float limited(float x,float lo,float hi) noexcept {return std::max(lo,std::min(hi,x));}
float bounded(float x,float lo,float hi,float fallback=0.f) noexcept {
    return std::isfinite(x) ? std::max(lo,std::min(hi,x)) : fallback;
}
float clean(float x) noexcept {
    return std::isfinite(x) && std::fabs(x)>1e-20f ? x : 0.f;
}
// Ring/reverb values are finite by construction: all external inputs and comb
// feedback writes are bounded, and the two allpasses have stable gain .5.
// Keep denormal suppression here; nonfinite guards remain at public/signal bounds.
float denormal(float x) noexcept {return std::fabs(x)>1e-20f ? x : 0.f;}
struct Slew {
    float value,increment=0,end;
    Slew(float initial):value(initial),end(initial){}
    operator float() const noexcept{return value;}
};
void target(Slew& state,float next,float a) noexcept {
    state.end=state.value+a*(next-state.value);
    if(state.end==state.value || std::fabs(state.end-next)<1e-9f)state.end=next;
    state.increment=(state.end-state.value)*(1.f/ControlInterval);
}
float coefficient(float seconds) noexcept { return 1.f-std::exp(-float(ControlInterval)/(Fs*seconds)); }

// Reset is constant-time. Distance from the write head tells whether a slot
// has been written since reset; no per-slot generation tags are needed.
struct Ring {
    std::vector<float> samples;
    std::size_t write=0,valid=0;
    explicit Ring(std::size_t size):samples(size,0.f){}
    float read(float delaySamples) const noexcept {
        const float delay=limited(delaySamples,1.f,float(samples.size()-2));
        const auto integral=std::size_t(delay);
        const float fraction=delay-float(integral);
        const auto one=write>=integral ? write-integral : write+samples.size()-integral;
        const auto two=one ? one-1 : samples.size()-1;
        const float x=integral<=valid ? samples[one] : 0.f;
        const float y=integral+1<=valid ? samples[two] : 0.f;
        return x+fraction*(y-x);
    }
    void push(float value) noexcept {
        samples[write]=denormal(value);
        if(valid<samples.size())++valid;
        if(++write==samples.size())write=0;
    }
    void reset() noexcept {valid=write=0;}
};

// A fixed integer delay reads at the write head. Unlike fractional taps it
// needs no second index or wrap calculation. Reset still invalidates in O(1).
struct FixedRing {
    std::vector<float> samples;
    std::size_t write=0,valid=0;
    explicit FixedRing(std::size_t size):samples(size,0.f){}
    // Every comb/allpass pairs exactly one read with one push. Count filling
    // here, so push needs no second validity comparison on each audio sample.
    float read() noexcept {
        if(valid<samples.size()){++valid;return 0.f;}
        return samples[write];
    }
    void push(float value) noexcept {
        samples[write]=denormal(value);
        if(++write==samples.size())write=0;
    }
    void reset() noexcept {write=valid=0;}
};

struct Biquad {
    float b0=1.f,b1=0.f,b2=0.f,a1=0.f,a2=0.f,z1=0.f,z2=0.f;
    float previousFrequency=-1; bool previousHighpass=false;
    void configure(float frequency,bool highpass) noexcept {
        if(frequency==previousFrequency && highpass==previousHighpass)return;
        previousFrequency=frequency;previousHighpass=highpass;
        const float w=2.f*Pi*bounded(frequency,20.f,Fs*.45f,1000.f)/Fs;
        // Web Audio lowpass/highpass Q=.707 is a dB property, not linear Q.
        const float alpha=std::sin(w)/(2.f*1.084809f), c=std::cos(w);
        const float denominator=1.f+alpha;
        const float numerator=highpass ? 1.f+c : 1.f-c;
        b0=.5f*numerator/denominator; b2=b0;
        b1=(highpass ? -numerator : numerator)/denominator;
        a1=-2.f*c/denominator; a2=(1.f-alpha)/denominator;
    }
    float process(float x) noexcept {
        const float y=b0*x+z1;
        z1=clean(b1*x-a1*y+z2); z2=clean(b2*x-a2*y);
        if (!std::isfinite(y) || std::fabs(y)>32.f) { reset(); return 0.f; }
        return clean(y);
    }
    void reset() noexcept { z1=0.f; z2=0.f; }
};

struct Character {
    float wet,drive,modDepth,modRate,panBase,spread,spreadTime,spreadPan;
    float flange,flangeTime,flangeDepth,flangeRate,noise,duck,wander,filterSweep;
};
// Time/fb/rev/HP/LP/ping are user parameters; the engine applies character
// defaults at selection. These values describe the continuing DSP character.
constexpr Character Characters[5]={
    {.62f,0.f,0.f,.2f,-.28f,.28f,.012f,.62f,0.f,.004f,0.f,.15f,0.f,.58f,0.f,0.f},
    {.62f,.28f,0.f,.2f,0.f,0.f,.008f,.6f,0.f,.004f,0.f,.15f,0.f,0.f,0.f,0.f},
    {.7f,.48f,.007f,.34f,0.f,.07f,.018f,.55f,.035f,.005f,.0012f,.11f,.0015f,0.f,.004f,300.f},
    {.72f,.1f,.0015f,.17f,0.f,.12f,.013f,.7f,.16f,.004f,.003f,.19f,0.f,0.f,0.f,1800.f},
    {.88f,.3f,0.f,.2f,0.f,0.f,.008f,.6f,0.f,.004f,0.f,.15f,0.f,0.f,0.f,350.f}
};

struct Stereo { float left,right; };
struct Panner {
    float previous=99,cosine=1,sine=0;
    void update(float pan,bool stereo) noexcept {
        if(pan==previous)return;
        previous=pan;
        const float angle=stereo ? (pan<=0 ? pan+1 : pan)*Pi*.5f : (pan+1)*Pi*.25f;
        const float phase=angle/(2*Pi);
        sine=modulationSine(phase);cosine=modulationSine(phase+.25f);
    }
    Stereo mono(float input,float pan) noexcept {
        pan=bounded(pan,-1.f,1.f);update(pan,false);
        return {input*cosine,input*sine};
    }
    Stereo stereo(Stereo input,float pan) noexcept {
        pan=bounded(pan,-1.f,1.f);update(pan,true);
        return pan<=0 ? Stereo{input.left+input.right*cosine,input.right*sine}
                      : Stereo{input.left*cosine,input.right+input.left*sine};
    }
};

struct Comb {
    FixedRing ring;
    float gain,damped=0.f;
    explicit Comb(int length):ring(std::size_t(length)),gain(std::pow(.001f,float(length)/Fs/1.8f)){}
    float process(float x) noexcept {
        const float y=ring.read();
        damped=denormal(.75f*y+.25f*damped);
        ring.push(limited(x+gain*damped,-8.f,8.f));
        return y;
    }
    void reset() noexcept { ring.reset(); damped=0.f; }
};
struct Allpass {
    FixedRing ring;
    explicit Allpass(int length):ring(std::size_t(length)){}
    float process(float x) noexcept {
        const float z=ring.read();
        const float output=denormal(z-.5f*x);
        ring.push(x+.5f*output);
        return output;
    }
    void reset() noexcept { ring.reset(); }
};

struct Reverb {
    bool pristine=true;
    std::array<Comb,4> left{{Comb(1557),Comb(1617),Comb(1491),Comb(1422)}};
    std::array<Comb,4> right{{Comb(1580),Comb(1640),Comb(1514),Comb(1445)}};
    std::array<Allpass,2> apLeft{{Allpass(225),Allpass(556)}};
    std::array<Allpass,2> apRight{{Allpass(248),Allpass(579)}};
    Stereo process(float x) noexcept {
        // Zero input into zero history is exactly zero. Once excited, always
        // run the network, including when its wet gain is zero, to keep tails.
        if(pristine){if(x==0.f)return {0.f,0.f};pristine=false;}
        Stereo y{0.f,0.f};
        for (auto& comb:left) y.left+=comb.process(x*.22f);
        for (auto& comb:right) y.right+=comb.process(x*.22f);
        for (auto& ap:apLeft) y.left=ap.process(y.left);
        for (auto& ap:apRight) y.right=ap.process(y.right);
        return y;
    }
    void reset() noexcept {
        pristine=true;
        for (auto& comb:left) comb.reset();
        for (auto& comb:right) comb.reset();
        for (auto& ap:apLeft) ap.reset();
        for (auto& ap:apRight) ap.reset();
    }
};
}

struct Effects::Impl {
    Ring delay{std::size_t(Fs*5.2f)+8}, spread{std::size_t(Fs*.05f)+8}, flange{std::size_t(Fs*.03f)+8};
    Biquad hpFilter,lpFilter;
    Reverb verb;
    Panner monoPan,stereoPan;
    std::array<float,33> drift{};
    std::uint32_t random=0x7a89c65du;
    unsigned count=0;
    float phase=0.f,panPhase=0.f,flangePhase=0.f,driftPhase=0.f;
    Slew time=.4166667f,feedback=.42f,send=1.f,hp=120.f,lp=7600.f;
    Slew wet=.62f,spreadGain=.28f,reverbGain=.16f,ping=0.f,panBase=-.28f;
    Slew modRate=.2f,modDepth=0.f,wander=0.f,filterSweep=0.f,panRate=1.2f;
    Slew spreadTime=.012f,spreadPan=.62f,flangeTime=.004f,flangeDepth=0.f,flangeRate=.15f,flangeGain=0.f,tapeNoise=0.f;
    float drive=0.f;
    const float a8=coefficient(.008f),a12=coefficient(.012f),a15=coefficient(.015f);
    const float a18=coefficient(.018f),a20=coefficient(.02f),a25=coefficient(.025f);
    const float a40=coefficient(.04f),a50=coefficient(.05f),a80=coefficient(.08f);
    const float a120=coefficient(.12f),a140=coefficient(.14f),a180=coefficient(.18f);
    bool wasFreeze=false,wasBend=false,freezeFeedbackRelease=false,freezeSendRelease=false;
    unsigned sendRelease=0;

    float noise() noexcept {
        random^=random<<13; random^=random>>17; random^=random<<5;
        return float(random>>8)*(2.f/16777215.f)-1.f;
    }
    Impl() {
        for (unsigned i=0;i<32;++i) drift[i]=noise();
        drift[32]=drift[0];
        hpFilter.configure(hp,true); lpFilter.configure(lp,false);
    }
    void reset() noexcept {
        delay.reset();spread.reset();flange.reset();verb.reset();
        hpFilter.reset();lpFilter.reset();
        phase=panPhase=flangePhase=driftPhase=0.f;
        count=sendRelease=0; wasFreeze=wasBend=freezeFeedbackRelease=freezeSendRelease=false;
    }
    float oscillator(float& p,float hz,bool square=false,bool audible=true) noexcept {
        const float result=!audible ? 0.f : square ? (p<.5f?1.f:-1.f) : modulationSine(p);
        p+=hz/Fs;
        if (p>=1.f) p-=1.f;
        return result;
    }
    // Parameter bounds, character choices and slew endpoints run at control rate.
    // Only the linear ramp and signal processing run per sample.
    void controls(const EchoConfig& config) noexcept {
        const int index=std::max(0,std::min(4,config.character));
        const Character& c=Characters[index];
        const float userTime=bounded(config.time,.05f,3.f,.4166667f);
        const float targetTime=config.bend ? std::min(5.2f,1.72f*userTime) : userTime;
        target(time,targetTime,config.bend ? a120 : (wasBend?a180:a50));
        // Keep the release time constant for the complete bend transition.
        if (std::fabs(time-targetTime)<.00001f) wasBend=config.bend;
        if (config.bend) wasBend=true;
        const float userFb=bounded(config.feedback,0.f,.88f,.42f);
        if (wasFreeze && !config.freeze) {
            sendRelease=unsigned(Fs*.04f);
            freezeFeedbackRelease=freezeSendRelease=true;
        }
        if (config.freeze) {
            sendRelease=0;freezeFeedbackRelease=freezeSendRelease=false;
        }
        target(feedback,config.freeze ? std::min(.96f,std::max(.955f,userFb)) : userFb,config.freeze?a40:(freezeFeedbackRelease?a50:a20));
        if (!config.freeze&&std::fabs(feedback-userFb)<.00001f) freezeFeedbackRelease=false;
        wasFreeze=config.freeze;
        const float targetSend=config.freeze || (index==4&&!config.playing) ? .0001f : 1.f;
        if (sendRelease>0) target(send,float(send),a12);
        else target(send,targetSend,config.freeze?a15:(freezeSendRelease?a20:a12));
        if (std::fabs(send-targetSend)<.00001f) freezeSendRelease=false;
        target(hp,bounded(config.hp,40.f,1200.f,120.f),a25);
        target(lp,bounded(config.lp,800.f,12000.f,7600.f),a25);
        const float mix=bounded(config.mix,0.f,1.f,1.f);
        const float polarity=config.invert ? -1.f : 1.f;
        const float duck=config.playing ? 1.f-c.duck : 1.f;
        target(wet,c.wet*mix*duck*polarity,config.playing?a18:a140);
        target(spreadGain,c.spread*mix*duck*polarity,config.playing?a18:a140);
        target(reverbGain,bounded(config.reverb,0.f,1.f,.16f)*(config.playing?1.f-.65f*c.duck:1.f),config.playing?a25:a180);
        target(ping,bounded(config.ping,0.f,1.f),a25);
        target(panBase,config.ping>.0001f?0.f:c.panBase,a25);
        target(panRate,bounded(1.f/(2.f*targetTime),.05f,10.f),a80);
        target(modRate,c.modRate,a40); target(modDepth,c.modDepth,a40);
        target(wander,c.wander,a40); target(filterSweep,c.filterSweep,a40);
        target(spreadTime,c.spreadTime,a40); target(spreadPan,c.spreadPan,a40);
        target(flangeTime,c.flangeTime,a40); target(flangeDepth,c.flangeDepth,a40);
        target(flangeRate,c.flangeRate,a40); target(flangeGain,c.flange,a40);
        target(tapeNoise,c.noise,a40);
        // Character drive follows selection at the next control boundary.
        drive=c.drive;
    }
    template<bool Final> void advance() noexcept {
        const auto step=[](Slew& s){s.value=Final ? s.end : s.value+s.increment;};
        step(time);
        step(feedback);
        step(send);
        step(hp);
        step(lp);
        step(wet);
        step(spreadGain);
        step(reverbGain);
        step(ping);
        step(panBase);
        step(panRate);
        step(modRate);
        step(modDepth);
        step(wander);
        step(filterSweep);
        step(spreadTime);
        step(spreadPan);
        step(flangeTime);
        step(flangeDepth);
        step(flangeRate);
        step(flangeGain);
        step(tapeNoise);
    }
    void process(float dry,const EchoConfig& config,float& l,float& r) noexcept {
        dry=bounded(dry,-8.f,8.f);
        if((count&(ControlInterval-1))==0)controls(config);
        if((count&(ControlInterval-1))==ControlInterval-1)advance<true>();else advance<false>();
        if(sendRelease>0)--sendRelease;
        const float modulation=oscillator(phase,modRate,false,modDepth!=0.f || filterSweep!=0.f);
        float driftValue=0.f;
        if(wander!=0.f){
            const auto segment=unsigned(driftPhase);
            const float position=driftPhase-float(segment);
            const float eased=position*position*(3.f-2.f*position);
            driftValue=drift[segment]+eased*(drift[segment+1]-drift[segment]);
        }
        driftPhase+=4.f/Fs;
        if (driftPhase>=32.f) driftPhase-=32.f;
        const float delayed=bounded(delay.read((time+modDepth*modulation+wander*driftValue)*Fs),-16.f,16.f);
        if ((count++&31u)==0) {
            hpFilter.configure(hp,true);
            lpFilter.configure(lp+filterSweep*modulation,false);
        }
        const float filtered=lpFilter.process(hpFilter.process(delayed));
        float saturation=filtered;
        if (drive>0.f) {
            const float limited=bounded(filtered,-1.f,1.f);
            saturation=limited/(1.f+1.8f*drive*std::fabs(limited));
        }
        // Feedback-only guard: the output limiter cannot protect internal state.
        delay.push(bounded(send*(dry+tapeNoise*noise())+feedback*saturation,-16.f,16.f));
        const float spreadTap=spreadGain!=0.f ? spread.read(spreadTime*Fs) : 0.f;
        spread.push(delayed);
        // The always-connected spread panner makes wetPanInput stereo. Its mono
        // main delay input is upmixed equally into both channels before panning.
        Stereo output{delayed*wet,delayed*wet};
        const Stereo side=monoPan.mono(spreadTap*spreadGain,spreadPan);
        output.left+=side.left; output.right+=side.right;
        const float pan=panBase+ping*oscillator(panPhase,panRate,true);
        output=stereoPan.stereo(output,pan);
        const float flangeMod=oscillator(flangePhase,flangeRate,false,flangeDepth!=0.f && flangeGain!=0.f);
        const float flangeTap=flangeGain!=0.f ? flange.read((flangeTime+flangeDepth*flangeMod)*Fs) : 0.f;
        flange.push(dry);
        const Stereo reverberation=verb.process(bounded(dry+delayed,-8.f,8.f));
        l=clean(dry+output.left+flangeTap*flangeGain+reverberation.left*reverbGain);
        r=clean(dry+output.right+flangeTap*flangeGain+reverberation.right*reverbGain);
        l=limited(l,-32.f,32.f);r=limited(r,-32.f,32.f);
    }
};

Effects::Effects():impl(new Impl){}
Effects::~Effects(){delete impl;}
void Effects::process(float dry,const EchoConfig& config,float& l,float& r) noexcept {
    impl->process(dry,config,l,r);
}
void Effects::reset() noexcept {impl->reset();}
}
