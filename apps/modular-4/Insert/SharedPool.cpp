#include "SharedPool.h"
#include <algorithm>
#include <cmath>

namespace speechshare_insert {
SharedPool& SharedPool::instance() noexcept {
    static SharedPool pool;
    return pool;
}
int SharedPool::acquire() noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    for (int i=0;i<maxInstances;++i) {
        bool expected=false;
        if (slots[i].occupied.compare_exchange_strong(expected,true,std::memory_order_acq_rel)) {
            auto& s=slots[i];
            s.active.store(false,std::memory_order_release);
            s.localMute.store(false,std::memory_order_relaxed);
            s.smart.store(false,std::memory_order_relaxed);
            s.solo.store(false,std::memory_order_relaxed);
            s.displayName.clear();s.pendingName.clear();s.nameEditPending=false;
            s.levelDb.store(0.f,std::memory_order_relaxed);
            s.score.store(0.0,std::memory_order_relaxed);
            s.rawGain.store(1.0,std::memory_order_relaxed);
            s.session.store(1,std::memory_order_relaxed);
            s.group.store(0,std::memory_order_relaxed);
            s.mode.store(int(Mode::Auto),std::memory_order_relaxed);
            s.weightDb.store(0.f,std::memory_order_relaxed);
            s.inputDb.store(-96.f,std::memory_order_relaxed);
            s.gainDb.store(0.f,std::memory_order_relaxed);
            s.outputDb.store(-96.f,std::memory_order_relaxed);
            s.commandMute.store(-1,std::memory_order_relaxed);
            s.commandMode.store(-1,std::memory_order_relaxed);
            s.commandGroup.store(-1,std::memory_order_relaxed);
            s.commandWeight.store(100000,std::memory_order_relaxed);
            s.commandSmart.store(-1,std::memory_order_relaxed);
            s.commandSolo.store(-1,std::memory_order_relaxed);
            s.commandLevel.store(100000,std::memory_order_relaxed);
            constexpr float defaults[5]={100.f,1.f,12.f,160.f,100.f};
            for(int j=0;j<5;++j) {
                s.advanced[std::size_t(j)].store(defaults[j],std::memory_order_relaxed);
                s.commandAdvanced[std::size_t(j)].store(-1,std::memory_order_relaxed);
            }
            // Increment last so a stale GUI token is not valid for this owner.
            s.generation.store(nextGeneration.fetch_add(1,std::memory_order_relaxed),
                               std::memory_order_release);
            return i;
        }
    }
    return -1;
}
void SharedPool::release(int id) noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    if (id<0||id>=maxInstances) return;
    auto& s=slots[id];
    // Invalidate GUI tokens before the slot can be reused.
    s.generation.store(0,std::memory_order_release);
    s.active.store(false,std::memory_order_release);
    s.occupied.store(false,std::memory_order_release);
}
void SharedPool::deactivate(int id) noexcept {
    if (id>=0&&id<maxInstances) slots[id].active.store(false,std::memory_order_release);
}
void SharedPool::setLocalMute(int id,bool muted) noexcept {
    if (id>=0&&id<maxInstances) slots[id].localMute.store(muted,std::memory_order_release);
}
void SharedPool::publish(int id,int session,int group,Mode mode,double score,double rawGain) noexcept {
    if (id<0||id>=maxInstances) return;
    auto& s=slots[id];
    s.active.store(false,std::memory_order_release);
    s.session.store(std::clamp(session,1,maxSessions),std::memory_order_relaxed);
    s.group.store(std::clamp(group,0,2),std::memory_order_relaxed);
    s.mode.store(int(mode),std::memory_order_relaxed);
    s.score.store(std::isfinite(score)&&score>0?score:0.0,std::memory_order_relaxed);
    s.rawGain.store(std::isfinite(rawGain)?std::clamp(rawGain,0.0,1.0):0.0,std::memory_order_relaxed);
    s.active.store(true,std::memory_order_release);
}
SessionControls SharedPool::getSessionControls(int session) const noexcept {
    const auto& s=sessionBanks[std::size_t(std::clamp(session,1,maxSessions)-1)];
    return {s.bypass.load(std::memory_order_acquire),
            s.muted.load(std::memory_order_acquire),
            s.gainDb.load(std::memory_order_relaxed)};
}
void SharedPool::setSessionControls(int session,const SessionControls& value) noexcept {
    if(!std::isfinite(value.gainDb))return;
    auto& s=sessionBanks[std::size_t(std::clamp(session,1,maxSessions)-1)];
    s.gainDb.store(std::clamp(value.gainDb,-36.f,12.f),std::memory_order_release);
    s.bypass.store(value.bypass,std::memory_order_release);
    s.muted.store(value.muted,std::memory_order_release);
}
SessionKeys SharedPool::getSessionKeys(int session) const noexcept {
    SessionKeys keys{};
    const auto& bank=keyBanks[std::size_t(std::clamp(session,1,maxSessions)-1)];
    keys.master=bank.master.load(std::memory_order_acquire);
    for(int g=0;g<3;++g) {
        const auto& src=bank.groups[std::size_t(g)];
        auto& dst=keys.groups[std::size_t(g)];
        dst.enabled=src.enabled.load(std::memory_order_relaxed);
        dst.generator=src.generator.load(std::memory_order_relaxed)==0
            ? KeyGenerator::Pink:KeyGenerator::Sine;
        dst.frequencyHz=std::clamp(src.frequencyHz.load(std::memory_order_relaxed),10.f,20000.f);
        dst.levelDb=std::clamp(src.levelDb.load(std::memory_order_relaxed),-96.f,0.f);
    }
    return keys;
}
void SharedPool::setKeyMaster(int session,bool enabled) noexcept {
    const std::lock_guard<std::mutex> lock(controlMutex);
    keyBanks[std::size_t(std::clamp(session,1,maxSessions)-1)].master.store(enabled,std::memory_order_release);
}
void SharedPool::setKeyGroup(int session,int group,const KeyGroupSettings& settings) noexcept {
    if(group<0||group>=3 || !std::isfinite(settings.frequencyHz) || !std::isfinite(settings.levelDb))return;
    const std::lock_guard<std::mutex> lock(controlMutex);
    auto& g=keyBanks[std::size_t(std::clamp(session,1,maxSessions)-1)].groups[std::size_t(group)];
    g.enabled.store(settings.enabled,std::memory_order_relaxed);
    g.generator.store(int(settings.generator==KeyGenerator::Pink?KeyGenerator::Pink:KeyGenerator::Sine),std::memory_order_relaxed);
    g.frequencyHz.store(std::clamp(settings.frequencyHz,10.f,20000.f),std::memory_order_relaxed);
    g.levelDb.store(std::clamp(settings.levelDb,-96.f,0.f),std::memory_order_release);
}
bool SharedPool::isSoloExcluded(int id,int session) const noexcept {
    bool anySolo=false;
    for(int i=0;i<maxInstances;++i){
        const auto& s=slots[std::size_t(i)];
        if(s.occupied.load(std::memory_order_acquire) &&
           s.session.load(std::memory_order_relaxed)==session &&
           s.solo.load(std::memory_order_acquire) &&
           !s.localMute.load(std::memory_order_acquire)) {anySolo=true;break;}
    }
    return anySolo && (id<0||id>=maxInstances||!slots[std::size_t(id)].solo.load(std::memory_order_acquire));
}
void SharedPool::setSolo(int id,bool enabled) noexcept {
    if(id>=0&&id<maxInstances)slots[std::size_t(id)].solo.store(enabled,std::memory_order_release);
}
void SharedPool::setLevelDb(int id,float db) noexcept {
    if(id>=0&&id<maxInstances && std::isfinite(db))
        slots[std::size_t(id)].levelDb.store(std::clamp(db,-90.f,12.f),std::memory_order_release);
}
void SharedPool::setDisplayName(int id,const std::string& name) {
    if(id<0||id>=maxInstances)return;
    std::lock_guard<std::mutex> lock(controlMutex);
    slots[std::size_t(id)].displayName=name.substr(0,36);
}
std::string SharedPool::getDisplayName(int id,std::uint32_t gen) const {
    std::lock_guard<std::mutex> lock(controlMutex);
    return validTarget(id,gen)?slots[std::size_t(id)].displayName:std::string{};
}
bool SharedPool::requestDisplayName(int id,std::uint32_t gen,const std::string& name) {
    std::lock_guard<std::mutex> lock(controlMutex);
    if(!validTarget(id,gen))return false;
    auto& s=slots[std::size_t(id)];
    s.pendingName=name.substr(0,36);s.nameEditPending=true;
    s.displayName=s.pendingName;
    return true;
}
bool SharedPool::takeDisplayNameEdit(int id,std::string& out) {
    std::lock_guard<std::mutex> lock(controlMutex);
    if(id<0||id>=maxInstances||!slots[std::size_t(id)].nameEditPending)return false;
    auto& s=slots[std::size_t(id)];
    out=s.pendingName;s.nameEditPending=false;
    return true;
}
MixStats SharedPool::query(int,int session,int group) const noexcept {
    MixStats stats{};
    bool anySolo=false;
    for (const auto& s:slots) {
        if(s.occupied.load(std::memory_order_acquire) &&
           s.session.load(std::memory_order_relaxed)==session &&
           s.solo.load(std::memory_order_acquire) &&
           !s.localMute.load(std::memory_order_acquire)) {anySolo=true;break;}
    }
    for (auto& s:slots) {
        if(anySolo && !s.solo.load(std::memory_order_acquire))continue;
        if (!s.active.load(std::memory_order_acquire) || s.localMute.load(std::memory_order_acquire)) continue;
        if (s.session.load(std::memory_order_relaxed)!=session ||
            s.group.load(std::memory_order_relaxed)!=group ||
            s.mode.load(std::memory_order_relaxed)!=int(Mode::Auto)) continue;
        ++stats.autoCount;
        stats.scoreSum+=std::max(0.0,s.score.load(std::memory_order_relaxed));
        const double gain=std::clamp(s.rawGain.load(std::memory_order_relaxed),0.0,1.0);
        stats.rawGainPower+=gain*gain;
    }
    return stats;
}
void SharedPool::publishView(int id,int session,int group,Mode mode,bool muted,bool smart,float weight,
                             float inDb,float gainDb,float outDb) noexcept {
    if (id<0||id>=maxInstances) return;
    auto& s=slots[id];
    s.session.store(std::clamp(session,1,maxSessions),std::memory_order_relaxed);
    s.group.store(std::clamp(group,0,2),std::memory_order_relaxed);
    s.mode.store(int(mode),std::memory_order_relaxed);
    // Do NOT update localMute here. The parameter listener can exclude a mute
    // immediately, even if this audio callback is still using older parameters.
    s.weightDb.store(weight,std::memory_order_relaxed);
    s.smart.store(smart,std::memory_order_relaxed);
    s.inputDb.store(inDb,std::memory_order_relaxed);
    s.gainDb.store(gainDb,std::memory_order_relaxed);
    s.outputDb.store(outDb,std::memory_order_relaxed);
    (void)muted;
}
void SharedPool::publishConfig(int id,int session,int group,Mode mode,bool smart,float weight) noexcept {
    if(id<0||id>=maxInstances) return;
    auto& s=slots[id];
    s.session.store(std::clamp(session,1,maxSessions),std::memory_order_relaxed);
    s.group.store(std::clamp(group,0,2),std::memory_order_relaxed);
    s.mode.store(int(mode),std::memory_order_relaxed);
    s.smart.store(smart,std::memory_order_relaxed);
    s.weightDb.store(weight,std::memory_order_relaxed);
}
void SharedPool::publishAdvanced(int id,const std::array<float,5>& values) noexcept {
    if (id<0||id>=maxInstances) return;
    for(int j=0;j<5;++j)
        if(std::isfinite(values[std::size_t(j)]))
            slots[std::size_t(id)].advanced[std::size_t(j)].store(values[std::size_t(j)],std::memory_order_relaxed);
}
int SharedPool::sessionViews(int session,ParticipantView* out,int capacity) const noexcept {
    if (!out||capacity<=0) return 0;
    int count=0;
    for(int i=0;i<maxInstances&&count<capacity;++i) {
        const auto& s=slots[i];
        if(!s.occupied.load(std::memory_order_acquire)) continue;
        const auto gen=s.generation.load(std::memory_order_acquire);
        if(gen==0||s.session.load(std::memory_order_relaxed)!=session) continue;
        auto& v=out[count++];
        v.slot=i;v.generation=gen;
        v.session=session;
        v.group=s.group.load(std::memory_order_relaxed);
        v.mode=s.mode.load(std::memory_order_relaxed);
        v.localMute=s.localMute.load(std::memory_order_acquire);
        v.smart=s.smart.load(std::memory_order_relaxed);
        v.solo=s.solo.load(std::memory_order_acquire);
        v.processing=s.active.load(std::memory_order_acquire);
        v.weightDb=s.weightDb.load(std::memory_order_relaxed);
        v.levelDb=s.levelDb.load(std::memory_order_relaxed);
        v.inputDb=s.inputDb.load(std::memory_order_relaxed);
        v.gainDb=s.gainDb.load(std::memory_order_relaxed);
        v.outputDb=s.outputDb.load(std::memory_order_relaxed);
        for(int j=0;j<5;++j)
            v.advanced[std::size_t(j)]=s.advanced[std::size_t(j)].load(std::memory_order_relaxed);
    }
    return count;
}
std::uint32_t SharedPool::generationOf(int id) const noexcept {
    return id>=0&&id<maxInstances?slots[id].generation.load(std::memory_order_acquire):0;
}
bool SharedPool::validTarget(int id,std::uint32_t generation) const noexcept {
    return id>=0&&id<maxInstances&&generation!=0&&
           slots[id].occupied.load(std::memory_order_acquire)&&
           slots[id].generation.load(std::memory_order_acquire)==generation;
}
bool SharedPool::requestMute(int id,std::uint32_t gen,bool value) noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    if(!validTarget(id,gen)) return false;
    slots[id].commandMute.store(value?1:0,std::memory_order_release);
    // Fast shared-score exclusion; the host parameter and fade are committed
    // by the owner's timer at the next message-thread callback.
    if(value) setLocalMute(id,true);
    return true;
}
bool SharedPool::requestMode(int id,std::uint32_t gen,int value) noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    if(!validTarget(id,gen)||value<0||value>3) return false;
    slots[id].commandMode.store(value,std::memory_order_release);
    if(value==int(Mode::Mute)) setLocalMute(id,true);
    return true;
}
bool SharedPool::requestGroup(int id,std::uint32_t gen,int value) noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    if(!validTarget(id,gen)||value<0||value>2) return false;
    slots[id].commandGroup.store(value,std::memory_order_release);
    return true;
}
bool SharedPool::requestWeight(int id,std::uint32_t gen,float value) noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    if(!validTarget(id,gen)||!std::isfinite(value)) return false;
    const auto tenths=std::clamp(int(std::lround(value*10.f)),-240,240);
    slots[id].commandWeight.store(tenths,std::memory_order_release);
    return true;
}
bool SharedPool::requestSmart(int id,std::uint32_t gen,bool enabled) noexcept {
    const std::lock_guard<std::mutex> controlLock(controlMutex);
    if(!validTarget(id,gen)) return false;
    slots[id].commandSmart.store(enabled?1:0,std::memory_order_release);
    return true;
}
bool SharedPool::requestSolo(int id,std::uint32_t gen,bool enabled) noexcept {
    const std::lock_guard<std::mutex> lock(controlMutex);
    if(!validTarget(id,gen))return false;
    slots[std::size_t(id)].commandSolo.store(enabled?1:0,std::memory_order_release);
    // Act on the shared gain calculations immediately.
    setSolo(id,enabled);
    return true;
}
bool SharedPool::requestLevel(int id,std::uint32_t gen,float value) noexcept {
    const std::lock_guard<std::mutex> lock(controlMutex);
    if(!validTarget(id,gen)||!std::isfinite(value))return false;
    slots[std::size_t(id)].commandLevel.store(
        std::clamp(int(std::lround(value*10.f)),-900,120),std::memory_order_release);
    return true;
}
bool SharedPool::requestAdvanced(int id,std::uint32_t gen,int parameter,float value) noexcept {
    if(parameter<0||parameter>=5||!std::isfinite(value))return false;
    constexpr float low[5]={10.f,.5f,1.f,10.f,0.f};
    constexpr float high[5]={500.f,3.f,120.f,1500.f,350.f};
    const std::lock_guard<std::mutex> lock(controlMutex);
    if(!validTarget(id,gen))return false;
    const int scaled=int(std::lround(std::clamp(value,low[parameter],high[parameter])*100.f));
    slots[std::size_t(id)].commandAdvanced[std::size_t(parameter)].store(scaled,std::memory_order_release);
    return true;
}
PendingEdits SharedPool::consumeEdits(int id) noexcept {
    PendingEdits p;
    if(id<0||id>=maxInstances) return p;
    auto& s=slots[id];
    p.mute=s.commandMute.exchange(-1,std::memory_order_acq_rel);
    p.mode=s.commandMode.exchange(-1,std::memory_order_acq_rel);
    p.group=s.commandGroup.exchange(-1,std::memory_order_acq_rel);
    p.weightTenths=s.commandWeight.exchange(100000,std::memory_order_acq_rel);
    p.smart=s.commandSmart.exchange(-1,std::memory_order_acq_rel);
    p.solo=s.commandSolo.exchange(-1,std::memory_order_acq_rel);
    p.levelTenths=s.commandLevel.exchange(100000,std::memory_order_acq_rel);
    for(int j=0;j<5;++j)
        p.advancedHundredths[std::size_t(j)]=s.commandAdvanced[std::size_t(j)].exchange(-1,std::memory_order_acq_rel);
    return p;
}
}
