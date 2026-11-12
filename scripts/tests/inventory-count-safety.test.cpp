#include "InventoryCountSafety.h"

#include <cassert>
#include <cstdint>
#include <limits>

int main()
{
    using k2040::CanAddInventoryCount;
    using k2040::ExpectedInventoryCountMatches;
    using k2040::InventoryCorrection;
    using k2040::PlanInventoryCorrection;
    constexpr auto max = std::numeric_limits<std::uint32_t>::max();

    assert(CanAddInventoryCount(4, 1));
    assert(CanAddInventoryCount(4, 0));
    assert(CanAddInventoryCount(max - 2, 2));
    assert(!CanAddInventoryCount(max - 2, 3));
    assert(!CanAddInventoryCount(max, 1));

    assert(ExpectedInventoryCountMatches(9, 6, 3));
    assert(!ExpectedInventoryCountMatches(10, 6, 3));
    assert(!ExpectedInventoryCountMatches(0, max, 1));
    assert(ExpectedInventoryCountMatches(max, max - 1, 1));
    assert(ExpectedInventoryCountMatches(max, max, 0));

    assert(PlanInventoryCorrection(5, 5) == InventoryCorrection::None);
    assert(PlanInventoryCorrection(4, 5) == InventoryCorrection::AddOne);
    assert(PlanInventoryCorrection(6, 5) == InventoryCorrection::RemoveOne);
    assert(PlanInventoryCorrection(3, 5) == InventoryCorrection::Reject);
    assert(PlanInventoryCorrection(7, 5) == InventoryCorrection::Reject);
    assert(PlanInventoryCorrection(0, max) == InventoryCorrection::Reject);
    assert(PlanInventoryCorrection(max, 0) == InventoryCorrection::Reject);
    assert(PlanInventoryCorrection(max, max - 1) == InventoryCorrection::RemoveOne);
    assert(PlanInventoryCorrection(max - 1, max) == InventoryCorrection::AddOne);
    assert(PlanInventoryCorrection(0, 1) == InventoryCorrection::AddOne);
    assert(PlanInventoryCorrection(1, 0) == InventoryCorrection::RemoveOne);
}
