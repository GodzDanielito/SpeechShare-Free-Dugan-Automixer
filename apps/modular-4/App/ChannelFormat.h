#pragma once
namespace speechshare_modular {
// Fixed, sample-independent channel-format conversions; no delay memory.
constexpr float monoToStereo = 0.7071067811865475f;
inline float stereoToMono(float left,float right) noexcept {
    return 0.5f*(left+right);
}
}
