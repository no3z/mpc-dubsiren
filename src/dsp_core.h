#pragma once
#include "simd.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace dub {
constexpr double Pi=3.14159265358979323846;
constexpr float Fs=44100.f;
// Control interval and largest processing chunk. Every stage works on
// chunks of at most this many samples, aligned to a global 16-sample grid.
constexpr int Chunk=16;
inline float bound(float x,float lo,float hi) noexcept{return std::max(lo,std::min(hi,x));}
inline double frac(double x) noexcept{return x>=0 && x<1 ? x : x>=1 && x<2 ? x-1 : x<0 && x>=-1 ? x+1 : x-std::floor(x);}

namespace simd {
// PolyBLEP/PolyBLAMP residuals for phase t in [0,1), increment dt in [0,.4].
// inv is 1/dt (+inf when dt is zero; those lanes select zero).
inline vf blep(vf t,vf dt,vf inv) noexcept{
    const vf one=splat(1.f);
    const vf u=mul(t,inv),v=mul(sub(t,one),inv);
    const vf head=sub(sub(add(u,u),mul(u,u)),one);
    const vf tail=add(add(madd(v,v,v),v),one);
    return select(lt(t,dt),head,select(gt(t,sub(one,dt)),tail,splat(0.f)));
}
inline vf blamp(vf t,vf dt,vf inv) noexcept{
    const vf one=splat(1.f),third=splat(1.f/3);
    const vf u=sub(one,mul(t,inv)),v=madd(one,sub(t,one),inv);
    const vf head=mul(mul(mul(u,u),u),third),tail=mul(mul(mul(v,v),v),third);
    return select(lt(t,dt),head,select(gt(t,sub(one,dt)),tail,splat(0.f)));
}
// Chromium normalises its band-limited saw and square tables by the Gibbs peak,
// so the BARZINE page's flat parts sit below +-1. Measured on the OscillatorNode:
// RMS ratio to an ideal +-1 wave is 0.848 at 30 Hz, 0.844 at 440 Hz, 0.830 at 1.76 kHz
// (saw 0.848 / 0.842 / 0.823). One constant keeps level within +-1 % over the range.
constexpr float PageEdgeGain=.84f;
// Band-limited oscillator: 0 sine, 1 triangle, 2 saw, 3 square.
inline vf oscillator(int wave,vf p,vf dt) noexcept{
    if(wave==0)return sin2pi(p);
    const vf one=splat(1.f),half=splat(.5f),inv=recip(dt);
    const vf q=wrap(add(p,half));
    if(wave==1){
        const vf tri=sub(one,mul(splat(4.f),abs(sub(p,half))));
        return madd(tri,mul(splat(4.f),dt),sub(blamp(p,dt,inv),blamp(q,dt,inv)));
    }
    if(wave==2)return mul(splat(PageEdgeGain),sub(sub(add(p,p),one),blep(p,dt,inv)));
    return mul(splat(PageEdgeGain),add(select(lt(p,half),one,splat(-1.f)),sub(blep(p,dt,inv),blep(q,dt,inv))));
}
// LFO shapes: 0 triangle, 1 square, 2 saw, 3 sine.
inline vf lfo(int shape,vf p) noexcept{
    const vf one=splat(1.f),half=splat(.5f);
    if(shape==0)return sub(one,mul(splat(4.f),abs(sub(wrap(add(p,splat(.25f))),half))));
    if(shape==1)return select(lt(p,half),one,splat(-1.f));
    if(shape==2)return sub(add(p,p),one);
    return sin2pi(p);
}
}

namespace simd {
// Output bound. The Web Audio limiter this master chain follows uses a 6 ms look-ahead; ours
// has none (no latency), so on bass-heavy presets peaks overshoot to ~+4 dB for a fraction
// of the samples. Identity up to 0.9, then a C1 knee asymptotic to 0.98 instead of a hard
// clip, which would crackle on those peaks. Never exceeds 0.98.
inline vf softBound(vf x) noexcept{
    const vf a=abs(x),u=mul(sub(a,splat(.9f)),splat(12.5f));        // 12.5 = 1/0.08
    const vf y=madd(splat(.9f),mul(u,recip(add(splat(1.f),u))),splat(.08f));
    const vf signedY=select(lt(x,splat(0.f)),sub(splat(0.f),y),y);
    return select(gt(a,splat(.9f)),signedY,x);
}
}

