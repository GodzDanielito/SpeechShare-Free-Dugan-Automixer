#include "../Insert/SharedPool.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace speechshare_insert;
static void require(bool ok,const char* msg) {
    if (!ok) {std::cerr << "FAIL " << msg << '\n'; std::exit(1);}
}
int main() {
    auto& pool=SharedPool::instance();
    constexpr int peers=16;
    for(int cycle=0;cycle<1800;++cycle) {
        std::array<int,peers> ids{};
        std::array<std::uint32_t,peers> gen{};
        for(int j=0;j<peers;++j) {
            ids[j]=pool.acquire();
            require(ids[j]>=0,"all sixteen slots acquired");
            gen[j]=pool.generationOf(ids[j]);
            require(gen[j]!=0,"generation valid");
            pool.publish(ids[j],j<8?1:2,j%3,Mode::Auto,.25,.75);
        }
        std::array<ParticipantView,maxInstances> views{};
        require(pool.sessionViews(1,views.data(),maxInstances)==8,"session 1 has 8 peers");
        require(pool.sessionViews(2,views.data(),maxInstances)==8,"session 2 has 8 peers");
        require(pool.query(ids[0],1,0).autoCount==3,"group 0 session 1 has three peers");
        require(pool.query(ids[8],2,0).autoCount==3,"group 0 session 2 has three peers");
        require(pool.requestMute(ids[0],gen[0],true),"remote mute accepted");
        require(pool.query(ids[0],1,0).autoCount==2,"mute excludes source from correct group");
        require(pool.query(ids[8],2,0).autoCount==3,"mute does not affect other session");
        require(pool.requestWeight(ids[3],gen[3],-7.5f),"weight request accepted");
        auto cmds=pool.consumeEdits(ids[3]);
        require(cmds.weightTenths==-75,"weight delivered in tenths");
        for(int j=0;j<peers;++j)pool.release(ids[j]);
        require(!pool.requestGroup(ids[0],gen[0],2),"stale group command rejected");
        require(pool.sessionViews(1,views.data(),maxInstances)==0,"released session now empty");
        require(pool.sessionViews(2,views.data(),maxInstances)==0,"released second session now empty");
    }
    std::cout << "1800 repeated 16-instance lifecycles, group/mute/session isolation PASSED\n";
}
