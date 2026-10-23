#include "../App/ModularPatch.h"
#include <cstdlib>
#define assert(expr) do { if(!(expr)) std::abort(); } while(false)
#include <cstdio>
using namespace speechshare_modular;
int main(){
    ModularPatch p;
    for(int i=0;i<8;++i){assert(p.input(i).kind==InputKind::AsioMono&&p.output(i).index==i);}
    for(int i=8;i<16;++i)assert(p.input(i).kind==InputKind::Off&&p.output(i).kind==OutputKind::Off);
    assert(p.setInput(0,{InputKind::AsioStereo,0}));
    assert(ModularPatch::isStereo(p.input(0).kind));
    assert(!p.setInput(0,{InputKind::Session1GroupStereo,0})); // no feedback
    assert(p.setInput(8,{InputKind::Session1GroupStereo,0}));
    assert(p.setInput(9,{InputKind::Session1ChannelMono,2}));
    assert(!p.setInput(9,{InputKind::Session1ChannelMono,8}));
    assert(p.setOutput(0,{OutputKind::Off,-1}));
    assert(p.setOutput(0,{OutputKind::AsioStereo,20}));
    assert(!p.setOutput(1,{OutputKind::AsioMono,21})); // overlaps stereo R
    assert(p.setGroupOutput(0,0,{OutputKind::AsioStereo,40}));
    assert(!p.setGroupOutput(1,0,{OutputKind::AsioMono,41}));
    assert(p.setGroupOutput(1,0,{OutputKind::AsioMono,45}));
    assert(!p.setGroupOutput(1,0,{OutputKind::AsioMono,128}));
    assert(!p.setInput(8,{InputKind::AsioStereo,127}));
    p.reset();
    assert(p.output(0).index==0&&p.input(8).kind==InputKind::Off);
    std::puts("MODULAR: mono/stereo, two-session DAG, groups and output uniqueness PASS");
}
