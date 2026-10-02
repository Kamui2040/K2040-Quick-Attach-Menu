#pragma once

#include <cstdint>
#include <string>

namespace k2040
{
    struct HotkeyState
    {
        bool altRequired = false;
        bool ctrlRequired = false;
        bool shiftRequired = false;
        uint32_t virtualKey = 0;
    };

    struct SharedHotkeyBinding
    {
        std::uint32_t keycode = 0;
        std::uint32_t modifiers = 0;
        std::string displayName;
        bool fromSharedRegistry = false;
    };

    HotkeyState ParseHotkey(const std::string& text);
    bool IsHotkeyPressedNow(const HotkeyState& hotkey);
    SharedHotkeyBinding GetSharedHotkeyBinding(const std::string& id);
    bool SetSharedHotkeyBinding(const std::string& id, std::uint32_t keycode, std::uint32_t modifiers);
    bool ResetSharedHotkeyBindings();
    void SetHotkeyCaptureActive(bool active);
    bool IsHotkeyCaptureActive();
    void SetMenuHotkeysOwnedByUi(bool active);
    void QueueMenuHotkeyActionFromUi(bool openBuilder);

    void InitializeHotkey();
    void PollHotkey();
}
