#include "vst_abi.h"
#include "siren.h"
#include "dsp_core.h"
#include "params.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>

static int failures=0;
static void require(bool ok,const char* what){if(!ok){if(failures<20)std::printf("FAIL %s\n",what);++failures;}}
static intptr_t host(AEffect*,int32_t,int32_t,intptr_t,void*,float){return 0;}
static void close(AEffect* a){a->dispatcher(a,1,0,0,nullptr,0);}
static std::string chunk(AEffect* a){void* p=nullptr;auto n=a->dispatcher(a,23,0,0,&p,0);require(p&&n>0,"chunk delivery");return p&&n>0?std::string(static_cast<char*>(p),n):std::string();}
// DFS2 chunk -> physical values by parameter index (transient FIRE/STOP read as 0).
static std::vector<float> physical(const std::string& text){
    std::vector<float> v(dub::P_Count,0.f);require(!text.compare(0,5,"DFS2 "),"DFS2 chunk header");
    std::map<std::string,int> index;for(int i=0;i<dub::P_Count;++i)index[PARAMS[i].key]=i;
    const char* cur=text.c_str()+5;int seen=0;
    while(*cur){
        while(*cur==' ')++cur;
        if(!*cur)break;
        const char* equals=std::strchr(cur,'=');require(equals!=nullptr,"key=value token");if(!equals)break;
        const auto found=index.find(std::string(cur,equals));require(found!=index.end(),"known chunk key");
        char* end=nullptr;const float value=std::strtof(equals+1,&end);cur=end;
        if(found!=index.end()){v[found->second]=value;++seen;}
    }
    require(seen==dub::P_Count-2,"chunk stores every persistent parameter");
    return v;
}
// Legacy positional DFS1 snapshot -> {key: value}.
static const char* const legacyKeys[51]={"fire","latch","preset","mode","wave","pitch","level","noise","lfo_wave","rate",
    "depth","attack","release","lfo2_rate","lfo2_amount","lfo3_rate","lfo3_amount","chop_rate","chop_amount","zap_sweep",
    "zap_time","repeat","filter_type","cutoff","resonance","delay_time","feedback","delay_mix","delay_hp","delay_lp",
    "ping","character","reverb","output","crush","invert","freeze","bend","fast","slow",
    "oct_up","oct_down","kill","filter_hold","stop","osc_low","osc_mid","osc_high","master_low","master_mid","master_high"};