// Resonant 12 dB lowpass (RBJ, Q in dB). Double precision state and
// coefficients keep low cutoffs with high resonance clean; the loop is serial.
struct Lowpass {
    double b0=1,b1=0,a1=0,a2=0,z1=0,z2=0;
    float cachedHz=-1,cachedQ=-1;
    void set(float hz,float qDb) noexcept{
        if(hz==cachedHz && qDb==cachedQ)return;
        cachedHz=hz;cachedQ=qDb;
        const double w=2*Pi*bound(hz,20,Fs*.45f)/Fs,c=std::cos(w),s=std::sin(w);
        const double q=std::exp(bound(qDb,0,20)*(2.302585092994045684/20)),alpha=s/(2*q),a0=1+alpha;
        a1=-2*c/a0;a2=(1-alpha)/a0;b0=(1-c)/(2*a0);b1=2*b0;
    }
    void reset() noexcept{z1=z2=0;}
    bool silent() const noexcept{return z1==0 && z2==0;}
    // Bounded coefficients, float input and |y|<=100 keep the double state
    // finite by induction; the comparison also rejects NaN/Inf.
    float process(float in) noexcept{
        const double y=b0*in+z1;z1=b1*in-a1*y+z2;z2=b0*in-a2*y;
        if(!(std::fabs(y)<=100)){z1=z2=0;return 0;}
        return float(y);
    }
    void scrub() noexcept{if(std::fabs(z1)<1e-15)z1=0;if(std::fabs(z2)<1e-15)z2=0;}
};

