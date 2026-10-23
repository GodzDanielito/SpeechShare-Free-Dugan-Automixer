#include "StandaloneAudio.h"
#include <cstring>
#include <algorithm>

namespace speechshare_live {
StandaloneAudio::StandaloneAudio(){
    for(auto& b:sourceBuffers)b.setSize(1,chunk);
    int initialSources=8;
    auto saved=juce::XmlDocument::parse(configFile().getSiblingFile("StandalonePatch.xml"));
    if(saved && saved->hasTagName("LIVE_PATCH")){
        initialSources=std::clamp(saved->getIntAttribute("sources",8),1,maxSources);
        for(auto* r:saved->getChildIterator()){
            if(!r->hasTagName("ROUTE"))continue;
            const int source=r->getIntAttribute("source",-1);
            routing.setInput(source,r->getIntAttribute("in",-1));
            routing.setOutput(source,r->getIntAttribute("out",-1));
        }
    }
    createProcessors(initialSources);
    if(auto states=juce::XmlDocument::parse(configFile().getSiblingFile("StandaloneChannels.xml"))){
        if(states->hasTagName("LIVE_CHANNELS")){
            for(auto* ch:states->getChildIterator()){
                const int idx=ch->getIntAttribute("index",-1);
                if(idx<0||idx>=sourceCount())continue;
                juce::MemoryBlock state;
                if(state.fromBase64Encoding(ch->getStringAttribute("state")))
                    processors[std::size_t(idx)]->setStateInformation(state.getData(),int(state.getSize()));
            }
        }
    }
    const auto error=openSavedDevice();
    if(error.isNotEmpty())deviceError=error;
    manager.addAudioCallback(this);
}
StandaloneAudio::~StandaloneAudio(){
    armed.store(false,std::memory_order_release);
    manager.removeAudioCallback(this);
    saveConfiguration();
    manager.closeAudioDevice();
    processors.clear();
}

juce::File StandaloneAudio::configFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("SpeechShare").getChildFile("StandaloneLive.xml");
}
void StandaloneAudio::createProcessors(int count){
    processors.clear();processors.reserve(std::size_t(count));
    for(int i=0;i<count;++i){
        auto p=std::make_unique<SpeechShareInsertProcessor>();
        p->setSourceName("CH " + juce::String(i+1).paddedLeft('0',2));
        // The INSERT DSP can process mono independently; each source is one participant.
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add(juce::AudioChannelSet::mono());
        layout.outputBuses.add(juce::AudioChannelSet::mono());
        p->setBusesLayout(layout);
        processors.push_back(std::move(p));
    }
}
void StandaloneAudio::setSourceCount(int count){
    count=std::clamp(count,1,maxSources);
    if(count==sourceCount())return;
    setArmed(false);
    manager.removeAudioCallback(this); // blocks until its audio callback stops
    std::vector<juce::MemoryBlock> previous;
    previous.resize(std::size_t(std::min(count,sourceCount())));
    for(std::size_t i=0;i<previous.size();++i)
        processors[i]->getStateInformation(previous[i]);
    createProcessors(count);
    for(std::size_t i=0;i<previous.size();++i)
        processors[i]->setStateInformation(previous[i].getData(),int(previous[i].getSize()));
    manager.addAudioCallback(this);
}
juce::String StandaloneAudio::openSavedDevice(){
    auto file=configFile();
    std::unique_ptr<juce::XmlElement> xml;
    if(file.existsAsFile())xml=juce::XmlDocument::parse(file);
    auto error=manager.initialise(2,2,xml.get(),false);
    // In a clean install the user selects a driver in PATCH > AUDIO DEVICE.
    // Avoid silently falling back to a wrong device when saved ASIO is missing.
    return error;
}
void StandaloneAudio::saveConfiguration() const {
    if(auto xml=manager.createStateXml()){
        auto config=configFile();config.getParentDirectory().createDirectory();
        xml->writeTo(config);
    }
    auto patchXml=std::make_unique<juce::XmlElement>("LIVE_PATCH");
    patchXml->setAttribute("sources",sourceCount());
    for(int i=0;i<sourceCount();++i){
        auto* r=patchXml->createNewChildElement("ROUTE");r->setAttribute("source",i);
        r->setAttribute("in",routing.getInput(i));r->setAttribute("out",routing.getOutput(i));
    }
    auto path=configFile().getSiblingFile("StandalonePatch.xml");
    patchXml->writeTo(path);
    auto channelXml=std::make_unique<juce::XmlElement>("LIVE_CHANNELS");
    for(int i=0;i<sourceCount();++i){
        juce::MemoryBlock state;
        processors[std::size_t(i)]->getStateInformation(state);
        auto* ch=channelXml->createNewChildElement("CHANNEL");
        ch->setAttribute("index",i);
        ch->setAttribute("state",state.toBase64Encoding());
    }
    channelXml->writeTo(configFile().getSiblingFile("StandaloneChannels.xml"));
}
void StandaloneAudio::audioDeviceAboutToStart(juce::AudioIODevice* device){
    ready.store(false,std::memory_order_release);
    if(!device)return;
    currentRate=device->getCurrentSampleRate();
    for(auto& p:processors){
        p->setRateAndBufferSizeDetails(currentRate,chunk);
        p->prepareToPlay(currentRate,chunk);
    }
    ready.store(true,std::memory_order_release);
}
void StandaloneAudio::audioDeviceStopped(){
    ready.store(false,std::memory_order_release);
    for(auto& p:processors)p->releaseResources();
}
void StandaloneAudio::audioDeviceError(const juce::String& message){
    ready.store(false,std::memory_order_release);
    deviceError=message;
}
void StandaloneAudio::audioDeviceIOCallbackWithContext(const float* const* inputs,int numInputs,
    float* const* outputs,int numOutputs,int samples,const juce::AudioIODeviceCallbackContext&){
    // Always clear EVERY output first, including channels not routed to a participant.
    // Never allocate memory, open UI, or wait on mutex in this callback.
    for(int out=0;out<numOutputs;++out)
        if(outputs[out])juce::FloatVectorOperations::clear(outputs[out],samples);
    if(!armed.load(std::memory_order_acquire) || !ready.load(std::memory_order_acquire))return;
    juce::ScopedNoDenormals noDenormals;
    juce::MidiBuffer midi; // empty; no MIDI processing
    for(int offset=0;offset<samples;offset+=chunk){
        const int n=std::min(chunk,samples-offset);
        for(int i=0;i<sourceCount();++i){
            const int in=routing.getInput(i),out=routing.getOutput(i);
            // Unpatched participants must never influence other microphones.
            if(in<0||in>=numInputs||!inputs[in]||out<0||out>=numOutputs||!outputs[out]){
                speechshare_insert::SharedPool::instance().deactivate(processors[std::size_t(i)]->instanceSlot());
                continue;
            }
            auto& block=sourceBuffers[std::size_t(i)];
            block.setSize(1,n,false,false,true); // existing capacity: avoidReallocating=true
            juce::FloatVectorOperations::copy(block.getWritePointer(0),inputs[in]+offset,n);
            processors[std::size_t(i)]->processBlock(block,midi);
            juce::FloatVectorOperations::add(outputs[out]+offset,block.getReadPointer(0),n);
        }
    }
}
juce::String StandaloneAudio::describeDevice() const {
    auto* dev=manager.getCurrentAudioDevice();
    if(!dev)return "NO AUDIO DEVICE | select driver in PATCH";
    const double sr=dev->getCurrentSampleRate();
    const int frames=dev->getCurrentBufferSizeSamples();
    const auto mode=manager.getCurrentAudioDeviceType();
    return mode + " | " + dev->getName() + " | " + juce::String(sr,0) + " Hz | " +
           juce::String(frames) + " samples | " +
           juce::String(dev->getInputChannelNames().size()) + " IN / " +
           juce::String(dev->getOutputChannelNames().size()) + " OUT | callback " +
           juce::String(1000.0*frames/std::max(1.0,sr),2) + " ms (not round-trip latency)";
}
}
