#include "Hotkey.h"
#include "ControllerShortcuts.h"

#include <F4SE/F4SE.h>
#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "Logger.h"
#include "PrismaBridge.h"
#include "RuntimeState.h"
#include "Settings.h"
#include "UserSettings.h"

namespace
{
    k2040::HotkeyState g_openMenuHotkey;
    k2040::HotkeyState g_openMenuBuilderHotkey;

    std::atomic_bool g_wasOpenPressedLastPoll = false;
    std::atomic_bool g_wasBuilderPressedLastPoll = false;
    std::atomic_bool g_wasEscapePressedLastPoll = false;
    std::atomic_bool g_hotkeyCaptureActive = false;
    std::atomic_bool g_hotkeyCaptureReleasePending = false;
    std::atomic_bool g_menuHotkeyUiForwardingActive = false;
    std::atomic_bool g_menuHotkeyReleasePending = false;
    std::atomic<std::uint64_t> g_lastQuickActionTick = 0;
    std::atomic<std::uint64_t> g_lastBuilderActionTick = 0;
    std::atomic<std::uint8_t> g_lastQuickActionSource = 0;
    std::atomic<std::uint8_t> g_lastBuilderActionSource = 0;

    constexpr auto kMcmRefreshInterval = std::chrono::milliseconds(750);
    constexpr std::string_view kMcmModName = "K2040_Quick_Attach_Menu";
    constexpr std::string_view kOpenMenuKeybindId = "openQuickMenu";
    constexpr std::string_view kOpenMenuBuilderKeybindId = "openMenuBuilder";
    const std::filesystem::path kMcmKeybindRegistryPath =
        std::filesystem::path("Data") / "MCM" / "Settings" / "Keybinds.json";

    std::chrono::steady_clock::time_point g_nextMcmRefresh{};
    std::optional<std::filesystem::file_time_type> g_loadedMcmWriteTime;
    bool g_loadedMcmRegistryExists = false;

    struct McmHotkeyOverrides
    {
        std::optional<k2040::HotkeyState> openMenu;
        std::optional<k2040::HotkeyState> openMenuBuilder;
    };

    std::optional<k2040::HotkeyState> ParseMcmHotkey(
        std::uint32_t keycode,
        std::uint32_t modifiers);
    void RefreshMcmHotkeys(bool force);

