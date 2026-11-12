#include "QuickMenuInputPolicy.h"

#include <cassert>
#include <cstdint>

int main()
{
    constexpr std::uint32_t kMovement = 1U << 0;
    constexpr std::uint32_t kLooking = 1U << 1;
    constexpr std::uint32_t kSneaking = 1U << 7;
    constexpr std::uint32_t kOther = 1U << 9;
    const auto blocked = [&](bool allowMovement) {
        return (kMovement | kLooking | kSneaking | kOther) &
            ~k2040::AllowedQuickMenuUserEvents(kMovement, kLooking, kSneaking, allowMovement);
    };
    assert((blocked(false) & kSneaking) == 0);
    assert((blocked(false) & kMovement) != 0);
    assert((blocked(false) & kLooking) != 0);
    assert((blocked(false) & kOther) != 0);
    assert((blocked(true) & (kSneaking | kMovement | kLooking)) == 0);
    assert((blocked(true) & kOther) != 0);
}
