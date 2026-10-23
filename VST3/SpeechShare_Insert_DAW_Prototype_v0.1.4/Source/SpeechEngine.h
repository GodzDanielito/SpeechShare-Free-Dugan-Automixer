#pragma once
// SpeechShare DSP engine: JUCE-independent, C++17.
// Port of the v1.5 gain sharing, dual envelope, adaptive floor and virtual keys.
// Copyright (c) 2026 SpeechShare contributors. Independent implementation.
#include <array>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace speechshare {
constexpr int maxMics = 32;
constexpr int maxChannels = maxMics * 2;
constexpr int groups = 3;

enum class ChannelMode : int { manual=0, automatic=1, mute=2, bypass=3 };
enum class Generator : int { pink=0, sine40=1, sine1000=2 };
enum class Routing : int { multi=0, sum=1, bypass=2 };

struct Controls {
    bool engine = true;
    bool smart = false;
    bool keyMaster = false;
    Routing routing = Routing::multi;
    int activeMics = 32;
    float rmsMs = 100.f, strength = 1.f, attackMs = 12.f,
          releaseMs = 160.f, lookaheadMs = 0.f, highPassHz = 100.f;
    std::array<float, 3> keyDb {{-56.f,-56.f,-56.f}};
    std::array<Generator, 3> keyType {{Generator::sine40,Generator::sine40,Generator::sine40}};
    std::array<bool, 3> keyEnabled {{true,true,true}};
    std::array<bool, 3> groupOverride {{false,false,false}}, groupMute {{false,false,false}};
    std::array<float, maxMics> weightDb {};
    std::array<ChannelMode, maxMics> mode {};
    std::array<int, maxMics> group {};
    std::array<bool, maxMics> overrideEnabled {};
    Controls() { mode.fill(ChannelMode::automatic); group.fill(0); }
};

struct Meters {
    std::array<float,maxMics> inputDb {}, outputDb {}, gainDb {};
    std::array<float,3> keyPostDb {}, keyCutDb {};
    Meters() { inputDb.fill(-96.f);outputDb.fill(-96.f); gainDb.fill(0.f);
               keyPostDb.fill(-96.f); keyCutDb.fill(0.f); }
};

class Engine {
public:
    void prepare(double sampleRate);
    void reset();
    // inOut: interleaved frames; channel count 2,4,...,64.
    // No allocations or locks on the audio thread.
    void process(float* interleaved, int frames, int channels, const Controls&);
    int latencySamples(const Controls&) const noexcept;
    const Meters& meters() const noexcept { return meter; }
private:
    double rate=48000.;
    int maxRms=0,maxDelay=0,window=1,windowPos=0,delay=0,delayPos=0,hop=0;
    bool previousSmart=true;
    int fadeRemaining=0;
    long long noiseSamples=0;
    uint32_t randomState=0x4c696e61u;
    std::vector<float> rmsRing, delayRing;
    struct Mic {
        float hpX[2]{}, hpY[2]{};
        double powerSum=0, fastPower=0, noisePower=0, slowPower=0, score=0;
        float inL=0,inR=0, gain=1,target=1, finalGain=1;
    };
    struct Key {
        double phase=0, b0=0,b1=0,b2=0,hpX=0,hpY=0,prePower=0,detPower=0,score=0;
    };
    std::array<Mic,maxMics> mics{};
    std::array<Key,3> keys{};
    std::array<double,3> groupSum{}, groupTarget{{1,1,1}}, groupGoalSmooth{{1,1,1}}, groupNorm{{1,1,1}};
    std::array<int,3> groupCount{};
    Meters meter{};
    static float db(double amp);
    double noise();
};
}
