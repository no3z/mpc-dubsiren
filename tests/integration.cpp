#include "vst_abi.h"
#include "siren.h"
#include "params.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <thread>

static thread_local bool inAudio=false;
static unsigned audioAllocations=0,audioFrees=0;
extern "C" {
void* __real_malloc(size_t); void* __real_calloc(size_t,size_t);
void* __real_realloc(void*,size_t); void __real_free(void*);
void* __wrap_malloc(size_t n){if(inAudio)++audioAllocations;return __real_malloc(n);}
void* __wrap_calloc(size_t n,size_t s){if(inAudio)++audioAllocations;return __real_calloc(n,s);}
void* __wrap_realloc(void* p,size_t n){if(inAudio)++audioAllocations;return __real_realloc(p,n);}
void __wrap_free(void* p){if(inAudio && p)++audioFrees;__real_free(p);}
}
void* operator new(size_t n){if(inAudio)++audioAllocations;if(void* p=__real_malloc(n))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{if(inAudio && p)++audioFrees;__real_free(p);}
void operator delete[](void* p) noexcept{::operator delete(p);}
void operator delete(void* p,size_t) noexcept{::operator delete(p);}
void operator delete[](void* p,size_t) noexcept{::operator delete(p);}

static int failures=0;
static void check(bool ok,const char* label){std::printf("%s %s\n",ok?"PASS":"FAIL",label);if(!ok)++failures;}
static intptr_t host(AEffect*,int32_t,int32_t,intptr_t,void*,float){return 0;}
static uint32_t rng=0x4653524e;
static uint32_t random32(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float random01(){return (random32()>>8)*(1.f/16777216.f);}
static void note(AEffect* a,bool on){MidiEvent m;m.midiData[0]=on?0x90:0x80;m.midiData[1]=60;m.midiData[2]=on?100:0;Events ev;ev.events[0]=&m;ev.events[1]=nullptr;a->dispatcher(a,25,0,0,&ev,0);}
static double renderHost(AEffect* a,int frames){
    float l[1024]={},r[1024]={};float* out[]={l,r};
    inAudio=true;a->processReplacing(a,nullptr,out,frames);inAudio=false;
    double e=0;for(int i=0;i<frames;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i])||std::abs(l[i])>1.001f||std::abs(r[i])>1.001f)++failures;e+=l[i]*l[i]+r[i]*r[i];}return e;
}
static void safeCore(dub::Siren& s){
    s.set(dub::P_wave,3);s.set(dub::P_mode,0);s.set(dub::P_pitch,440);
    s.set(dub::P_depth,0);s.set(dub::P_lfo2_amount,0);s.set(dub::P_lfo3_amount,0);
    s.set(dub::P_zap_sweep,0);s.set(dub::P_repeat,0);s.set(dub::P_noise,0);
    s.set(dub::P_delay_mix,0);s.set(dub::P_reverb,0);s.set(dub::P_crush,0);
    s.set(dub::P_chop_amount,0);s.set(dub::P_level,50.f);s.set(dub::P_output,50.f);
    s.set(dub::P_cutoff,20000);s.set(dub::P_resonance,0);s.set(dub::P_filter_type,0);
    s.set(dub::P_attack,.003f);s.set(dub::P_release,.01f);
}
static void checkCore(){
    dub::Siren s;float l[128],r[128];
    safeCore(s);s.set(dub::P_latch,1);
    bool finite=true;double energy=0;
    for(int wave=0;wave<5;++wave){s.set(dub::P_wave,float(wave));for(int block=0;block<100;++block){
        inAudio=true;s.render(l,r,128);inAudio=false;
        for(int i=0;i<128;++i){finite&=std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=1.001f&&std::abs(r[i])<=1.001f;energy+=l[i]*l[i]+r[i]*r[i];}
    }}
    check(finite,"all oscillator waveforms produce finite bounded float output");check(energy>1e-4,"latch produces sustained audio");
    {dub::Siren whole,split;safeCore(whole);safeCore(split);whole.set(dub::P_latch,1);split.set(dub::P_latch,1);
     float aL[512],aR[512],bL[512],bR[512];whole.render(aL,aR,512);
     split.render(bL,bR,17);split.render(bL+17,bR+17,111);split.render(bL+128,bR+128,384);
     bool continuous=true;for(int i=0;i<512;++i)continuous&=std::abs(aL[i]-bL[i])<1e-6f&&std::abs(aR[i]-bR[i])<1e-6f;
     check(continuous,"oscillator/envelope state remains continuous across split render blocks");}
    safeCore(s);s.set(dub::P_latch,1);for(int i=0;i<100;++i)s.render(l,r,128);
    int crossings=0;float previous=l[127];
    for(int remaining=44100;remaining>0;){int n=std::min(remaining,128);s.render(l,r,n);for(int i=0;i<n;++i){if(previous<=0&&l[i]>0)++crossings;previous=l[i];}remaining-=n;}
    std::printf("Measured sine crossings / second: %d\n",crossings);
    check(std::abs(crossings-440)<=2,"440 Hz oscillator frequency within two cycles per second");
    s.set(dub::P_latch,0);unsigned char off[]={0xb0,123,0};s.midi(off,3);
    for(int i=0;i<2000;++i)s.render(l,r,128);
    double tail=0;for(int i=0;i<128;++i)tail+=l[i]*l[i]+r[i]*r[i];
    check(tail<1e-8,"release and empty effects decay to silence");
    {dub::Siren stopped;safeCore(stopped);stopped.set(dub::P_mode,1);stopped.set(dub::P_fire,1);stopped.set(dub::P_stop,1);
     inAudio=true;stopped.render(l,r,128);inAudio=false;double stoppedEnergy=0;
     for(int i=0;i<128;++i)stoppedEnergy+=l[i]*l[i]+r[i]*r[i];
     check(stoppedEnergy<1e-12,"STOP cancels a pending FIRE before the next ZAP render");
     unsigned char on[]={0x90,60,100};stopped.midi(on,3);stopped.set(dub::P_stop,1);
     inAudio=true;stopped.render(l,r,128);inAudio=false;stoppedEnergy=0;
     for(int i=0;i<128;++i)stoppedEnergy+=l[i]*l[i]+r[i]*r[i];
     check(stoppedEnergy<1e-12,"STOP cancels a pending MIDI note before the next ZAP render");
     stopped.set(dub::P_fire,1);double refired=0;
     for(int block=0;block<30;++block){inAudio=true;stopped.render(l,r,128);inAudio=false;for(int i=0;i<128;++i)refired+=l[i]*l[i]+r[i]*r[i];}
     check(refired>1e-4,"FIRE after STOP remains audible");}
    finite=true;
    for(int step=0;step<2000;++step){
        for(int k=0;k<6;++k){int id=int(random32()%dub::P_Count);const param_t& p=PARAMS[id];float v=p.nopts?float(random32()%p.nopts):p.min+random01()*(p.max-p.min);s.set(id,v);}
        s.set(dub::P_latch,1);s.set(dub::P_delay_time,(step&1)?.001f:3.f);s.set(dub::P_feedback,(step%11==0)?115.f:88.f);s.set(dub::P_delay_mix,100);
        inAudio=true;s.render(l,r,128);inAudio=false;
        for(int i=0;i<128;++i)finite&=std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=1.001f&&std::abs(r[i])<=1.001f;
    }
    check(finite,"random parameter changes and rapid feedback/delay extremes remain finite and bounded");
}
int main(){
    check(VSTPluginMain(nullptr)==nullptr,"NULL host callback rejected");
    AEffect* a=VSTPluginMain(host);AEffect* b=VSTPluginMain(host);
    check(a&&b&&a!=b,"independent VST instances created");if(!a||!b)return 1;
    check(a->magic==0x56737450 && a->numParams==NPARAMS && a->numInputs==0 && a->numOutputs==2,"VST2 instrument ABI identity and channels");
    check(a->dispatcher(a,10,0,0,nullptr,44100)==1 && a->dispatcher(a,10,0,0,nullptr,48000)==0,"fixed-rate wrapper accepts 44100 Hz and rejects unsupported 48000 Hz");
    {char display[24]={};a->dispatcher(a,7,dub::P_attack,0,display,0);
     check(!std::strcmp(display,"0.010"),"10 ms attack displays as 0.010 seconds");
     a->dispatcher(a,7,dub::P_lfo2_rate,0,display,0);
     check(!std::strcmp(display,"0.17"),"slow LFO2 displays as 0.17 Hz");
     bool bounded=true;for(int i=0;i<NPARAMS;++i){struct {char text[24];unsigned char guard;} out{};out.guard=0x5a;a->dispatcher(a,7,i,0,out.text,0);bounded&=out.guard==0x5a;}
     check(bounded,"every parameter display stays within its 24 byte MPC buffer");}
    double silence=0;for(int i=0;i<20;++i)silence+=renderHost(b,128);check(silence<1e-10,"untriggered VST instance is silent");
    a->setParameter(a,dub::P_pitch,.25f);const float before=a->getParameter(a,dub::P_pitch);
    for(float invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()})a->setParameter(a,dub::P_pitch,invalid);
    check(a->getParameter(a,dub::P_pitch)==before,"nonfinite VST parameter values ignored");
    a->setParameter(a,-1,1);a->setParameter(a,NPARAMS,1);check(a->getParameter(a,-1)==0 && a->getParameter(a,NPARAMS)==0,"invalid parameter indices ignored");
    check(a->dispatcher(a,25,0,0,nullptr,0)==0 && a->dispatcher(a,24,0,5,nullptr,0)==0,"NULL events and chunks rejected");
    void* state=nullptr;intptr_t bytes=a->dispatcher(a,23,0,0,&state,0);
    check(bytes>0 && bytes<=8192 && state,"bounded state chunk produced");
    if(bytes>0 && state){check(b->dispatcher(b,24,0,bytes,state,0)==1,"state chunk accepted");
        bool restored=true;for(int i=0;i<dub::P_Count;++i)restored&=std::abs(a->getParameter(a,i)-b->getParameter(b,i))<1e-5;
        check(restored,"state chunk round trip restores every public parameter");}
    check(a->dispatcher(a,24,0,8193,(void*)"bad",0)==0,"oversized chunk rejected");
    const char* malformed="DFS1 1 nan ";a->dispatcher(a,24,0,std::strlen(malformed)+1,(void*)malformed,0);
    check(a->getParameter(a,dub::P_pitch)==before,"malformed chunk leaves parameter state unchanged");
    a->setParameter(a,dub::P_latch,0);note(a,true);double audio=0;for(int i=0;i<300;++i)audio+=renderHost(a,(i%3==0)?17:(i%3==1)?128:511);check(audio>1e-4,"MIDI note produces audio across arbitrary host block sizes");note(a,false);
    a->setParameter(a,dub::P_latch,0);a->setParameter(a,dub::P_release,0);
    for(int i=0;i<3000;++i)renderHost(a,128);
    {float l[128],r[128];std::fill_n(l,128,1.f);std::fill_n(r,128,1.f);float* out[]={l,r};b->process(b,nullptr,out,128);bool kept=true;for(int i=0;i<128;++i)kept&=std::abs(l[i]-1)<1e-4&&std::abs(r[i]-1)<1e-4;check(kept,"legacy process adds silence to existing host samples");}
    checkCore();
    {std::thread controls([a](){for(int i=0;i<5000;++i){a->setParameter(a,dub::P_pitch,float(i%1000)/999);a->setParameter(a,dub::P_wave,float(i%5)/4);a->setParameter(a,dub::P_fire,(i%83==0)?1:0);}});
     for(int i=0;i<1000;++i)renderHost(a,128);
     controls.join();check(true,"concurrent parameter setters and audio render stress completed");}
    check(audioAllocations==0 && audioFrees==0,"audio rendering makes no intercepted heap allocations or frees");
    std::printf("Audio heap operations: allocations=%u frees=%u\n",audioAllocations,audioFrees);
    a->dispatcher(a,1,0,0,nullptr,0);b->dispatcher(b,1,0,0,nullptr,0);
    std::printf("%s: %d failures\n",failures?"FAILED":"PASSED",failures);return failures?1:0;
}
