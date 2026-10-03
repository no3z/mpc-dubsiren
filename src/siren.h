#pragma once
#include "param_ids.h"
namespace dub {
constexpr float SampleRate = 44100.0f;
class Siren {
public:
    Siren();
    ~Siren();
    Siren(const Siren&) = delete;
    Siren& operator=(const Siren&) = delete;
    void set(int id, float physical) noexcept;
    float get(int id) const noexcept;
    void restore(const float* values) noexcept;
    void midi(const unsigned char* msg, int len) noexcept;
    void render(float* left, float* right, int frames) noexcept;
private:
    struct Impl;
    Impl* impl;
};
}
