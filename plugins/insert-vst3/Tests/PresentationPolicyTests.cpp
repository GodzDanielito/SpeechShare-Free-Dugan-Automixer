#include "../Insert/PresentationPolicy.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace speechshare_insert;
int main() {
    assert(canonicalDisplayDb(-0.00001f) == 0.f);
    assert(!std::signbit(canonicalDisplayDb(-0.00001f)));
    assert(canonicalDisplayDb(-0.049f) == 0.f);
    assert(canonicalDisplayDb(0.049f) == 0.f);
    assert(canonicalDisplayDb(-0.051f) < 0.f);
    assert(canonicalDisplayDb(-9.5f) == -9.5f);
    assert(canonicalDisplayDb(NAN) == -96.f);
    assert(canonicalDisplayDb(INFINITY) == -96.f);
    assert(shouldFollowHostTrackName(false,"LAV GERRY"));
    assert(!shouldFollowHostTrackName(true,"LAV GERRY"));
    assert(!shouldFollowHostTrackName(false,""));
    std::cout << "Display formatting and track-name policy PASSED\n";
}
