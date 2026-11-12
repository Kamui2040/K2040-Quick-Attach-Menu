#pragma once

#include <cstdint>
#include <limits>

namespace k2040
{
    // Inventory and loose-return quantities are uint32_t. Check before adding,
    // rather than trusting wraparound or accepting an impossible expected count.
    [[nodiscard]] constexpr bool CanAddInventoryCount(
        std::uint32_t before, std::uint32_t amount) noexcept
    {
        return amount <= std::numeric_limits<std::uint32_t>::max() - before;
    }

    [[nodiscard]] constexpr bool ExpectedInventoryCountMatches(
        std::uint32_t actual, std::uint32_t before, std::uint32_t amount) noexcept
    {
        return actual >= before && actual - before == amount;
    }

    enum class InventoryCorrection
    {
        None,
        AddOne,
        RemoveOne,
        Reject
    };

    [[nodiscard]] constexpr InventoryCorrection PlanInventoryCorrection(
        std::uint32_t current, std::uint32_t expected) noexcept
    {
        if (current == expected) return InventoryCorrection::None;
        if (current < expected && expected - current == 1u)
            return InventoryCorrection::AddOne;
        if (current > expected && current - expected == 1u)
            return InventoryCorrection::RemoveOne;
        return InventoryCorrection::Reject;
    }
}
