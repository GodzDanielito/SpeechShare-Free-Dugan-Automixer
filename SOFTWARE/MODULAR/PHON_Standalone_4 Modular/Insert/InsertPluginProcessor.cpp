#include "InsertPluginProcessor.h"
#include "InsertPluginEditor.h"
#include "PresentationPolicy.h"
#include <algorithm>

SpeechShareInsertProcessor::BusesProperties SpeechShareInsertProcessor::buses() {
    return BusesProperties().withInput("Source",juce::AudioChannelSet::stereo(),true)
                            .withOutput("Processed source",juce::AudioChannelSet::stereo(),true);
}
juce::AudioProcessorValueTreeState::ParameterLayout SpeechShareInsertProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout p;
    auto choice=[&p](const String& id,const String& title,StringArray labels,int value){
        p.add(std::make_unique<AudioParameterChoice>(ParameterID{id,1},title,labels,value));
    };
    auto number=[&p](const String& id,const String& title,float low,float high,float step,float value){
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},title,NormalisableRange<float>(low,high,step),value));
    };
    choice("session","Session (1-8)",{"1","2","3","4","5","6","7","8"},0);
    choice("group","Group",{"A","B","C"},0);
    choice("mode","Mode",{"MAN","AUTO","MUTE","BYPASS"},1);
    choice("smart","Detector",{"Classic","Smart (experimental)"},0);
    number("weight","Weight dB",-24.f,24.f,.1f,0.f);
    number("rms","Detector RMS ms",10.f,500.f,1.f,100.f);
    number("strength","Strength",.5f,3.f,.01f,1.f);
    number("attack","Attack ms",1.f,120.f,1.f,12.f);
    number("release","Release ms",10.f,1500.f,1.f,160.f);
    number("hpf","Detector HPF Hz",0.f,350.f,1.f,100.f);
    // Appended parameter ID: existing Session/Mode/etc. keep their IDs.
    p.add(std::make_unique<AudioParameterBool>(ParameterID{"channel_mute",1},"MUTE channel",false));
    // v0.0.6: append only: previous parameter IDs and ordering are unchanged.
    p.add(std::make_unique<AudioParameterBool>(ParameterID{"channel_solo",1},"SOLO channel",false));
    number("channel_level","Channel output gain dB",-90.f,12.f,.1f,0.f);
    return p;
}
SpeechShareInsertProcessor::SpeechShareInsertProcessor()
: juce::AudioProcessor(buses()),state(*this,nullptr,"INSERT_PARAMS",createLayout()) {
    auto pointer=[this](const char* name){return state.getRawParameterValue(name);};
    session=pointer("session");group=pointer("group");mode=pointer("mode");smart=pointer("smart");
    weight=pointer("weight");rms=pointer("rms");strength=pointer("strength");
    attack=pointer("attack");release=pointer("release");hpf=pointer("hpf");
    channelMute=pointer("channel_mute");
    channelSolo=pointer("channel_solo");channelLevel=pointer("channel_level");
    slot=speechshare_insert::SharedPool::instance().acquire();
    state.addParameterListener("channel_mute",this);
    state.addParameterListener("mode",this);
    state.addParameterListener("channel_solo",this);
    updateLocalMute();
    speechshare_insert::SharedPool::instance().setSolo(slot,channelSolo->load()>.5f);
    speechshare_insert::SharedPool::instance().setLevelDb(slot,channelLevel->load());
    // Host-independent control mailbox. Runs only on JUCE message thread,
    // never from audio processBlock. Timers stay alive while editor is closed.
    startTimerHz(60); // lower cross-instance GUI command latency (~17 ms nominal)
}
SpeechShareInsertProcessor::~SpeechShareInsertProcessor() {
    stopTimer();
    state.removeParameterListener("mode",this);
    state.removeParameterListener("channel_solo",this);
    state.removeParameterListener("channel_mute",this);
    speechshare_insert::SharedPool::instance().release(slot);
}
bool SpeechShareInsertProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    if (b.inputBuses.size()!=1||b.outputBuses.size()!=1) return false;
    const auto in=b.getMainInputChannelSet(),out=b.getMainOutputChannelSet();
    return in==out && (in==juce::AudioChannelSet::mono() || in==juce::AudioChannelSet::stereo());
}
void SpeechShareInsertProcessor::prepareToPlay(double sampleRate,int) {
    dsp.prepare(sampleRate);
    setLatencySamples(0);
    const auto controls=speechshare_insert::SharedPool::instance().getSessionControls(currentSession());
    postGainSmoothed=std::pow(10.,std::clamp(double(channelLevel->load()+controls.gainDb),-120.,24.)/20.);
}
void SpeechShareInsertProcessor::parameterChanged(const juce::String& param,float value) {
    // Callback value is authoritative even if APVTS raw atomic changes later.
    const bool direct = param=="channel_mute" ? value>.5f : channelMute->load()>.5f;
    const bool modeMute = param=="mode" ? int(value)==int(speechshare_insert::Mode::Mute) :
        int(mode->load())==int(speechshare_insert::Mode::Mute);
    speechshare_insert::SharedPool::instance().setLocalMute(slot,direct||modeMute);
    if(param=="channel_solo") speechshare_insert::SharedPool::instance().setSolo(slot,value>.5f);
}
void SpeechShareInsertProcessor::updateLocalMute() noexcept {
    speechshare_insert::SharedPool::instance().setLocalMute(slot,
        channelMute->load()>.5f || int(mode->load())==int(speechshare_insert::Mode::Mute));
}
void SpeechShareInsertProcessor::releaseResources() {
    speechshare_insert::SharedPool::instance().deactivate(slot);
}
void SpeechShareInsertProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals guard;
    const auto chans=buffer.getNumChannels();
    if (!(chans==1||chans==2)) return;
    speechshare_insert::Settings s;
    s.session=int(session->load())+1;
    s.group=int(group->load());
    s.mode=static_cast<speechshare_insert::Mode>(std::clamp(int(mode->load()),0,3));
    s.smart=smart->load()>.5f;
    const auto controls=speechshare_insert::SharedPool::instance().getSessionControls(s.session);
    const bool soloExcluded=speechshare_insert::SharedPool::instance().isSoloExcluded(slot,s.session);
    s.channelMute=channelMute->load()>.5f || controls.muted || soloExcluded;
    if(controls.bypass)s.mode=speechshare_insert::Mode::Bypass;
    s.weightDb=weight->load();s.rmsMs=rms->load();s.strength=strength->load();
    s.attackMs=attack->load();s.releaseMs=release->load();s.detectorHPF=hpf->load();
    dsp.process(buffer.getWritePointer(0),chans==2?buffer.getWritePointer(1):nullptr,chans,buffer.getNumSamples(),s,slot);
    // Post-automix output faders: no impact on Dugan weight/detection.
    // Smoothed to avoid hard discontinuities on manual or host automation moves.
    const double gainDb=std::clamp(double(channelLevel->load()+controls.gainDb),-120.,24.);
    const double gainTarget=std::pow(10.,gainDb/20.);
    const double coeff=std::exp(-1./(std::max(8000.,getSampleRate())*.003));
    for(int sample=0;sample<buffer.getNumSamples();++sample) {
        postGainSmoothed=gainTarget+coeff*(postGainSmoothed-gainTarget);
        for(int ch=0;ch<chans;++ch)
            buffer.getWritePointer(ch)[sample]*=float(postGainSmoothed);
    }
    const auto m=dsp.meters(); // audio-thread only: exchange via atomics
    const float postDb=float(20.*std::log10(std::max(postGainSmoothed,1.e-8)));
    speechshare_insert::SharedPool::instance().publishView(
        slot,s.session,s.group,
        static_cast<speechshare_insert::Mode>(std::clamp(int(mode->load()),0,3)),
        s.channelMute,s.smart,s.weightDb,m.inputDb,m.gainDb,m.outputDb+postDb);
}
juce::AudioProcessorEditor* SpeechShareInsertProcessor::createEditor() {
    return new SpeechShareInsertEditor(*this);
}
void SpeechShareInsertProcessor::commitParameter(const char* id,float plainValue) {
    auto* p=state.getParameter(id);
    if (!p) return;
    const float normalized=p->convertTo0to1(plainValue);
    if (std::abs(p->getValue()-normalized)<1.e-5f) return;
    p->beginChangeGesture();
    p->setValueNotifyingHost(normalized);
    p->endChangeGesture();
}
void SpeechShareInsertProcessor::setOwnSession(int value) {
    commitParameter("session",float(juce::jlimit(1,8,value)-1));
}
speechshare_insert::SessionKeys SpeechShareInsertProcessor::currentKeys() const noexcept {
    return speechshare_insert::SharedPool::instance().getSessionKeys(currentSession());
}
void SpeechShareInsertProcessor::setKeyMaster(bool enabled) {
    speechshare_insert::SharedPool::instance().setKeyMaster(currentSession(),enabled);
    updateHostDisplay(); // session non-parameter state changed: notify DAW
}
void SpeechShareInsertProcessor::setKeyGroup(int group,const speechshare_insert::KeyGroupSettings& value) {
    speechshare_insert::SharedPool::instance().setKeyGroup(currentSession(),group,value);
    updateHostDisplay();
}
void SpeechShareInsertProcessor::timerCallback() {
    // Work is done on the message thread. UI requests never call remote
    // processor objects directly and never write APVTS on the audio thread.
    const auto commands=speechshare_insert::SharedPool::instance().consumeEdits(slot);
    if(commands.mute!=-1) commitParameter("channel_mute",float(commands.mute));
    if(commands.mode!=-1) commitParameter("mode",float(commands.mode));
    if(commands.group!=-1) commitParameter("group",float(commands.group));
    if(commands.weightTenths!=100000) commitParameter("weight",float(commands.weightTenths)*.1f);
    if(commands.smart!=-1) commitParameter("smart",float(commands.smart));
    if(commands.solo!=-1) commitParameter("channel_solo",float(commands.solo));
    if(commands.levelTenths!=100000) commitParameter("channel_level",float(commands.levelTenths)*.1f);
    constexpr const char* advancedIds[5]={"rms","strength","attack","release","hpf"};
    for(int j=0;j<5;++j)
        if(commands.advancedHundredths[std::size_t(j)]!=-1)
            commitParameter(advancedIds[j],float(commands.advancedHundredths[std::size_t(j)])*.01f);
    std::string pendingName;
    if(speechshare_insert::SharedPool::instance().takeDisplayNameEdit(slot,pendingName))
        setSourceName(juce::String::fromUTF8(pendingName.c_str()));
    speechshare_insert::SharedPool::instance().setLevelDb(slot,channelLevel->load());
    speechshare_insert::SharedPool::instance().publishAdvanced(slot,
        {{rms->load(),strength->load(),attack->load(),release->load(),hpf->load()}});
    speechshare_insert::SharedPool::instance().publishConfig(slot,int(session->load())+1,
        int(group->load()),static_cast<speechshare_insert::Mode>(std::clamp(int(mode->load()),0,3)),
        smart->load()>.5f,weight->load());
}
speechshare_insert::SessionControls SpeechShareInsertProcessor::currentControls() const noexcept {
    return speechshare_insert::SharedPool::instance().getSessionControls(currentSession());
}
void SpeechShareInsertProcessor::setSessionControls(const speechshare_insert::SessionControls& controls) {
    speechshare_insert::SharedPool::instance().setSessionControls(currentSession(),controls);
    updateHostDisplay();
}
juce::String SpeechShareInsertProcessor::sourceName() const {
    return state.state.getProperty("source_name",juce::String()).toString();
}
bool SpeechShareInsertProcessor::hasCustomSourceName() const {
    // Legacy projects saved an explicit user label but had no manual flag.
    // Treat these labels as intentional so an upgrade cannot overwrite them.
    return bool(state.state.getProperty("source_name_manual",sourceName().isNotEmpty()));
}
void SpeechShareInsertProcessor::refreshSourceDisplayName() {
    const auto explicitName=sourceName();
    const auto label=hasCustomSourceName() && explicitName.isNotEmpty()
                     ? explicitName : hostTrackName;
    speechshare_insert::SharedPool::instance().setDisplayName(slot,label.toStdString());
}
void SpeechShareInsertProcessor::updateTrackProperties(const TrackProperties& properties) {
    // JUCE calls this from the message thread, if the VST3 host exposes it.
    // Empty/partial updates must not erase the last known host track name.
    if (!properties.name.has_value()) return;
    const auto candidate=properties.name->trim().substring(0,36);
    if (candidate.isEmpty() || candidate==hostTrackName) return;
    hostTrackName=candidate;
    if (speechshare_insert::shouldFollowHostTrackName(hasCustomSourceName(),
                                                      candidate.toStdString()))
        refreshSourceDisplayName();
}
void SpeechShareInsertProcessor::setSourceName(const juce::String& name) {
    const auto clean=name.trim().substring(0,36);
    state.state.setProperty("source_name",clean,nullptr);
    // Clearing a label intentionally returns to automatic host naming.
    state.state.setProperty("source_name_manual",clean.isNotEmpty(),nullptr);
    refreshSourceDisplayName();
    updateHostDisplay();
}
void SpeechShareInsertProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto xml=state.copyState().createXml();
    if (!xml) return;
    // Every instance saves the complete shared session-key bank so the project
    // can restore it even when its first instance is a different participant.
    auto* keyRoot=xml->createNewChildElement("SESSION_KEYS");
    keyRoot->setAttribute("version",1);
    auto& pool=speechshare_insert::SharedPool::instance();
    for(int sessionNumber=1;sessionNumber<=speechshare_insert::maxSessions;++sessionNumber) {
        const auto data=pool.getSessionKeys(sessionNumber);
        auto* sessionXml=keyRoot->createNewChildElement("SESSION");
        sessionXml->setAttribute("id",sessionNumber);
        sessionXml->setAttribute("master",data.master);
        const auto ctrl=pool.getSessionControls(sessionNumber);
        sessionXml->setAttribute("bypass",ctrl.bypass);
        sessionXml->setAttribute("mainMute",ctrl.muted);
        sessionXml->setAttribute("mainGainDb",double(ctrl.gainDb));
        for(int g=0;g<3;++g) {
            const auto& k=data.groups[std::size_t(g)];
            auto* keyXml=sessionXml->createNewChildElement("KEY");
            keyXml->setAttribute("group",g);
            keyXml->setAttribute("enabled",k.enabled);
            keyXml->setAttribute("generator",int(k.generator));
            keyXml->setAttribute("frequencyHz",double(k.frequencyHz));
            keyXml->setAttribute("levelDb",double(k.levelDb));
        }
    }
    copyXmlToBinary(*xml,dest);
}
void SpeechShareInsertProcessor::setStateInformation(const void* data,int bytes) {
    if (auto xml=getXmlFromBinary(data,bytes)) {
        if (xml->hasTagName(state.state.getType().toString())) {
            // Explicitly preserve older v0.0.4 states without SESSION_KEYS.
            // Shared bank is not cleared when no KEY data was saved.
            if (auto* root=xml->getChildByName("SESSION_KEYS")) {
                auto& pool=speechshare_insert::SharedPool::instance();
                for(auto* sessionXml:root->getChildIterator()) {
                    if (!sessionXml->hasTagName("SESSION")) continue;
                    const int id=sessionXml->getIntAttribute("id",-1);
                    if (id<1||id>speechshare_insert::maxSessions) continue;
                    pool.setKeyMaster(id,sessionXml->getBoolAttribute("master",false));
                    speechshare_insert::SessionControls controls;
                    controls.bypass=sessionXml->getBoolAttribute("bypass",false);
                    controls.muted=sessionXml->getBoolAttribute("mainMute",false);
                    controls.gainDb=float(sessionXml->getDoubleAttribute("mainGainDb",0.));
                    pool.setSessionControls(id,controls);
                    for(auto* keyXml:sessionXml->getChildIterator()) {
                        if(!keyXml->hasTagName("KEY"))continue;
                        const int g=keyXml->getIntAttribute("group",-1);
                        if(g<0||g>=3)continue;
                        speechshare_insert::KeyGroupSettings k;
                        k.enabled=keyXml->getBoolAttribute("enabled",true);
                        k.generator=keyXml->getIntAttribute("generator",1)==0 ?
                          speechshare_insert::KeyGenerator::Pink:speechshare_insert::KeyGenerator::Sine;
                        k.frequencyHz=float(keyXml->getDoubleAttribute("frequencyHz",40.));
                        k.levelDb=float(keyXml->getDoubleAttribute("levelDb",-56.));
                        pool.setKeyGroup(id,g,k);
                    }
                }
                xml->removeChildElement(root,true);
            }
            // The extra XML child is removed before passing state to APVTS.
            // Respect existing names from old projects as explicit overrides.
            auto restored=juce::ValueTree::fromXml(*xml);
            if (!restored.hasProperty("source_name_manual") &&
                restored.getProperty("source_name",juce::String()).toString().isNotEmpty())
                restored.setProperty("source_name_manual",true,nullptr);
            state.replaceState(restored);
            updateLocalMute();
            speechshare_insert::SharedPool::instance().setSolo(slot,channelSolo->load()>.5f);
            speechshare_insert::SharedPool::instance().setLevelDb(slot,channelLevel->load());
            refreshSourceDisplayName();
        }
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new SpeechShareInsertProcessor();}
