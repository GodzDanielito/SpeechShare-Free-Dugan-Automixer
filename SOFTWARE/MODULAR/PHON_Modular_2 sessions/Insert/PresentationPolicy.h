#pragma once
#include <cmath>
#include <string>

// Presentation-only helpers: never called by the audio DSP.
namespace speechshare_insert {
inline float canonicalDisplayDb(float db) noexcept {
    if (!std::isfinite(db)) return -96.f;
    if (std::abs(db) < .05f) return 0.f;
    return db;
}
// A host-provided track name must never overwrite a deliberate user label.
inline bool shouldFollowHostTrackName(bool customNameSet, const std::string& hostName) noexcept {
    return !customNameSet && !hostName.empty();
}
}
