#pragma once
// Pure controller arbitration. If the existing Minimap action and the
// Quick Menu could both trigger on one input edge, Minimap owns it.
#include <cstdint>

namespace k2040::hub_chord
{
    [[nodiscard]] constexpr bool Rising(
        std::uint32_t primary, std::uint32_t held,
        std::uint32_t before, std::uint32_t down) noexcept
    {
        return primary && (down & (primary | held)) == (primary | held) &&
            (before & primary) != primary;
    }

    [[nodiscard]] constexpr bool QuickShouldFire(
        std::uint32_t primary, std::uint32_t held,
        std::uint32_t minimapPrimary, std::uint32_t minimapHeld,
        std::uint32_t before, std::uint32_t down) noexcept
    {
        return Rising(primary,held,before,down) &&
            !Rising(minimapPrimary,minimapHeld,before,down);
    }
}
