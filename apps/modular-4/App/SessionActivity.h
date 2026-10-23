#pragma once
#include "ModularPatch.h"
#include <array>
#include <algorithm>

// Pure, allocation-free routing activity analysis.  A connected ASIO port is
// considered live even when its audio samples are zero: this must NEVER act
// like a noise gate.  The hardware channel list is mapped to availability
// before processing; internal edges are evaluated in forward-only order.
namespace speechshare_modular {
struct SessionActivity {
    std::array<bool, participants> source{};
    std::array<std::array<bool, groupsPerSession>, sessions> group{};
    std::array<bool, sessions> session{};
    int awakeCount=0;

    static SessionActivity evaluate(const ModularPatch& patch,
                                    const bool* asioAvailable,int asioCount,
                                    const std::array<int,participants>& assignedGroups) noexcept {
        SessionActivity out;
        for(int s=0;s<sessions;++s){
            for(int lane=0;lane<participantsPerSession;++lane){
                const int id=s*participantsPerSession+lane;
                const auto input=patch.input(id);
                bool valid=false;
                switch(input.kind){
                    case InputKind::AsioMono:
                    case InputKind::AsioStereo:
                        if(asioAvailable && input.index>=0 && input.index<asioCount &&
                           asioAvailable[input.index]){
                            valid= input.kind==InputKind::AsioMono ||
                                 (input.index+1<asioCount&&asioAvailable[input.index+1]);
                        }
                        break;
                    case InputKind::InternalChannelMono:
                    case InputKind::InternalChannelStereo:
                        valid=input.index>=0 && input.index<participants &&
                              input.index/participantsPerSession<s && out.source[std::size_t(input.index)];
                        break;
                    case InputKind::InternalGroupMono:
                    case InputKind::InternalGroupStereo:
                        valid=input.index>=0 && input.index<sessions*groupsPerSession &&
                              input.index/groupsPerSession<s &&
                              out.group[std::size_t(input.index/groupsPerSession)]
                                       [std::size_t(input.index%groupsPerSession)];
                        break;
                    case InputKind::Off: break;
                }
                out.source[std::size_t(id)]=valid;
                if(valid){
                    out.session[std::size_t(s)]=true;
                    out.group[std::size_t(s)]
                             [std::size_t(std::clamp(assignedGroups[std::size_t(id)],0,groupsPerSession-1))]=true;
                }
            }
            if(out.session[std::size_t(s)])++out.awakeCount;
        }
        return out;
    }
};
}
