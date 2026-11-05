#include "NativeMcmBindingRecovery.h"
#include <cassert>
#include <vector>

int main()
{
    using k2040::NativeMcmRecord;
    using k2040::NativeMcmRecordState;
    using k2040::ClassifyNativeMcmRecords;
    const auto classify=[](const std::vector<NativeMcmRecord>& entries) {
        return ClassifyNativeMcmRecords(entries,"K2040_Quick_Attach_Menu","openMenuBuilder");
    };
    const NativeMcmRecord unbound{"K2040_Quick_Attach_Menu", "openMenuBuilder", 0, 0};
    const NativeMcmRecord mouse5{"K2040_Quick_Attach_Menu", "openMenuBuilder", 260, 0};
    const NativeMcmRecord quick{"K2040_Quick_Attach_Menu", "openQuickMenu", 259, 0};
    const NativeMcmRecord other{"K2040_Quest_Journal", "questTrackerToggle", 0, 0};
    assert(classify({}) == NativeMcmRecordState::Missing);
    assert(classify({mouse5}) == NativeMcmRecordState::Unique);
    assert(classify({unbound}) == NativeMcmRecordState::Unique);
    assert(classify({unbound,mouse5,quick}) == NativeMcmRecordState::StaleUnboundDuplicate);
    assert(classify({unbound,mouse5,other}) == NativeMcmRecordState::Ambiguous);
    assert(classify({unbound,mouse5,{"K2040_Quick_Attach_Menu","openMenuBuilder",75,3}}) ==
           NativeMcmRecordState::Ambiguous);
    assert(classify({unbound,unbound,mouse5}) == NativeMcmRecordState::Ambiguous);
    assert(classify({{"K2040_Quick_Attach_Menu","openMenuBuilder",0,1}}) ==
           NativeMcmRecordState::Ambiguous);
    assert(classify({quick,other}) == NativeMcmRecordState::Missing);
    const auto bound = k2040::SingleBoundNativeMcmRecord({unbound,mouse5},
        "K2040_Quick_Attach_Menu", "openMenuBuilder");
    assert(bound && bound->keycode==260 && bound->modifiers==0);
    assert(!k2040::SingleBoundNativeMcmRecord({unbound,mouse5,
        {"K2040_Quick_Attach_Menu","openMenuBuilder",75,3}},
        "K2040_Quick_Attach_Menu", "openMenuBuilder"));
    assert(k2040::NativeMcmChordOccupied({mouse5,other},260,0));
    assert(k2040::NativeMcmChordOccupied({mouse5,other},0,0));
    assert(!k2040::NativeMcmChordOccupied({mouse5,other},75,3));

    // Regression: PauseMenu first sees one registered action, then MCM
    // registers Mouse 4 and accidentally leaves Builder at 0 plus Mouse 5.
    // The late snapshot must be re-examined in the *same* PauseMenu session.
    k2040::NativeMcmChangeGate gate;
    const std::vector<NativeMcmRecord> first{unbound};
    const std::vector<NativeMcmRecord> late{unbound,quick,mouse5};
    assert(!gate.ShouldScan(first));
    assert(gate.ShouldScan(first));
    assert(!gate.ShouldScan(first));
    assert(!gate.ShouldScan(late));
    assert(gate.ShouldScan({mouse5,unbound,quick})); // Same map, different order
    assert(!gate.ShouldScan(late));
    const std::vector<NativeMcmRecord> repaired{quick,mouse5};
    assert(!gate.ShouldScan(repaired));
    assert(gate.ShouldScan(repaired));
    assert(!gate.ShouldScan(repaired));
    gate.Reset();
    assert(!gate.ShouldScan(late));
    assert(gate.ShouldScan(late));

}
