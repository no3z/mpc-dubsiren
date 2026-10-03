#include "siren.h"
#include "params.h"
extern "C" {
#include "engine.h"
}
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
static void* create(const char*) { try {return new dub::Siren;} catch(...) {return nullptr;} }
static void destroy(void* p) {delete static_cast<dub::Siren*>(p);}
static void midi(void* p,const uint8_t* msg,int len) {static_cast<dub::Siren*>(p)->midi(msg,len);}
static void set(void* p,const char* key,const char* val) {
    auto& s=*static_cast<dub::Siren*>(p);
    if(!std::strcmp(key,"state")) {
        if(std::strncmp(val,"DFS1 ",5)) return;
        float v[dub::P_Count]; const char* cur=val+5;
        for(int i=0;i<dub::P_Count;++i) {char* end=nullptr;v[i]=std::strtof(cur,&end);if(end==cur || !std::isfinite(v[i]))return;cur=end;}
        s.restore(v); return;
    }
    for(int i=0;i<dub::P_Count;++i)if(!std::strcmp(key,PARAMS[i].key)){char* end=nullptr;const float v=std::strtof(val,&end);if(end!=val)s.set(i,v);return;}
}
static int get(void* p,const char* key,char* buf,int cap) {
    if(!buf || cap<1)return 0;
    auto& s=*static_cast<dub::Siren*>(p);
    if(!std::strcmp(key,"state")) {
        int used=std::snprintf(buf,cap,"DFS1 ");
        for(int i=0;i<dub::P_Count && used<cap;++i) {
            const int n=std::snprintf(buf+used,cap-used,"%.9g ",s.get(i)); if(n<0 || n>=cap-used)return 0;used+=n;
        }
        return used<cap ? used+1 : 0;
    }
    for(int i=0;i<dub::P_Count;++i)if(!std::strcmp(key,PARAMS[i].key)){std::snprintf(buf,cap,"%.9g",s.get(i));return 1;}
    return 0;
}
static void render(void* p,int16_t* out,int n) {
    auto& s=*static_cast<dub::Siren*>(p);float l[128],r[128];
    for(int offset=0;offset<n;offset+=128) {
        const int count=std::min(128,n-offset);s.render(l,r,count);
        for(int i=0;i<count;++i){out[2*(i+offset)]=static_cast<int16_t>(std::lrint(std::max(-1.f,std::min(.999969f,l[i]))*32768.f));out[2*(i+offset)+1]=static_cast<int16_t>(std::lrint(std::max(-1.f,std::min(.999969f,r[i]))*32768.f));}
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
