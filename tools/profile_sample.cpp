// Statistical profiling of unchanged release DSP; no instrumentation in DSP.
#include "siren.h"
#ifdef PROFILE_WRAPPER
#include "../tests/vst_abi.h"
#include "params.h"
static intptr_t host(AEffect*,int32_t,int32_t,intptr_t,void*,float){return 0;}
#endif
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ucontext.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <map>
#include <cstdint>
static uintptr_t pcs[100000];static volatile sig_atomic_t count=0;
static void sample(int,siginfo_t*,void* context){
#ifdef __arm__
 if(count<100000)pcs[count++]=((ucontext_t*)context)->uc_mcontext.arm_pc;
#endif
}
int main(int argc,char**argv){
 float l[128],r[128];
#ifdef PROFILE_WRAPPER
 AEffect* fx=VSTPluginMain(host);float* output[]={l,r};fx->dispatcher(fx,0,0,0,nullptr,0);fx->dispatcher(fx,10,0,0,nullptr,44100);
#else
 dub::Siren s;
#endif
 int n=argc>1?atoi(argv[1]):120;
 struct sigaction action{};action.sa_sigaction=sample;action.sa_flags=SA_SIGINFO;sigaction(SIGPROF,&action,nullptr);
 itimerval timer{};timer.it_interval.tv_usec=1000;timer.it_value=timer.it_interval;setitimer(ITIMER_PROF,&timer,nullptr);
#ifdef PROFILE_WRAPPER
 unsigned random=0x4653524e;
 for(int k=0;k<n*44100/128;++k){for(int j=0;j<NPARAMS;++j){random^=random<<13;random^=random>>17;random^=random<<5;fx->setParameter(fx,j,(random>>8)*(1.f/16777216));}fx->processReplacing(fx,nullptr,output,128);}
#else
 for(int p=0;p<12;++p){s.set(dub::P_stop,1);s.set(dub::P_preset,p);s.set(dub::P_latch,1);for(int k=0;k<n*44100/128/12;++k)s.render(l,r,128);}
#endif
 timer={};setitimer(ITIMER_PROF,&timer,nullptr);
 std::map<uintptr_t,int> histogram;for(int k=0;k<count;++k)++histogram[pcs[k]];
 for(auto item:histogram){Dl_info info{};dladdr((void*)item.first,&info);printf("%lx %d %s %s\n",(unsigned long)item.first,item.second,info.dli_fname?info.dli_fname:"?",info.dli_sname?info.dli_sname:"internal");}
}
