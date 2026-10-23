#include "../App/ModularPatch.h"
#include <cstdio>
#include <cstdlib>
using namespace speechshare_modular;
static void require(bool ok, const char* name){
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}
}
int main(){
    require(sessions==4 && participants==32,"four sessions / 32 sources");
    ModularPatch p;
    for(int i=0;i<8;++i){
        require(p.input(i).kind==InputKind::AsioMono && p.output(i).index==i,"default S1 1:1");
    }
    for(int i=8;i<32;++i){
        require(p.input(i).kind==InputKind::Off && p.output(i).kind==OutputKind::Off,"other sessions OFF");
    }
    // S1 can feed S2, S3 and S4, both mono and stereo.
    require(p.setInput(8,{InputKind::InternalChannelStereo,0}),"S1 CH -> S2 CH stereo");
    require(p.setInput(16,{InputKind::InternalChannelMono,5}),"S1 CH -> S3 CH mono");
    require(p.setInput(24,{InputKind::InternalGroupStereo,2}),"S1 Group C -> S4 CH stereo");
    // S2 can feed S3 or S4, and S3 can feed S4.
    require(p.setInput(17,{InputKind::InternalGroupMono,3}),"S2 Group A -> S3");
    require(p.setInput(25,{InputKind::InternalChannelStereo,8}),"S2 CH1 -> S4 stereo");
    require(p.setInput(26,{InputKind::InternalGroupStereo,7}),"S3 Group B -> S4 stereo");
    require(p.setInput(27,{InputKind::InternalChannelMono,23}),"S3 CH8 -> S4 mono");
    // Same-session edges, backward edges, and self-edges must fail.
    require(!p.setInput(3,{InputKind::InternalChannelMono,0}),"S1 no internal sources");
    require(!p.setInput(8,{InputKind::InternalChannelMono,8}),"S2 no self loop");
    require(!p.setInput(16,{InputKind::InternalChannelMono,23}),"S3 no same-session edge");
    require(!p.setInput(17,{InputKind::InternalGroupStereo,6}),"S3 no same-session group edge");
    require(!p.setInput(8,{InputKind::InternalChannelMono,16}),"S2 no S3 feedback");
    require(!p.setInput(24,{InputKind::InternalGroupMono,9}),"S4 no S4 group feedback");
    require(!p.setInput(27,{InputKind::InternalChannelMono,32}),"out-of-range CH");
    require(!p.setInput(27,{InputKind::InternalGroupStereo,12}),"out-of-range group");
    // Hardware output reservations, including overlap with stereo pairs.
    require(p.setOutput(0,{OutputKind::Off,-1}),"release OUT1");
    require(p.setOutput(1,{OutputKind::Off,-1}),"release OUT2");
    require(p.setOutput(16,{OutputKind::AsioStereo,0}),"S3 CH1 stereo OUT1+2");
    require(!p.setGroupOutput(3,0,{OutputKind::AsioMono,1}),"S4 Group A cannot take R");
    require(p.setGroupOutput(3,2,{OutputKind::AsioStereo,42}),"S4 Group C stereo OUT43+44");
    require(!p.setOutput(17,{OutputKind::AsioMono,43}),"S3 CH2 cannot take group R");
    require(p.setGroupOutput(2,1,{OutputKind::AsioMono,40}),"S3 Group B mono OUT41");
    p.reset();
    for(int i=8;i<32;++i)
        require(p.input(i).kind==InputKind::Off && p.output(i).kind==OutputKind::Off,"reset S2-4 OFF");
    for(int s=0;s<4;++s)for(int g=0;g<3;++g)
        require(p.groupOutput(s,g).kind==OutputKind::Off,"reset group outs OFF");
    std::puts("FOUR SESSIONS: 32 channels, forward DAG mono/stereo, groups, output isolation PASS");
}
