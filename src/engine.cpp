#include "siren.h"
#include "params.h"
#include "simd.h"
extern "C" {
#include "engine.h"
}
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
namespace {
// 1.0.x saved DFS1 chunks: 51 positional physical values in this key order.
const char* const LegacyKeys[51]={"fire","latch","preset","mode","wave","pitch","level","noise","lfo_wave","rate",
    "depth","attack","release","lfo2_rate","lfo2_amount","lfo3_rate","lfo3_amount","chop_rate","chop_amount","zap_sweep",
    "zap_time","repeat","filter_type","cutoff","resonance","delay_time","feedback","delay_mix","delay_hp","delay_lp",
    "ping","character","reverb","output","crush","invert","freeze","bend","fast","slow",
    "oct_up","oct_down","kill","filter_hold","stop","osc_low","osc_mid","osc_high","master_low","master_mid","master_high"};
// 1.0.x engine waveform IDs (SQUARE, SAW, TRIANGLE, SINE, NOISE) as WAVE options.
constexpr float LegacyWave[5]={3,2,1,0,4};
int find(const char* key,std::size_t length) {
    for(int i=0;i<dub::P_Count;++i)if(!std::strncmp(key,PARAMS[i].key,length) && !PARAMS[i].key[length])return i;
    return -1;
}
bool number(const char*& cur,float& value) {
    char* end=nullptr;value=std::strtof(cur,&end);
    if(end==cur || !std::isfinite(value))return false;
    cur=end;return true;
}
// Both parsers are all-or-nothing: a malformed chunk leaves the state unchanged.
bool legacy(const char* cur,float* values) {
    for(const char* key:LegacyKeys){
        float value;if(!number(cur,value))return false;
        const int i=find(key,std::strlen(key));if(i<0)continue;
        if(i==dub::P_wave)value=LegacyWave[int(std::max(0.f,std::min(4.f,value))+.5f)];
        values[i]=value;
    }
    // 1.0.x depth was absolute Hz; 2.0 depth is a percentage of pitch.
    if(values[dub::P_pitch]>0)values[dub::P_depth]=std::min(100.f,values[dub::P_depth]/values[dub::P_pitch]*100.f);
    return true;
}
bool keyed(const char* cur,float* values) {
    while(*cur){
        while(*cur==' ')++cur;
        if(!*cur)break;
        const char* equals=std::strchr(cur,'=');const char* space=std::strchr(cur,' ');
        if(!equals || (space && space<equals))return false;
        const int i=find(cur,std::size_t(equals-cur));
        cur=equals+1;float value;if(!number(cur,value))return false;
        if(*cur && *cur!=' ')return false;
        if(i>=0)values[i]=value; // unknown keys come from other versions
    }
    return true;
}
}
static void* create(const char*) { try {return new dub::Siren;} catch(...) {return nullptr;} }
static void destroy(void* p) {delete static_cast<dub::Siren*>(p);}
static void midi(void* p,const uint8_t* msg,int len) {static_cast<dub::Siren*>(p)->midi(msg,len);}
static void set(void* p,const char* key,const char* val) {
    auto& s=*static_cast<dub::Siren*>(p);
    if(!std::strcmp(key,"state")) {
        float v[dub::P_Count];for(int i=0;i<dub::P_Count;++i)v[i]=PHYSICAL_DEFAULTS[i];
        const bool ok=!std::strncmp(val,"DFS2 ",5) ? keyed(val+5,v) : !std::strncmp(val,"DFS1 ",5) && legacy(val+5,v);
        if(ok)s.restore(v);
        return;
    }
    for(int i=0;i<dub::P_Count;++i)if(!std::strcmp(key,PARAMS[i].key)){char* end=nullptr;const float v=std::strtof(val,&end);if(end!=val)s.set(i,v);return;}
}
static int get(void* p,const char* key,char* buf,int cap) {
    if(!buf || cap<1)return 0;
    auto& s=*static_cast<dub::Siren*>(p);
    if(!std::strcmp(key,"state")) {
        int used=std::snprintf(buf,cap,"DFS2");
        for(int i=0;i<dub::P_Count && used<cap;++i) {
            if(i==dub::P_fire || i==dub::P_stop)continue; // transient triggers
            const int n=std::snprintf(buf+used,cap-used," %s=%.9g",PARAMS[i].key,s.get(i)); if(n<0 || n>=cap-used)return 0;used+=n;
        }
        return used<cap ? used+1 : 0;
    }
    for(int i=0;i<dub::P_Count;++i)if(!std::strcmp(key,PARAMS[i].key)){std::snprintf(buf,cap,"%.9g",s.get(i));return 1;}
    return 0;
}
static void render(void* p,int16_t* out,int n) {
    using namespace dub::simd;
    auto& s=*static_cast<dub::Siren*>(p);alignas(16) float l[128],r[128];
    for(int offset=0;offset<n;offset+=128) {
        const int count=std::min(128,n-offset);s.render(l,r,count);
        int i=0;
        for(;i+4<=count;i+=4){
            const vf scale=splat(32768.f);
            store_stereo_s16(out+2*(offset+i),round_int(mul(clamp(load(l+i),-1.f,.999969f),scale)),
                             round_int(mul(clamp(load(r+i),-1.f,.999969f),scale)));
        }
        for(;i<count;++i){out[2*(i+offset)]=static_cast<int16_t>(std::lrint(std::max(-1.f,std::min(.999969f,l[i]))*32768.f));out[2*(i+offset)+1]=static_cast<int16_t>(std::lrint(std::max(-1.f,std::min(.999969f,r[i]))*32768.f));}
    }
}
extern "C" const mpc_engine_t* mpc_engine(void) {static const mpc_engine_t api={create,destroy,midi,set,get,render,nullptr};return &api;}

// Private numeric control bridge: preserve the existing exported VST/engine ABI.
extern "C" void dub_force_set_physical(void* instance,int index,float value) {
    static_cast<dub::Siren*>(instance)->set(index,value);
}
extern "C" int dub_force_get_physical(void* instance,int index,float* value) {
    if(!value || index<0 || index>=dub::P_Count)return 0;
    *value=static_cast<dub::Siren*>(instance)->get(index);return 1;
}
