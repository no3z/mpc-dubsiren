#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace dub {
constexpr double Pi=3.14159265358979323846;
inline float bound(float x,float lo,float hi) noexcept{return std::max(lo,std::min(hi,x));}
inline float safe(float x) noexcept{return std::isfinite(x) && std::fabs(x)>1e-20f ? bound(x,-32.f,32.f) : 0;}
inline double frac(double x) noexcept{return x>=0 && x<1 ? x : x>=1 && x<2 ? x-1 : x<0 && x>=-1 ? x+1 : x-std::floor(x);}
inline float blep(double t,double dt) noexcept {
    if(dt<=0)return 0;
    if(t<dt){t/=dt;return float(t+t-t*t-1);}
    if(t>1-dt){t=(t-1)/dt;return float(t*t+t+t+1);}
    return 0;
}
inline float blamp(double t,double dt) noexcept {
    if(dt<=0)return 0;
    if(t<dt){const double u=1-t/dt;return float(u*u*u/3);}
    if(t>1-dt){const double u=1+(t-1)/dt;return float(u*u*u/3);}
    return 0;
}
inline float wave(double p,double dt,int type) noexcept {
    switch(type){
    case 0:return (p<.5 ? 1.f : -1.f)+blep(p,dt)-blep(frac(p+.5),dt);
    case 1:return float(2*p-1)-blep(p,dt);
    case 2:return float(1-4*std::fabs(p-.5))+float(4*dt)*(blamp(p,dt)-blamp(frac(p+.5),dt));
    default:return float(std::sin(2*Pi*p));
    }
}
inline float lfoWave(double p,int type) noexcept {
    if(type==0)return float(1-4*std::fabs(frac(p+.25)-.5));
    if(type==1)return p<.5 ? 1.f : -1.f;
    if(type==2)return float(2*p-1);
    return float(std::sin(2*Pi*p));
}
struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    float cachedHz=-1,cachedQ=-1,cachedGain=999,cachedSr=-1;int cachedType=-1;
    bool unity=true;
    float eqHz=-1;double eqCos=1,eqSin=0;
    void reset() noexcept{z1=z2=0;}
    float process(float in) noexcept {
        if(unity && z1==0 && z2==0)return in;
        const double y=b0*in+z1;z1=b1*in-a1*y+z2;z2=b2*in-a2*y;
        if(!std::isfinite(y) || std::fabs(y)>100 || !std::isfinite(z1) || !std::isfinite(z2)){reset();return 0;}
        if(std::fabs(z1)<1e-25)z1=0;
        if(std::fabs(z2)<1e-25)z2=0;
        return float(y);
    }
    void tone(int type,float hz,float qDb,float sr=44100) noexcept {
        if(type==cachedType && hz==cachedHz && qDb==cachedQ && sr==cachedSr)return;
        cachedType=type;cachedHz=hz;cachedQ=qDb;cachedSr=sr;unity=false;
        const double w=2*Pi*bound(hz,20,sr*.45f)/sr,c=std::cos(w),s=std::sin(w);
        const double q=std::exp(bound(qDb,0,20)*(2.302585092994045684/20)),alpha=s/(2*q),a0=1+alpha;
        a1=-2*c/a0;a2=(1-alpha)/a0;
        if(type==1){b0=alpha/a0;b1=0;b2=-b0;}
        else {b0=(type==2 ? 1+c : 1-c)/(2*a0);b1=(type==2 ? -2 : 2)*b0;b2=b0;}
    }
    void eq(int type,float hz,float gainDb) noexcept {
        if(type==cachedType && hz==cachedHz && gainDb==cachedGain)return;
        cachedType=type;cachedHz=hz;cachedGain=gainDb;unity=gainDb==0;
        if(hz!=eqHz){eqHz=hz;const double w=2*Pi*hz/44100;eqCos=std::cos(w);eqSin=std::sin(w);}
        const double c=eqCos,s=eqSin,A=std::exp(bound(gainDb,-24,12)*(2.302585092994045684/40));
        const double alpha=s/(2*.8),beta=2*std::sqrt(A)*(s/std::sqrt(2.));
        double a0;
        if(type==1){a0=1+alpha/A;b0=(1+alpha*A)/a0;b1=-2*c/a0;b2=(1-alpha*A)/a0;a1=-2*c/a0;a2=(1-alpha/A)/a0;}
        else if(type==0){a0=(A+1)+(A-1)*c+beta;b0=A*((A+1)-(A-1)*c+beta)/a0;b1=2*A*((A-1)-(A+1)*c)/a0;b2=A*((A+1)-(A-1)*c-beta)/a0;a1=-2*((A-1)+(A+1)*c)/a0;a2=((A+1)+(A-1)*c-beta)/a0;}
        else {a0=(A+1)-(A-1)*c+beta;b0=A*((A+1)+(A-1)*c+beta)/a0;b1=-2*A*((A-1)+(A+1)*c)/a0;b2=A*((A+1)+(A-1)*c-beta)/a0;a1=2*((A-1)-(A+1)*c)/a0;a2=((A+1)-(A-1)*c-beta)/a0;}
    }
};
// Native soft-knee compressor: source settings, independent browser-internal approximation.
struct Compressor {
    float gain=1,cachedThreshold=999,cachedKnee=999,minimum=0,cachedRelease=-1,releaseCoeff=0;
    void process(float& l,float& r,float threshold,float knee,float ratio,float release) noexcept {
        const float level=std::max(std::fabs(l),std::fabs(r));
        // Below the knee, gain reduction is exactly zero. Cache the boundary.
        if(threshold!=cachedThreshold || knee!=cachedKnee){cachedThreshold=threshold;cachedKnee=knee;minimum=std::pow(10.f,(threshold-knee*.5f)/20);}
        if(release!=cachedRelease){cachedRelease=release;releaseCoeff=1-std::exp(-1/(44100*release));}
        float reduction=0;
        if(level>minimum){
        const float db=20*std::log10(std::max(level,1e-10f)),over=db-threshold;
        if(over>knee*.5f)reduction=over*(1-1/ratio);
        else if(knee>0 && over>-knee*.5f){const float v=over+knee*.5f;reduction=(1-1/ratio)*v*v/(2*knee);}
        }
        const float target=reduction>0 ? std::pow(10.f,-reduction/20) : 1.f;
        // Exponential attack/release, no lookahead; final hard bound catches attack transients.
        const float coeff=target<gain ? .00753044f : releaseCoeff;
        gain+=coeff*(target-gain);l*=gain;r*=gain;
    }
};
}
