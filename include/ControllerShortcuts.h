#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace k2040
{
    // Independent of keyboard/MCM hotkeys. "None" disables the action.
    void InitializeControllerShortcuts();
    std::string GetControllerShortcut(std::string_view action);
    bool SetControllerShortcut(std::string_view action, std::string_view binding);
    bool ResetControllerShortcuts();

    // Called by the existing 25 ms physical-input poller. Never invokes game
    // or Prisma APIs; the caller queues game-thread actions separately.
    void PollControllerShortcutEdges(bool suppressed, bool& quick, bool& builder);
}
