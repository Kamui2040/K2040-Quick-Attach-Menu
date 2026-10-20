#include "ControllerShortcuts.h"

#include "Logger.h"

#include <Windows.h>
#include <Xinput.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace
{
    constexpr std::uint32_t kLeftTrigger = 0x10000u;
    constexpr std::uint32_t kRightTrigger = 0x20000u;
    constexpr auto kSettingsFile = "Data/F4SE/Plugins/K2040_Quick_Attach_Menu/ControllerShortcuts.ini";

    struct Button
    {
        std::string_view name;
        std::uint32_t mask;
    };

    constexpr std::array<Button, 16> kButtons{{
        { "A", XINPUT_GAMEPAD_A }, { "B", XINPUT_GAMEPAD_B },
        { "X", XINPUT_GAMEPAD_X }, { "Y", XINPUT_GAMEPAD_Y },
        { "LB", XINPUT_GAMEPAD_LEFT_SHOULDER }, { "RB", XINPUT_GAMEPAD_RIGHT_SHOULDER },
        { "LT", kLeftTrigger }, { "RT", kRightTrigger },
        { "Back", XINPUT_GAMEPAD_BACK }, { "Start", XINPUT_GAMEPAD_START },
        { "LS", XINPUT_GAMEPAD_LEFT_THUMB }, { "RS", XINPUT_GAMEPAD_RIGHT_THUMB },
        { "DUp", XINPUT_GAMEPAD_DPAD_UP }, { "DDown", XINPUT_GAMEPAD_DPAD_DOWN },
        { "DLeft", XINPUT_GAMEPAD_DPAD_LEFT }, { "DRight", XINPUT_GAMEPAD_DPAD_RIGHT }
    }};

    std::string g_quickBinding = "None";
    std::atomic<std::uint32_t> g_quickMask{ 0 };
    std::atomic_bool g_requireControllerRelease{ true };

    std::optional<std::uint32_t> ParseBinding(std::string_view value)
    {
        if (value == "None") return 0;
        if (value.empty() || value.size() > 24) return std::nullopt;
        const auto separator = value.find('+');
        if (separator != std::string_view::npos &&
            (separator == 0 || separator == value.size() - 1 ||
             value.find('+', separator + 1) != std::string_view::npos)) {
            return std::nullopt;
        }

        const auto first = separator == std::string_view::npos ? value : value.substr(0, separator);
        const auto second = separator == std::string_view::npos ? std::string_view{} : value.substr(separator + 1);
        const auto toMask = [](std::string_view text) {
            for (const auto& button : kButtons) {
                if (button.name == text) return button.mask;
            }
            return std::uint32_t{ 0 };
        };
        const auto a = toMask(first);
        if (!a) return std::nullopt;
        if (separator == std::string_view::npos) return a;
        const auto b = toMask(second);
        if (!b || b == a) return std::nullopt;
        return a | b;
    }

    bool PersistBindings(std::string_view quick)
    {
        const std::filesystem::path target(kSettingsFile);
        const auto temporary = std::filesystem::path(target.string() + ".tmp");
        std::error_code error;
        std::filesystem::create_directories(target.parent_path(), error);
        if (error) return false;

        {
            std::ofstream output(temporary, std::ios::trunc | std::ios::binary);
            if (!output) return false;
            output << "[ControllerShortcuts]\nQuick=" << quick << "\n";
            output.flush();
            if (!output.good()) return false;
        }

        if (!MoveFileExA(temporary.string().c_str(), target.string().c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::filesystem::remove(temporary, error);
            return false;
        }
        return true;
    }

    using XInputStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);

    XInputStateFn ResolveXInputState()
    {
        static XInputStateFn getState = [] {
            for (const wchar_t* library : { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll" }) {
                auto* module = LoadLibraryW(library);
                if (!module) continue;
                if (auto* proc = GetProcAddress(module, "XInputGetState")) {
                    return reinterpret_cast<XInputStateFn>(proc);
                }
                FreeLibrary(module);
            }
            return static_cast<XInputStateFn>(nullptr);
        }();
        return getState;
    }

    std::uint32_t ReadControllerButtons()
    {
        auto getState = ResolveXInputState();
        if (!getState) return 0;

        // Read one physical/Steam Input virtual controller, never combine
        // buttons from different players into a synthetic shortcut.
        for (DWORD player = 0; player < XUSER_MAX_COUNT; ++player) {
            XINPUT_STATE state{};
            if (getState(player, &state) != ERROR_SUCCESS) continue;
            std::uint32_t down = state.Gamepad.wButtons;
            if (state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) down |= kLeftTrigger;
            if (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) down |= kRightTrigger;
            if (down) return down;
        }
        return 0;
    }
}

namespace k2040
{
    void InitializeControllerShortcuts()
    {
        std::string quick = "None";
        std::ifstream source(kSettingsFile);
        if (source) {
            std::string line;
            while (std::getline(source, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.starts_with("Quick=")) quick = line.substr(6);
            }
        }

        if (!ParseBinding(quick)) {
            log::Warn("Invalid Quick Menu controller shortcut; left unassigned.");
            quick = "None";
        }
        // Older files may contain Builder=; it is deliberately ignored.
        g_quickBinding = quick;
        g_quickMask = ParseBinding(quick).value_or(0);
        g_requireControllerRelease = true;
    }

    std::string GetControllerShortcut(std::string_view action)
    {
        if (action == "quick") return g_quickBinding;
        return "None";
    }

    bool SetControllerShortcut(std::string_view action, std::string_view binding)
    {
        const auto mask = ParseBinding(binding);
        if (action != "quick" || !mask) return false;
        if (!PersistBindings(binding)) return false;
        g_quickBinding = binding;
        g_quickMask = *mask;
        g_requireControllerRelease = true;
        return true;
    }

    bool ResetControllerShortcuts()
    {
        std::error_code error;
        std::filesystem::remove(kSettingsFile, error);
        if (error) return false;
        g_quickBinding = "None";
        g_quickMask = 0;
        g_requireControllerRelease = true;
        return true;
    }

    int ReadControllerStickSector()
    {
        const auto getState = ResolveXInputState();
        if (!getState) return -1;
        for (DWORD player = 0; player < XUSER_MAX_COUNT; ++player) {
            XINPUT_STATE state{};
            if (getState(player, &state) != ERROR_SUCCESS) continue;
            const auto& pad = state.Gamepad;
            // Prefer the left stick, with right stick as a fallback.
            // A generous dead zone prevents accidental selection drift.
            const auto quantize = [](SHORT x, SHORT y) -> int {
                const auto xx = static_cast<double>(x);
                const auto yy = static_cast<double>(y);
                if (xx * xx + yy * yy < 12500.0 * 12500.0) return -1;
                constexpr double kPi = 3.14159265358979323846;
                double angle = std::atan2(xx, yy);
                if (angle < 0) angle += 2.0 * kPi;
                return static_cast<int>(angle * 72.0 / (2.0 * kPi)) % 72;
            };
            const int left = quantize(pad.sThumbLX, pad.sThumbLY);
            if (left >= 0) return left;
            return quantize(pad.sThumbRX, pad.sThumbRY);
        }
        return -1;
    }

    void PollControllerShortcutEdges(bool suppressed, bool& quick)
    {
        quick = false;
        const auto down = ReadControllerButtons();
        static std::uint32_t previous = 0;
        const auto before = previous;
        previous = down;

        // A newly assigned shortcut never fires from buttons held during its
        // assignment. Both buttons must be released before activation.
        if (g_requireControllerRelease.load()) {
            if (down == 0) g_requireControllerRelease = false;
            return;
        }
        if (suppressed || down == 0) return;

        const auto quickMask = g_quickMask.load();
        quick = quickMask && (down & quickMask) == quickMask &&
            (before & quickMask) != quickMask;
    }
}