    std::string NormalizeToken(std::string token)
    {
        token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
        }), token.end());

        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

        return token;
    }

    std::optional<std::uint32_t> ParseUnsigned(std::string_view text)
    {
        std::uint32_t value = 0;
        const auto* first = text.data();
        const auto* last = first + text.size();
        const auto result = std::from_chars(first, last, value);

        if (result.ec != std::errc{} || result.ptr != last) {
            return std::nullopt;
        }

        return value;
    }

    std::optional<std::size_t> FindJsonMemberValueStart(
        std::string_view object,
        std::string_view member)
    {
        const std::string needle = std::string("\"") + std::string(member) + "\"";
        std::size_t searchFrom = 0;

        while (true) {
            const auto memberStart = object.find(needle, searchFrom);
            if (memberStart == std::string_view::npos) {
                return std::nullopt;
            }

            auto cursor = memberStart + needle.size();
            while (cursor < object.size() && std::isspace(static_cast<unsigned char>(object[cursor])) != 0) {
                ++cursor;
            }

            if (cursor < object.size() && object[cursor] == ':') {
                ++cursor;
                while (cursor < object.size() && std::isspace(static_cast<unsigned char>(object[cursor])) != 0) {
                    ++cursor;
                }
                return cursor;
            }

            searchFrom = memberStart + needle.size();
        }
    }

    std::optional<std::string> ReadJsonStringMember(
        std::string_view object,
        std::string_view member)
    {
        const auto valueStart = FindJsonMemberValueStart(object, member);
        if (!valueStart || *valueStart >= object.size() || object[*valueStart] != '"') {
            return std::nullopt;
        }

        std::string value;
        bool escaped = false;

        for (auto cursor = *valueStart + 1; cursor < object.size(); ++cursor) {
            const char c = object[cursor];
            if (escaped) {
                switch (c) {
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    case '/': value.push_back('/'); break;
                    case 'b': value.push_back('\b'); break;
                    case 'f': value.push_back('\f'); break;
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    default: return std::nullopt;
                }
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                return value;
            } else {
                value.push_back(c);
            }
        }

        return std::nullopt;
    }

    std::optional<std::uint32_t> ReadJsonUnsignedMember(
        std::string_view object,
        std::string_view member)
    {
        const auto valueStart = FindJsonMemberValueStart(object, member);
        if (!valueStart || *valueStart >= object.size()) {
            return std::nullopt;
        }

        auto valueEnd = *valueStart;
        while (valueEnd < object.size() && std::isdigit(static_cast<unsigned char>(object[valueEnd])) != 0) {
            ++valueEnd;
        }

        if (valueEnd == *valueStart) {
            return std::nullopt;
        }

        return ParseUnsigned(object.substr(*valueStart, valueEnd - *valueStart));
    }

    struct JsonObjectRange
    {
        std::size_t start = 0;
        std::size_t end = 0;
    };

    std::optional<JsonObjectRange> FindMcmKeybindObject(std::string_view json, std::string_view id)
    {
        const auto keybindsStart = FindJsonMemberValueStart(json, "keybinds");
        if (!keybindsStart || *keybindsStart >= json.size() || json[*keybindsStart] != '[') return std::nullopt;

        bool inString = false;
        bool escaped = false;
        std::size_t objectStart = std::string_view::npos;
        std::uint32_t objectDepth = 0;
        for (auto cursor = *keybindsStart + 1; cursor < json.size(); ++cursor) {
            const char c = json[cursor];
            if (inString) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') inString = false;
                continue;
            }
            if (c == '"') {
                inString = true;
            } else if (c == '{') {
                if (objectDepth == 0) objectStart = cursor;
                ++objectDepth;
            } else if (c == '}') {
                if (objectDepth == 0) return std::nullopt;
                --objectDepth;
                if (objectDepth == 0 && objectStart != std::string_view::npos) {
                    const auto object = json.substr(objectStart, cursor - objectStart + 1);
                    const auto modName = ReadJsonStringMember(object, "modName");
                    const auto objectId = ReadJsonStringMember(object, "id");
                    if (modName && objectId && *modName == kMcmModName && *objectId == id) {
                        return JsonObjectRange{ objectStart, cursor + 1 };
                    }
                    objectStart = std::string_view::npos;
                }
            } else if (c == ']' && objectDepth == 0) {
                break;
            }
        }
        return std::nullopt;
    }

    bool ReplaceJsonUnsignedMember(std::string& object, std::string_view member, std::uint32_t value)
    {
        const auto start = FindJsonMemberValueStart(object, member);
        if (!start || *start >= object.size()) return false;
        auto end = *start;
        while (end < object.size() && std::isdigit(static_cast<unsigned char>(object[end])) != 0) ++end;
        if (end == *start) return false;
        object.replace(*start, end - *start, std::to_string(value));
        return true;
    }

    std::string KeycodeName(std::uint32_t keycode)
    {
        if (keycode == 258) return "Mouse 3";
        if (keycode == 259) return "Mouse 4";
        if (keycode == 260) return "Mouse 5";
        if ((keycode >= 'A' && keycode <= 'Z') || (keycode >= '0' && keycode <= '9')) {
            return std::string(1, static_cast<char>(keycode));
        }
        if (keycode >= VK_F1 && keycode <= VK_F24) return "F" + std::to_string(keycode - VK_F1 + 1);
        switch (keycode) {
        case VK_TAB: return "Tab";
        case VK_RETURN: return "Enter";
        case VK_SPACE: return "Space";
        case VK_BACK: return "Backspace";
        case VK_LEFT: return "Left";
        case VK_RIGHT: return "Right";
        case VK_UP: return "Up";
        case VK_DOWN: return "Down";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        default: return "Key " + std::to_string(keycode);
        }
    }

    std::string BindingDisplayName(std::uint32_t keycode, std::uint32_t modifiers)
    {
        std::string result;
        if ((modifiers & 0x2u) != 0) result += "Ctrl+";
        if ((modifiers & 0x1u) != 0) result += "Shift+";
        if ((modifiers & 0x4u) != 0) result += "Alt+";
        result += KeycodeName(keycode);
        return result;
    }

    std::optional<k2040::SharedHotkeyBinding> ReadSharedHotkeyBinding(std::string_view id)
    {
        std::ifstream file(kMcmKeybindRegistryPath, std::ios::binary);
        if (!file.is_open()) return std::nullopt;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        if (!file.good() && !file.eof()) return std::nullopt;
        const auto json = buffer.str();
        const auto range = FindMcmKeybindObject(json, id);
        if (!range) return std::nullopt;
        const std::string_view object(json.data() + range->start, range->end - range->start);
        const auto keycode = ReadJsonUnsignedMember(object, "keycode");
        const auto modifiers = ReadJsonUnsignedMember(object, "modifiers");
        if (!keycode || !modifiers || !ParseMcmHotkey(*keycode, *modifiers)) return std::nullopt;
        return k2040::SharedHotkeyBinding{
            *keycode,
            *modifiers,
            BindingDisplayName(*keycode, *modifiers),
            true
        };
    }

    struct PendingSharedBinding
    {
        std::string_view id;
        std::uint32_t keycode = 0;
        std::uint32_t modifiers = 0;
    };

    bool WriteSharedHotkeyBindings(const std::vector<PendingSharedBinding>& bindings)
    {
        std::ifstream file(kMcmKeybindRegistryPath, std::ios::binary);
        if (!file.is_open()) {
            k2040::log::Warn("Shared MCM keybinding registry could not be opened for update.");
            return false;
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        if (!file.good() && !file.eof()) {
            k2040::log::Warn("Shared MCM keybinding registry could not be read completely.");
            return false;
        }

        std::string json = buffer.str();
        for (const auto& binding : bindings) {
            if (!ParseMcmHotkey(binding.keycode, binding.modifiers)) return false;
            const auto range = FindMcmKeybindObject(json, binding.id);
            if (!range) {
                k2040::log::Warn(std::string("Shared MCM keybinding entry is missing: ") + std::string(binding.id));
                return false;
            }
            std::string object = json.substr(range->start, range->end - range->start);
            if (!ReplaceJsonUnsignedMember(object, "keycode", binding.keycode) ||
                !ReplaceJsonUnsignedMember(object, "modifiers", binding.modifiers)) {
                k2040::log::Warn(std::string("Shared MCM keybinding entry is malformed: ") + std::string(binding.id));
                return false;
            }
            json.replace(range->start, range->end - range->start, object);
        }

        auto temp = kMcmKeybindRegistryPath;
        temp += ".k2040.tmp";
        {
            std::ofstream output(temp, std::ios::binary | std::ios::trunc);
            if (!output.is_open()) return false;
            output << json;
            output.flush();
            if (!output.good()) return false;
        }

        const auto tempNative = temp.native();
        const auto pathNative = kMcmKeybindRegistryPath.native();
        if (!MoveFileExW(tempNative.c_str(), pathNative.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::error_code ec;
            std::filesystem::remove(temp, ec);
            k2040::log::Warn("Shared MCM keybinding registry replacement failed.");
            return false;
        }

        g_loadedMcmWriteTime.reset();
        g_loadedMcmRegistryExists = false;
        RefreshMcmHotkeys(true);
        return true;
    }

    std::optional<k2040::HotkeyState> ParseMcmHotkey(
        std::uint32_t keycode,
        std::uint32_t modifiers)
    {
        if ((modifiers & ~0x7u) != 0) {
            return std::nullopt;
        }

        std::uint32_t virtualKey = keycode;
        switch (keycode) {
            case 258: virtualKey = VK_MBUTTON; break;
            case 259: virtualKey = VK_XBUTTON1; break;
            case 260: virtualKey = VK_XBUTTON2; break;
            default:
                if (keycode > 255) {
                    return std::nullopt;
                }
                break;
        }

        if (virtualKey == 0) {
            return std::nullopt;
        }

        k2040::HotkeyState state;
        state.virtualKey = virtualKey;
        state.shiftRequired = (modifiers & 0x1u) != 0;
        state.ctrlRequired = (modifiers & 0x2u) != 0;
        state.altRequired = (modifiers & 0x4u) != 0;
        return state;
    }

    bool ApplyMcmKeybindObject(std::string_view object, McmHotkeyOverrides& overrides)
    {
        const auto modName = ReadJsonStringMember(object, "modName");
        const auto keybindId = ReadJsonStringMember(object, "id");
        if (!modName || !keybindId || *modName != kMcmModName) {
            return true;
        }

        const auto keycode = ReadJsonUnsignedMember(object, "keycode");
        const auto modifiers = ReadJsonUnsignedMember(object, "modifiers");
        if (!keycode || !modifiers) {
            k2040::log::Warn(std::string("Ignoring malformed native MCM keybinding for ") + *keybindId + ".");
            return true;
        }

        const auto hotkey = ParseMcmHotkey(*keycode, *modifiers);
        if (!hotkey) {
            k2040::log::Warn(std::string("Ignoring unsupported native MCM keybinding for ") + *keybindId + ".");
            return true;
        }

        if (*keybindId == kOpenMenuKeybindId) {
            overrides.openMenu = hotkey;
        } else if (*keybindId == kOpenMenuBuilderKeybindId) {
            overrides.openMenuBuilder = hotkey;
        }

        return true;
    }

    bool ParseMcmKeybindRegistry(std::string_view json, McmHotkeyOverrides& overrides)
    {
        const auto keybindsStart = FindJsonMemberValueStart(json, "keybinds");
        if (!keybindsStart) {
            // MCM writes {"version":1} until its first keybind is registered.
            return true;
        }

        if (*keybindsStart >= json.size() || json[*keybindsStart] != '[') {
            return false;
        }

        bool inString = false;
        bool escaped = false;
        std::size_t objectStart = std::string_view::npos;
        std::uint32_t objectDepth = 0;

        for (auto cursor = *keybindsStart + 1; cursor < json.size(); ++cursor) {
            const char c = json[cursor];

            if (inString) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    inString = false;
                }
                continue;
            }

            if (c == '"') {
                inString = true;
                continue;
            }

            if (c == '{') {
                if (objectDepth == 0) {
                    objectStart = cursor;
                }
                ++objectDepth;
            } else if (c == '}') {
                if (objectDepth == 0) {
                    return false;
                }
                --objectDepth;
                if (objectDepth == 0) {
                    if (!ApplyMcmKeybindObject(
                            json.substr(objectStart, cursor - objectStart + 1),
                            overrides)) {
                        return false;
                    }
                    objectStart = std::string_view::npos;
                }
            } else if (c == ']' && objectDepth == 0) {
                return true;
            }
        }

        return false;
    }

    bool LoadMcmHotkeyOverrides(McmHotkeyOverrides& overrides)
    {
        std::ifstream file(kMcmKeybindRegistryPath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return file.good() || file.eof()
            ? ParseMcmKeybindRegistry(buffer.str(), overrides)
            : false;
    }

    void ResetHotkeyEdgeState()
    {
        g_wasOpenPressedLastPoll.store(false);
        g_wasBuilderPressedLastPoll.store(false);
        g_wasEscapePressedLastPoll.store(false);
    }

    void ApplyConfiguredHotkeys(const McmHotkeyOverrides* overrides)
    {
        g_openMenuHotkey = k2040::ParseHotkey(k2040::GetSettings().openMenuHotkey);
        g_openMenuBuilderHotkey = k2040::ParseHotkey(k2040::GetSettings().openMenuBuilderHotkey);

        if (overrides) {
            if (overrides->openMenu) {
                g_openMenuHotkey = *overrides->openMenu;
            }
            if (overrides->openMenuBuilder) {
                g_openMenuBuilderHotkey = *overrides->openMenuBuilder;
            }
        }

        ResetHotkeyEdgeState();
    }

    void RefreshMcmHotkeys(bool force)
    {
        const auto now = std::chrono::steady_clock::now();
        if (!force && now < g_nextMcmRefresh) {
            return;
        }
        g_nextMcmRefresh = now + kMcmRefreshInterval;

        std::error_code ec;
        const bool exists = std::filesystem::exists(kMcmKeybindRegistryPath, ec);
        if (ec) {
            return;
        }

        if (!exists) {
            if (force || g_loadedMcmRegistryExists) {
                ApplyConfiguredHotkeys(nullptr);
                g_loadedMcmRegistryExists = false;
                g_loadedMcmWriteTime.reset();
                if (!force) {
                    k2040::log::Info("Native MCM keybinding registry removed; restored F4SE plugin INI hotkeys.");
                }
            }
            return;
        }

        const auto writeTime = std::filesystem::last_write_time(kMcmKeybindRegistryPath, ec);
        if (ec) {
            return;
        }

        if (!force && g_loadedMcmRegistryExists && g_loadedMcmWriteTime && *g_loadedMcmWriteTime == writeTime) {
            return;
        }

        McmHotkeyOverrides overrides;
        if (!LoadMcmHotkeyOverrides(overrides)) {
            return;
        }

        ApplyConfiguredHotkeys(&overrides);
        g_loadedMcmRegistryExists = true;
        g_loadedMcmWriteTime = writeTime;
        k2040::log::Info("Native MCM keybinding overrides loaded.");
    }

    uint32_t KeyNameToVK(const std::string& key)
    {
        if (key.size() == 1) {
            const char c = key[0];

            if (c >= 'A' && c <= 'Z') {
                return static_cast<uint32_t>(c);
            }

            if (c >= '0' && c <= '9') {
                return static_cast<uint32_t>(c);
            }
        }

        if (key == "F1") return VK_F1;
        if (key == "F2") return VK_F2;
        if (key == "F3") return VK_F3;
        if (key == "F4") return VK_F4;
        if (key == "F5") return VK_F5;
        if (key == "F6") return VK_F6;
        if (key == "F7") return VK_F7;
        if (key == "F8") return VK_F8;
        if (key == "F9") return VK_F9;
        if (key == "F10") return VK_F10;
        if (key == "F11") return VK_F11;
        if (key == "F12") return VK_F12;

        return 0;
    }

    bool IsPhysicalKeyDown(uint32_t virtualKey)
    {
        if (virtualKey == 0) {
            return false;
        }

        return (GetAsyncKeyState(static_cast<int>(virtualKey)) & 0x8000) != 0;
    }

    enum class HotkeyAction
    {
        ToggleQuickMenu,
        ToggleBuilderMenu,
        CloseFocusedMenu
    };

    enum class HotkeyActionSource : std::uint8_t
    {
        NativePoller = 1,
        FocusedView = 2
    };

    bool OpenRequestedMenu(k2040::PrismaBridge& prisma, bool openBuilder)
    {
        k2040::log::Info(openBuilder ? "Menu-builder hotkey pressed." : "Quick-menu hotkey pressed.");

        if (!prisma.BeginOpenFromHotkey()) {
            k2040::log::Info("Open-menu hotkey ignored because another menu is active or gameplay isolation could not be established.");
            return false;
        }

        const auto weaponInfo = k2040::GetEquippedWeaponInfo();
        const bool cheatMode = !openBuilder && k2040::GetQuickMenuPreferences().cheatMode;
        auto menu = k2040::BuildEcoWeaponMenu_ReadOnly(weaponInfo,
            openBuilder || cheatMode, cheatMode);

        if (menu.valid) {
            k2040::RegisterOrUpdateWeapon(menu);
            const auto& userSettings = k2040::GetUserSettingsDiagnostics();
            if (userSettings.dirty && userSettings.saveAllowed) {
                k2040::SaveUserPreferences();
            }
        }

        k2040::ApplyVisibilityPreferences(menu);

        if (openBuilder) {
            prisma.OpenMenuBuilder(weaponInfo, menu);
        } else {
            prisma.OpenMenu(weaponInfo, menu);
        }

        return true;
    }

    void HandleHotkeyActionOnGameThread(HotkeyAction action)
    {
        auto& prisma = k2040::GetPrismaBridge();

        if (action == HotkeyAction::CloseFocusedMenu) {
            if (prisma.IsMenuFocused()) {
                k2040::log::Info("Escape released while the quick menu is focused; closing on the game thread.");
                prisma.CloseMenu();
            }
            return;
        }

        const bool targetMenuIsBuilder = action == HotkeyAction::ToggleBuilderMenu;

        if (!prisma.IsMenuFocused()) {
            OpenRequestedMenu(prisma, targetMenuIsBuilder);
            return;
        }

        const bool currentMenuIsBuilder = prisma.IsMenuBuilderOpen();
        if (targetMenuIsBuilder != currentMenuIsBuilder) {
            k2040::log::Info(targetMenuIsBuilder
                ? "Builder hotkey requested while the quick menu is open; switching on the game thread."
                : "Quick-menu hotkey requested while the builder is open; switching on the game thread.");

            prisma.CloseMenuForSwitch();
            if (!OpenRequestedMenu(prisma, targetMenuIsBuilder)) {
                k2040::log::Warn("Menu switch could not open the requested menu; completing a normal close.");
                prisma.CloseMenu();
            }
            return;
        }

        k2040::log::Info(currentMenuIsBuilder
            ? "Builder hotkey requested while the builder is open; closing on the game thread."
            : "Quick-menu hotkey requested while the quick menu is open; closing on the game thread.");
        prisma.CloseMenu();
    }

    void QueueHotkeyAction(HotkeyAction action, HotkeyActionSource source)
    {
        if (action == HotkeyAction::ToggleQuickMenu || action == HotkeyAction::ToggleBuilderMenu) {
            auto& lastTick = action == HotkeyAction::ToggleQuickMenu
                ? g_lastQuickActionTick
                : g_lastBuilderActionTick;
            auto& lastSource = action == HotkeyAction::ToggleQuickMenu
                ? g_lastQuickActionSource
                : g_lastBuilderActionSource;

            const std::uint64_t now = GetTickCount64();
            const std::uint8_t sourceValue = static_cast<std::uint8_t>(source);
            const std::uint8_t previousSource = lastSource.exchange(sourceValue);
            const std::uint64_t previousTick = lastTick.exchange(now);

            // The same physical press can be seen by both the native poller and
            // the focused browser. Suppress only cross-source duplicates; two
            // deliberate presses from the same source remain valid.
            if (previousSource != 0 &&
                previousSource != sourceValue &&
                previousTick != 0 &&
                now >= previousTick &&
                now - previousTick < 750) {
                k2040::log::Info("Duplicate cross-source opener hotkey signal ignored.");
                return;
            }
        }

        const auto* taskInterface = F4SE::GetTaskInterface();
        if (!taskInterface) {
            k2040::log::Warn("Hotkey action ignored because the F4SE game-thread task interface is unavailable.");
            return;
        }

        taskInterface->AddTask([action]() {
            HandleHotkeyActionOnGameThread(action);
        });
    }
}

