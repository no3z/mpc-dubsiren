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
#include <string>
#include <vector>

static int failures=0;
static void require(bool ok,const char* what){if(!ok){if(failures<20)std::printf("FAIL %s\n",what);++failures;}}
static intptr_t host(AEffect*,int32_t,int32_t,intptr_t,void*,float){return 0;}
static void close(AEffect* a){a->dispatcher(a,1,0,0,nullptr,0);}
static std::string chunk(AEffect* a){void* p=nullptr;auto n=a->dispatcher(a,23,0,0,&p,0);require(p&&n>0,"chunk delivery");return p&&n>0?std::string(static_cast<char*>(p),n):std::string();}
static std::vector<float> physical(const std::string& text){std::vector<float> v;const char* cur=text.c_str()+5;for(int i=0;i<dub::P_Count;++i){char* end=nullptr;v.push_back(std::strtof(cur,&end));cur=end;}return v;}
static void name(AEffect* a,int id,const char* expected){char buf[24]{};a->dispatcher(a,7,id,0,buf,0);require(!std::strcmp(buf,expected),"native enum value label");}
static void mapping(){
    AEffect* a=VSTPluginMain(host);require(a!=nullptr,"controls instance");if(!a)return;
    const char* waves[]={"SINE","TRIANGLE","SAW","SQUARE","NOISE"};
    for(int i=0;i<5;++i){a->setParameter(a,dub::P_wave,i/4.f);require(a->getParameter(a,dub::P_wave)==i/4.f,"exact discrete wave position");name(a,dub::P_wave,waves[i]);}
    a->setParameter(a,dub::P_wave,0);
    for(int i=1;i<5;++i){a->setParameter(a,dub::P_wave,a->getParameter(a,dub::P_wave)+.001f);require(a->getParameter(a,dub::P_wave)==i/4.f,"one clockwise wave step");name(a,dub::P_wave,waves[i]);}
    for(int i=3;i>=0;--i){a->setParameter(a,dub::P_wave,a->getParameter(a,dub::P_wave)-.001f);require(a->getParameter(a,dub::P_wave)==i/4.f,"one counterclockwise wave step");name(a,dub::P_wave,waves[i]);}
    for(int k=0;k<1000;++k){a->setParameter(a,dub::P_wave,k/999.f);float v=a->getParameter(a,dub::P_wave);require(v>=0&&v<=1&&v*4==std::round(v*4),"no ambiguous intermediate wave values");}
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
    require(peak<=.98001,"rapid preset hard output bound");std::printf("PRESET_SWITCH blocks=2400 peak=%.6f bound_frames=%u\n",peak,boundaries);close(a);
}
static void compatibility(const char* baseline){
    void* lib=dlopen(baseline,RTLD_NOW|RTLD_LOCAL);require(lib!=nullptr,"load existing 1.0.1 for compatibility");if(!lib)return;
    auto entry=reinterpret_cast<AEffect*(*)(AudioMaster)>(dlsym(lib,"VSTPluginMain"));require(entry!=nullptr,"old VST entry");if(!entry){dlclose(lib);return;}
    AEffect* old=entry(host);AEffect* current=VSTPluginMain(host);
    for(int preset=0;preset<12;++preset){old->setParameter(old,dub::P_preset,preset/11.f);auto saved=chunk(old);current->dispatcher(current,24,0,saved.size(),saved.data(),0);require(physical(saved)==physical(chunk(current)),"legacy factory physical state preserved");}
    const char* oldWaves[]={"SQUARE","SAW","TRIANGLE","SINE","NOISE"};
    for(int w=0;w<5;++w)for(float hz:{60.f,110.f,620.f,2100.f,2400.f}){
        old->setParameter(old,dub::P_wave,w/4.f);old->setParameter(old,dub::P_pitch,(hz-60.f)/2340.f);
        auto saved=chunk(old);current->dispatcher(current,24,0,saved.size(),saved.data(),0);
        require(physical(saved)==physical(chunk(current)),"legacy custom physical state preserved");name(current,dub::P_wave,oldWaves[w]);
    }
    close(old);close(current);dlclose(lib);std::puts("LEGACY_COMPAT 12 factory states + 25 custom states preserved");
}
static void legacyFixtures(){
    std::ifstream file("tests/fixtures/legacy-1.0.1.chunks");require(bool(file),"open saved legacy fixtures");
    AEffect* a=VSTPluginMain(host);std::string saved;int count=0;
    const char* oldWaves[]={"SQUARE","SAW","TRIANGLE","SINE","NOISE"};
    while(std::getline(file,saved)){
        require(a->dispatcher(a,24,0,saved.size(),saved.data(),0)!=0,"restore original 1.0.1 fixture");
        const auto values=physical(saved);require(values==physical(chunk(a)),"all 51 legacy fixture targets preserved");
        name(a,dub::P_wave,oldWaves[int(values[dub::P_wave])]);++count;
    }
    require(count==37,"12 legacy factory and 25 custom fixtures");close(a);
    std::puts("LEGACY_FIXTURES 37 states / 1887 physical targets preserved");
}
static void filterSafety(){
    {dub::Biquad l,r,referenceL,referenceR;
     for(int k=0;k<4096;++k){if(k%32==0){const float gain=float((k/32)%37)-24;l.eq(0,120,gain);r.coefficientsFrom(l);referenceL.eq(0,120,gain);referenceR.eq(0,120,gain);l.scrub();r.scrub();referenceL.scrub();referenceR.scrub();}
       float a=std::sin(k*.11f),b=std::cos(k*.07f);const float expectedA=referenceL.process(a),expectedB=referenceR.process(b);l.processStereo(r,a,b);
       require(a==expectedA&&b==expectedB,"shared stereo coefficients preserve independent exact output");}}
    for(int type=0;type<3;++type)for(float hz:{20.f,200.f,1000.f,20000.f})for(float gain:{-24.f,0.f,12.f}){
        dub::Biquad q;q.eq(type,hz,gain);
        for(int k=0;k<4096;++k){if(k%32==0)q.scrub();float out=q.process(k==0?1.f:0.f);require(std::isfinite(out)&&std::isfinite(q.z1)&&std::isfinite(q.z2),"EQ impulse history finite");}
        q.tone(type,hz,20);
        for(float value:{INFINITY,NAN,1e38f,-1e38f,0.f})require(std::isfinite(q.process(value))&&std::isfinite(q.z1)&&std::isfinite(q.z2),"filter rejects invalid/extreme input");
    }
    dub::Biquad q;q.z1=1e-26;q.z2=-1e-26;q.scrub();require(q.z1==0&&q.z2==0,"control-rate filter cleanup");
}
static void dry(dub::Siren& s){s.set(dub::P_wave,3);s.set(dub::P_depth,0);s.set(dub::P_noise,0);s.set(dub::P_delay_mix,0);s.set(dub::P_reverb,0);s.set(dub::P_cutoff,9000);s.set(dub::P_resonance,0);s.set(dub::P_level,50);s.set(dub::P_output,50);}
static void lowPitch(){
    float l[128],r[128];
    for(float hz:{10.f,20.f,30.f,50.f,100.f,620.f,2400.f}){
        dub::Siren s;dry(s);s.set(dub::P_pitch,hz);s.set(dub::P_latch,1);for(int k=0;k<500;++k)s.render(l,r,128);
        int crossings=0;float previous=l[127];double peak=0;
        for(int k=0;k<44100*6/128;++k){s.render(l,r,128);for(int i=0;i<128;++i){if(previous<=0&&l[i]>0)++crossings;previous=l[i];peak=std::max(peak,double(std::fabs(l[i])));}}
        double measured=crossings/(double(44100*6/128*128)/44100);require(std::fabs(measured-hz)<.5,"low/max oscillator frequency accuracy");require(peak>1e-5,"low pitch audible core energy");std::printf("PITCH requested=%.0f measured=%.4f peak=%.6f\n",hz,measured,peak);
    }
    {dub::Siren s;dry(s);s.set(dub::P_pitch,10);s.set(dub::P_mode,1);s.set(dub::P_zap_sweep,-48);s.set(dub::P_zap_time,1.2f);for(int k=0;k<500;++k)s.render(l,r,128);s.set(dub::P_fire,1);int crossings=0;bool armed=false;
     for(int k=0;k<44100*2/128;++k){s.render(l,r,128);for(float sample:l){if(sample<-1e-5f)armed=true;else if(sample>1e-5f&&armed){++crossings;armed=false;}}}
     require(crossings>0&&crossings<12,"low ZAP descends below old 30 Hz floor");std::printf("LOW_ZAP 10 Hz descending sweep crossings=%d\n",crossings);}
    double peak=0;double sum=0;unsigned long frames=0;
    for(int mode=0;mode<8;++mode){dub::Siren s;s.set(dub::P_character,mode%5);s.set(dub::P_pitch,mode&1?20:10);s.set(dub::P_latch,1);s.set(dub::P_depth,1400);s.set(dub::P_lfo2_amount,100);s.set(dub::P_lfo3_amount,100);s.set(dub::P_feedback,88);s.set(dub::P_delay_time,.05);s.set(dub::P_reverb,100);s.set(dub::P_resonance,20);s.set(dub::P_cutoff,200);s.set(dub::P_chop_amount,mode>=4?100:0);s.set(dub::P_crush,mode>=4?1:0);if(mode==7){s.set(dub::P_mode,1);s.set(dub::P_zap_sweep,-48);s.set(dub::P_repeat,8);}
      for(int k=0;k<44100*30/128;++k){s.render(l,r,128);for(int i=0;i<128;++i){require(std::isfinite(l[i])&&std::isfinite(r[i]),"low pitch extreme FX finite");peak=std::max(peak,double(std::max(std::fabs(l[i]),std::fabs(r[i]))));sum+=l[i];++frames;}}}
    require(peak<=.98001,"low pitch extreme FX bounded");std::printf("LOW_EXTREMES seconds=240 peak=%.6f mean_L=%+.6f\n",peak,sum/frames);
    // A waveform change begins with the existing small blend step, not a hard replacement.
    dub::Siren a,b;dry(a);dry(b);a.set(dub::P_latch,1);b.set(dub::P_latch,1);float bl[128],br[128];for(int k=0;k<300;++k){a.render(l,r,128);b.render(bl,br,128);}a.set(dub::P_wave,0);a.render(l,r,128);b.render(bl,br,128);require(std::fabs(l[0]-bl[0])<.02,"active wave switch starts smoothly");
}
int main(int argc,char** argv){mapping();lowPitch();filterSafety();legacyFixtures();if(argc>1)compatibility(argv[1]);std::printf("PERFORMANCE CONTROLS %s failures=%d\n",failures?"FAIL":"PASS",failures);return failures?1:0;}
