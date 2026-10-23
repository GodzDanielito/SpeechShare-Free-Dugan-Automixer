#include "../Insert/InsertDSP.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace speechshare_insert;
static void check(bool ok,const char* message) {
    if(!ok) {std::cerr<<"FAIL "<<message<<"\n";std::exit(1);}
}
static void runTwo(Settings s1,Settings s2,float input1,float input2,int channels2,
                   float& result1,float& result2) {
    auto& pool=SharedPool::instance();
    const int id1=pool.acquire(),id2=pool.acquire();check(id1>=0&&id2>=0,"slots allocated");
    InsertDSP d1,d2;d1.prepare(48000);d2.prepare(48000);
    std::array<float,128> a{},b{},bRight{};
    for(int n=0;n<450;++n) {
        a.fill(input1);b.fill(input2);bRight.fill(-input2);
        d1.process(a.data(),nullptr,1,128,s1,id1);
        d2.process(b.data(),channels2==2?bRight.data():nullptr,channels2,128,s2,id2);
        if (n==449) {result1=a.back();result2=b.back();if(channels2==2)
            check(std::abs(bRight.back()+result2)<.000001,"stereo linked gain");}
    }
    pool.release(id1);pool.release(id2);
}
int main() {
    Settings s,s2;s.rmsMs=s2.rmsMs=20;s.attackMs=s2.attackMs=1;s.releaseMs=s2.releaseMs=1;
    s.detectorHPF=s2.detectorHPF=0;
    float a=0,b=0;
    runTwo(s,s2,.1f,.1f,1,a,b);
    std::cout<<"two equal AUTO mono: "<<a<<", "<<b<<" (target 0.07071)\n";
    check(std::abs(a-.07071f)<.012&&std::abs(b-.07071f)<.012,"two equal auto gain-share");
    runTwo(s,s2,.1f,.1f,2,a,b);
    check(std::abs(a-.07071f)<.012&&std::abs(b-.07071f)<.012,"mono plus stereo AUTO");
    s2.group=1;runTwo(s,s2,.1f,.1f,2,a,b);
    check(std::abs(a-.1f)<.01&&std::abs(b-.1f)<.01,"independent groups");
    s2.group=0;s2.session=2;runTwo(s,s2,.1f,.1f,1,a,b);
    check(std::abs(a-.1f)<.01&&std::abs(b-.1f)<.01,"independent sessions");
    s2.session=1;s2.mode=Mode::Mute;runTwo(s,s2,.1f,.1f,1,a,b);
    check(std::abs(a-.1f)<.01&&std::abs(b)<1.e-5,"mute removes participant");
    // Dedicated channel MUTE: immediately excluded even if the DAW no longer
    // calls process() on its instance, and audio fades to silence in ~20 ms.
    {
        auto& pool=SharedPool::instance();
        int first=pool.acquire(),second=pool.acquire();
        check(first>=0&&second>=0,"dedicated mute slots allocated");
        InsertDSP da,db;da.prepare(48000);db.prepare(48000);
        Settings p,q;p.rmsMs=q.rmsMs=20;p.detectorHPF=q.detectorHPF=0;
        p.attackMs=q.attackMs=1;p.releaseMs=q.releaseMs=1;
        std::array<float,128> aa{},bb{};
        for(int n=0;n<350;++n) {
            aa.fill(.1f);bb.fill(.8f);
            da.process(aa.data(),nullptr,1,128,p,first);
            db.process(bb.data(),nullptr,1,128,q,second);
        }
        check(aa.back()<.065f,"before mute, loud peer attenuates quiet source");
        pool.setLocalMute(second,true);
        check(pool.query(first,1,0).autoCount==1,"mute excludes peer immediately");
        q.channelMute=true;
        for(int n=0;n<250;++n) {
            aa.fill(.1f);bb.fill(.8f);
            da.process(aa.data(),nullptr,1,128,p,first);
            db.process(bb.data(),nullptr,1,128,q,second);
        }
        check(aa.back()>.09f,"quiet source recovers while peer is muted");
        check(std::abs(bb.back())<.00001f,"mute closes audio output");
        pool.setLocalMute(second,false);
        q.channelMute=false;
        for(int n=0;n<350;++n) {
            aa.fill(.1f);bb.fill(.8f);
            da.process(aa.data(),nullptr,1,128,p,first);
            db.process(bb.data(),nullptr,1,128,q,second);
        }
        check(aa.back()<.065f,"unmuted peer re-enters gain sharing");
        check(bb.back()>.10f,"unmuted peer audio restored");
        pool.release(first);pool.release(second);
    }
    // GUI mailbox regression: session overview is read-only data and remote
    // parameter commands are consumed by the target instance (message thread).
    {
        auto& pool=SharedPool::instance();
        const int owner=pool.acquire();
        check(owner>=0,"GUI slot acquired");
        const auto gen=pool.generationOf(owner);
        pool.publishView(owner,3,1,Mode::Auto,false,false,-2.f,-18.f,-6.f,-24.f);
        std::array<ParticipantView,maxInstances> snapshot{};
        const int count=pool.sessionViews(3,snapshot.data(),int(snapshot.size()));
        check(count==1,"one participant in session 3");
        check(snapshot[0].slot==owner&&snapshot[0].generation==gen,"GUI source identity");
        check(snapshot[0].group==1&&std::abs(snapshot[0].gainDb+6.f)<.0001f,"GUI meter snapshot");
        check(pool.requestMute(owner,gen,true),"remote mute request accepted");
        check(pool.sessionViews(3,snapshot.data(),int(snapshot.size()))==1 && snapshot[0].localMute,
              "remote mute excludes source immediately");
        check(pool.requestMode(owner,gen,int(Mode::Manual)),"remote mode accepted");
        check(pool.requestGroup(owner,gen,2),"remote group accepted");
        check(pool.requestWeight(owner,gen,-8.2f),"remote weight accepted");
        check(pool.requestSmart(owner,gen,true),"session SMART request accepted");
        const auto edits=pool.consumeEdits(owner);
        check(edits.mute==1&&edits.mode==0&&edits.group==2&&edits.weightTenths==-82&&edits.smart==1,
              "remote edits delivered to owning processor");
        check(pool.consumeEdits(owner).mute==-1&&pool.consumeEdits(owner).smart==-1,
              "edits consumed once");
        pool.publishConfig(owner,3,1,Mode::Auto,true,-2.f);
        check(pool.sessionViews(3,snapshot.data(),int(snapshot.size()))==1&&snapshot[0].smart,
              "central view shows SMART after owner confirms parameter");
        check(pool.requestSmart(owner,gen,false),"session CLASSIC request accepted");
        check(pool.consumeEdits(owner).smart==0,"CLASSIC update delivered");
        pool.release(owner);
        check(!pool.requestMute(owner,gen,false),"stale GUI token rejected");
        check(!pool.requestSmart(owner,gen,true),"stale SMART token rejected");
        const int reused=pool.acquire();
        check(reused==owner,"slot reused for regression");
        check(pool.generationOf(reused)!=gen,"slot generation changed");
        check(!pool.requestMute(reused,gen,true),"stale request cannot mute new instance");
        check(!pool.requestSmart(reused,gen,true),"stale request cannot change detector on reused slot");
        pool.release(reused);
    }
    // Virtual reference KEY regressions, including inter-session isolation,
    // per-group enable, pink-noise reference, editable sine frequency and
    // absence of any generator audio at the plugin output.
    {
        auto& pool=SharedPool::instance();
        KeyGroupSettings keyA;
        keyA.enabled=true;keyA.generator=KeyGenerator::Sine;
        keyA.frequencyHz=40.f;keyA.levelDb=-18.f;
        pool.setKeyGroup(1,0,keyA);
        pool.setKeyMaster(1,false);
        pool.setKeyMaster(2,false);
        Settings input;input.rmsMs=20;input.attackMs=1;input.releaseMs=8;
        input.detectorHPF=0;input.session=1;
        auto runSingle=[&](Settings settings,float amp,int frames=48000) {
            int id=pool.acquire();check(id>=0,"KEY test acquired");
            InsertDSP dsp;dsp.prepare(48000);
            std::array<float,128> data{};
            float out=0.f;
            for(int done=0;done<frames;done+=128) {
                data.fill(amp);
                dsp.process(data.data(),nullptr,1,128,settings,id);
                out=data.back();
            }
            pool.release(id);
            return out;
        };
        const float dry=runSingle(input,.05f);
        check(std::abs(dry-.05f)<.004f,"KEY master off: unchanged single source");
        pool.setKeyMaster(1,true);
        const float keyed=runSingle(input,.05f);
        std::cout<<"KEY sine 40 Hz: dry="<<dry<<", keyed="<<keyed<<"\n";
        check(keyed<dry*.67f,"KEY attenuates auto without other audio sources");
        check(runSingle(input,0.f)==0.f,"virtual sine key is never audible");
        Settings groupB=input;groupB.group=1;
        check(std::abs(runSingle(groupB,.05f)-dry)<.004f,"KEY A does not affect group B");
        Settings sessionTwo=input;sessionTwo.session=2;
        check(std::abs(runSingle(sessionTwo,.05f)-dry)<.004f,"KEY A does not affect session 2");
        // With two competing microphones, KEY must not disappear when
        // the Classic normalization compensates asynchronous raw gains.
        keyA.frequencyHz=40.f;keyA.levelDb=-24.f;
        pool.setKeyGroup(1,0,keyA);
        Settings pair=input;pair.detectorHPF=0;
        float pairA=0.f,pairB=0.f;
        runTwo(pair,pair,.05f,.05f,1,pairA,pairB);
        check(pairA<.033f && pairB<.033f,
              "KEY competes with TWO real participants after normalization");
        // Sine frequency is an actual DSP parameter, not just GUI metadata.
        keyA.levelDb=-20.f;
        input.detectorHPF=150.f;
        keyA.frequencyHz=40.f;
        pool.setKeyGroup(1,0,keyA);
        const float low=runSingle(input,.05f);
        keyA.frequencyHz=1000.f;
        pool.setKeyGroup(1,0,keyA);
        const float high=runSingle(input,.05f);
        std::cout<<"HPF sine frequency: 40 Hz="<<low<<", 1000 Hz="<<high<<"\n";
        check(high<low*.8f,"editable sine frequency changes HPF detector gain");
        keyA.generator=KeyGenerator::Pink;
        pool.setKeyGroup(1,0,keyA);
        const float pink=runSingle(input,.05f);
        check(std::isfinite(pink)&&pink<dry*.9f,"pink virtual key attenuates auto");
        check(runSingle(input,0.f)==0.f,"virtual pink-noise key is never audible");
        keyA.enabled=false;
        pool.setKeyGroup(1,0,keyA);
        check(std::abs(runSingle(input,.05f)-dry)<.004f,"individual group KEY off");
        keyA.enabled=true;
        keyA.frequencyHz=40000.f;keyA.levelDb=10.f;
        pool.setKeyGroup(1,0,keyA);
        const auto bounded=pool.getSessionKeys(1);
        check(bounded.groups[0].frequencyHz==20000.f && bounded.groups[0].levelDb==0.f,
              "KEY frequency/level clamped");
        pool.setKeyMaster(1,false); // leave singleton clean for other tests
        pool.setKeyGroup(1,0,KeyGroupSettings{});
    }
    // v0.0.6 overview/regressions: Solo must gate other participants in the
    // SAME session only. A stale GUI token may never manipulate a new owner.
    {
        auto& pool=SharedPool::instance();
        int a=pool.acquire(),b=pool.acquire(),c=pool.acquire();
        check(a>=0&&b>=0&&c>=0,"v006 source slots acquired");
        const auto gb=pool.generationOf(b);
        pool.publish(a,1,0,Mode::Auto,.8,.9);
        pool.publish(b,1,0,Mode::Auto,.8,.9);
        pool.publish(c,2,0,Mode::Auto,.5,.9);
        check(pool.query(a,1,0).autoCount==2,"two sources share before SOLO");
        check(pool.requestSolo(a,pool.generationOf(a),true),"SOLO accepted");
        check(pool.isSoloExcluded(b,1),"SOLO gates other session participant");
        check(!pool.isSoloExcluded(a,1),"SOLO owner remains audible");
        check(!pool.isSoloExcluded(c,2),"SOLO doesn't affect separate session");
        check(pool.query(a,1,0).autoCount==1,"SOLO excludes peer gain sharing score");
        check(pool.requestLevel(b,gb,-12.4f),"remote channel level command");
        check(pool.requestDisplayName(b,gb,"LAV 02"),"remote name change");
        check(pool.getDisplayName(b,gb)=="LAV 02","name shown without touching DSP thread");
        std::string pending;
        check(pool.takeDisplayNameEdit(b,pending)&&pending=="LAV 02","name edit delivered once");
        check(!pool.takeDisplayNameEdit(b,pending),"no repeated name edit");
        const auto edits=pool.consumeEdits(b);
        check(edits.levelTenths==-124,"level mailbox commits to owner");
        const auto before=pool.generationOf(b);
        check(pool.requestAdvanced(b,before,0,83.f),"RMS session edit accepted");
        check(pool.requestAdvanced(b,before,1,1.25f),"Strength session edit accepted");
        check(pool.requestAdvanced(b,before,2,24.f),"Attack session edit accepted");
        check(pool.requestAdvanced(b,before,3,310.f),"Release session edit accepted");
        check(pool.requestAdvanced(b,before,4,210.f),"HPF session edit accepted");
        const auto timings=pool.consumeEdits(b);
        check(timings.advancedHundredths[0]==8300 && timings.advancedHundredths[1]==125 &&
              timings.advancedHundredths[2]==2400 && timings.advancedHundredths[3]==31000 &&
              timings.advancedHundredths[4]==21000,"advanced GUI mailboxes delivered exactly");
        check(pool.requestAdvanced(b,before,3,1000.f),"Release 1000ms must not conflict with mailbox sentinel");
        check(pool.consumeEdits(b).advancedHundredths[3]==100000,"Release 1000ms delivered");
        pool.publishAdvanced(b,{{83.f,1.25f,24.f,310.f,210.f}});
        std::array<ParticipantView,maxInstances> advancedViews{};
        check(pool.sessionViews(1,advancedViews.data(),int(advancedViews.size()))==2 &&
              std::abs(advancedViews[1].advanced[3]-310.f)<.001f,"advanced state visible to all GUI hosts");

        pool.setLevelDb(b,-12.4f);
        std::array<ParticipantView,maxInstances> views{};
        check(pool.sessionViews(1,views.data(),int(views.size()))==2,"same session sources visible");
        check(std::abs(views[1].levelDb+12.4f)<.0001f,"channel fader value visible");
        SessionControls ctrl;
        ctrl.bypass=true;ctrl.muted=true;ctrl.gainDb=-8.f;
        pool.setSessionControls(1,ctrl);
        auto restored=pool.getSessionControls(1);
        check(restored.bypass&&restored.muted&&std::abs(restored.gainDb+8.f)<1.e-5f,
              "session Bypass/Mute/Gain independently stored");
        auto other=pool.getSessionControls(2);
        check(!other.bypass&&!other.muted&&other.gainDb==0.f,"session controls isolated");
        ctrl.bypass=false;ctrl.muted=false;ctrl.gainDb=0.f;
        pool.setSessionControls(1,ctrl);
        pool.release(a);pool.release(b);pool.release(c);
        check(!pool.requestLevel(b,gb,0.f),"stale level edit rejected");
        check(!pool.requestDisplayName(b,gb,"STUCK"),"stale name edit rejected");
    }
    std::cout<<"Insert shared-pool core + dedicated MUTE + virtual KEY + v0.0.6 channel controls tests PASSED (not a JUCE/REAPER validation).\n";
}