namespace k2040
{
    HotkeyState ParseHotkey(const std::string& text)
    {
        HotkeyState state;

        std::stringstream ss(text);
        std::string token;

        while (std::getline(ss, token, '+')) {
            token = NormalizeToken(token);

            if (token == "ALT") {
                state.altRequired = true;
            } else if (token == "CTRL" || token == "CONTROL") {
                state.ctrlRequired = true;
            } else if (token == "SHIFT") {
                state.shiftRequired = true;
            } else {
                state.virtualKey = KeyNameToVK(token);
            }
        }

        return state;
    }

    bool IsHotkeyPressedNow(const HotkeyState& hotkey)
    {
        if (hotkey.virtualKey == 0) {
            return false;
        }

        const bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const SHORT keyState = GetAsyncKeyState(static_cast<int>(hotkey.virtualKey));
        const bool key = (keyState & 0x8000) != 0;

        // Treat only the high-order bit as the current physical state. The
        // low-order "pressed since last query" bit is not a held-state signal
        // and can prevent a clean release edge under Wine/Proton.
        // Require an exact modifier chord. This prevents Ctrl+Shift+K from
        // also matching the ordinary Shift+K quick-menu binding.
        if (hotkey.altRequired != alt) return false;
        if (hotkey.ctrlRequired != ctrl) return false;
        if (hotkey.shiftRequired != shift) return false;

        return key;
    }

