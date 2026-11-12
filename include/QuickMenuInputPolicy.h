#pragma once

#include <cstdint>

namespace k2040
{
    // Disabling the sneaking user-event flag can interrupt the player's
    // current stance. Preserve it regardless of whether menu movement is on.
    [[nodiscard]] constexpr std::uint32_t AllowedQuickMenuUserEvents(
        std::uint32_t movement, std::uint32_t looking,
        std::uint32_t sneaking, bool allowMovement) noexcept
    {
        return sneaking | (allowMovement ? movement | looking : 0U);
    }
}
