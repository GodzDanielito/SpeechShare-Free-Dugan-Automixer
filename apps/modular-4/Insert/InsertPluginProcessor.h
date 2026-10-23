#pragma once
#include <JuceHeader.h>
#include "InsertDSP.h"
#include <atomic>

class SpeechShareInsertProcessor final : public juce::AudioProcessor,
                                         private juce::AudioProcessorValueTreeState::Listener,
                                         private juce::Timer {
public:
    SpeechShareInsertProcessor();
    ~SpeechShareInsertProcessor() override;
    const juce::String getName() const override { return "SpeechShare Insert Prototype"; }
    void prepareToPlay(double sampleRate,int samplesPerBlock) override;
    void releaseResources() override;
    // Standalone only: called on the audio thread when an inactive route wakes.
    // Keeps old detector history out of the new signal path (no allocation).
    void resetDetectorForActivation() noexcept { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    double getTailLengthSeconds() const override {return 0.0;}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int index) override {return index==0?"Default":juce::String();}
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState state;
    int currentSession() const noexcept {return int(session->load())+1;}
    int instanceSlot() const noexcept {return slot;}
    void setOwnSession(int oneBasedSession); // UI/message thread only
    speechshare_insert::SessionKeys currentKeys() const noexcept;
    void setKeyMaster(bool enabled); // session-wide, UI/message thread only
    void setKeyGroup(int group,const speechshare_insert::KeyGroupSettings& value);
    speechshare_insert::SessionControls currentControls() const noexcept;
    void setSessionControls(const speechshare_insert::SessionControls& value);
    juce::String sourceName() const;
    void setSourceName(const juce::String& name);
    // Optional host metadata, supplied on the JUCE message thread by hosts
    // that implement the VST3 track-properties callback.
    void updateTrackProperties(const TrackProperties& properties) override;
private:
    void parameterChanged(const juce::String& parameterID,float newValue) override;
    void timerCallback() override;
    void commitParameter(const char* id,float plainValue);
    void updateLocalMute() noexcept;
    static BusesProperties buses();
    std::atomic<float> *session{},*group{},*mode{},*weight{},*rms{},*strength{},*attack{},*release{},*hpf{},*smart{},*channelMute{},*channelSolo{},*channelLevel{};
    bool hasCustomSourceName() const;
    void refreshSourceDisplayName();
    juce::String hostTrackName;
    double postGainSmoothed=1.;
    speechshare_insert::InsertDSP dsp;
    int slot=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeechShareInsertProcessor)
};