    SharedHotkeyBinding GetSharedHotkeyBinding(const std::string& id)
    {
        if (const auto shared = ReadSharedHotkeyBinding(id)) return *shared;

        const auto& state = id == kOpenMenuBuilderKeybindId
            ? g_openMenuBuilderHotkey
            : g_openMenuHotkey;
        std::uint32_t keycode = state.virtualKey;
        if (keycode == VK_MBUTTON) keycode = 258;
        else if (keycode == VK_XBUTTON1) keycode = 259;
        else if (keycode == VK_XBUTTON2) keycode = 260;
        const std::uint32_t modifiers =
            (state.shiftRequired ? 0x1u : 0u) |
            (state.ctrlRequired ? 0x2u : 0u) |
            (state.altRequired ? 0x4u : 0u);
        return { keycode, modifiers, BindingDisplayName(keycode, modifiers), false };
    }

    bool SetSharedHotkeyBinding(const std::string& id, std::uint32_t keycode, std::uint32_t modifiers)
    {
        if (id != kOpenMenuKeybindId && id != kOpenMenuBuilderKeybindId) return false;
        if (!WriteSharedHotkeyBindings({ PendingSharedBinding{ id, keycode, modifiers } })) return false;
        log::Info(std::string("Shared MCM keybinding updated: ") + id + ".");
        return true;
    }

