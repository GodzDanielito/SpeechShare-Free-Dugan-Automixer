#pragma once
#include <JuceHeader.h>
#include "ModularPatch.h"
#include "SessionActivity.h"
#include "../Insert/InsertPluginProcessor.h"
#include "../Insert/SharedPool.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace speechshare_live {
constexpr int maxSources=speechshare_modular::participants;
constexpr int maxDeviceChannels=speechshare_modular::maxPorts;
static_assert(maxSources<=speechshare_insert::maxInstances, "Four session engine exceeds SharedPool capacity");
class StandaloneAudio final:public juce::AudioIODeviceCallback {
public:
    StandaloneAudio();
    ~StandaloneAudio() override;
    juce::AudioDeviceManager& devices() noexcept {return manager;}
    speechshare_modular::ModularPatch& patch() noexcept {return routing;}
    const speechshare_modular::ModularPatch& patch() const noexcept {return routing;}
    SpeechShareInsertProcessor* primary(int session=1) noexcept {
        if(session<1||session>speechshare_modular::sessions)return nullptr;
        const auto i=std::size_t((session-1)*speechshare_modular::participantsPerSession);
        return i<processors.size()?processors[i].get():nullptr;
    }
    int sourceCount() const noexcept {return int(processors.size());}
    int perSessionCount() const noexcept {return speechshare_modular::participantsPerSession;}
    void setArmed(bool enabled) noexcept {armed.store(enabled,std::memory_order_release);}
    bool isArmed() const noexcept {return armed.load(std::memory_order_acquire);}
    bool isAudioReady() const noexcept {return ready.load(std::memory_order_acquire);}
    // Callback execution time as a percentage of one device buffer deadline.
    // NOT OS CPU %, driver latency, or round-trip latency.
    float callbackLoadPercent() const noexcept {return loadPercent.load(std::memory_order_relaxed);}
    float callbackPeakPercent() const noexcept {return peakPercent.load(std::memory_order_relaxed);}
    bool sessionAwake(int s) const noexcept {
        if(s<0||s>=speechshare_modular::sessions)return false;
        return (awakeMask.load(std::memory_order_acquire)&(1u<<unsigned(s)))!=0u;
    }
    int awakeSessionCount() const noexcept {
        unsigned mask=awakeMask.load(std::memory_order_acquire);
        int count=0;
        while(mask){count+=int(mask&1u);mask>>=1u;}
        return count;
    }
    juce::String describeDevice() const;
    juce::String openSavedDevice();
    void saveConfiguration() const;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& message) override;
    void audioDeviceIOCallbackWithContext(const float* const* inputs,int numInputs,
        float* const* outputs,int numOutputs,int samples,const juce::AudioIODeviceCallbackContext&) override;
private:
    static constexpr int chunk=1024;
    void createProcessors();
    void deactivateProcessedSources() noexcept;
    void clearPerformance() noexcept;
    juce::File configFile() const;
    juce::AudioDeviceManager manager;
    speechshare_modular::ModularPatch routing;
    std::vector<std::unique_ptr<SpeechShareInsertProcessor>> processors;
    // Two channels per mono/stereo participant and for each group mix.
    std::array<juce::AudioBuffer<float>,speechshare_modular::participants> sourceBuffers;
    std::array<std::array<juce::AudioBuffer<float>,3>,speechshare_modular::sessions> groupBuffers;
    // Callback-owned state; no dynamic allocations and no message-thread writes.
    std::array<bool,speechshare_modular::participants> processedLastBlock{};
    std::array<float,speechshare_modular::participants> wakeFade{};
    std::atomic<unsigned> awakeMask{0};
    std::atomic<float> loadPercent{0.f},peakPercent{0.f};
    std::atomic<bool> armed{false},ready{false};
    double currentRate=48000.0;
    juce::String deviceError;
};
}
