#pragma once
#include <JuceHeader.h>
#include "PatchRouting.h"
#include "../Insert/InsertPluginProcessor.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace speechshare_live {
// This standalone application deliberately does not rely on JUCE's plugin
// standalone wrapper: that wrapper would expose only one stereo bus.
class StandaloneAudio final : public juce::AudioIODeviceCallback {
public:
    StandaloneAudio();
    ~StandaloneAudio() override;
    juce::AudioDeviceManager& devices() noexcept {return manager;}
    PatchRouting& patch() noexcept {return routing;}
    const PatchRouting& patch() const noexcept {return routing;}
    SpeechShareInsertProcessor* primary() noexcept {return processors.empty()?nullptr:processors.front().get();}
    int sourceCount() const noexcept {return int(processors.size());}
    void setSourceCount(int count); // message thread, DISARMS and reinitializes participants
    void setArmed(bool enabled) noexcept {armed.store(enabled,std::memory_order_release);}
    bool isArmed() const noexcept {return armed.load(std::memory_order_acquire);}
    bool isAudioReady() const noexcept {return ready.load(std::memory_order_acquire);}
    juce::String describeDevice() const;
    juce::String openSavedDevice();
    void saveConfiguration() const; // call while disarmed
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& message) override;
    void audioDeviceIOCallbackWithContext(const float* const* inputs,int numInputs,
        float* const* outputs,int numOutputs,int samples,const juce::AudioIODeviceCallbackContext&) override;
private:
    static constexpr int chunk=1024;
    void createProcessors(int count);
    juce::File configFile() const;
    juce::AudioDeviceManager manager;
    PatchRouting routing;
    std::vector<std::unique_ptr<SpeechShareInsertProcessor>> processors;
    // Owned buffers preallocated before the audio callback (no per-block heap allocations).
    std::array<juce::AudioBuffer<float>,maxSources> sourceBuffers;
    std::atomic<bool> armed{false},ready{false};
    double currentRate=48000.0;
    juce::String deviceError;
};
}