    bool ResetSharedHotkeyBindings()
    {
        const bool saved = WriteSharedHotkeyBindings({
            PendingSharedBinding{ kOpenMenuKeybindId, static_cast<std::uint32_t>('K'), 0x1u },
            PendingSharedBinding{ kOpenMenuBuilderKeybindId, static_cast<std::uint32_t>('K'), 0x3u }
        });
        if (saved) log::Info("Shared MCM keybindings restored to fresh-install defaults.");
        return saved;
    }

    void SetHotkeyCaptureActive(bool active)
    {
        const bool wasActive = g_hotkeyCaptureActive.exchange(active);
        if (active == wasActive) {
            return;
        }

        g_hotkeyCaptureReleasePending = !active;
        ResetHotkeyEdgeState();
    }

    bool IsHotkeyCaptureActive()
    {
        return g_hotkeyCaptureActive.load();
    }

    void SetMenuHotkeyUiForwardingActive(bool active)
    {
        const bool wasActive = g_menuHotkeyUiForwardingActive.exchange(active);
        if (active == wasActive) {
            return;
        }

        if (active) {
            g_menuHotkeyReleasePending = false;
        } else {
            g_menuHotkeyReleasePending = true;
        }
    }

    void QueueMenuHotkeyActionFromUi(bool openBuilder)
    {
        log::Info(openBuilder
            ? "Focused Prisma view forwarded the menu-builder hotkey."
            : "Focused Prisma view forwarded the quick-menu hotkey.");
        QueueHotkeyAction(
            openBuilder ? HotkeyAction::ToggleBuilderMenu : HotkeyAction::ToggleQuickMenu,
            HotkeyActionSource::FocusedView);
    }

