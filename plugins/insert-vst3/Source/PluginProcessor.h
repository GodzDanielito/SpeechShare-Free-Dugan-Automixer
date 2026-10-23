#pragma once
#include <JuceHeader.h>
#include "SpeechEngine.h"
#include <array>
#include <atomic>
#include <memory>

class SpeechShareProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater {
public:
    SpeechShareProcessor();
    ~SpeechShareProcessor() override { cancelPendingUpdate(); }
    const juce::String getName() const override;
    void prepareToPlay(double sampleRate,int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int index) override { return index == 0 ? juce::String("Default") : juce::String(); }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    juce::AudioProcessorValueTreeState state;
    float micMeter(int index,int kind) const noexcept;
    float keyMeter(int g) const noexcept;
    float keyCut(int g) const noexcept;
    static juce::String mid(int i,const char* name);
    static juce::String kid(int i,const char* name);
    static juce::String gid(int i,const char* name);
private:
    static BusesProperties defaultBuses();
    void handleAsyncUpdate() override;
    speechshare::Controls readControls() const;
    struct ParamCache {
        std::atomic<float> *engine{}, *smart{}, *keyMaster{}, *routing{}, *micCount{},
            *rms{}, *strength{}, *attack{}, *release{}, *lookahead{}, *hpf{};
        std::array<std::atomic<float>*,3> keyEnabled{},keyDb{},keyType{},groupMute{},groupOverride{};
        std::array<std::atomic<float>*,speechshare::maxMics> weight{},mode{},group{},overrideEnabled{};
    } param;
    void cacheParameterPointers();
    speechshare::Engine engine;
    static constexpr int chunkFrames=128;
    std::array<float,chunkFrames*speechshare::maxChannels> scratch{};
    std::array<std::atomic<float>,speechshare::maxMics> micIn{}, micOut{}, micGain{};
    std::array<std::atomic<float>,3> keyIn{}, keyReduction{};
    std::atomic<int> pendingLatency{0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeechShareProcessor)
};
