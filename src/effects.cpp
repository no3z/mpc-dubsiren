// Native port of the BARZINE Siren Deck echo topology (CLEAN character).
// Based on BARZINE Siren Deck; source credits are in NOTICE.md.
#include "effects.h"
#include "dsp_core.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace dub {
namespace {
constexpr float MaxSeconds=3.f;
// Web Audio breaks the delay feedback cycle with one render quantum: the first repeat lands
// at T, but every pass through the feedback path takes 128 samples more (measured on
// Chromium: repeat k arrives at k*T+(k-1)*128 samples).
constexpr float LoopLatency=128.f;
float bounded(float x,float lo,float hi,float fallback=0.f) noexcept {
    return std::isfinite(x) ? std::max(lo,std::min(hi,x)) : fallback;
}
// Linear per-sample ramp toward a one-pole endpoint chosen every 16 samples.
struct Slew {
    float value,increment=0,end;
    Slew(float initial):value(initial),end(initial){}
    operator float() const noexcept{return value;}
    bool steady() const noexcept{return increment==0.f && end==value;}
    void target(float next,float a) noexcept {
        end=value+a*(next-value);
        if(end==value || std::fabs(end-next)<1e-9f)end=next;
        increment=(end-value)*(1.f/Chunk);
    }
    // Fill n ramp values, then repeat the last one into the padding lanes.
    // The sample that closes the control interval lands exactly on the endpoint.
    void ramp(float* out,int n,int padded,bool closes) noexcept {
        float v=value;
        for(int j=0;j<n;++j){v+=increment;out[j]=v;}
        if(closes)out[n-1]=end;
        value=out[n-1];
        for(int j=n;j<padded;++j)out[j]=value;
    }
    // Same endpoint without per-sample values, for controls used once per chunk.
    void advance(int n,bool closes) noexcept {value=closes ? end : value+float(n)*increment;}
};
float coefficient(float seconds) noexcept {return 1.f-std::exp(-float(Chunk)/(Fs*seconds));}

// Feedback-loop filter with the Web Audio Q=.707 dB property (linear Q 1.0848).
struct LoopFilter {
    float b0,b1,b2,a1,a2,z1=0.f,z2=0.f;
    LoopFilter(float hz,bool highpass) noexcept {
        const float w=2.f*float(Pi)*hz/Fs,alpha=std::sin(w)/(2.f*1.084809f),c=std::cos(w);
        const float den=1.f+alpha,num=highpass ? 1.f+c : 1.f-c;
        b0=b2=.5f*num/den;b1=(highpass ? -num : num)/den;a1=-2.f*c/den;a2=(1.f-alpha)/den;
    }
    float process(float x) noexcept {
        const float y=b0*x+z1;
        z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
        if(!(std::fabs(y)<=32.f)){reset();return 0.f;}
        return y;
    }
    void reset() noexcept{z1=z2=0.f;}
    void scrub() noexcept{if(std::fabs(z1)<1e-20f)z1=0.f;if(std::fabs(z2)<1e-20f)z2=0.f;}
};
}

struct Effects::Impl {
    // Reset is constant-time: `valid` counts writes since reset, and reads
    // older than that return silence without clearing the buffer.
    std::vector<float> ring=std::vector<float>(std::size_t(Fs*MaxSeconds+LoopLatency)+128,0.f);
    std::size_t write=0,valid=0,quiet=0;
    LoopFilter hp{120.f,true},lp{7600.f,false};
    Slew time=.4166667f,feedback=.42f,wet=.62f,ping=0.f,panRate=1.2f;
    unsigned count=0;
    float panPhase=0.f,cachedPing=0.f,near=1.f,far=1.f;
    const float a18=coefficient(.018f),a20=coefficient(.02f),a25=coefficient(.025f);
    const float a50=coefficient(.05f),a80=coefficient(.08f);

