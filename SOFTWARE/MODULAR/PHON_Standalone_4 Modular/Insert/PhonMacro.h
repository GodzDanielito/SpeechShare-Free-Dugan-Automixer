#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Pure, testable UI macro. No audio thread state and no additional DSP.
// Advanced parameter order matches SharedPool: RMS, Strength, Attack, Release, HPF.
namespace speechshare_phon {
struct DetectorSettings {
    float rmsMs;
    float strength;
    float attackMs;
    float releaseMs;
    float hpfHz;
};
constexpr float clampPosition(float value) noexcept {
    return value < 0.f ? 0.f : (value > 100.f ? 100.f : value);
}
constexpr float lerp(float a,float b,float t) noexcept { return a+(b-a)*t; }
constexpr DetectorSettings soft{100.f, .60f, 28.f, 350.f,100.f};
constexpr DetectorSettings neutral{100.f,1.00f, 12.f,160.f,100.f};
constexpr DetectorSettings aggressive{100.f,2.20f, 4.f,70.f,100.f};
inline DetectorSettings mapping(float dial) noexcept {
    const float p=clampPosition(dial);
    const bool low=p<=50.f;
    const auto& a=low?soft:neutral;
    const auto& b=low?neutral:aggressive;
    const float t=low?p/50.f:(p-50.f)/50.f;
    return {lerp(a.rmsMs,b.rmsMs,t), lerp(a.strength,b.strength,t),
            lerp(a.attackMs,b.attackMs,t), lerp(a.releaseMs,b.releaseMs,t),100.f};
}
inline float positionForStrength(float strength) noexcept {
    if(!std::isfinite(strength))return 50.f;
    if(strength<=neutral.strength)
        return clampPosition(50.f*(strength-soft.strength)/(neutral.strength-soft.strength));
    return clampPosition(50.f+50.f*(strength-neutral.strength)/(aggressive.strength-neutral.strength));
}
inline std::array<float,5> advancedValues(float dial) noexcept {
    const auto s=mapping(dial);
    return {s.rmsMs,s.strength,s.attackMs,s.releaseMs,s.hpfHz};
}
}