static std::map<std::string,float> legacy(const std::string& text){
    std::map<std::string,float> v;const char* cur=text.c_str()+5;
    for(const char* key:legacyKeys){char* end=nullptr;v[key]=std::strtof(cur,&end);cur=end;}
    return v;
}
static void name(AEffect* a,int id,const char* expected){char buf[24]{};a->dispatcher(a,7,id,0,buf,0);require(!std::strcmp(buf,expected),"native enum value label");}
// Every 2.0 parameter restored from a 1.0.x chunk equals its legacy physical value.
static const char* const legacyWaveNames[]={"SQUARE","SAW","TRIANGLE","SINE","NOISE"};
static void migrated(AEffect* a,const std::string& saved){
    require(!saved.compare(0,5,"DFS1 "),"legacy chunk header");
    require(a->dispatcher(a,24,0,saved.size(),const_cast<char*>(saved.data()),0)==1,"legacy chunk accepted");
    const auto old=legacy(saved);const auto now=physical(chunk(a));
    for(int i=0;i<dub::P_Count;++i){
        if(i==dub::P_fire || i==dub::P_stop || i==dub::P_wave || i==dub::P_depth)continue;
        require(now[i]==old.at(PARAMS[i].key),"legacy physical value migrated by key");
    }
    const float hz=now[dub::P_pitch]*now[dub::P_depth]*.01f,expected=std::min(old.at("depth"),old.at("pitch"));
    require(std::fabs(hz-expected)<=expected*1e-5f+1e-4f,"legacy Hz depth becomes the same sweep as a pitch percentage");
    name(a,dub::P_wave,legacyWaveNames[int(old.at("wave"))]);
}
static void mapping(){
    AEffect* a=VSTPluginMain(host);require(a!=nullptr,"controls instance");if(!a)return;
    const char* waves[]={"SINE","TRIANGLE","SAW","SQUARE","NOISE"};
    for(int i=0;i<5;++i){a->setParameter(a,dub::P_wave,i/4.f);require(a->getParameter(a,dub::P_wave)==i/4.f,"exact discrete wave position");name(a,dub::P_wave,waves[i]);}
    a->setParameter(a,dub::P_wave,0);
    for(int i=1;i<5;++i){a->setParameter(a,dub::P_wave,a->getParameter(a,dub::P_wave)+.001f);require(a->getParameter(a,dub::P_wave)==i/4.f,"one clockwise wave step");name(a,dub::P_wave,waves[i]);}
    for(int i=3;i>=0;--i){a->setParameter(a,dub::P_wave,a->getParameter(a,dub::P_wave)-.001f);require(a->getParameter(a,dub::P_wave)==i/4.f,"one counterclockwise wave step");name(a,dub::P_wave,waves[i]);}
    for(int k=0;k<1000;++k){a->setParameter(a,dub::P_wave,k/999.f);float v=a->getParameter(a,dub::P_wave);require(v>=0&&v<=1&&v*4==std::round(v*4),"no ambiguous intermediate wave values");}
    const char* shapes[]={"TRIANGLE","SQUARE","SAW","SINE"};
    for(int i=0;i<4;++i){a->setParameter(a,dub::P_lfo_wave,i/3.f);name(a,dub::P_lfo_wave,shapes[i]);}
    for(float hz:{10.f,20.f,30.f,40.f,50.f,60.f,80.f,100.f,150.f,620.f,2400.f}){
        const float n=std::log(hz/10.f)/std::log(240.f);a->setParameter(a,dub::P_pitch,n);
        auto v=physical(chunk(a));require(std::fabs(v[dub::P_pitch]-hz)<hz*2e-6f,"log pitch physical mapping");
        require(std::fabs(a->getParameter(a,dub::P_pitch)-n)<2e-6f,"log pitch inverse mapping");
        char display[24]{};a->dispatcher(a,7,dub::P_pitch,0,display,0);require(std::fabs(std::atof(display)-hz)<.6f,"pitch displays Hz");
    }
    a->setParameter(a,dub::P_pitch,std::log(20.f/10)/std::log(240.f));float start=physical(chunk(a))[dub::P_pitch];
    a->setParameter(a,dub::P_pitch,a->getParameter(a,dub::P_pitch)+.001f);
    require(physical(chunk(a))[dub::P_pitch]-start<.2f,"small encoder step near 20 Hz");
    require(a->dispatcher(a,27,dub::P_pitch,0,(void*)"20",0)==1,"native physical numeric editing");
    require(std::fabs(physical(chunk(a))[dub::P_pitch]-20)<.0001f,"numeric edit maps to 20 Hz");
    require(a->dispatcher(a,27,dub::P_wave,0,(void*)"TRIANGLE",0)==1,"native wave text editing");name(a,dub::P_wave,"TRIANGLE");
    const auto before=chunk(a);
    require(a->dispatcher(a,27,dub::P_pitch,0,(void*)"nan",0)==0,"numeric edit rejects NaN");
    require(a->dispatcher(a,27,dub::P_wave,0,(void*)"invalid",0)==0,"text edit rejects unknown wave");
    require(a->dispatcher(a,27,dub::P_pitch,0,nullptr,0)==0,"numeric edit rejects null pointer");
    require(chunk(a)==before,"invalid edit preserves state");
    for(const char* bad:{"DFS2 pitch=nan","DFS2 pitch","DFS2 pitch=5x","DFS1 1 2 3","DFS3 pitch=50"}){
        a->dispatcher(a,24,0,std::strlen(bad)+1,(void*)bad,0);require(chunk(a)==before,"malformed chunk rejected whole");
    }
    const char* future="DFS2 pitch=50 sparkle=3 cutoff=1000";a->dispatcher(a,24,0,std::strlen(future)+1,(void*)future,0);
    {const auto v=physical(chunk(a));require(v[dub::P_pitch]==50 && v[dub::P_cutoff]==1000 && v[dub::P_wave]==3,"unknown keys ignored, missing keys default");}
    require(a->dispatcher(a,27,dub::P_wave,0,(void*)"1e38",0)==1,"huge finite enum text bounded safely");name(a,dub::P_wave,"NOISE");
    float l[128],r[128];float* out[]={l,r};double peak=0;unsigned boundaries=0;
    MidiEvent m;m.midiData[0]=0x90;m.midiData[1]=60;m.midiData[2]=100;Events e;e.events[0]=&m;e.events[1]=nullptr;a->dispatcher(a,25,0,0,&e,0);
    for(int k=0;k<2400;++k){
        a->setParameter(a,dub::P_preset,(k%12)/11.f);name(a,dub::P_preset,PARAMS[dub::P_preset].opts[k%12]);
        // Include rapid direct wave selection and maximum feedback in active MIDI audio.
        a->setParameter(a,dub::P_wave,(k%5)/4.f);a->setParameter(a,dub::P_feedback,1);a->processReplacing(a,nullptr,out,128);
        for(int i=0;i<128;++i){require(std::isfinite(l[i])&&std::isfinite(r[i]),"rapid preset output finite");peak=std::max(peak,double(std::max(std::fabs(l[i]),std::fabs(r[i]))));boundaries+=std::fabs(l[i])>=.97999f||std::fabs(r[i])>=.97999f;}
        if(k%31==0){auto saved=chunk(a);AEffect* b=VSTPluginMain(host);b->dispatcher(b,24,0,saved.size(),saved.data(),0);for(int j=0;j<dub::P_Count;++j)require(a->getParameter(a,j)==b->getParameter(b,j),"rapid preset state round trip");close(b);}
    }
    require(peak<=.98004,"rapid preset hard output bound");std::printf("PRESET_SWITCH blocks=2400 peak=%.6f bound_frames=%u\n",peak,boundaries);close(a);
}
static void compatibility(const char* baseline){
    void* lib=dlopen(baseline,RTLD_NOW|RTLD_LOCAL);require(lib!=nullptr,"load a 1.0.x build for compatibility");if(!lib)return;
    auto entry=reinterpret_cast<AEffect*(*)(AudioMaster)>(dlsym(lib,"VSTPluginMain"));require(entry!=nullptr,"old VST entry");if(!entry){dlclose(lib);return;}
    AEffect* old=entry(host);AEffect* current=VSTPluginMain(host);
    for(int preset=0;preset<12;++preset){old->setParameter(old,2,preset/11.f);migrated(current,chunk(old));}
    for(int w=0;w<5;++w)for(float n:{0.f,.1f,.5f,.9f,1.f}){old->setParameter(old,4,w/4.f);old->setParameter(old,5,n);migrated(current,chunk(old));}
    close(old);close(current);dlclose(lib);std::puts("LEGACY_COMPAT 12 factory states + 25 custom states migrated");
}
static void legacyFixtures(){
    std::ifstream file("tests/fixtures/legacy-1.0.1.chunks");require(bool(file),"open saved legacy fixtures");
    AEffect* a=VSTPluginMain(host);std::string saved;int count=0;
    while(std::getline(file,saved)){migrated(a,saved);++count;}
    require(count==37,"12 legacy factory and 25 custom fixtures");close(a);
    std::puts("LEGACY_FIXTURES 37 DFS1 states migrated; all 18 persistent 2.0 parameters match");
}
static void filterSafety(){
    for(float hz:{20.f,200.f,1000.f,9000.f,20000.f})for(float q:{0.f,10.f,20.f}){
        dub::Lowpass f;f.set(hz,q);
        // 20 Hz at Q 20 dB rings for seconds before the scrub reaches exact zero.
        for(int k=0;k<44100*10;++k){if(k%16==0)f.scrub();float out=f.process(k==0?1.f:0.f);require(std::isfinite(out)&&std::isfinite(f.z1)&&std::isfinite(f.z2),"lowpass impulse history finite");}
        require(f.silent(),"lowpass impulse tail scrubs to exact zero");
        for(float value:{INFINITY,NAN,1e38f,-1e38f,0.f})require(std::isfinite(f.process(value))&&std::isfinite(f.z1)&&std::isfinite(f.z2),"lowpass rejects invalid/extreme input");
    }
    dub::Lowpass f;f.z1=1e-16;f.z2=-1e-16;f.scrub();require(f.silent(),"control-rate filter cleanup");
    // Steady-state gain of a 440 Hz sine through the master compressor and limiter, measured on
    // Chromium's DynamicsCompressorNode with the same settings (the BARZINE page's master chain).
    struct Point{double inDb,gainDb;};
    const auto sineGain=[](dub::Compressor& c,double inDb){
        const double amp=std::pow(10.0,inDb/20);double in=0,out=0;c.reset();
        for(int k=0;k<44100*3/16;++k){
            alignas(16) float l[16],r[16],dry[16];
            for(int j=0;j<16;++j)dry[j]=l[j]=r[j]=float(amp*std::sin(2*dub::Pi*440*double(16*k+j)/44100.0));
            c.process(l,r,16);
            if(k*16>44100*2)for(int j=0;j<16;++j){in+=double(dry[j])*dry[j];out+=double(l[j])*l[j];}
        }
        return 10*std::log10(out/in);
    };
    dub::Compressor c(-24,30,12,.25f),lim(-1,0,20,.08f);
    for(const Point& p:{Point{-60,3.66},Point{-24,3.66},Point{-20,3.56},Point{-12,2.58},Point{-6,.83},Point{-3,-.54},Point{0,-2.31}})
        require(std::fabs(sineGain(c,p.inDb)-p.gainDb)<.05,"compressor matches the Web Audio node's static curve and makeup gain");
    for(const Point& p:{Point{-60,.57},Point{-6,.57},Point{0,-.30}})
        require(std::fabs(sineGain(lim,p.inDb)-p.gainDb)<.05,"limiter matches the Web Audio node's static curve and makeup gain");
    float l[16],r[16];
    for(int k=0;k<44100/16;++k){for(int j=0;j<16;++j)l[j]=r[j]=.7f*std::sin(.05f*float(16*k+j));c.process(l,r,16);}
    require(c.gain<.99f,"compressor reduces gain on a loud signal");
    for(int k=0;k<44100;++k){std::fill_n(l,16,0.f);std::fill_n(r,16,0.f);c.process(l,r,16);}
    require(c.gain>.9995f && c.det>.9995f && std::sin(1.57079633f*c.gain)>.99995f,"compressor releases to unity on silence");
}
// Vector math against libm over the ranges the engine uses.
static void simdAccuracy(){
    using namespace dub::simd;
    double sine=0,logs=0,exps=0;
    for(int k=-40000;k<=40000;++k){
        alignas(16) float in[4],out[4];for(int j=0;j<4;++j)in[j]=float(k*4+j)/65536.f;
        store(out,sin2pi(load(in)));for(int j=0;j<4;++j)sine=std::max(sine,std::fabs(out[j]-std::sin(2*dub::Pi*in[j])));
        for(int j=0;j<4;++j)in[j]=std::ldexp(1.f+float((k+40000)%1000)/1000.f,k%40);
        store(out,log2(load(in)));for(int j=0;j<4;++j)logs=std::max(logs,std::fabs(out[j]-std::log2(double(in[j]))));
        for(int j=0;j<4;++j)in[j]=float(k*4+j)/2600.f;
        store(out,exp2(load(in)));for(int j=0;j<4;++j)exps=std::max(exps,std::fabs(out[j]/std::exp2(double(in[j]))-1));
    }
    std::printf("SIMD sin2pi max_abs=%.2e log2 max_abs=%.2e exp2 max_rel=%.2e\n",sine,logs,exps);
    require(sine<3e-7,"vector sine accuracy");require(logs<2e-6,"vector log2 accuracy");require(exps<1e-6,"vector exp2 accuracy");
    alignas(16) float in[4]={-.5f,.5f,1.5f,2.5f};alignas(16) int16_t pcm[8];
    store_stereo_s16(pcm,round_int(load(in)),round_int(splat(-32768.f)));
    require(pcm[0]==0&&pcm[2]==0&&pcm[4]==2&&pcm[6]==2&&pcm[1]==-32768,"round-to-nearest-even interleaved PCM");
}
static void dry(dub::Siren& s){s.set(dub::P_wave,0);s.set(dub::P_depth,0);s.set(dub::P_delay_mix,0);s.set(dub::P_cutoff,9000);s.set(dub::P_resonance,0);s.set(dub::P_output,50);}
static void lowPitch(){
    float l[128],r[128];
    for(float hz:{10.f,20.f,30.f,50.f,100.f,620.f,2400.f}){
        dub::Siren s;dry(s);s.set(dub::P_pitch,hz);s.set(dub::P_latch,1);for(int k=0;k<500;++k)s.render(l,r,128);
        int crossings=0;float previous=l[127];double peak=0;
        for(int k=0;k<44100*6/128;++k){s.render(l,r,128);for(int i=0;i<128;++i){if(previous<=0&&l[i]>0)++crossings;previous=l[i];peak=std::max(peak,double(std::fabs(l[i])));}}
        double measured=crossings/(double(44100*6/128*128)/44100);require(std::fabs(measured-hz)<.5,"low/max oscillator frequency accuracy");require(peak>1e-5,"low pitch audible core energy");std::printf("PITCH requested=%.0f measured=%.4f peak=%.6f\n",hz,measured,peak);
    }
    // With the default 77% depth every PITCH value transposes the whole sweep:
    // the average frequency over whole LFO cycles follows the knob down to 10 Hz.
    for(float hz:{10.f,15.f,20.f,30.f,40.f,60.f,80.f,120.f,160.f,320.f,620.f,1200.f,2400.f}){
        dub::Siren s;s.set(dub::P_wave,0);s.set(dub::P_delay_mix,0);s.set(dub::P_cutoff,9000);s.set(dub::P_rate,1);s.set(dub::P_pitch,hz);s.set(dub::P_latch,1);
        for(int k=0;k<44100/128*2;++k)s.render(l,r,128);
        int crossings=0;float previous=l[127];
        for(int k=0;k<44100*6/128;++k){s.render(l,r,128);for(int i=0;i<128;++i){if(previous<=0&&l[i]>0)++crossings;previous=l[i];}}
        const double mean=crossings/(double(44100*6/128*128)/44100);
        require(std::fabs(mean/hz-1)<.05+.5/hz,"siren sweep follows PITCH across the whole range");
        std::printf("SWEEP pitch=%.0f mean=%.2f\n",hz,mean);
    }
    {dub::Siren s;dry(s);s.set(dub::P_pitch,10);s.set(dub::P_mode,1);s.set(dub::P_zap_sweep,-48);for(int k=0;k<500;++k)s.render(l,r,128);s.set(dub::P_fire,1);int crossings=0;bool armed=false;
     for(int k=0;k<44100*2/128;++k){s.render(l,r,128);for(float sample:l){if(sample<-1e-5f)armed=true;else if(sample>1e-5f&&armed){++crossings;armed=false;}}}
     require(crossings>0&&crossings<12,"low ZAP descends below old 30 Hz floor");std::printf("LOW_ZAP 10 Hz descending sweep crossings=%d\n",crossings);}
    double peak=0;double sum=0;unsigned long frames=0;
    for(int mode=0;mode<8;++mode){dub::Siren s;s.set(dub::P_wave,mode%5);s.set(dub::P_lfo_wave,mode%4);s.set(dub::P_pitch,mode&1?20:10);s.set(dub::P_latch,1);s.set(dub::P_depth,100);s.set(dub::P_rate,mode>=4?24:3);s.set(dub::P_feedback,88);s.set(dub::P_delay_time,.05);s.set(dub::P_ping,mode*14);s.set(dub::P_resonance,20);s.set(dub::P_cutoff,200);if(mode==7){s.set(dub::P_mode,1);s.set(dub::P_zap_sweep,-48);}
      for(int k=0;k<44100*30/128;++k){if(mode==7&&k%40==0)s.set(dub::P_fire,1);s.render(l,r,128);for(int i=0;i<128;++i){require(std::isfinite(l[i])&&std::isfinite(r[i]),"low pitch extreme FX finite");peak=std::max(peak,double(std::max(std::fabs(l[i]),std::fabs(r[i]))));sum+=l[i];++frames;}}}
    require(peak<=.98001,"low pitch extreme FX bounded");std::printf("LOW_EXTREMES seconds=240 peak=%.6f mean_L=%+.6f\n",peak,sum/frames);
    // A waveform change begins with the existing small blend step, not a hard replacement.
    dub::Siren a,b;dry(a);dry(b);a.set(dub::P_latch,1);b.set(dub::P_latch,1);float bl[128],br[128];for(int k=0;k<300;++k){a.render(l,r,128);b.render(bl,br,128);}a.set(dub::P_wave,3);a.render(l,r,128);b.render(bl,br,128);require(std::fabs(l[0]-bl[0])<.02,"active wave switch starts smoothly");
}
int main(int argc,char** argv){mapping();simdAccuracy();lowPitch();filterSafety();legacyFixtures();if(argc>1)compatibility(argv[1]);std::printf("PERFORMANCE CONTROLS %s failures=%d\n",failures?"FAIL":"PASS",failures);return failures?1:0;}
