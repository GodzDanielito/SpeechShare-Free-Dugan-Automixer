#pragma once
// Process-local, lock-free DSP score exchange for SpeechShare Insert PROTOTYPE.
// No locks, allocation or JUCE calls on the audio thread. Cross-instance GUI
// commands travel through atomic mailboxes; the owning processor commits them
// to its own host parameters on the JUCE message thread.
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace speechshare_insert {
constexpr int maxInstances = 32;
constexpr int maxSessions = 8;
enum class KeyGenerator : int { Pink=0, Sine=1 };
struct KeyGroupSettings {
    bool enabled=true;
    KeyGenerator generator=KeyGenerator::Sine;
    float frequencyHz=40.f;  // only used for sine generator
    float levelDb=-56.f;
};
struct SessionControls {
    bool bypass=false;
    bool muted=false;
    float gainDb=0.f;
};
struct SessionKeys {
    bool master=false;  // backward compatibility: version 0.0.4 has no keys
    std::array<KeyGroupSettings,3> groups{};
};
enum class Mode : int { Manual=0, Auto=1, Mute=2, Bypass=3 };

struct MixStats {
    double scoreSum=0.0;
    double rawGainPower=0.0;
    int autoCount=0;
};

struct ParticipantView {
    int slot=-1;
    std::uint32_t generation=0;
    int session=1,group=0,mode=int(Mode::Auto);
    bool localMute=false, processing=false, smart=false, solo=false;
    float weightDb=0.f,levelDb=0.f,inputDb=-96.f,gainDb=0.f,outputDb=-96.f;
    // RMS ms, Strength, Attack ms, Release ms, Detector HPF Hz.
    std::array<float,5> advanced{{100.f,1.f,12.f,160.f,100.f}};
};

struct PendingEdits {
    int mute=-1,mode=-1,group=-1,smart=-1,solo=-1;
    int levelTenths=100000;
    int weightTenths=100000; // sentinel outside [-240, 240]
    std::array<int,5> advancedHundredths{{-1,-1,-1,-1,-1}};
};

class SharedPool final {
public:
    static SharedPool& instance() noexcept;
    int acquire() noexcept;
    void release(int id) noexcept;
    void publish(int id,int session,int group,Mode mode,double score,double rawGain) noexcept;
    void deactivate(int id) noexcept;
    void setLocalMute(int id,bool muted) noexcept;
    MixStats query(int id,int session,int group) const noexcept;

    // Session-wide virtual KEY settings. Editor/host state writes are serialized
    // on message threads. DSP reads atomics only, never the control mutex.
    SessionKeys getSessionKeys(int session) const noexcept;
    SessionControls getSessionControls(int session) const noexcept;
    void setSessionControls(int session,const SessionControls&) noexcept;
    void setKeyMaster(int session,bool enabled) noexcept;
    void setKeyGroup(int session,int group,const KeyGroupSettings& settings) noexcept;

    // Presentation data are published by each processing instance. Never
    // modify the DSP math: these atomics are for the overview GUI only.
    void publishView(int id,int session,int group,Mode mode,bool muted,bool smart,float weight,
                     float inDb,float gainDb,float outDb) noexcept;
    void publishConfig(int id,int session,int group,Mode mode,bool smart,float weight) noexcept;
    void publishAdvanced(int id,const std::array<float,5>& values) noexcept;
    int sessionViews(int session,ParticipantView* out,int capacity) const noexcept;
    bool isSoloExcluded(int id,int session) const noexcept;
    void setSolo(int id,bool enabled) noexcept;
    void setLevelDb(int id,float db) noexcept;
    void setDisplayName(int id,const std::string& name);
    std::string getDisplayName(int id,std::uint32_t generation) const;
    bool requestDisplayName(int id,std::uint32_t generation,const std::string& name);
    bool takeDisplayNameEdit(int id,std::string& out);
    std::uint32_t generationOf(int id) const noexcept;

    // GUI sends commands using slot+generation to prevent addressing a newly
    // opened plugin after an old slot is released. Host parameter writes are
    // deferred to the owner's message-thread timer.
    bool requestMute(int id,std::uint32_t generation,bool value) noexcept;
    bool requestMode(int id,std::uint32_t generation,int value) noexcept;
    bool requestGroup(int id,std::uint32_t generation,int value) noexcept;
    bool requestWeight(int id,std::uint32_t generation,float value) noexcept;
    bool requestSmart(int id,std::uint32_t generation,bool enabled) noexcept;
    bool requestSolo(int id,std::uint32_t generation,bool enabled) noexcept;
    bool requestLevel(int id,std::uint32_t generation,float value) noexcept;
    bool requestAdvanced(int id,std::uint32_t generation,int parameter,float value) noexcept;
    PendingEdits consumeEdits(int id) noexcept;
private:
    struct Slot {
        std::atomic<bool> occupied{false};
        std::atomic<bool> active{false};
        std::atomic<bool> localMute{false};
        std::atomic<bool> smart{false},solo{false};
        std::string displayName, pendingName;
        bool nameEditPending=false;
        std::atomic<std::uint32_t> generation{0};
        std::atomic<int> session{1},group{0},mode{int(Mode::Auto)};
        std::atomic<double> score{0.0},rawGain{1.0};
        std::atomic<float> weightDb{0.f},levelDb{0.f},inputDb{-96.f},gainDb{0.f},outputDb{-96.f};
        std::array<std::atomic<float>,5> advanced{};
        std::atomic<int> commandMute{-1},commandMode{-1},commandGroup{-1},commandWeight{100000},commandSmart{-1},commandSolo{-1},commandLevel{100000};
        std::array<std::atomic<int>,5> commandAdvanced{};
    };
    struct KeyAtoms {
        std::atomic<bool> master{false};
        struct Group {
            std::atomic<bool> enabled{true};
            std::atomic<int> generator{int(KeyGenerator::Sine)};
            std::atomic<float> frequencyHz{40.f};
            std::atomic<float> levelDb{-56.f};
        };
        std::array<Group,3> groups{};
    };
    struct SessionAtoms {
        std::atomic<bool> bypass{false},muted{false};
        std::atomic<float> gainDb{0.f};
    };
    std::array<SessionAtoms,maxSessions> sessionBanks{};
    std::array<KeyAtoms,maxSessions> keyBanks{};
    std::array<Slot,maxInstances> slots{};
    std::atomic<std::uint32_t> nextGeneration{1};
    // GUI/lifecycle only. No audio-thread method takes this mutex.
    mutable std::mutex controlMutex;
    bool validTarget(int id,std::uint32_t generation) const noexcept;
};
}
