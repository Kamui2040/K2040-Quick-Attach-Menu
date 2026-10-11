// Standalone Linux/Windows unit test for shared input arbitration.
#include "HubChordPolicy.h"
#include <cassert>
#include <cstdio>

int main()
{
    constexpr std::uint32_t RB=0x0200u, LB=0x0100u, LT=1u<<16u, UP=0x0001u;
    using k2040::hub_chord::QuickShouldFire;
    assert(QuickShouldFire(RB,0,0,0,0,RB));          // Single button.
    assert(!QuickShouldFire(RB,LB,0,0,0,RB));       // Modifier required.
    assert(QuickShouldFire(RB,LB,0,0,LB,LB|RB));    // Chord and rising primary.
    assert(!QuickShouldFire(RB,LB,0,0,LB|RB,LB|RB));// No held repeat.
    assert(QuickShouldFire(UP,LT,0,0,LT,LT|UP));    // LT + D-pad Up.
    assert(!QuickShouldFire(RB,0,RB,0,0,RB));       // Exact Minimap collision.
    assert(!QuickShouldFire(RB,LB,RB,0,LB,LB|RB));  // Mini bare RB wins.
    assert(!QuickShouldFire(RB,0,RB,LB,LB,LB|RB));  // Mini held LB wins.
    assert(QuickShouldFire(RB,0,UP,LT,0,RB));       // Independent assignments.
    assert(QuickShouldFire(RB,0,UP,LT,LT|UP,LT|UP|RB)); // Mini already held.
    assert(!QuickShouldFire(0,0,0,0,0,RB));         // Unbound.
    std::puts("PASS: Quick Attach and Minimap chord arbitration");
}
