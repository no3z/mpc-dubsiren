#include "effects.h"
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
int main() {
    dub::Effects effects;
    const unsigned allocationsAfterConstruction=allocationCount;
    dub::EchoConfig c;
    float l=0.f,r=0.f;
    // Quiet fresh buffers and reverb have no independent noise for CLEAN.
    for (int i=0;i<44100;++i) {
        effects.process(0.f,c,l,r);
        require(l==0.f&&r==0.f,"fresh clean silence");
    }
    // Remove spread/reverb using DESK; feedback must not attenuate first repeat.
    c.character=1;c.time=.05f;c.feedback=0.f;c.reverb=0.f;c.playing=false;
    for (int i=0;i<44100*4;++i) effects.process(0.f,c,l,r);
    effects.reset();
    effects.process(1.f,c,l,r);
    require(l>.99f&&r>.99f,"dry remains present");
    float firstEcho=0.f;
    for (int i=1;i<2400;++i) {
        effects.process(0.f,c,l,r);
        if (i>=2203&&i<=2207) firstEcho+=std::fabs(l);
        if (i<2203) require(std::fabs(l)<1e-6f,"no early echo");
    }
    require(firstEcho>.55f,"first repeat bypasses feedback filters");
    // Reset invalidates every delay/reverb state immediately, including stale
    // previously populated ring slots, without clearing their allocations.
    effects.reset();
    for (int i=0;i<44100;++i) {
        effects.process(0.f,c,l,r);
        require(l==0.f&&r==0.f,"reset discards tails");
    }
    // Panning must deliver audible alternating left/right energy in held signal.
    c.character=0;c.time=.05f;c.feedback=.7f;c.reverb=.4f;c.ping=1.f;c.playing=true;
    double difference=0.f;
    for (int i=0;i<88200;++i) {
        const float dry=.2f*std::sin(6.28318530718f*620.f*float(i)/44100.f);
        effects.process(dry,c,l,r);
        require(std::isfinite(l)&&std::isfinite(r),"finite stereo");
        difference+=std::fabs(l-r);
    }
    require(difference>100.f,"stereo panning/spread operates");
    // Fast edits exercise modulated delays, interpolation wrap, filters, freeze,
    // polarity and all five characters at high feedback; invalid controls guarded.
    unsigned rng=13579;
    for (int i=0;i<44100*12;++i) {
        if (i%97==0) {
            rng=rng*1664525u+1013904223u;
            c.character=int(rng%5);c.time=.05f+float((rng>>8)%296)/100.f;
            c.hp=40.f+float(rng%1161);c.lp=800.f+float((rng>>4)%11201);
            c.feedback=.88f;c.mix=float(rng%101)/100.f;c.ping=float((rng>>3)%101)/100.f;
            c.freeze=(rng&1u)!=0;c.bend=(rng&2u)!=0;c.playing=(rng&4u)!=0;c.invert=(rng&8u)!=0;
            c.reverb=1.f;
        }
        effects.process(.5f*std::sin(float(i)*.13f),c,l,r);
        require(std::isfinite(l)&&std::isfinite(r),"finite randomized extreme output");
        require(std::fabs(l)<=32.f&&std::fabs(r)<=32.f,"bounded randomized output");
    }
    c.time=std::numeric_limits<float>::quiet_NaN();c.feedback=std::numeric_limits<float>::infinity();
    c.hp=-std::numeric_limits<float>::infinity();c.lp=std::numeric_limits<float>::quiet_NaN();
    c.mix=std::numeric_limits<float>::quiet_NaN();c.ping=std::numeric_limits<float>::infinity();
    for (int i=0;i<10000;++i) {
        effects.process(std::numeric_limits<float>::quiet_NaN(),c,l,r);
        require(std::isfinite(l)&&std::isfinite(r),"invalid controls/input guarded");
    }
    require(allocationCount==allocationsAfterConstruction,"audio processing/reset perform no heap allocation");
    std::puts("effects_test PASSED");
}
