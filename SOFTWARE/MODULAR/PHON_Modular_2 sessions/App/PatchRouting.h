#pragma once
#include <array>
#include <atomic>
#include <algorithm>

namespace speechshare_live {
constexpr int maxSources=32;
constexpr int maxDeviceChannels=128;

// A single (mono) source is one Dugan participant.
// -1 means unpatched. Routing is independent of the ASIO driver's names.
// All writes occur while the audio engine is DISARMED; reads are atomic.
class PatchRouting final {
public:
    PatchRouting() noexcept {resetOneToOne();}
    void resetOneToOne() noexcept {
        for(int i=0;i<maxSources;++i){input[i].store(i,std::memory_order_relaxed);output[i].store(i,std::memory_order_relaxed);}
    }
    int getInput(int source) const noexcept {return validSource(source)?input[std::size_t(source)].load(std::memory_order_relaxed):-1;}
    int getOutput(int source) const noexcept {return validSource(source)?output[std::size_t(source)].load(std::memory_order_relaxed):-1;}
    void setInput(int source,int deviceIndex) noexcept {
        if(validSource(source)&&validEndpoint(deviceIndex))input[std::size_t(source)].store(deviceIndex,std::memory_order_release);
    }
    void setOutput(int source,int deviceIndex) noexcept {
        if(!validSource(source)||!validEndpoint(deviceIndex))return;
        // No two participants can write into the same hardware output.
        if(deviceIndex>=0){
            for(int i=0;i<maxSources;++i)
                if(i!=source && getOutput(i)==deviceIndex)output[std::size_t(i)].store(-1,std::memory_order_release);
        }
        output[std::size_t(source)].store(deviceIndex,std::memory_order_release);
    }
    static bool validEndpoint(int index) noexcept {return index>=-1&&index<maxDeviceChannels;}
private:
    static bool validSource(int source) noexcept {return source>=0&&source<maxSources;}
    std::array<std::atomic<int>,maxSources> input{};
    std::array<std::atomic<int>,maxSources> output{};
};
}
