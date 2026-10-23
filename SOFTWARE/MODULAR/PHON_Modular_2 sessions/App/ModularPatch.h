#pragma once
#include <array>
#include <algorithm>

namespace speechshare_modular {
constexpr int sessions = 2;
constexpr int participantsPerSession = 8;
constexpr int participants = sessions * participantsPerSession;
constexpr int groupsPerSession = 3;
constexpr int maxPorts = 128;

// A stereo source is ONE participant. Both channels retain their identities
// and the automixer applies one linked gain coefficient to L and R.
enum class InputKind : int {
    Off=0, AsioMono, AsioStereo, Session1ChannelMono, Session1ChannelStereo,
    Session1GroupMono, Session1GroupStereo
};
enum class OutputKind : int { Off=0, AsioMono, AsioStereo };
struct InputRoute { InputKind kind=InputKind::Off; int index=-1; };
struct OutputRoute { OutputKind kind=OutputKind::Off; int index=-1; };

// UI / control thread: edits ONLY while the engine is disarmed. The audio
// callback reads this validated snapshot without locks or allocation.
class ModularPatch final {
public:
    ModularPatch() noexcept { reset(); }
    static bool isStereo(InputKind k) noexcept {
        return k==InputKind::AsioStereo||k==InputKind::Session1ChannelStereo||k==InputKind::Session1GroupStereo;
    }
    static bool isStereo(OutputKind k) noexcept { return k==OutputKind::AsioStereo; }
    static bool validInput(int dest,InputRoute r) noexcept {
        if(dest<0||dest>=participants) return false;
        switch(r.kind) {
            case InputKind::Off: return r.index==-1;
            case InputKind::AsioMono: return r.index>=0&&r.index<maxPorts;
            case InputKind::AsioStereo: return r.index>=0&&r.index+1<maxPorts;
            case InputKind::Session1ChannelMono:
            case InputKind::Session1ChannelStereo:
                return dest>=participantsPerSession&&r.index>=0&&r.index<participantsPerSession;
            case InputKind::Session1GroupMono:
            case InputKind::Session1GroupStereo:
                return dest>=participantsPerSession&&r.index>=0&&r.index<groupsPerSession;
        }
        return false;
    }
    static bool validOutput(OutputRoute r) noexcept {
        switch(r.kind) {
            case OutputKind::Off: return r.index==-1;
            case OutputKind::AsioMono: return r.index>=0&&r.index<maxPorts;
            case OutputKind::AsioStereo: return r.index>=0&&r.index+1<maxPorts;
        }
        return false;
    }
    void reset() noexcept {
        for(int i=0;i<participants;++i){
            sourceIn[i] = i<participantsPerSession ? InputRoute{InputKind::AsioMono,i} : InputRoute{};
            sourceOut[i] = i<participantsPerSession ? OutputRoute{OutputKind::AsioMono,i} : OutputRoute{};
        }
        for(auto& x:groupOut) x={};
    }
    bool setInput(int dest,InputRoute r) noexcept {
        if(!validInput(dest,r))return false;
        sourceIn[std::size_t(dest)]=r;return true;
    }
    bool setOutput(int source,OutputRoute r) noexcept {
        if(source<0||source>=participants||!validOutput(r)||!freeOutputs(r,source,-1))return false;
        sourceOut[std::size_t(source)]=r;return true;
    }
    bool setGroupOutput(int session,int group,OutputRoute r) noexcept {
        if(session<0||session>=sessions||group<0||group>=groupsPerSession||!validOutput(r))return false;
        const int id=session*groupsPerSession+group;
        if(!freeOutputs(r,-1,id))return false;
        groupOut[std::size_t(id)]=r;return true;
    }
    InputRoute input(int source) const noexcept {return source>=0&&source<participants?sourceIn[std::size_t(source)]:InputRoute{};}
    OutputRoute output(int source) const noexcept {return source>=0&&source<participants?sourceOut[std::size_t(source)]:OutputRoute{};}
    OutputRoute groupOutput(int session,int group) const noexcept {
        return session>=0&&session<sessions&&group>=0&&group<groupsPerSession?
            groupOut[std::size_t(session*groupsPerSession+group)]:OutputRoute{};
    }
    // Validate configurations loaded from disk exactly like GUI edits.
    // Rejecting a conflict, rather than silently taking over an output,
    // prevents accidental rerouting in a broadcast setup.
private:
    static bool overlaps(OutputRoute a,OutputRoute b) noexcept {
        if(a.kind==OutputKind::Off||b.kind==OutputKind::Off)return false;
        const int aw=isStereo(a.kind)?2:1,bw=isStereo(b.kind)?2:1;
        return a.index<b.index+bw && b.index<a.index+aw;
    }
    bool freeOutputs(OutputRoute r,int exceptSource,int exceptGroup) const noexcept {
        for(int i=0;i<participants;++i)
            if(i!=exceptSource&&overlaps(r,sourceOut[std::size_t(i)]))return false;
        for(int i=0;i<sessions*groupsPerSession;++i)
            if(i!=exceptGroup&&overlaps(r,groupOut[std::size_t(i)]))return false;
        return true;
    }
    std::array<InputRoute,participants> sourceIn{};
    std::array<OutputRoute,participants> sourceOut{};
    std::array<OutputRoute,sessions*groupsPerSession> groupOut{};
};
} // namespace speechshare_modular
