#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace {
using namespace juce;
using Params=AudioProcessorValueTreeState::ParameterLayout;
using R=NormalisableRange<float>;
using Pid=ParameterID;
static void addFloat(Params& p,const String& id,const String& title,
                     float min,float max,float step,float value) {
    p.add(std::make_unique<AudioParameterFloat>(Pid{id,1},title,R(min,max,step),value));
}
static void addBool(Params& p,const String& id,const String& title,bool value) {
    p.add(std::make_unique<AudioParameterBool>(Pid{id,1},title,value));
}
static void addChoice(Params& p,const String& id,const String& title,
                      StringArray names,int value) {
    p.add(std::make_unique<AudioParameterChoice>(Pid{id,1},title,names,value));
}
}
SpeechShareProcessor::BusesProperties SpeechShareProcessor::defaultBuses() {
#if SPEECHSHARE_MULTI
    const auto layout=juce::AudioChannelSet::create7point1point2(); // VST3-representable 10-channel arrangement for five stereo lavs
#else
    const auto layout=juce::AudioChannelSet::stereo();
#endif
    return BusesProperties().withInput("Microphones",layout,true)
                                             .withOutput("Processed microphones",layout,true);
}
juce::String SpeechShareProcessor::mid(int i,const char* name) {return "mic_"+juce::String(i)+"_"+name;}
juce::String SpeechShareProcessor::kid(int i,const char* name) {return "key_"+juce::String(i)+"_"+name;}
juce::String SpeechShareProcessor::gid(int i,const char* name) {return "group_"+juce::String(i)+"_"+name;}

juce::AudioProcessorValueTreeState::ParameterLayout SpeechShareProcessor::makeLayout() {
    Params p;
    addBool(p,"engine","Engine",true);
    addBool(p,"smart","Smart engine",false);
    addBool(p,"key_master","KEY master",false);
    addChoice(p,"meter","Meter display",{"Gain","Input","Output"},0);
    addChoice(p,"routing","Routing",{"Multitrack","Summed stereo","Bypass"},0);
    addFloat(p,"mic_count","Active microphones",1,32,1,32);
    addFloat(p,"rms","RMS ms",10,500,1,100);
    addFloat(p,"strength","Strength",.5f,3,.01f,1);
    addFloat(p,"attack","Attack ms",1,120,1,12);
    addFloat(p,"release","Release ms",10,1500,1,160);
    addFloat(p,"lookahead","Lookahead ms",0,30,1,0);
    addFloat(p,"hpf","Detector HPF Hz",0,350,1,100);
    for(int g=0;g<3;++g) {
        addBool(p,kid(g,"enabled"),"KEY "+juce::String::charToString('A'+g)+" on",true);
        addFloat(p,kid(g,"db"),"KEY "+juce::String::charToString('A'+g)+" dBFS",-96,-24,.1f,-56);
        addChoice(p,kid(g,"type"),"KEY "+juce::String::charToString('A'+g)+" generator",{"Pink noise","Sine 40 Hz","Sine 1000 Hz"},1);
        addBool(p,gid(g,"mute"),"Group mute",false);
        addBool(p,gid(g,"override"),"Group override",false);
    }
    for(int i=0;i<speechshare::maxMics;++i) {
        addFloat(p,mid(i,"weight"),"Mic "+juce::String(i+1)+" weight dB",-24,24,.1f,0);
        addChoice(p,mid(i,"mode"),"Mic "+juce::String(i+1)+" mode",{"MAN","AUTO","MUTE","BYPASS"},1);
        addChoice(p,mid(i,"group"),"Mic "+juce::String(i+1)+" group",{"A","B","C"},0);
        addBool(p,mid(i,"override"),"Mic "+juce::String(i+1)+" override",false);
    }
    return p;
}
SpeechShareProcessor::SpeechShareProcessor():juce::AudioProcessor(defaultBuses()),state(*this,nullptr,"PARAMETERS",makeLayout()) {
    cacheParameterPointers();
    for(auto& a:micIn)a.store(-96);for(auto& a:micOut)a.store(-96);
    for(auto& a:micGain)a.store(0);for(auto& a:keyIn)a.store(-96);
    for(auto& a:keyReduction)a.store(0);
}
const juce::String SpeechShareProcessor::getName() const {
#if SPEECHSHARE_MULTI
    return "SpeechShare Multitrack";
#else
    return "SpeechShare Stereo";
#endif
}
bool SpeechShareProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    if(b.inputBuses.size()!=1 || b.outputBuses.size()!=1) return false;
    const int in=b.getMainInputChannels(), out=b.getMainOutputChannels();
#if SPEECHSHARE_MULTI
    // VST3 cannot represent arbitrary JUCE discrete channel sets.
    // Constrain the Fourier compatibility candidate to supported, named speaker sets.
    const auto inputSet = b.getMainInputChannelSet();
    const auto outputSet = b.getMainOutputChannelSet();
    return inputSet == outputSet
        && !inputSet.isDiscreteLayout()
        && in >= 2 && in <= 16 && (in % 2) == 0;
#else
    return in==2 && out==2;
