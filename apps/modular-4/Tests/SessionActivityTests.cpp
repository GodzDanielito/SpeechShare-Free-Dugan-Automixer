#include "../App/SessionActivity.h"
#include <cstdio>
#include <cstdlib>
using namespace speechshare_modular;
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){
    ModularPatch patch;
    std::array<int,participants> groups{};
    bool hw[4]={true,true,false,false};
    auto plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==1,"only session 1 awake by default");
    check(plan.source[0]&&plan.source[1]&&!plan.source[2],"physical port validation");
    check(plan.group[0][0]&&!plan.group[1][0],"only first group active");
    // S1 Group A feeds S2 CH 1, then S2 output feeds S3, and so on.
    check(patch.setInput(8,{InputKind::InternalGroupStereo,0}),"S1 group -> S2");
    check(patch.setInput(16,{InputKind::InternalChannelStereo,8}),"S2 source -> S3");
    check(patch.setInput(24,{InputKind::InternalGroupMono,6}),"S3 group -> S4");
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==4 && plan.source[24],"all four awake when connected");
    // The full graph must go to sleep when its first physical source is gone.
    hw[0]=false;hw[1]=false;
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==0 && !plan.source[24],"cascade sleeps on disconnected ports");
    // A live silent port MUST stay awake: no voice-triggered wake latency.
    hw[0]=true;
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==4,"connected port wakes entire graph");
    // Reassign source to group B: group A tap must stop, not play stale bus data.
    groups[0]=1;
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==1 && plan.group[0][1] && !plan.group[0][0],"group reassignment sleeps dependents");
    // Stereo ASIO requires BOTH input ports.
    check(patch.setInput(0,{InputKind::AsioStereo,0}),"stereo ASIO route");
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==0,"missing right channel invalidates stereo");
    hw[1]=true;
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==4 && plan.source[0],"stereo pair wakes when both present; S1 CH2 also enables Group A");
    // A configured hardware output cannot wake a session with no input.
    check(patch.setOutput(8,{OutputKind::AsioMono,40}),"S2 output set");
    check(patch.setInput(8,{}),"disconnect S2 source");
    plan=SessionActivity::evaluate(patch,hw,4,groups);
    check(plan.awakeCount==1 && !plan.session[1],"unused hardware output does not wake session");
    std::puts("PASS: graph sleep, 4-session cascading, stereo, reassignment, hardware-only outputs");
}
