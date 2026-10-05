#pragma once

namespace dub {
struct EchoConfig {
    float time=.4166667f, feedback=.42f, mix=1.f, ping=0.f;
};

// Dub tape echo: fractional feedback delay with fixed loop HP/LP filters and
// square-LFO ping-pong panning. Dry passes through at unity gain.
class Effects {
public:
    Effects();
    ~Effects();
    Effects(const Effects&)=delete;
    Effects& operator=(const Effects&)=delete;
    // n <= 16 samples that do not cross the 16-sample control grid. All three
    // buffers are padded to a multiple of four; padding lanes are scratch.
    void process(const float* dry,float* left,float* right,int n,const EchoConfig&) noexcept;
    void reset() noexcept;
private:
    struct Impl;
    Impl* impl;
};
}