    void InitializeHotkey()
    {
        ApplyConfiguredHotkeys(nullptr);
        RefreshMcmHotkeys(true);
        InitializeControllerShortcuts();

        log::Info("Quick-menu toggle hotkey initialized. Default: Shift+K.");
        log::Info("Menu-builder toggle hotkey initialized. Default: Ctrl+Shift+K.");
        log::Info("Either opener switches to its menu while the other menu is active; Escape closes without switching.");
    }

    void PollHotkey()
    {
        RefreshMcmHotkeys(false);

        const bool openPressedNow = IsHotkeyPressedNow(g_openMenuHotkey);
        const bool builderPressedNow = IsHotkeyPressedNow(g_openMenuBuilderHotkey);
        // Independent XInput shortcuts use the same game-thread action queue
        // as keyboard and mouse. They do not read/write MCM key assignments.
        bool controllerQuickEdge = false;
        PollControllerShortcutEdges(
            g_hotkeyCaptureActive.load() || g_hotkeyCaptureReleasePending.load() ||
                g_menuHotkeyReleasePending.load(),
            controllerQuickEdge);
        // Angle samples are enqueued only while Quick Menu is active. Reading
        // XInput on this background thread is safe; invoking PrismaUI is not.
        static int previousStickSector = -1;
        static auto previousStickDispatch = std::chrono::steady_clock::time_point{};
        if (GetPrismaBridge().IsControllerInputActive() &&
            !g_hotkeyCaptureActive.load()) {
            const int sector = ReadControllerStickSector();
            const auto now = std::chrono::steady_clock::now();
            // Forward neutral exactly once so the browser drops stale angles
            // before an A/Confirm press. Continuous input is navigation only.
            const bool released = sector < 0 && previousStickSector >= 0;
            const bool newAngle = sector >= 0 && sector != previousStickSector &&
                now - previousStickDispatch >= std::chrono::milliseconds(70);
            const bool heldRepeat = sector >= 0 && sector == previousStickSector &&
                now - previousStickDispatch >= std::chrono::milliseconds(190);
            if (released || newAngle || heldRepeat) {
                previousStickSector = sector;
                previousStickDispatch = now;
                if (const auto* tasks = F4SE::GetTaskInterface()) {
                    tasks->AddTask([sector]() {
                        GetPrismaBridge().OnControllerStickSector(sector);
                    });
                }
            }
        } else {
            previousStickSector = -1;
        }
        const bool escapePressedNow = IsPhysicalKeyDown(VK_ESCAPE);
        const bool escapeWasPressed = g_wasEscapePressedLastPoll.exchange(escapePressedNow);
        const bool escapeReleasedEdge = !escapePressedNow && escapeWasPressed;

        if (g_hotkeyCaptureActive.load()) {
            g_wasOpenPressedLastPoll.store(openPressedNow);
            g_wasBuilderPressedLastPoll.store(builderPressedNow);
            return;
        }
        if (g_hotkeyCaptureReleasePending.load()) {
            if (!openPressedNow && !builderPressedNow) {
                g_hotkeyCaptureReleasePending = false;
                g_wasOpenPressedLastPoll.store(false);
                g_wasBuilderPressedLastPoll.store(false);
                log::Info("Hotkey capture completed after the assigned key was released.");
            }
            return;
        }

        // Browser forwarding is supplemental while Prisma has focus. The native
        // poller stays active as a fallback because CEF may not emit DOM events
        // for mouse back/forward buttons under Proton.

        // A browser-forwarded opener may close the menu while its key/button is
        // still physically held. Do not let the native poller immediately turn
        // that same press into a reopen after focus returns to the game.
        if (g_menuHotkeyReleasePending.load()) {
            if (!openPressedNow && !builderPressedNow) {
                g_menuHotkeyReleasePending = false;
                g_wasOpenPressedLastPoll.store(false);
                g_wasBuilderPressedLastPoll.store(false);
                log::Info("Focused-view hotkey forwarding released after opener keys were released.");
            }
            if (escapeReleasedEdge) {
                QueueHotkeyAction(HotkeyAction::CloseFocusedMenu, HotkeyActionSource::NativePoller);
            }
            return;
        }

        const bool openWasPressed = g_wasOpenPressedLastPoll.exchange(openPressedNow);
        const bool builderWasPressed = g_wasBuilderPressedLastPoll.exchange(builderPressedNow);
        const bool openPressedEdge = openPressedNow && !openWasPressed;
        const bool builderPressedEdge = builderPressedNow && !builderWasPressed;

        if (openPressedNow != openWasPressed) {
            log::Info(openPressedNow
                ? "Quick-menu hotkey physical state changed to down."
                : "Quick-menu hotkey physical state changed to up.");
        }
        if (builderPressedNow != builderWasPressed) {
            log::Info(builderPressedNow
                ? "Menu-builder hotkey physical state changed to down."
                : "Menu-builder hotkey physical state changed to up.");
        }

        // The polling thread is deliberately limited to physical-key state.
        // All game/Prisma/menu/input-layer work is queued onto the F4SE game
        // thread because BSInputEnableManager notifications can synchronously
        // drive PlayerControls and the Havok animation graph.
        if (builderPressedEdge) {
            log::Info("Native poller observed the menu-builder hotkey edge.");
            QueueHotkeyAction(HotkeyAction::ToggleBuilderMenu, HotkeyActionSource::NativePoller);
        } else if (openPressedEdge || controllerQuickEdge) {
            log::Info("Native poller observed the quick-menu hotkey edge.");
            QueueHotkeyAction(HotkeyAction::ToggleQuickMenu, HotkeyActionSource::NativePoller);
        }

        if (escapeReleasedEdge) {
            QueueHotkeyAction(HotkeyAction::CloseFocusedMenu, HotkeyActionSource::NativePoller);
        }
    }
}
