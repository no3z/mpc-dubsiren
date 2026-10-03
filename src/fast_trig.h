#pragma once
#include <array>
#include <cmath>
namespace dub {
// Shared, immutable table initialized when the library loads. No audio allocation.
inline const std::array<float,8193> modulationSines=[]{
    std::array<float,8193> table{};
    for(unsigned i=0;i<=8192;++i)table[i]=float(std::sin(6.2831853071795864769*i/8192));
    table[8192]=table[0];return table;
}();
// Unit-cycle phase, linearly interpolated; peak error below 1.5e-7.
inline float modulationSine(float phase) noexcept {
    if(phase>=1.f)phase-=1.f;
    const float index=phase*8192.f;const unsigned i=unsigned(index);
    return modulationSines[i]+(index-float(i))*(modulationSines[i+1]-modulationSines[i]);
}
}
