#pragma once

#include <cstdint>

namespace k2040
{
    // Only for the Prisma Settings recorder. Native MCM may allow additional
    // keys, so do not restrict MCM's existing bindings or INI fallback.
    // CEF under Wine/Proton has reported keyCode=144 (VK_NUMLOCK) for Shift,
    // Ctrl, and Alt modifier presses. This must never replace a working bind.
    constexpr bool IsSettingsCaptureKeycode(std::uint32_t keycode)
    {
        if (keycode == 258 || keycode == 259 || keycode == 260) return true;
        if (keycode == 0 || keycode > 255) return false;
        if (keycode == 16 || keycode == 17 || keycode == 18 || keycode == 20 ||
            (keycode >= 91 && keycode <= 93) ||
            (keycode >= 144 && keycode <= 145) ||
            (keycode >= 160 && keycode <= 165) ||
            keycode == 224 || keycode == 229) return false;
        return true;
    }
}
