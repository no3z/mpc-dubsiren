#include "effects.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>

static unsigned allocationCount=0;
void* operator new(std::size_t bytes) {
    ++allocationCount;
    if (void* p=std::malloc(bytes)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}

static void require(bool value,const char* message) {
    if (!value) { std::fprintf(stderr,"effects test failed: %s\n",message);std::exit(1); }
}
// Streams samples through the 16-sample chunk contract, like Siren::render.
struct Runner {
    dub::Effects fx;
    alignas(16) float in[16]={},left[16]={},right[16]={};
    int fill=0;unsigned grid=0;
    float l=0,r=0;
    // Odd chunk lengths exercise padding; they never cross the 16-sample grid.
    int length=16;
    void process(float dry,const dub::EchoConfig& c) {
        in[fill++]=dry;
        // >= flushes a partly filled chunk when length shrinks; it still ends on the grid.
        if(fill>=std::min(length,16-int(grid%16))){
            for(int j=fill;j<16;++j)in[j]=0.f;
            fx.process(in,left,right,fill,c);grid+=unsigned(fill);fill=0;
        }
    }
    // One sample in, one sample out: run single-sample chunks.
    void step(float dry,const dub::EchoConfig& c) {
        alignas(16) float x[4]={dry,0,0,0},a[4],b[4];
        fx.process(x,a,b,1,c);++grid;l=a[0];r=b[0];
    }
    void reset() {fx.reset();grid=0;fill=0;}
};
int main() {
    Runner run;
    dub::Effects whole,single;
    const unsigned allocationsAfterConstruction=allocationCount;
    dub::EchoConfig c;
    for (int i=0;i<44100;++i) {
        run.step(0.f,c);
        require(run.l==0.f&&run.r==0.f,"fresh silence");
    }
    // Dry passes at unity; no early echo; first repeat is unfiltered at 62% wet.
    c.time=.05f;c.feedback=0.f;
    for (int i=0;i<44100*4;++i) run.step(0.f,c);
    run.reset();
    run.step(1.f,c);
    require(run.l==1.f&&run.r==1.f,"dry remains present at unity");
    float firstEcho=0.f;
    for (int i=1;i<2400;++i) {
        run.step(0.f,c);
        if (i>=2203&&i<=2207) firstEcho+=std::fabs(run.l);
        if (i<2203) require(std::fabs(run.l)<1e-6f,"no early echo");
    }
    require(firstEcho>.55f,"first repeat bypasses feedback filters");
    // Feedback repeats decay through the loop filters.
    run.reset();c.feedback=.5f;
    for(int i=0;i<44100;++i)run.step(0.f,c);
    run.reset();run.step(1.f,c);
    // Every pass through the feedback path takes 128 samples more, as in Web Audio:
    // repeat k arrives at k*T+(k-1)*128 samples.
    constexpr int T=2205,loopLatency=128;
    double second=0,third=0,early=0;
    for(int i=1;i<T*3+2*loopLatency+40;++i){
        run.step(0.f,c);
        if(i>T*2-20&&i<T*2+20)early+=std::fabs(run.l);
        if(i>T*2+loopLatency-20&&i<T*2+loopLatency+20)second+=std::fabs(run.l);
        if(i>T*3+2*loopLatency-20&&i<T*3+2*loopLatency+20)third+=std::fabs(run.l);
    }
    require(early<1e-3,"second repeat is delayed by the 128-sample feedback latency");
    require(second>.05&&third>.01&&third<second,"feedback repeats decay");
    // Reset invalidates every delay state immediately without clearing memory.
    run.reset();
    for (int i=0;i<44100;++i) {
        run.step(0.f,c);
        require(run.l==0.f&&run.r==0.f,"reset discards tails");
    }
    // Chunked vector paths (steady and ramping delay) match one-sample chunks.
    {dub::EchoConfig e;e.feedback=.8f;double worst=0;
     alignas(16) float x[16],l1[16],r1[16];
     for(int k=0;k<44100*6/16;++k){
         if(k==44100*2/16)e.time=.9f;
         if(k==44100*4/16)e.time=.12f;
         for(int j=0;j<16;++j){const int i=16*k+j;x[j]=(i/4410)%2 ? .3f*std::sin(.03f*i) : 0.f;}
         whole.process(x,l1,r1,16,e);
         for(int j=0;j<16;++j){alignas(16) float y[4]={x[j],0,0,0},a[4],b[4];single.process(y,a,b,1,e);
             worst=std::max(worst,double(std::max(std::fabs(a[0]-l1[j]),std::fabs(b[0]-r1[j]))));}
     }
     std::printf("chunked vs single-sample echo max difference %.3g\n",worst);
     require(worst<1e-5,"chunked vector echo matches single-sample processing");}
    // Ping pong alternates channels; zero ping keeps them identical.
    c.time=.05f;c.feedback=.7f;c.ping=1.f;
    double difference=0.f;
    for (int i=0;i<88200;++i) {
        const float dry=.2f*std::sin(6.28318530718f*620.f*float(i)/44100.f);
        run.process(dry,c);
        if(run.fill==0)for(int j=0;j<16;++j){require(std::isfinite(run.left[j])&&std::isfinite(run.right[j]),"finite stereo");difference+=std::fabs(run.left[j]-run.right[j]);}
    }
    require(difference>100.f,"ping pong panning operates");
    c.ping=0.f;for(int i=0;i<44100;++i)run.process(.1f,c);
    for(int i=0;i<4410;++i){run.process(.2f*std::sin(.05f*i),c);if(run.fill==0)for(int j=0;j<16;++j)require(run.left[j]==run.right[j],"zero ping is centered");}
    // Fast edits at high feedback with odd chunk lengths; invalid controls guarded.
    unsigned rng=13579;run.length=5;
    for (int i=0;i<44100*12;++i) {
        if (i%97==0) {
            rng=rng*1664525u+1013904223u;
            c.time=.05f+float((rng>>8)%296)/100.f;
            c.feedback=.88f;c.mix=float(rng%101)/100.f;c.ping=float((rng>>3)%101)/100.f;
        }
        run.process(.5f*std::sin(float(i)*.13f),c);
        if(run.fill==0)for(int j=0;j<16;++j){
            require(std::isfinite(run.left[j])&&std::isfinite(run.right[j]),"finite randomized extreme output");
            require(std::fabs(run.left[j])<=32.f&&std::fabs(run.right[j])<=32.f,"bounded randomized output");
        }
    }
    c.time=std::numeric_limits<float>::quiet_NaN();c.feedback=std::numeric_limits<float>::infinity();
    c.mix=std::numeric_limits<float>::quiet_NaN();c.ping=std::numeric_limits<float>::infinity();
    for (int i=0;i<10000;++i) {
        run.step(std::numeric_limits<float>::quiet_NaN(),c);
        require(std::isfinite(run.l)&&std::isfinite(run.r),"invalid controls/input guarded");
    }
    require(allocationCount==allocationsAfterConstruction,"audio processing/reset perform no heap allocation");
    std::puts("effects_test PASSED");
}
