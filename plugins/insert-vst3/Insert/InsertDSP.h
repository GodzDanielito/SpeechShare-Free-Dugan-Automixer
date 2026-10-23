#pragma once
#include "SharedPool.h"
#include <array>
#include <vector>
#include <cstdint>

namespace speechshare_insert {
struct Settings {
    int session=1, group=0;
    Mode mode=Mode::Auto;
    bool channelMute=false; // quick plugin mute: distinct from AUTO / MAN / BYPASS
    bool smart=false;
    float weightDb=0.f, rmsMs=100.f, strength=1.f;
    float attackMs=12.f, releaseMs=160.f, detectorHPF=100.f;
};
struct Metrics { float inputDb=-96.f,gainDb=0.f,outputDb=-96.f; };

// Source-local detector / smoother. A mono or stereo input is ONE participant.
// IMPORTANT: approximation of SpeechEngine Classic/Smart. Virtual KEY scores
// follow the reference denominator/normalisation design, but distributed
// instances cannot reproduce the sample-exact central engine. No lookahead
// or deterministic offline scheduling yet. Original SpeechEngine untouched.
class InsertDSP final {
public:
    void prepare(double sr);
    void reset();
    void process(float* left,float* right,int channels,int frames,const Settings& settings,int slot) noexcept;
    Metrics meters() const noexcept {return meter;}
    // Diagnostics only: virtual reference detector never reaches the audio output.
    float keyDetectorDb() const noexcept {return keyDbMeter;}
private:
    double rate=48000.,powerSum=0.,fastPower=0.,noisePower=0.,rawGain=1.,targetGain=1.,normalization=1.,normalizationTarget=1.,muteEnvelope=1.;
    std::array<double,2> hpX{},hpY{};
    std::vector<float> rmsRing;
    int maxWindow=1,window=1,position=0,hop=0;
    int previousSession=-1,previousGroup=-1;
    struct VirtualKey {
        double phase=0., b0=0.,b1=0.,b2=0., hpX=0., hpY=0., detectorPower=0.;
        std::uint32_t random=0x4c696e61u;
        void tick(double frequencyStep,double amplitude,double highPassCoefficient,
                  double detectorSmoothing,KeyGenerator generator) noexcept;
    } key;
    float keyDbMeter=-96.f;
    Metrics meter{};
};
}
