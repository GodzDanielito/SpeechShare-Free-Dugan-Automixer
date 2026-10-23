#pragma once
#include <JuceHeader.h>
#include "InsertPluginProcessor.h"
#include "SharedPool.h"
#include <array>
#include <memory>

// A session-wide control surface, hosted in any per-track plugin editor.
// Peer parameter edits are delivered via SharedPool mailboxes to the owning
// processor on the message thread. No editor holds a pointer to peer objects.
class SpeechShareInsertEditor final : public juce::AudioProcessorEditor,
                                       private juce::Timer {
public:
    explicit SpeechShareInsertEditor(SpeechShareInsertProcessor&);
    ~SpeechShareInsertEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class ChannelStrip;
    class BroadcastLookAndFeel;
    class RightPanel;
    void timerCallback() override;
    void refreshParticipants();
    void refreshSessionControls();
    void refreshKeys();
    void refreshAdvanced();
    void updateKeyTabs();
    void pushAdvanced(int parameter);
    void pushKey(int group);
    void setDetector(bool smart);
    void updateSessionControls();
    void exportPreset();
    void importPreset();
    void changeBank(int step);
    void setViewSize(int count);
    SpeechShareInsertProcessor& processor;
    std::unique_ptr<BroadcastLookAndFeel> skin;
    juce::ImageComponent backstageLogo;
    juce::Label heading,statusLabel,sessionInfo,keyHeading,stripHeading,footnote;
    juce::ComboBox sessionSelector;
    juce::TextButton classicButton{"CLASSIC"},smartButton{"SMART"},smartHelp{"?"};
    juce::TextButton presetCopy{"COPY PRESET"},presetPaste{"PASTE PRESET"};
    juce::TextButton masterMute{"MASTER MUTE"},masterBypass{"GLOBAL BYPASS"};
    juce::Slider masterGain;
    juce::Label masterGainLabel;
    juce::TextButton bankPrev{"<"},bankNext{">"},view8{"8 CH"};
    juce::ToggleButton keyMaster{"KEY MASTER"};
    std::array<juce::TextButton,3> keyTabs;
    std::array<juce::Label,3> keyLabels;
    std::array<juce::ToggleButton,3> keyEnable;
    std::array<juce::ComboBox,3> keyType;
    std::array<juce::Slider,3> keyFrequency,keyLevel;
    juce::Label advancedHeading,advancedState,lookaheadNotice;
    std::array<juce::Label,5> advancedLabels;
    std::array<juce::Slider,5> advancedSliders;
    std::unique_ptr<RightPanel> rightContent;
    juce::Viewport rightViewport;
    juce::Viewport stripViewport;
    juce::Component stripContent;
    std::array<std::unique_ptr<ChannelStrip>,speechshare_insert::maxInstances> strips;
    int visibleTotal=0,bank=0,bankSize=8;
    int displayedPerPage=8,stripColumnWidth=84;
    int selectedKeyGroup=0;
    bool refreshing=false;
    // Suppress early JUCE/host resize callbacks until child viewports exist.
    bool editorReady=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeechShareInsertEditor)
};
