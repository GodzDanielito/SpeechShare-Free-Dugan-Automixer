#include "StandaloneAudio.h"
#include "ChannelFormat.h"
#include <algorithm>
#include <cmath>
#include <chrono>
namespace speechshare_live {
using namespace speechshare_modular;
namespace {

void sendToOutputs(const juce::AudioBuffer<float>& src,OutputRoute route,
                   float* const* outs,int numOutputs,int offset,int count) noexcept {
    if(route.kind==OutputKind::Off||route.index<0||route.index>=numOutputs)return;
    auto* a=outs[route.index];
    if(!a)return;
    const float* l=src.getReadPointer(0);
    const float* r=src.getNumChannels()>1?src.getReadPointer(1):nullptr;
    if(route.kind==OutputKind::AsioMono){
        if(!r)juce::FloatVectorOperations::add(a+offset,l,count);
        else for(int n=0;n<count;++n)a[offset+n]+=stereoToMono(l[n],r[n]);
    }else if(route.index+1<numOutputs&&outs[route.index+1]){
        float* b=outs[route.index+1];
        if(r){
            juce::FloatVectorOperations::add(a+offset,l,count);
            juce::FloatVectorOperations::add(b+offset,r,count);
        }else for(int n=0;n<count;++n){a[offset+n]+=monoToStereo*l[n];b[offset+n]+=monoToStereo*l[n];}
    }
}
}
StandaloneAudio::StandaloneAudio(){
    for(auto& b:sourceBuffers)b.setSize(2,chunk);
    for(auto& session:groupBuffers)for(auto& b:session)b.setSize(2,chunk);
    if(auto saved=juce::XmlDocument::parse(configFile().getSiblingFile("StandaloneModular4SleepPatch.xml"))){
        if(saved->hasTagName("MODULAR_PATCH")){
            routing.reset();
            // Release default hardware output reservations before restoring custom routes.
            for(int i=0;i<participants;++i)routing.setOutput(i,{});
            for(auto* node:saved->getChildIterator()){
                if(node->hasTagName("SOURCE")){
                    int i=node->getIntAttribute("id",-1);
                    routing.setInput(i,{InputKind(node->getIntAttribute("inType",0)),node->getIntAttribute("in",-1)});
                    routing.setOutput(i,{OutputKind(node->getIntAttribute("outType",0)),node->getIntAttribute("out",-1)});
                }
                if(node->hasTagName("GROUP")){
                    routing.setGroupOutput(node->getIntAttribute("session",-1),node->getIntAttribute("group",-1),
                        {OutputKind(node->getIntAttribute("type",0)),node->getIntAttribute("out",-1)});
                }
            }
        }
    }
    createProcessors();
    if(auto states=juce::XmlDocument::parse(configFile().getSiblingFile("StandaloneModular4SleepChannels.xml"))){
        if(states->hasTagName("LIVE_CHANNELS"))for(auto* ch:states->getChildIterator()){
            const int i=ch->getIntAttribute("index",-1);
            if(i<0||i>=sourceCount())continue;
            juce::MemoryBlock state;
            if(state.fromBase64Encoding(ch->getStringAttribute("state")))
                processors[std::size_t(i)]->setStateInformation(state.getData(),int(state.getSize()));
        }
    }
    for(int i=0;i<participants;++i)processors[std::size_t(i)]->setOwnSession(i/participantsPerSession+1);
    auto error=openSavedDevice();if(error.isNotEmpty())deviceError=error;
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
        .getChildFile("SpeechShare").getChildFile("StandalonePHONModular4Sleep.xml");
}
void StandaloneAudio::createProcessors(){
    processors.reserve(participants);
    for(int i=0;i<participants;++i){
        auto p=std::make_unique<SpeechShareInsertProcessor>();
        p->setOwnSession(i/participantsPerSession+1);
        p->setSourceName("S"+juce::String(i/participantsPerSession+1)+" CH "+
                         juce::String(i%participantsPerSession+1).paddedLeft('0',2));
        // Maximal layout: adjusted to mono/stereo just before processing.
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add(juce::AudioChannelSet::stereo());
        layout.outputBuses.add(juce::AudioChannelSet::stereo());
        p->setBusesLayout(layout);
        processors.push_back(std::move(p));
    }
}
juce::String StandaloneAudio::openSavedDevice(){
    std::unique_ptr<juce::XmlElement> xml;
    if(configFile().existsAsFile())xml=juce::XmlDocument::parse(configFile());
    return manager.initialise(2,2,xml.get(),false);
}
void StandaloneAudio::saveConfiguration() const {
    auto config=configFile();config.getParentDirectory().createDirectory();
    if(auto xml=manager.createStateXml())xml->writeTo(config);
    auto patchXml=std::make_unique<juce::XmlElement>("MODULAR_PATCH");
    patchXml->setAttribute("version",1);
    for(int i=0;i<participants;++i){
        auto r=routing.input(i);auto o=routing.output(i);
        auto* x=patchXml->createNewChildElement("SOURCE");x->setAttribute("id",i);
        x->setAttribute("inType",int(r.kind));x->setAttribute("in",r.index);
        x->setAttribute("outType",int(o.kind));x->setAttribute("out",o.index);
    }
    for(int s=0;s<sessions;++s)for(int g=0;g<3;++g){
        auto o=routing.groupOutput(s,g);
        auto* x=patchXml->createNewChildElement("GROUP");
        x->setAttribute("session",s);x->setAttribute("group",g);
        x->setAttribute("type",int(o.kind));x->setAttribute("out",o.index);
    }
    patchXml->writeTo(config.getSiblingFile("StandaloneModular4SleepPatch.xml"));
    auto channelXml=std::make_unique<juce::XmlElement>("LIVE_CHANNELS");
    for(int i=0;i<sourceCount();++i){
        juce::MemoryBlock mem;
        processors[std::size_t(i)]->getStateInformation(mem);
        auto* x=channelXml->createNewChildElement("CHANNEL");
        x->setAttribute("index",i);x->setAttribute("state",mem.toBase64Encoding());
    }
    channelXml->writeTo(config.getSiblingFile("StandaloneModular4SleepChannels.xml"));
}
void StandaloneAudio::deactivateProcessedSources() noexcept {
    for(int i=0;i<participants;++i){
        if(processedLastBlock[std::size_t(i)])
            speechshare_insert::SharedPool::instance().deactivate(processors[std::size_t(i)]->instanceSlot());
        processedLastBlock[std::size_t(i)]=false;
        wakeFade[std::size_t(i)]=0.f;
    }
    awakeMask.store(0u,std::memory_order_release);
}
void StandaloneAudio::clearPerformance() noexcept {
    loadPercent.store(0.f,std::memory_order_release);
    peakPercent.store(0.f,std::memory_order_release);
}
void StandaloneAudio::audioDeviceAboutToStart(juce::AudioIODevice* device){
    ready.store(false,std::memory_order_release);
    deactivateProcessedSources();
    clearPerformance();
    if(!device)return;
    currentRate=device->getCurrentSampleRate();
    for(auto& p:processors){p->setRateAndBufferSizeDetails(currentRate,chunk);p->prepareToPlay(currentRate,chunk);}
    ready.store(true,std::memory_order_release);
}
void StandaloneAudio::audioDeviceStopped(){
    ready.store(false,std::memory_order_release);
    deactivateProcessedSources();
    clearPerformance();
    for(auto& p:processors)p->releaseResources();
}
void StandaloneAudio::audioDeviceError(const juce::String& message){
    ready.store(false,std::memory_order_release);deviceError=message;
    deactivateProcessedSources();
    clearPerformance();
}
void StandaloneAudio::audioDeviceIOCallbackWithContext(const float* const* inputs,int numInputs,
    float* const* outputs,int numOutputs,int samples,const juce::AudioIODeviceCallbackContext&){
    const auto callbackStart=std::chrono::steady_clock::now();
    for(int i=0;i<numOutputs;++i)
        if(outputs[i])juce::FloatVectorOperations::clear(outputs[i],samples);
    if(!armed.load(std::memory_order_acquire)||!ready.load(std::memory_order_acquire)||samples<=0){
        deactivateProcessedSources();
        clearPerformance();
        return;
    }
    juce::ScopedNoDenormals guard;
    juce::MidiBuffer midi;

    // Evaluate physical and forward-only internal connections once per callback.
    // Crucially: a valid physical port is ACTIVE even when it carries silence.
    std::array<bool,maxDeviceChannels> hw{};
    for(int i=0;i<std::min(numInputs,maxDeviceChannels);++i)hw[std::size_t(i)]=inputs&&inputs[i];
    std::array<int,participants> assignedGroups{};
    for(int i=0;i<participants;++i)
        if(auto* p=processors[std::size_t(i)]->state.getRawParameterValue("group"))
            assignedGroups[std::size_t(i)]=std::clamp(int(p->load()),0,2);
    const auto plan=SessionActivity::evaluate(routing,hw.data(),
                                              std::min(numInputs,maxDeviceChannels),assignedGroups);
    unsigned mask=0u;
    for(int session=0;session<sessions;++session)
        if(plan.session[std::size_t(session)])mask|=1u<<unsigned(session);
    awakeMask.store(mask,std::memory_order_release);

    for(int i=0;i<participants;++i){
        const bool live=plan.source[std::size_t(i)];
        auto& previous=processedLastBlock[std::size_t(i)];
        if(!live && previous){
            speechshare_insert::SharedPool::instance().deactivate(processors[std::size_t(i)]->instanceSlot());
            wakeFade[std::size_t(i)]=0.f;
        }else if(live && !previous){
            // Reset stale detector state before resuming. No allocations.
            processors[std::size_t(i)]->resetDetectorForActivation();
            wakeFade[std::size_t(i)]=0.f;
        }
        previous=live;
    }

    for(int offset=0;offset<samples;offset+=chunk){
        const int n=std::min(chunk,samples-offset);
        for(int session=0;session<sessions;++session){
            if(!plan.session[std::size_t(session)])continue; // Full-session DSP sleep.
            for(int g=0;g<groupsPerSession;++g){
                auto& mix=groupBuffers[std::size_t(session)][std::size_t(g)];
                mix.clear(0,0,n);mix.clear(1,0,n);
            }
            for(int lane=0;lane<participantsPerSession;++lane){
                const int i=session*participantsPerSession+lane;
                if(!plan.source[std::size_t(i)])continue;
                auto& b=sourceBuffers[std::size_t(i)];
                const auto route=routing.input(i);
                const bool stereo=ModularPatch::isStereo(route.kind);
                b.setSize(stereo?2:1,n,false,true,true);
                switch(route.kind){
                    case InputKind::AsioMono:
                    case InputKind::AsioStereo:
                        juce::FloatVectorOperations::copy(b.getWritePointer(0),inputs[route.index]+offset,n);
                        if(stereo)
                            juce::FloatVectorOperations::copy(b.getWritePointer(1),inputs[route.index+1]+offset,n);
                        break;
                    case InputKind::InternalChannelMono:
                    case InputKind::InternalChannelStereo: {
                        const auto& src=sourceBuffers[std::size_t(route.index)];
                        const auto* l=src.getReadPointer(0);
                        const auto* r=src.getNumChannels()>1?src.getReadPointer(1):nullptr;
                        if(stereo){
                            if(r){b.copyFrom(0,0,l,n);b.copyFrom(1,0,r,n);}
                            else {b.copyFrom(0,0,l,n,monoToStereo);b.copyFrom(1,0,l,n,monoToStereo);}
                        }else if(r){
                            for(int j=0;j<n;++j)b.getWritePointer(0)[j]=stereoToMono(l[j],r[j]);
                        }else b.copyFrom(0,0,l,n);
                        break;
                    }
                    case InputKind::InternalGroupMono:
                    case InputKind::InternalGroupStereo: {
                        const auto& src=groupBuffers[std::size_t(route.index/groupsPerSession)]
                                                    [std::size_t(route.index%groupsPerSession)];
                        const auto* l=src.getReadPointer(0),*r=src.getReadPointer(1);
                        if(stereo){b.copyFrom(0,0,l,n);b.copyFrom(1,0,r,n);}
                        else for(int j=0;j<n;++j)b.getWritePointer(0)[j]=stereoToMono(l[j],r[j]);
                        break;
                    }
                    case InputKind::Off: break; // excluded by activity plan
                }
                // One participant for both sides of a stereo pair.
                processors[std::size_t(i)]->processBlock(b,midi);
                auto& fade=wakeFade[std::size_t(i)];
                if(fade<1.f){
                    const float step=1.f/float(std::max(1.0,currentRate*0.003));
                    for(int j=0;j<n;++j){
                        fade=std::min(1.f,fade+step);
                        b.getWritePointer(0)[j]*=fade;
                        if(stereo)b.getWritePointer(1)[j]*=fade;
                    }
                }
                sendToOutputs(b,routing.output(i),outputs,numOutputs,offset,n);
                const int group=assignedGroups[std::size_t(i)];
                auto& bus=groupBuffers[std::size_t(session)][std::size_t(group)];
                if(stereo){
                    bus.addFrom(0,0,b,0,0,n);bus.addFrom(1,0,b,1,0,n);
                }else{
                    bus.addFrom(0,0,b,0,0,n,monoToStereo);
                    bus.addFrom(1,0,b,0,0,n,monoToStereo);
                }
            }
            // Internal taps are read ONLY when the activity plan marks their
            // parent group live; sleeping groups may retain old buffer samples.
            for(int g=0;g<groupsPerSession;++g)
                if(plan.group[std::size_t(session)][std::size_t(g)])
                    sendToOutputs(groupBuffers[std::size_t(session)][std::size_t(g)],
                        routing.groupOutput(session,g),outputs,numOutputs,offset,n);
        }
    }
    const double ms=std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-callbackStart).count();
    const float percent=float(100.0*ms/(1000.0*double(samples)/std::max(8000.0,currentRate)));
    loadPercent.store(percent,std::memory_order_relaxed);
    const float oldPeak=peakPercent.load(std::memory_order_relaxed);
    peakPercent.store(std::max(percent,oldPeak*0.995f),std::memory_order_relaxed);
}

juce::String StandaloneAudio::describeDevice() const {
    auto* dev=manager.getCurrentAudioDevice();
    if(!dev)return "NO AUDIO DEVICE | select ASIO in PATCH";
    const double sr=dev->getCurrentSampleRate();
    const int frames=dev->getCurrentBufferSizeSamples();
    return manager.getCurrentAudioDeviceType()+" | "+dev->getName()+" | "+juce::String(sr,0)+" Hz | "+
       juce::String(frames)+" samples | "+juce::String(dev->getInputChannelNames().size())+" IN / "+
       juce::String(dev->getOutputChannelNames().size())+" OUT | callback "+
       juce::String(1000.0*frames/std::max(1.0,sr),2)+" ms (not round-trip)";
}
}
