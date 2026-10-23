#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class SpeechShareEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit SpeechShareEditor(SpeechShareProcessor&);
    ~SpeechShareEditor() override;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    using State=juce::AudioProcessorValueTreeState;
    using SliderAttachment=State::SliderAttachment;
    using ButtonAttachment=State::ButtonAttachment;
    using ComboAttachment=State::ComboBoxAttachment;
    class MicStrip final : public juce::Component {
    public:
        explicit MicStrip(SpeechShareProcessor&);
        void setIndex(int);
        void refresh();
        void resized() override;
        void paint(juce::Graphics&) override;
    private:
        SpeechShareProcessor& processor;
        int index=0;
        juce::Slider weight;
        juce::ComboBox group;
        juce::ToggleButton overrideButton{"OVERRIDE"};
        juce::TextButton man{"MAN"},autoButton{"AUTO"},mute{"MUTE"},bypass{"BYPASS"};
        std::unique_ptr<SliderAttachment> weightAttach;
        std::unique_ptr<ComboAttachment> groupAttach;
        std::unique_ptr<ButtonAttachment> overrideAttach;
        void mode(int);
    };
    class KeyStrip final : public juce::Component {
    public:
        KeyStrip(SpeechShareProcessor&,int);
        void resized() override;
        void paint(juce::Graphics&) override;
    private:
        SpeechShareProcessor& processor;
        int groupNumber;
        juce::ComboBox type;
        juce::Slider level;
        juce::ToggleButton enabled{"KEY ON"};
        std::unique_ptr<ComboAttachment> typeAttach;
        std::unique_ptr<SliderAttachment> levelAttach;
        std::unique_ptr<ButtonAttachment> enabledAttach;
    };
    class MasterPane final : public juce::Component {
    public:
        explicit MasterPane(SpeechShareProcessor&);
        void refreshed();
        void resized() override;
        void paint(juce::Graphics&) override;
    private:
        SpeechShareProcessor& processor;
        juce::ToggleButton engine{"ENGINE ON"},keyMaster{"KEY ON"},smart{"SMART"};
        juce::TextButton meterButton{"METER: GAIN"};
        juce::ComboBox routing;
        juce::Slider micCount;
        std::unique_ptr<ButtonAttachment> engineAttach,keyAttach,smartAttach;
        std::unique_ptr<ComboAttachment> routingAttach;
        std::unique_ptr<SliderAttachment> countAttach;
        std::array<juce::Slider,6> advanced;
        std::array<juce::Label,6> advancedLabels;
        std::array<std::unique_ptr<SliderAttachment>,6> advancedAttach;
        std::array<juce::ToggleButton,3> groupMute,groupOverride;
        std::array<std::unique_ptr<ButtonAttachment>,3> muteAttach,overrideAttach;
    };
    void timerCallback() override;
    SpeechShareProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;
    juce::Label header,subtitle;
    std::array<juce::TextButton,4> pages;
    std::array<std::unique_ptr<MicStrip>,8> mics;
    std::array<std::unique_ptr<KeyStrip>,3> keys;
    MasterPane master;
    int page=0;
    void changePage(int);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeechShareEditor)
};
