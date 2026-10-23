#include "../App/ChannelFormat.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace speechshare_modular;
int main(){
    assert(stereoToMono(1.f,-1.f)==0.f);
    assert(stereoToMono(1.f,1.f)==1.f);
    assert(std::abs(2.f*monoToStereo*monoToStereo-1.f)<1.e-6f);
    // Reconstructing from the common mono duplicate preserves polarity.
    assert(stereoToMono(-.4f,-.4f)<0.f);
    std::puts("MONO/STEREO channel format conversions PASS");
}
