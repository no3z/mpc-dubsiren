#pragma once

namespace dub {
struct EchoConfig {
    float time=.4166667f, feedback=.42f, mix=1.f, hp=120.f, lp=7600.f;
    float ping=0.f, reverb=.16f;
    int character=0; // CLEAN, DESK, SMOKE, ORBIT, CLASH
    bool invert=false, freeze=false, bend=false, playing=false;
};

class Effects {
public:
    Effects();
    ~Effects();
    Effects(const Effects&)=delete;
    Effects& operator=(const Effects&)=delete;
    void process(float dry, const EchoConfig&, float& left, float& right) noexcept;
    void reset() noexcept;
private:
    struct Impl;
    Impl* impl;
};
}