/*
 * Compressor below: algorithm and constants ported from Chromium's Blink DynamicsCompressor.
 * Original license notice:
 *
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// Web Audio DynamicsCompressorNode, ported from Chromium's Blink
// DynamicsCompressor (BSD-3-Clause, notice above and in NOTICE.md). The BARZINE page uses this node as its master
// compressor and output limiter, and its curve is much gentler than threshold,
// knee and ratio suggest: the knee extends ABOVE the threshold, the output has an
// automatic makeup gain of (1/curve(1.0))^0.6, and release adapts to the amount of
// compression. Differences from the node: no 6 ms look-ahead (the instrument adds
// no latency) and a start state already settled on silence.
// Detection and the shaping curve run four lanes at a time; the detector and gain
// smoothing are serial. Division length (32 samples) and constants follow Blink.
struct Compressor {
    static constexpr float PiOver2=1.57079632679489662f,Db=6.02059991f; // Db = 20*log10(2)
    float linThr,kneeThr,dbKneeThr,dbYKnee,slope,k,postGain;
    float satFrames,attackFrames,ra,rb,rc,rd,re;
    float det=1,gain=1,maxAttack=-1,envRate=0,desired=1;
    int phase=0;

    static float toLin(float db) noexcept{return std::pow(10.f,db/20.f);}
    static float toDb(float x) noexcept{return 20.f*std::log10(x);}
    static float kneeCurve(float x,float lin,float kk) noexcept{return x<lin ? x : lin+(1.f-std::exp(-kk*(x-lin)))/kk;}
    float saturate(float x) const noexcept{
        if(x<kneeThr)return kneeCurve(x,linThr,k);
        return toLin(dbYKnee+slope*(toDb(x)-dbKneeThr));
    }
    Compressor(float thresholdDb,float kneeDb,float ratio,float releaseSeconds,float attackSeconds=.003f) noexcept{
        linThr=toLin(thresholdDb);slope=1.f/ratio;
        // k: exponential knee whose end slope is 1/ratio (Blink KAtSlope).
        const float dbX=thresholdDb+kneeDb,x=toLin(dbX);
        float x2=1,dbX2=0;
        if(!(x<linThr)){x2=x*1.001f;dbX2=toDb(x2);}
        float minK=.1f,maxK=10000.f,kk=5.f,s=1.f;
        for(int i=0;i<15;++i){
            if(!(x<linThr))s=(toDb(kneeCurve(x2,linThr,kk))-toDb(kneeCurve(x,linThr,kk)))/(dbX2-dbX);
            if(s<slope)maxK=kk;else minK=kk;
            kk=std::sqrt(minK*maxK);
        }
        k=kk;dbKneeThr=dbX;kneeThr=x;dbYKnee=toDb(kneeCurve(kneeThr,linThr,k));
        postGain=std::pow(1.f/saturate(1.f),.6f);
        satFrames=.0025f*Fs;attackFrames=std::max(.001f,attackSeconds)*Fs;
        // Adaptive release: quartic through (0,.09) (1,.16) (2,.42) (3,.98) of the release time.
        constexpr float Z1=.09f,Z2=.16f,Z3=.42f,Z4=.98f;
        const float frames=Fs*releaseSeconds;
        ra=frames*(0.9999999999999998f*Z1+1.8432219684323923e-16f*Z2-1.9373394351676423e-16f*Z3+8.824516011816245e-18f*Z4);
        rb=frames*(-1.5788320352845888f*Z1+2.3305837032074286f*Z2-0.9141194204840429f*Z3+0.1623677525612032f*Z4);
        rc=frames*(0.5334142869106424f*Z1-1.272736789213631f*Z2+0.9258856042207512f*Z3-0.18656310191776226f*Z4);
        rd=frames*(0.08783463138207234f*Z1-0.1694162967925622f*Z2+0.08588057951595272f*Z3-0.00429891410546283f*Z4);
        re=frames*(-0.042416883008123074f*Z1+0.1115693827987602f*Z2-0.09764676325265872f*Z3+0.028494263462021576f*Z4);
    }
    void reset() noexcept{det=gain=1;maxAttack=-1;envRate=0;desired=1;phase=0;}
    // Envelope rate for the next 32 samples, from the state at their start.
    void division() noexcept{
        if(!std::isfinite(det))det=1;
        det=std::min(det,1.f);
        desired=std::asin(det)/PiOver2;
        const bool releasing=desired>gain;
        float diff=desired==0 ? (releasing ? -1.f : 1.f) : toDb(gain/desired);
        if(releasing){
            maxAttack=-1;if(!std::isfinite(diff))diff=-1;
            const float x=.25f*(bound(diff,-12.f,0.f)+12.f),x2=x*x;
            const float calc=ra+rb*x+rc*x2+rd*x2*x+re*x2*x2;
            envRate=toLin(5.f/calc);
        }else{
            if(!std::isfinite(diff))diff=1;
            if(maxAttack==-1 || maxAttack<diff)maxAttack=diff;
            envRate=1.f-std::pow(.25f/std::max(.5f,maxAttack),1.f/attackFrames);
        }
    }
    // l/r hold n <= 16 samples padded with finite values to a multiple of four.
    // A chunk never crosses a 32-sample division: chunks sit on the 16-sample grid.
    void process(float* l,float* r,int n) noexcept{
        using namespace simd;
        const int vectors=(n+3)/4;
        if(phase==0)division();
        float loudest=0;
        for(int v=0;v<vectors;++v)loudest=std::max(loudest,hmax(max(abs(load(l+4*v)),abs(load(r+4*v)))));
        const int next=(phase+n)&31;
        // Silence on a settled compressor stays silence. Float rounding stalls the detector
        // just under 1 and the pre-warp gain at ~0.9966 (output gain within 1.5e-5 of unity).
        if(loudest==0.f && det>.9999f && gain>.99f){det=gain=1;phase=next;return;}
        alignas(16) float att[Chunk],rate[Chunk],g[Chunk];
        // Below the linear threshold the curve is the identity (attenuation 1): skip the vector math.
        if(loudest<=std::max(1e-4f,linThr)){
            std::fill_n(att,4*vectors,1.f);
            std::fill_n(rate,4*vectors,std::exp2(2.f/(satFrames*Db))-1.f);
        }else{
            for(int v=0;v<vectors;++v){
                const vf x=max(abs(load(l+4*v)),abs(load(r+4*v)));
                const vf xs=max(x,splat(1e-4f)),lx=log2(xs);
                // Below the threshold the curve is linear; inside the knee it is exponential.
                const vf e=exp2(mul(sub(xs,splat(linThr)),splat(-k*1.44269504f)));
                const vf knee=mul(madd(splat(linThr),sub(splat(1.f),e),splat(1.f/k)),recip(xs));
                // Past the knee a constant ratio in dB.
                const vf dbOut=madd(splat(dbYKnee),sub(mul(lx,splat(Db)),splat(dbKneeThr)),splat(slope));
                const vf ratio=exp2(sub(mul(dbOut,splat(1.f/Db)),lx));
                vf a=select(lt(xs,splat(kneeThr)),select(lt(xs,splat(linThr)),splat(1.f),knee),ratio);
                a=min(select(gt(x,splat(1e-4f)),a,splat(1.f)),splat(1.f));
                a=max(a,splat(1e-6f));
                store(att+4*v,a);
                const vf dbAtt=max(splat(2.f),mul(log2(a),splat(-Db)));
                store(rate+4*v,sub(exp2(mul(dbAtt,splat(1.f/(satFrames*Db)))),splat(1.f)));
            }
        }
        // Serial part: detector (instant attack, 2.5 ms release) and gain smoothing.
        for(int j=0;j<n;++j){
            const float a=att[j];
            det+=(a-det)*(a>det ? rate[j] : 1.f);
            det=std::min(det,1.f);
            if(envRate<1.f)gain+=(desired-gain)*envRate;
            else gain=std::min(1.f,gain*envRate);
            g[j]=gain;
        }
        for(int j=n;j<4*vectors;++j)g[j]=gain;
        for(int v=0;v<vectors;++v){
            const vf total=mul(splat(postGain),sin2pi(mul(load(g+4*v),splat(.25f))));
            store(l+4*v,mul(load(l+4*v),total));store(r+4*v,mul(load(r+4*v),total));
        }
        if(std::fabs(det)<1e-30f)det=0;
        if(std::fabs(gain)<1e-30f)gain=0;
        phase=next;
    }
};
}