#endif
}
void SpeechShareProcessor::prepareToPlay(double sr,int) {
    engine.prepare(sr);
    pendingLatency.store(engine.latencySamples(readControls()));
    setLatencySamples(pendingLatency.load());
}
void SpeechShareProcessor::cacheParameterPointers() {
    auto get=[this](const juce::String& id){return state.getRawParameterValue(id);};
    param.engine=get("engine"); param.smart=get("smart"); param.keyMaster=get("key_master");
    param.routing=get("routing"); param.micCount=get("mic_count"); param.rms=get("rms");
    param.strength=get("strength");param.attack=get("attack");param.release=get("release");
    param.lookahead=get("lookahead");param.hpf=get("hpf");
    for(int g=0;g<3;++g) {
        param.keyEnabled[g]=get(kid(g,"enabled"));param.keyDb[g]=get(kid(g,"db"));
        param.keyType[g]=get(kid(g,"type"));param.groupMute[g]=get(gid(g,"mute"));
        param.groupOverride[g]=get(gid(g,"override"));
    }
    for(int i=0;i<speechshare::maxMics;++i) {
        param.weight[i]=get(mid(i,"weight"));param.mode[i]=get(mid(i,"mode"));
        param.group[i]=get(mid(i,"group"));param.overrideEnabled[i]=get(mid(i,"override"));
    }
}
speechshare::Controls SpeechShareProcessor::readControls() const {
    speechshare::Controls c;
    c.engine=param.engine->load()>.5f;c.smart=param.smart->load()>.5f;
    c.keyMaster=param.keyMaster->load()>.5f;
    c.routing=static_cast<speechshare::Routing>(std::clamp(int(param.routing->load()),0,2));
    c.activeMics=std::clamp(int(param.micCount->load()),1,32);
    c.rmsMs=param.rms->load();c.strength=param.strength->load();
    c.attackMs=param.attack->load();c.releaseMs=param.release->load();
    c.lookaheadMs=param.lookahead->load();c.highPassHz=param.hpf->load();
    for(int g=0;g<3;++g) {
        c.keyEnabled[g]=param.keyEnabled[g]->load()>.5f;
        c.keyDb[g]=param.keyDb[g]->load();
        c.keyType[g]=static_cast<speechshare::Generator>(std::clamp(int(param.keyType[g]->load()),0,2));
        c.groupMute[g]=param.groupMute[g]->load()>.5f;
        c.groupOverride[g]=param.groupOverride[g]->load()>.5f;
    }
    for(int i=0;i<speechshare::maxMics;++i) {
        c.weightDb[i]=param.weight[i]->load();
        c.mode[i]=static_cast<speechshare::ChannelMode>(std::clamp(int(param.mode[i]->load()),0,3));
        c.group[i]=std::clamp(int(param.group[i]->load()),0,2);
        c.overrideEnabled[i]=param.overrideEnabled[i]->load()>.5f;
    }
    return c;
}

void SpeechShareProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels=std::min(buffer.getNumChannels(),speechshare::maxChannels);
    const int frames=buffer.getNumSamples();
    if(channels<2 || channels%2!=0) {buffer.clear();return;}
    auto c=readControls();
    const int latency=engine.latencySamples(c);
    if(latency!=pendingLatency.load()) {pendingLatency.store(latency);triggerAsyncUpdate();}
    for(int start=0;start<frames;start+=chunkFrames) {
        const int count=std::min(chunkFrames,frames-start);
        for(int f=0;f<count;++f)
            for(int ch=0;ch<channels;++ch) scratch[std::size_t(f)*channels+ch]=buffer.getReadPointer(ch)[start+f];
        engine.process(scratch.data(),count,channels,c);
        for(int f=0;f<count;++f)
            for(int ch=0;ch<channels;++ch) buffer.getWritePointer(ch)[start+f]=scratch[std::size_t(f)*channels+ch];
    }
    const auto& m=engine.meters();
    for(int i=0;i<speechshare::maxMics;++i){micIn[i].store(m.inputDb[i]);micOut[i].store(m.outputDb[i]);micGain[i].store(m.gainDb[i]);}
    for(int i=0;i<3;++i){keyIn[i].store(m.keyPostDb[i]);keyReduction[i].store(m.keyCutDb[i]);}
}
void SpeechShareProcessor::handleAsyncUpdate() {setLatencySamples(pendingLatency.load());}
float SpeechShareProcessor::micMeter(int index,int kind) const noexcept {
    if(index<0||index>=speechshare::maxMics)return -96;
    return kind==0?micGain[index].load():kind==1?micIn[index].load():micOut[index].load();
}
float SpeechShareProcessor::keyMeter(int g) const noexcept {return g>=0&&g<3 ? keyIn[g].load():-96.f;}
float SpeechShareProcessor::keyCut(int g) const noexcept {return g>=0&&g<3 ? keyReduction[g].load():0.f;}
void SpeechShareProcessor::getStateInformation(juce::MemoryBlock& destination) {
    auto xml=state.copyState().createXml();
    copyXmlToBinary(*xml,destination);
}
void SpeechShareProcessor::setStateInformation(const void* ptr,int bytes) {
    if(auto xml=getXmlFromBinary(ptr,bytes)) {
        if(xml->hasTagName(state.state.getType().toString())) state.replaceState(juce::ValueTree::fromXml(*xml));
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new SpeechShareProcessor();}
