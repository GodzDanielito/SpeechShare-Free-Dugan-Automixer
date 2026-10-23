#pragma once
#include <JuceHeader.h>
#include "ModularPatch.h"
#include "../Insert/InsertPluginProcessor.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace speechshare_live {
constexpr int maxSources=speechshare_modular::participants;
constexpr int maxDeviceChannels=speechshare_modular::maxPorts;
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
    juce::File configFile() const;
    juce::AudioDeviceManager manager;
    speechshare_modular::ModularPatch routing;
    std::vector<std::unique_ptr<SpeechShareInsertProcessor>> processors;
    // Two channels per mono/stereo participant and for each group mix.
    std::array<juce::AudioBuffer<float>,speechshare_modular::participants> sourceBuffers;
    std::array<std::array<juce::AudioBuffer<float>,3>,speechshare_modular::sessions> groupBuffers;
    std::atomic<bool> armed{false},ready{false};
    double currentRate=48000.0;
    juce::String deviceError;
};
}
