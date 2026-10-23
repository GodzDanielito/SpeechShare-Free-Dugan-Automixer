#include "../App/PatchRouting.h"
#include <cassert>
#include <cstdio>
using namespace speechshare_live;
int main(){
    PatchRouting p;
    for(int i=0;i<maxSources;i++) {assert(p.getInput(i)==i);assert(p.getOutput(i)==i);}
    p.setInput(2,63); p.setOutput(2,63);
    assert(p.getInput(2)==63 && p.getOutput(2)==63);
    p.setOutput(4,63); assert(p.getOutput(4)==63 && p.getOutput(2)==-1);
    p.setInput(4,-1);assert(p.getInput(4)==-1);
    p.setInput(4,128);assert(p.getInput(4)==-1);
    p.setOutput(3,-2);assert(p.getOutput(3)==3);
    p.resetOneToOne();assert(p.getInput(2)==2&&p.getOutput(2)==2);
    assert(p.getInput(-1)==-1&&p.getOutput(32)==-1);
    // A cascade of live-console repatches must never create output collisions.
    for(int step=0;step<10000;++step){
        const int src=(step*17)%maxSources;
        const int dest=(step*37)%maxDeviceChannels;
        p.setOutput(src,dest);
        bool used[maxDeviceChannels]{};
        for(int i=0;i<maxSources;++i){
            const int port=p.getOutput(i);
            if(port<0)continue;
            assert(!used[port]);used[port]=true;
        }
    }
    std::puts("PATCH routing & uniqueness stress: PASS");
}
