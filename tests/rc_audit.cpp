#include "vst_abi.h"
#include "siren.h"
#include "params.h"
extern "C" {
#include "engine.h"
}
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

static intptr_t host(AEffect*,int32_t,int32_t,intptr_t,void*,float){return 0;}
static uint32_t rng=0x72636175;
static uint32_t random32(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float uniform(){return (random32()>>8)*(1.f/16777216.f);}
static int failures=0;
static void require(bool value,const char* label){if(!value){if(failures<30)std::printf("FAIL %s\n",label);++failures;}}
static void close(AEffect* fx){fx->dispatcher(fx,1,0,0,nullptr,0);}
static std::string chunk(AEffect* fx){void* p=nullptr;const auto n=fx->dispatcher(fx,23,0,0,&p,0);require(p&&n>0&&n<=8192,"valid bounded chunk");return p&&n>0?std::string(static_cast<char*>(p),n):std::string();}
static float canonical(AEffect* fx,int id){return fx->getParameter(fx,id);}
static std::array<float,dub::P_Count> snapshot(AEffect* fx){std::array<float,dub::P_Count> v{};for(int i=0;i<dub::P_Count;++i)v[i]=canonical(fx,i);return v;}
static void compare(AEffect* fx,const std::array<float,dub::P_Count>& expected){for(int i=0;i<dub::P_Count;++i)if(std::fabs(canonical(fx,i)-expected[i])>2e-6f){std::printf("FAIL restored parameter %d %s expected %.9g actual %.9g\n",i,PARAMS[i].key,expected[i],canonical(fx,i));++failures;}}
static void stateAudit(){
 AEffect* a=VSTPluginMain(host);require(a!=nullptr,"state source created");if(!a)return;
 for(int trial=0;trial<256;++trial){
  a->setParameter(a,dub::P_preset,float(trial%12)/11);
  a->setParameter(a,dub::P_character,float(trial%5)/4);
  for(int i=0;i<dub::P_Count;++i){if(i==dub::P_preset||i==dub::P_character||i==dub::P_fire||i==dub::P_stop)continue;
   const auto& p=PARAMS[i];float n=p.nopts?float(random32()%p.nopts)/(p.nopts-1):trial%16==0?0:trial%16==1?1:uniform();a->setParameter(a,i,n);}
  const auto expected=snapshot(a);const auto saved=chunk(a);
  AEffect* restored=VSTPluginMain(host);require(restored!=nullptr,"fresh restore instance");if(!restored)break;
  require(restored->dispatcher(restored,24,0,saved.size(),const_cast<char*>(saved.data()),0)==1,"restore accepted");compare(restored,expected);
  // Restore into a changed existing instance too, without invoking preset/character side effects.
  restored->setParameter(restored,dub::P_preset,float((trial+1)%12)/11);
  restored->dispatcher(restored,24,0,saved.size(),const_cast<char*>(saved.data()),0);compare(restored,expected);close(restored);
 }
 for(int preset=0;preset<12;++preset){a->setParameter(a,dub::P_preset,float(preset)/11);const auto expected=snapshot(a);auto saved=chunk(a);AEffect* b=VSTPluginMain(host);b->dispatcher(b,24,0,saved.size(),const_cast<char*>(saved.data()),0);compare(b,expected);close(b);}
 // Fired pulses, active notes and UI overlays are intentionally transient.
 a->setParameter(a,dub::P_preset,0);a->setParameter(a,dub::P_latch,0);a->setParameter(a,dub::P_fire,1);
 a->setParameter(a,51,1);MidiEvent m;m.midiData[0]=0x90;m.midiData[1]=60;m.midiData[2]=100;Events event;event.events[0]=&m;event.events[1]=nullptr;a->dispatcher(a,25,0,0,&event,0);
 auto saved=chunk(a);AEffect* b=VSTPluginMain(host);b->dispatcher(b,24,0,saved.size(),const_cast<char*>(saved.data()),0);float l[128],r[128];float* out[]={l,r};b->processReplacing(b,nullptr,out,128);double energy=0;for(int i=0;i<128;++i)energy+=l[i]*l[i]+r[i]*r[i];require(energy<1e-12,"FIRE/MIDI/audio tail not persisted");require(b->getParameter(b,51)==0,"preset popup not persisted");close(b);
 a->setParameter(a,dub::P_stop,1);a->setParameter(a,dub::P_latch,1);saved=chunk(a);b=VSTPluginMain(host);b->dispatcher(b,24,0,saved.size(),const_cast<char*>(saved.data()),0);energy=0;for(int k=0;k<100;++k){b->processReplacing(b,nullptr,out,128);for(int i=0;i<128;++i)energy+=l[i]*l[i]+r[i]*r[i];}require(energy>1e-4,"restored LATCH ON resumes sound");close(b);close(a);
 std::printf("STATE: 256 random snapshots restored fresh and in-place, 12 factory states, transient exclusions, latch behavior; failures=%d\n",failures);
}
struct Meter {double peak=0,sumL=0,sumR=0,energy=0;uint64_t samples=0,limited=0;
 void add(float* l,float* r,int n){for(int i=0;i<n;++i){require(std::isfinite(l[i])&&std::isfinite(r[i]),"finite output");require(std::fabs(l[i])<=.98001f&&std::fabs(r[i])<=.98001f,"hard output bound");peak=std::max(peak,double(std::max(std::fabs(l[i]),std::fabs(r[i]))));sumL+=l[i];sumR+=r[i];energy+=double(l[i])*l[i]+double(r[i])*r[i];limited+=std::fabs(l[i])>=.97999f||std::fabs(r[i])>=.97999f;++samples;}}
 void print(const char* label){std::printf("LEVEL %-22s peak=%.6f rms=%.6f DC_L=%+.6f DC_R=%+.6f bound_frames=%llu/%llu\n",label,peak,std::sqrt(energy/(2*samples)),sumL/samples,sumR/samples,(unsigned long long)limited,(unsigned long long)samples);}
};
static void render(dub::Siren& s,float* l,float* r,int frames,Meter& m){s.render(l,r,frames);m.add(l,r,frames);}
static void levels(){float l[128],r[128];
 for(int p=0;p<12;++p){dub::Siren s;s.set(dub::P_preset,p);s.set(dub::P_latch,1);Meter m;for(int k=0;k<44100*8/128;++k)render(s,l,r,128,m);require(m.energy>1e-5,"factory preset audible");char label[48];std::snprintf(label,sizeof(label),"preset %02d %s",p,PARAMS[dub::P_preset].opts[p]);m.print(label);}
 for(int mode=0;mode<8;++mode){dub::Siren s;s.set(dub::P_character,mode%5);s.set(dub::P_latch,1);s.set(dub::P_level,100);s.set(dub::P_output,100);s.set(dub::P_resonance,20);s.set(dub::P_cutoff,mode&1?200:9000);s.set(dub::P_delay_time,.05);s.set(dub::P_feedback,88);s.set(dub::P_delay_mix,100);s.set(dub::P_reverb,100);s.set(dub::P_pitch,mode==0?60:620);
 if(mode>=3){s.set(dub::P_depth,1400);s.set(dub::P_rate,24);s.set(dub::P_lfo2_amount,100);s.set(dub::P_lfo3_amount,100);s.set(dub::P_chop_amount,100);}
 if(mode==6){s.set(dub::P_crush,1);s.set(dub::P_osc_low,12);s.set(dub::P_master_low,12);}
 Meter m,steady;for(int k=0;k<44100*30/128;++k){if(mode==5&&k==44100*2/128){s.set(dub::P_freeze,1);s.set(dub::P_latch,0);}if(mode==7&&k%5==0)s.set(dub::P_fire,1);render(s,l,r,128,m);if(k>44100*20/128)steady.add(l,r,128);}
 char label[48];std::snprintf(label,sizeof(label),"maximum case %d",mode);m.print(label);std::snprintf(label,sizeof(label),"case %d final10sec",mode);steady.print(label);
 }
}
static long resident(){std::ifstream f("/proc/self/statm");long total=0,rss=0;f>>total>>rss;return rss;}
static void stress(int seconds){dub::Siren s,probe;char saved[8192];const mpc_engine_t* api=mpc_engine();float l[128],r[128];Meter m;const long before=resident();const auto start=std::chrono::steady_clock::now();long rssMin=before,rssMax=before;unsigned chunks=0;const int blocks=seconds*44100/128;
 for(int k=0;k<blocks;++k){
  if(k%173==0)s.set(dub::P_preset,float(random32()%12));
  if(k%19==0){for(int j=0;j<6;++j){const int id=random32()%dub::P_Count;if(id==dub::P_preset||id==dub::P_stop)continue;const auto& p=PARAMS[id];s.set(id,p.nopts?float(random32()%p.nopts):p.min+uniform()*(p.max-p.min));}}
  s.set(dub::P_latch,1);s.set(dub::P_feedback,88);s.set(dub::P_level,100);s.set(dub::P_output,100);s.set(dub::P_lfo2_amount,100);s.set(dub::P_lfo3_amount,100);
  if(k%7==0)s.set(dub::P_fire,1);if(k%257==0)s.set(dub::P_freeze,(k/257)&1);if(k%911==0)s.set(dub::P_stop,1);
  render(s,l,r,128,m);
  if(k%4096==0){require(api->get_param(&s,"state",saved,sizeof(saved))>0,"stress state serialize");api->set_param(&probe,"state",saved);for(int id=0;id<dub::P_Count;++id){float v=s.get(id);const auto& p=PARAMS[id];require(std::isfinite(v)&&v>=(p.nopts?0:p.min)&&v<=(p.nopts?float(p.nopts-1):p.max),"stress parameter range");require(v==probe.get(id),"stress state round-trip");}const auto r=resident();rssMin=std::min(rssMin,r);rssMax=std::max(rssMax,r);++chunks;}
 }
 m.print("long random stress");const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::printf("STRESS audio_seconds=%d blocks=%d wall_seconds=%.3f RSS_pages_before=%ld after=%ld sampled_min=%ld max=%ld checkpoints=%u\n",seconds,blocks,elapsed,before,resident(),rssMin,rssMax,chunks);
}
int main(int argc,char**argv){stateAudit();levels();stress(argc>1?std::atoi(argv[1]):1800);std::printf("RC AUDIT %s failures=%d\n",failures?"FAIL":"PASS",failures);return failures?1:0;}
