#include "ControllerShortcuts.h"

#include "Logger.h"
#include "HubChordPolicy.h"

#include <Windows.h>
#include <Xinput.h>

#include <array>
#include <chrono>
#include <charconv>
#include <sstream>
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
    const std::filesystem::path kHubDefaultFile =
        std::filesystem::path("Data") / "MCM" / "Config" / "K2040_Quick_Attach_Menu" / "settings.ini";
    const std::filesystem::path kHubUserFile =
        std::filesystem::path("Data") / "MCM" / "Settings" / "K2040_Quick_Attach_Menu.ini";
    // Pre-correction experimental Hub bindings are read-only fallbacks.
    // Never rewrite them: they may also contain another mod's preferences.
    const std::filesystem::path kOldHubUserFile =
        std::filesystem::path("Data") / "MCM" / "Settings" / "K2040_Mods.ini";
    const std::filesystem::path kMinimapUserFile =
        std::filesystem::path("Data") / "MCM" / "Settings" / "K2040_Prisma_Minimap.ini";
    constexpr std::size_t kHubIniMaxBytes = 128u * 1024u;
    constexpr std::array<std::string_view, 17> kHubButtonNames{
        "None", "A", "B", "X", "Y", "DUp", "DDown", "DLeft", "DRight",
        "LB", "RB", "LT", "RT", "Start", "Back", "LS", "RS"
    };
    struct HubControllerSettings
    {
        bool installed = false;
        bool quickConfigured = false;
        unsigned quickPrimary = 0;
        unsigned quickHeld = 0;
        unsigned miniPrimary = 0;
        unsigned miniHeld = 0;
    };
    std::atomic_bool g_hubControllerEnabled{false};
    std::atomic<std::uint32_t> g_hubQuickPrimary{0};
    std::atomic<std::uint32_t> g_hubQuickHeld{0};
    std::atomic<std::uint32_t> g_hubMiniPrimary{0};
    std::atomic<std::uint32_t> g_hubMiniHeld{0};


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
    std::atomic<std::uint32_t> g_quickMask{0};
    std::atomic_bool g_requireControllerRelease{true};

    std::string TrimLine(std::string value)
    {
        const auto front = value.find_first_not_of(" \t\r\n");
        if (front == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(front, last - front + 1);
    }

    bool HubInstalled()
    {
        // The parent K2040_Mods configuration is deliberately optional.
        std::error_code error;
        return std::filesystem::is_regular_file(kHubDefaultFile, error) && !error;
    }

    std::optional<unsigned> ParseHubButton(std::string_view name)
    {
        for (unsigned i = 0; i < kHubButtonNames.size(); ++i)
            if (kHubButtonNames[i] == name) return i;
        return std::nullopt;
    }

    std::uint32_t HubMask(unsigned index)
    {
        if (index >= kHubButtonNames.size() || !index) return 0;
        for (const auto& button : kButtons)
            if (button.name == kHubButtonNames[index]) return button.mask;
        return 0;
    }

    struct IniBinding
    {
        bool present = false;
        unsigned primary = 0;
        unsigned held = 0;
    };

    IniBinding ReadIniBinding(const std::filesystem::path& file,
        std::string_view primaryField, std::string_view heldField)
    {
        IniBinding result;
        std::error_code error;
        if (!std::filesystem::is_regular_file(file, error) || error)
            return result;
        const auto size = std::filesystem::file_size(file, error);
        if (error || size > kHubIniMaxBytes) return result;
        std::ifstream source(file);
        if (!source) return result;

        bool inController = false;
        bool sawPrimary = false, sawHeld = false;
        std::string line;
        const auto number = [](std::string_view value) -> unsigned {
            unsigned parsed = 0;
            const auto out = std::from_chars(value.data(), value.data()+value.size(), parsed);
            return out.ec == std::errc{} && out.ptr == value.data()+value.size() &&
                parsed <= 16 ? parsed : 0;
        };
        while (std::getline(source, line)) {
            const std::string clean = TrimLine(line);
            if (clean.starts_with("[") && clean.ends_with("]")) {
                inController = clean == "[Controller]";
                continue;
            }
            if (!inController || clean.starts_with("#") || clean.starts_with(";"))
                continue;
            const auto equals = clean.find('=');
            if (equals == std::string::npos) continue;
            const auto field = TrimLine(clean.substr(0,equals));
            const auto value = TrimLine(clean.substr(equals+1));
            if (field == primaryField) {
                result.primary = number(value);
                sawPrimary = true;
            } else if (field == heldField) {
                result.held = number(value);
                sawHeld = true;
            }
        }
        if (!source.eof() && source.fail()) return {};
        result.present = sawPrimary && sawHeld;
        if (result.primary && result.primary == result.held)
            result.primary = result.held = 0;
        if (!result.primary) result.held = 0;
        return result;
    }

    HubControllerSettings ReadHubControllerSettings()
    {
        HubControllerSettings result;
        if (!HubInstalled()) return result;
        result.installed = true;

        // New child-owned ModSettingInt values take priority, including a
        // deliberate Unbound (0). If absent, use the old experimental parent
        // values without modifying them. Legacy standalone INI stays a final
        // fallback when neither MCM user setting has been assigned.
        auto own = ReadIniBinding(kHubUserFile,
            "iQuickAttachPadPrimary", "iQuickAttachPadModifier");
        if (!own.present)
            own = ReadIniBinding(kOldHubUserFile,
                "iQuickAttachPadPrimary", "iQuickAttachPadModifier");
        result.quickConfigured = own.present;
        result.quickPrimary = own.primary;
        result.quickHeld = own.held;

        // Minimap currently has experimental parent settings; it will
        // eventually own a separate child INI. Support both locations so
        // obvious simultaneous controller chord collisions fail closed.
        auto mini = ReadIniBinding(kMinimapUserFile,
            "iMinimapPadPrimary", "iMinimapPadModifier");
        if (!mini.present)
            mini = ReadIniBinding(kOldHubUserFile,
                "iMinimapPadPrimary", "iMinimapPadModifier");
        result.miniPrimary = mini.primary;
        result.miniHeld = mini.held;
        return result;
    }

    void RefreshHubController()
    {
        const auto settings = ReadHubControllerSettings();
        const bool active = settings.installed && settings.quickConfigured;
        const auto primary = active ? HubMask(settings.quickPrimary) : 0u;
        const auto held = active ? HubMask(settings.quickHeld) : 0u;
        if (g_hubControllerEnabled.exchange(active) != active ||
            g_hubQuickPrimary.exchange(primary) != primary ||
            g_hubQuickHeld.exchange(held) != held)
            g_requireControllerRelease = true;
        g_hubMiniPrimary = HubMask(settings.miniPrimary);
        g_hubMiniHeld = HubMask(settings.miniHeld);
    }

    bool WriteHubController(unsigned primary, unsigned held)
    {
        if (!HubInstalled() || primary > 16 || held > 16 ||
            (!primary && held) || (primary && primary == held)) return false;
        std::error_code error;
        std::string data;
        if (std::filesystem::is_regular_file(kHubUserFile,error) && !error) {
            const auto size = std::filesystem::file_size(kHubUserFile,error);
            if (error || size > kHubIniMaxBytes) return false;
            std::ifstream input(kHubUserFile,std::ios::binary);
            if (!input) return false;
            std::ostringstream buffer;
            buffer << input.rdbuf();
            if (!input.good() && !input.eof()) return false;
            data = buffer.str();
        }
        std::istringstream lines(data);
        std::string rewritten;
        std::string line;
        bool inController=false, foundSection=false;
        bool wrotePrimary=false,wroteHeld=false;
        const auto writeMissing = [&]() {
            if (!wrotePrimary) rewritten += "iQuickAttachPadPrimary=" + std::to_string(primary) + "\r\n";
            if (!wroteHeld) rewritten += "iQuickAttachPadModifier=" + std::to_string(held) + "\r\n";
        };
        while (std::getline(lines,line)) {
            const auto clean=TrimLine(line);
            if (clean.starts_with("[") && clean.ends_with("]")) {
                if (inController) writeMissing();
                inController=clean=="[Controller]";
                if (inController) foundSection=true;
            }
            if (inController && clean.starts_with("iQuickAttachPadPrimary=")) {
                rewritten += "iQuickAttachPadPrimary="+std::to_string(primary)+"\r\n";
                wrotePrimary=true;
            } else if (inController && clean.starts_with("iQuickAttachPadModifier=")) {
                rewritten += "iQuickAttachPadModifier="+std::to_string(held)+"\r\n";
                wroteHeld=true;
            } else rewritten += line+"\n";
        }
        if (inController) writeMissing();
        if (!foundSection) {
            rewritten += "[Controller]\r\n";
            writeMissing();
        }
        if (rewritten.size() > kHubIniMaxBytes) return false;
        std::filesystem::create_directories(kHubUserFile.parent_path(),error);
        if (error) return false;
        auto temp=kHubUserFile;
        temp += ".k2040-quick.tmp";
        {
            std::ofstream output(temp,std::ios::binary|std::ios::trunc);
            if (!output) return false;
            output << rewritten;
            output.flush();
            if (!output.good()) return false;
        }
        if (!MoveFileExW(temp.c_str(),kHubUserFile.c_str(),
             MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
            std::filesystem::remove(temp,error);
            return false;
        }
        RefreshHubController();
        return true;
    }

    std::string HubControllerName()
    {
        if (!g_hubControllerEnabled) return g_quickBinding;
        const auto settings=ReadHubControllerSettings();
        if (!settings.quickPrimary) return "None";
        std::string name(kHubButtonNames[settings.quickPrimary]);
        if (settings.quickHeld)
            name=std::string(kHubButtonNames[settings.quickHeld])+"+"+name;
        return name;
    }


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

        // Copy an existing experimental Hub or standalone assignment once
        // into this mod's own user INI. Never touch the old files and never
        // overwrite a pre-existing child INI, including an explicit Unbound.
        std::error_code migrationError;
        if (HubInstalled() &&
            !std::filesystem::exists(kHubUserFile, migrationError) && !migrationError) {
            const auto old = ReadIniBinding(kOldHubUserFile,
                "iQuickAttachPadPrimary", "iQuickAttachPadModifier");
            if (old.present) {
                if (!WriteHubController(old.primary, old.held))
                    log::Warn("Could not copy the experimental MCM shortcut into the mod-owned settings.");
            } else if (g_quickMask.load() != 0) {
                const auto sep = quick.find('+');
                const std::string_view value(quick);
                const auto primaryName = sep == std::string::npos
                    ? value : value.substr(sep + 1);
                const auto heldName = sep == std::string::npos
                    ? std::string_view("None") : value.substr(0, sep);
                const auto primary = ParseHubButton(primaryName);
                const auto held = ParseHubButton(heldName);
                if (primary && held && !WriteHubController(*primary, *held))
                    log::Warn("Could not copy the existing controller shortcut into the mod-owned settings.");
            }
        }
        RefreshHubController();
    }

    std::string GetControllerShortcut(std::string_view action)
    {
        if (action == "quick") return HubControllerName();
        return "None";
    }

    bool SetControllerShortcut(std::string_view action, std::string_view binding)
    {
        const auto mask = ParseBinding(binding);
        if (action != "quick" || !mask) return false;
        if (HubInstalled()) {
            const auto sep=binding.find('+');
            const auto main = sep == std::string_view::npos
                ? binding : binding.substr(sep+1);
            const auto heldName = sep == std::string_view::npos
                ? std::string_view("None") : binding.substr(0,sep);
            const auto primary = ParseHubButton(main);
            const auto held = ParseHubButton(heldName);
            return primary && held && WriteHubController(*primary,*held);
        }
        if (!PersistBindings(binding)) return false;
        g_quickBinding = binding;
        g_quickMask = *mask;
        g_requireControllerRelease = true;
        return true;
    }

    bool ResetControllerShortcuts()
    {
        if (HubInstalled() && !WriteHubController(0,0)) return false;
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
        static auto nextHubRefresh = std::chrono::steady_clock::time_point{};
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextHubRefresh) {
            RefreshHubController();
            nextHubRefresh = now + std::chrono::milliseconds(750);
        }
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

        if (g_hubControllerEnabled) {
            const auto primary = g_hubQuickPrimary.load();
            const auto held = g_hubQuickHeld.load();
            const auto mini = g_hubMiniPrimary.load();
            const auto miniHeld = g_hubMiniHeld.load();
            // Fail closed on the Minimap's conflicting controller edge.
            // This prevents duplicate Quick Attach dispatch, but full
            // cross-mod arbitration still needs a shared runtime adapter.
            quick = hub_chord::QuickShouldFire(
                primary, held, mini, miniHeld, before, down);
            return;
        }
        const auto quickMask = g_quickMask.load();
        quick = quickMask && (down & quickMask) == quickMask &&
            (before & quickMask) != quickMask;
    }
}