    void reset() noexcept {
        write=valid=quiet=0;hp.reset();lp.reset();panPhase=0.f;count=0;
    }
    void controls(const EchoConfig& config) noexcept {
        const float userTime=bounded(config.time,.05f,MaxSeconds,.4166667f);
        time.target(userTime,a50);
        feedback.target(bounded(config.feedback,0.f,.88f,.42f),a20);
        wet.target(.62f*bounded(config.mix,0.f,1.f,1.f),a18);
        ping.target(bounded(config.ping,0.f,1.f),a25);
        panRate.target(bounded(1.f/(2.f*userTime),.05f,10.f),a80);
    }
    float read(float delaySamples,std::size_t head,std::size_t filled) const noexcept {
        const std::size_t size=ring.size();
        const float delay=std::max(1.f,std::min(float(size-2),delaySamples));
        const auto integral=std::size_t(delay);
        const float fraction=delay-float(integral);
        const auto one=head>=integral ? head-integral : head+size-integral;
        const auto two=one ? one-1 : size-1;
        const float x=integral<=filled ? ring[one] : 0.f;
        const float y=integral+1<=filled ? ring[two] : 0.f;
        return x+fraction*(y-x);
    }
    void process(const float* input,float* left,float* right,int n,const EchoConfig& config) noexcept {
        using namespace simd;
        const int vectors=(n+3)/4;
        const bool closes=(count+unsigned(n))%Chunk==0;
        if(count%Chunk==0)controls(config);
        const bool steadyTime=time.steady();
        alignas(16) float dry[Chunk],times[Chunk],gains[Chunk],levels[Chunk],delayed[Chunk],loop[Chunk];
        const int padded=4*vectors;
        time.ramp(times,n,padded,closes);feedback.ramp(gains,n,padded,closes);wet.ramp(levels,n,padded,closes);
        ping.advance(n,closes);panRate.advance(n,closes);
        count+=unsigned(n);
        float loudest=0.f;
        for(int v=0;v<vectors;++v){
            const vf x=clamp(finite(load(input+4*v)),-8.f,8.f);
            store(dry+4*v,x);loudest=std::max(loudest,hmax(abs(x)));
        }
        const float increment=panRate/Fs;
        if(loudest==0.f && quiet>=ring.size()){
            // Every ring slot is zero and nothing enters: the output is exact silence.
            std::fill_n(left,4*vectors,0.f);std::fill_n(right,4*vectors,0.f);
            panPhase+=float(n)*increment;if(panPhase>=1.f)panPhase-=1.f;
            return;
        }
        // The shortest delay (2205 samples) exceeds a chunk, so every read
        // precedes this chunk's writes. Two taps: the wet output at T and the
        // feedback source LoopLatency samples further back.
        const std::size_t size=ring.size();
        alignas(16) float tapped[Chunk];
        const auto fetch=[&](float extra,float* out) noexcept {
            bool contiguous=false;
            if(steadyTime){
                const float delay=std::max(1.f,std::min(float(size-2),times[0]*Fs+extra));
                const auto integral=std::size_t(delay);
                const auto base=write>=integral ? write-integral : write+size-integral;
                if(integral+1<=valid && base>=1 && base+4*vectors<=size){
                    contiguous=true;
                    const vf fraction=splat(delay-float(integral));
                    for(int v=0;v<vectors;++v){
                        const vf x=load(&ring[base+4*v]),y=load(&ring[base-1+4*v]);
                        store(out+4*v,clamp(madd(x,fraction,sub(y,x)),-16.f,16.f));
                    }
                }
            }
            if(!contiguous){
                for(int j=0;j<n;++j){
                    std::size_t head=write+j;if(head>=size)head-=size;
                    out[j]=std::max(-16.f,std::min(16.f,read(times[j]*Fs+extra,head,std::min(size,valid+j))));
                }
                for(int j=n;j<4*vectors;++j)out[j]=0.f;
            }
        };
        fetch(0.f,delayed);fetch(LoopLatency,tapped);
        // The first repeat is unfiltered; later repeats pass HP then LP.
        for(int j=0;j<n;++j)loop[j]=lp.process(hp.process(tapped[j]));
        for(int j=n;j<4*vectors;++j)loop[j]=0.f;
        hp.scrub();lp.scrub();
        bool silentWrite=true;
        for(int v=0;v<vectors;++v){
            const vf w=guard(madd(load(dry+4*v),load(gains+4*v),load(loop+4*v)),16.f);
            store(loop+4*v,w);silentWrite=silentWrite && hmax(abs(w))==0.f;
        }
        const std::size_t first=std::min(std::size_t(n),size-write);
        std::memcpy(&ring[write],loop,first*sizeof(float));
        std::memcpy(&ring[0],loop+first,(std::size_t(n)-first)*sizeof(float));
        write=(write+std::size_t(n))%size;valid=std::min(size,valid+std::size_t(n));
        quiet=silentWrite ? quiet+std::size_t(n) : 0;
        // Ping-pong: square LFO at 1/(2*delay) between equal-power positions
        // +ping and -ping (Web Audio stereo panner on an upmixed mono wet).
        if(ping!=cachedPing){
            cachedPing=ping;const float angle=ping*float(Pi)*.5f;
            near=std::cos(angle);far=1.f+std::sin(angle);
        }
        const vf nearGain=splat(near),farGain=splat(far),half=splat(.5f);
        alignas(16) static const float lanes[4]={0.f,1.f,2.f,3.f};
        vf phase=madd(splat(panPhase),load(lanes),splat(increment));
        const vf step=splat(4.f*increment);
        for(int v=0;v<vectors;++v){
            // First half of the LFO cycle pans toward +ping (right).
            const vm toRight=lt(wrap(phase),half);
            const vf x=load(dry+4*v),w=mul(load(delayed+4*v),load(levels+4*v));
            store(left+4*v,clamp(madd(x,w,select(toRight,nearGain,farGain)),-32.f,32.f));
            store(right+4*v,clamp(madd(x,w,select(toRight,farGain,nearGain)),-32.f,32.f));
            phase=add(phase,step);
        }
        panPhase+=float(n)*increment;if(panPhase>=1.f)panPhase-=1.f;
    }
};

Effects::Effects():impl(new Impl){}
Effects::~Effects(){delete impl;}
void Effects::process(const float* dry,float* left,float* right,int n,const EchoConfig& config) noexcept {
    impl->process(dry,left,right,n,config);
}
void Effects::reset() noexcept {impl->reset();}
}
