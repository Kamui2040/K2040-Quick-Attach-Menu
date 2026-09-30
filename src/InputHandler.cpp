#include "InputHandler.h"

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "Hotkey.h"
#include "Logger.h"

namespace
{
    std::atomic_bool g_registered{ false };
    std::atomic_bool g_running{ false };

    void HotkeyThreadProc()
    {
        k2040::log::Info("Hotkey polling thread started.");

        while (g_running.load(std::memory_order_relaxed)) {
            k2040::PollHotkey();
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        k2040::log::Info("Hotkey polling thread stopped.");
    }
}

namespace k2040
{
    bool RegisterInputHandler()
    {
        bool expected = false;

        if (!g_registered.compare_exchange_strong(expected, true)) {
            log::Info("Input handler already registered.");
            return true;
        }

        g_running.store(true, std::memory_order_relaxed);

        std::thread(HotkeyThreadProc).detach();

        log::Info("Input handler registered. Hotkey polling is now active.");
        return true;
    }
}
