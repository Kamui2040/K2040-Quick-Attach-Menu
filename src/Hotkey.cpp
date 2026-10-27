#include "McmKeybindRegistry.h"
#include "NativeMcmBindingRecovery.h"
#include "SettingsKeyCaptureValidation.h"
#include "SettingsNativeMouseCapture.h"
#include "Hotkey.h"
#include "ControllerShortcuts.h"

#include <F4SE/F4SE.h>
#include <RE/U/UI.h>
#include <Windows.h>
#include "McmRegistryPersistence.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <iterator>
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
    std::atomic<std::uint32_t> g_quickOpenerVirtualKey{0};
    std::atomic<std::uint32_t> g_builderOpenerVirtualKey{0};

    std::atomic_bool g_wasOpenPressedLastPoll = false;
    std::atomic_bool g_wasBuilderPressedLastPoll = false;
    std::atomic_bool g_wasEscapePressedLastPoll = false;
    std::atomic_bool g_hotkeyCaptureActive = false;
    std::atomic<std::uint64_t> g_hotkeyCaptureGeneration{0};
    std::atomic_bool g_hotkeyCaptureReleasePending = false;
    std::atomic_bool g_menuHotkeyUiForwardingActive = false;
    std::atomic_bool g_menuHotkeyReleasePending = false;
    std::atomic<std::uint64_t> g_lastQuickActionTick = 0;
    std::atomic<std::uint64_t> g_lastBuilderActionTick = 0;
    // Native MCM caches bindings independently from Keybinds.json. Only the
    // Pause-menu Scaleform bridge can update its live keybind manager.
    struct McmLiveBinding
    {
        std::uint32_t keycode = 0;
        std::uint32_t modifiers = 0;
        std::uint32_t originalKeycode = 0;
        std::uint32_t originalModifiers = 0;
        bool registerMissing = false; // Only an explicit Settings capture can register an absent ID.
    };
    std::mutex g_mcmSyncMutex;
    std::optional<McmLiveBinding> g_mcmQuickPending;
    std::optional<McmLiveBinding> g_mcmBuilderPending;
    std::atomic_bool g_mcmSyncPending = false;
    std::atomic_bool g_mcmSyncTaskQueued = false;
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
        std::string_view object, std::string_view member)
    {
        return k2040::FindMcmObjectMemberValue(object, member);
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

    std::vector<JsonObjectRange> FindMcmKeybindObjects(std::string_view json, std::string_view id)
    {
        const auto keybindsStart = FindJsonMemberValueStart(json, "keybinds");
        if (!keybindsStart) return {};
        const auto array = k2040::ScanMcmKeybindArray(json, *keybindsStart);
        if (!array) return {};

        std::vector<JsonObjectRange> matches;
        for (const auto& [start, end] : array->objects) {
            const auto object = json.substr(start, end - start);
            const auto modName = ReadJsonStringMember(object, "modName");
            const auto objectId = ReadJsonStringMember(object, "id");
            if (modName && objectId && *modName == kMcmModName && *objectId == id)
                matches.push_back(JsonObjectRange{ start, end });
        }
        return matches;
    }

    std::optional<JsonObjectRange> FindMcmKeybindObject(std::string_view json, std::string_view id)
    {
        const auto matches = FindMcmKeybindObjects(json, id);
        // The native registry parser applies later assignments last; use the
        // same entry when displaying/editing a binding before reconciliation.
        if (matches.empty()) return std::nullopt;
        return matches.back();
    }

    std::optional<bool> ReconcileMcmKeybindDuplicates(std::string& registry)
    {
        std::string cleaned = registry;
        bool changed = false;
        for (const auto id : { kOpenMenuKeybindId, kOpenMenuBuilderKeybindId }) {
            const auto matches = FindMcmKeybindObjects(cleaned, id);
            if (matches.size() < 2) continue;
            const auto& retained = matches.back();
            const std::string_view object(cleaned.data() + retained.start,
                                          retained.end - retained.start);
            const auto key = ReadJsonUnsignedMember(object, "keycode");
            const auto modifiers = ReadJsonUnsignedMember(object, "modifiers");
            if (!key || !modifiers || !ParseMcmHotkey(*key, *modifiers)) {
                return std::nullopt;  // Do not discard a possibly recoverable assignment.
            }
            std::vector<std::pair<std::size_t, std::size_t>> ranges;
            ranges.reserve(matches.size());
            for (const auto& item : matches) ranges.emplace_back(item.start, item.end);
            if (!k2040::RemoveEarlierMcmKeybindObjects(cleaned, ranges)) return std::nullopt;
            changed = true;
        }
        registry.swap(cleaned);
        return changed;
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
        const auto matches = FindMcmKeybindObjects(json, id);
        // Runtime parsing ignores invalid entries and retains the last valid
        // one. Show the same effective assignment even before deduplication.
        for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
            const std::string_view object(json.data() + it->start, it->end - it->start);
            const auto keycode = ReadJsonUnsignedMember(object, "keycode");
            const auto modifiers = ReadJsonUnsignedMember(object, "modifiers");
            if (!keycode || !modifiers || !ParseMcmHotkey(*keycode, *modifiers)) continue;
            return k2040::SharedHotkeyBinding{
                *keycode,
                *modifiers,
                *keycode ? BindingDisplayName(*keycode, *modifiers) : "Unbound",
                true
            };
        }
        return std::nullopt;
    }

    struct PendingSharedBinding
    {
        std::string_view id;
        std::uint32_t keycode = 0;
        std::uint32_t modifiers = 0;
    };

    void SeedDefaultMcmBindings()
    {
        // Classic MCM's OptionsList.as dereferences GetKeybind(...).keycode
        // without checking for null. A missing registry record leaves its
        // hotkey picker unable to perform its very first assignment.
        // Never reset a saved binding, including a deliberate Unbound (0).
        try {
            const auto config=std::filesystem::path("Data/MCM/Config/K2040_Quick_Attach_Menu/config.json");
            if (!std::filesystem::is_regular_file(config)) return;
            if (!std::filesystem::is_regular_file("Data/F4SE/Plugins/mcm.dll") &&
                !std::filesystem::is_regular_file("Data/F4SE/Plugins/F4SEMenuFramework.dll")) return;
            // Native MCM has its own loaded registry. Never mutate the on-disk
            // file while mcm.dll can later overwrite it on menu close.
            // First native registration is handled through Scaleform below.
            if (GetModuleHandleW(L"mcm.dll")) return;
            const auto marker=std::filesystem::path("Data/MCM/Settings/K2040_Quick_Attach_Menu.hotkeys-initialized");
            const bool initialized=std::filesystem::is_regular_file(marker);
            const bool existing=std::filesystem::exists(kMcmKeybindRegistryPath);
            std::string json;
            if (existing) {
                if (std::filesystem::file_size(kMcmKeybindRegistryPath)>1024*1024) return;
                std::ifstream f(kMcmKeybindRegistryPath,std::ios::binary);
                if (!f) return;
                json.assign(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
                if ((!f.good()&&!f.eof())||json.size()>1024*1024) return;
            } else json="{\"version\":1,\"keybinds\":[]}\n";

            auto keybinds=FindJsonMemberValueStart(json,"keybinds");
            if (!keybinds) {
                const auto end=json.find_last_not_of(" \t\r\n");
                if (end==json.npos || json[end]!='}') return;
                json.insert(end,",\"keybinds\":[]");
                keybinds=FindJsonMemberValueStart(json,"keybinds");
            }
            if (!keybinds || *keybinds>=json.size() || json[*keybinds]!='[') return;
            const auto reconciled = ReconcileMcmKeybindDuplicates(json);
            if (!reconciled) {
                k2040::log::Warn("MCM keybind duplicate repair skipped: registry is not safely editable.");
                return;
            }
            bool changed=*reconciled;
            if (changed) k2040::log::Info("Removed older duplicate Quick Attach hotkeys; kept most recent assignments.");
            for (const auto [id,key,mods]:
                 {std::tuple<std::string_view,std::uint32_t,std::uint32_t>
                     {kOpenMenuKeybindId,'K',1u},
                     {kOpenMenuBuilderKeybindId,'K',3u}}) {
                if (FindMcmKeybindObject(json,id)) continue;
                // Classic MCM stores keybinds by physical chord, not action ID.
                // Inserting a synthetic (0,0) placeholder can displace another
                // cleared action and create duplicate IDs when native MCM later
                // saves its live manager. Preserve missing/cleared registrations.
                if (initialized && GetModuleHandleW(L"mcm.dll")) continue;
                std::uint32_t selected = initialized ? 0 : key;
                // MCM may also register shortcuts from other mods. Avoid
                // claiming a default already assigned in the same registry.
                const auto array = k2040::ScanMcmKeybindArray(json, *keybinds);
                if (!array) return;
                for (const auto& [start, end] : array->objects) {
                    const auto object = std::string_view(json).substr(start, end - start);
                    const auto otherKey = ReadJsonUnsignedMember(object, "keycode");
                    const auto otherMod = ReadJsonUnsignedMember(object, "modifiers");
                    if (otherKey && otherMod && *otherKey == selected && *otherMod == mods)
                        selected = 0;
                }
                const std::string record = std::string("{\"id\":\"") + std::string(id) +
                  "\",\"keycode\":" + std::to_string(selected) +
                  ",\"modName\":\"K2040_Quick_Attach_Menu\",\"modifiers\":" +
                  std::to_string(selected ? mods : 0) + "}";
                json.insert(array->closingBracket,
                    std::string(array->objects.empty() ? "" : ",") + record);
                changed=true;
            }
            if (changed) {
                if(json.size()>1024*1024)return;
                std::error_code ec;
                std::filesystem::create_directories(kMcmKeybindRegistryPath.parent_path(),ec);
                if(ec)return;
                auto temporary=kMcmKeybindRegistryPath;
                temporary+=".k2040-quick-init.tmp";
                {std::ofstream f(temporary,std::ios::binary|std::ios::trunc);
                 if(!f)return;f<<json;f.flush();if(!f)return;}
                if(!MoveFileExW(temporary.c_str(),kMcmKeybindRegistryPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
                    std::filesystem::remove(temporary,ec);return;
                }
            }
            if(!initialized){
                std::error_code ec;
                std::filesystem::create_directories(marker.parent_path(),ec);
                if(ec)return;
                std::ofstream f(marker,std::ios::trunc|std::ios::binary);
                if(f)f<<"initialized=1\n";
            }
        }catch(const std::exception& e){
            k2040::log::Warn(std::string("MCM hotkey default initialization skipped: ")+e.what());
        }catch(...){}
    }

    bool WriteSharedHotkeyBindings(const std::vector<PendingSharedBinding>& bindings)
    {
        // Unbind through classic MCM removes a record entirely. Seed an
        // explicit Unbound record so PrismaUI can reassign immediately.
        SeedDefaultMcmBindings();
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
        const auto saved = k2040::SaveSharedMcmRegistry(tempNative, pathNative, buffer.str(), json);
        if (!saved.saved) {
            std::error_code ec;
            std::filesystem::remove(temp, ec);
            k2040::log::Warn("Shared MCM keybinding registry replacement failed: Windows error " +
                std::to_string(saved.replaceError) + ", fallback error " +
                std::to_string(saved.fallbackError) +
                (saved.restoreFailed ? "; automatic restore failed, recovery backup retained." : "."));
            return false;
        }
        if (saved.usedInPlaceFallback) {
            k2040::log::Warn("MCM registry atomic replacement blocked (Windows error " +
                std::to_string(saved.replaceError) +
                "); safely updated verified live file with recovery backup.");
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

        if (virtualKey == 0 && modifiers != 0) {
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
            // MCM writes {"version":1} until a hotkey is registered.
            return true;
        }
        const auto array = k2040::ScanMcmKeybindArray(json, *keybindsStart);
        if (!array) return false;
        for (const auto& [start, end] : array->objects) {
            if (!ApplyMcmKeybindObject(json.substr(start, end - start), overrides))
                return false;
        }
        return true;
    }

    bool LoadMcmHotkeyOverrides(McmHotkeyOverrides& overrides)
    {
        std::ifstream file(kMcmKeybindRegistryPath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        if(!file.good() && !file.eof())return false;
        if(!ParseMcmKeybindRegistry(buffer.str(),overrides))return false;
        // MCM's ClearKeybind removes records. Once this mod has registered
        // defaults, missing records mean explicitly Unbound, not INI fallback.
        if(std::filesystem::is_regular_file(
                "Data/MCM/Settings/K2040_Quick_Attach_Menu.hotkeys-initialized")){
            if(!FindMcmKeybindObject(buffer.str(),kOpenMenuKeybindId))
                overrides.openMenu=k2040::HotkeyState{};
            if(!FindMcmKeybindObject(buffer.str(),kOpenMenuBuilderKeybindId))
                overrides.openMenuBuilder=k2040::HotkeyState{};
        }
        return true;
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

        g_quickOpenerVirtualKey.store(g_openMenuHotkey.virtualKey, std::memory_order_release);
        g_builderOpenerVirtualKey.store(g_openMenuBuilderHotkey.virtualKey, std::memory_order_release);
        ResetHotkeyEdgeState();
    }

    bool StageNativeMcmLiveSync(std::string_view id, std::uint32_t keycode,
                                std::uint32_t modifiers)
    {
        if (!GetModuleHandleW(L"mcm.dll")) return false;
        // The MCM file is authoritative only at load and after its menu closes.
        // Capture its previous value for a stale-write guard. Do not write it:
        // native MCM serializes its own *live* keybinds on MCM close.
        const auto existing = ReadSharedHotkeyBinding(id);
        // An absent action is not a valid target for RemapKeybind. Permit
        // *explicit* Settings capture to register it using native MCM only
        // after GetAllKeybinds confirms that no live record for this ID exists.
        // Never synthesize an Unbound (0,0) placeholder in the shared file.
        const McmLiveBinding pending{ keycode, modifiers,
            existing ? existing->keycode : 0,
            existing ? existing->modifiers : 0,
            !existing };
        {
            std::lock_guard lock(g_mcmSyncMutex);
            if (id == kOpenMenuKeybindId) g_mcmQuickPending = pending;
            else if (id == kOpenMenuBuilderKeybindId) g_mcmBuilderPending = pending;
            else return false;
        }
        g_mcmSyncPending.store(true, std::memory_order_release);
        k2040::log::Info("Settings keybinding queued for MCM remap (not yet persisted).");
        return true;
    }

    // Called from an F4SE task on the game thread; no private MCM addresses,
    // hooks, or calls into its DLL. The public Scaleform action returns a bool.
    void TryNativeMcmLiveSyncOnGameThread()
    {
        const auto finish = []() {
            g_mcmSyncTaskQueued.store(false, std::memory_order_release);
        };
        if (!GetModuleHandleW(L"mcm.dll")) { finish(); return; }
        // Bound Settings-request retries per Pause-menu session. Passive
        // duplicate checks and fresh-registration checks are separate.
        static unsigned int attemptsThisPause = 0;
        static k2040::NativeMcmChangeGate duplicateScan;
        static bool warnedBridgeThisPause = false;
        static bool loggedBridgeSuccessThisPause = false;
        auto* ui = RE::UI::GetSingleton();
        if (!ui || !ui->GetMenuOpen(RE::BSFixedString("PauseMenu"))) {
            attemptsThisPause = 0;
            duplicateScan.Reset();
            warnedBridgeThisPause = false;
            loggedBridgeSuccessThisPause = false;
            finish();
            return;
        }
        const auto menu = ui->GetMenu(RE::BSFixedString("PauseMenu"));
        if (!menu || !menu->uiMovie) {
            if (!warnedBridgeThisPause) {
                k2040::log::Warn("Native MCM live binding check deferred: PauseMenu movie unavailable.");
                warnedBridgeThisPause = true;
            }
            finish();
            return;
        }

        // Read MCM's *live* map. Editing disk here would race its save-on-close.
        const auto readLive = [&]() -> std::optional<std::vector<k2040::NativeMcmRecord>> {
            Scaleform::GFx::Value result;
            if (!menu->uiMovie->Invoke("root.mcm.GetAllKeybinds", &result, nullptr, 0) ||
                !result.IsArray()) {
                if (!warnedBridgeThisPause) {
                    k2040::log::Warn("Native MCM live binding check deferred: GetAllKeybinds bridge unavailable or invalid.");
                    warnedBridgeThisPause = true;
                }
                return std::nullopt;
            }
            std::vector<k2040::NativeMcmRecord> records;
            if (result.GetArraySize() > 2048u) return std::nullopt;
            for (unsigned i = 0; i < result.GetArraySize(); ++i) {
                Scaleform::GFx::Value entry;
                if (!result.GetElement(i, &entry) || !entry.IsObject()) return std::nullopt;
                Scaleform::GFx::Value owner, id, key, mods;
                if (!entry.GetMember("modName", &owner) || !owner.IsString() ||
                    !entry.GetMember("keybindID", &id) || !id.IsString() ||
                    !entry.GetMember("keycode", &key) || !key.IsInt() ||
                    !entry.GetMember("modifiers", &mods) || !mods.IsInt() ||
                    key.GetInt() < 0 || mods.GetInt() < 0) return std::nullopt;
                records.push_back({owner.GetString(), id.GetString(),
                    static_cast<std::uint32_t>(key.GetInt()),
                    static_cast<std::uint32_t>(mods.GetInt())});
            }
            if (!loggedBridgeSuccessThisPause) {
                unsigned own = 0;
                for (const auto& record : records)
                    if (record.modName == kMcmModName &&
                        (record.id == kOpenMenuKeybindId || record.id == kOpenMenuBuilderKeybindId))
                        ++own;
                k2040::log::Info("Native MCM live GetAllKeybinds bridge available; Quick Attach records: " +
                    std::to_string(own) + ".");
                loggedBridgeSuccessThisPause = true;
            }
            return records;
        };

        // Native MCM owns its map. Register fresh-install factory bindings
        // only through that map, never by editing a registry that MCM might
        // subsequently save over. A persistent marker distinguishes a fresh
        // install from a deliberately cleared action on later launches.
        const auto marker = std::filesystem::path(
            "Data/MCM/Settings/K2040_Quick_Attach_Menu.hotkeys-initialized");
        if (!std::filesystem::is_regular_file(marker)) {
            auto records = readLive();
            if (!records) { finish(); return; }
            bool complete = true;
            for (const auto [id, defaultKey, defaultMods] :
                {std::tuple<std::string_view, std::uint32_t, std::uint32_t>
                    {kOpenMenuKeybindId, 'K', 1u},
                    {kOpenMenuBuilderKeybindId, 'K', 3u}}) {
                const auto state = k2040::ClassifyNativeMcmRecords(*records, kMcmModName, id);
                if (state == k2040::NativeMcmRecordState::Ambiguous ||
                    state == k2040::NativeMcmRecordState::StaleUnboundDuplicate) {
                    complete = false;
                    break;
                }
                if (state != k2040::NativeMcmRecordState::Missing ||
                    k2040::NativeMcmChordOccupied(*records, defaultKey, defaultMods)) continue;
                Scaleform::GFx::Value args[4], result;
                args[0] = "K2040_Quick_Attach_Menu";
                const std::string bindId(id);
                args[1] = bindId.c_str();
                args[2] = static_cast<std::int32_t>(defaultKey);
                args[3] = static_cast<std::int32_t>(defaultMods);
                if (!menu->uiMovie->Invoke("root.mcm.SetKeybind", &result, args, 4) ||
                    !result.IsBoolean() || !result.GetBoolean()) {
                    complete = false;
                    break;
                }
                records = readLive();
                if (!records || k2040::ClassifyNativeMcmRecords(*records, kMcmModName, id) !=
                        k2040::NativeMcmRecordState::Unique) {
                    complete = false;
                    break;
                }
            }
            if (complete) {
                std::error_code ec;
                std::filesystem::create_directories(marker.parent_path(), ec);
                if (!ec) {
                    std::ofstream out(marker, std::ios::binary | std::ios::trunc);
                    if (out) out << "initialized=1\n";
                }
            }
        }

        // MCM's own hotkey UI can register additional chords *after* this
        // PauseMenu opens. Inspect each new live snapshot after two identical
        // observations; one startup scan misses exactly that failure mode.
        // Only the sole stale 0/0 record shadowing one real chord is removable.
        const auto records = readLive();
        if (!records) { finish(); return; }
        if (duplicateScan.ShouldScan(*records)) {
            const auto describe = [&](std::string_view id) {
                const auto state = k2040::ClassifyNativeMcmRecords(*records, kMcmModName, id);
                switch (state) {
                case k2040::NativeMcmRecordState::Missing: return "missing";
                case k2040::NativeMcmRecordState::Unique: return "unique";
                case k2040::NativeMcmRecordState::StaleUnboundDuplicate: return "stale-unbound-duplicate";
                case k2040::NativeMcmRecordState::Ambiguous: return "ambiguous";
                }
                return "unknown";
            };
            k2040::log::Info(std::string("Native MCM binding snapshot: quick=") +
                describe(kOpenMenuKeybindId) + "; builder=" + describe(kOpenMenuBuilderKeybindId));
            for (const auto id : {kOpenMenuKeybindId, kOpenMenuBuilderKeybindId}) {
                const auto state = k2040::ClassifyNativeMcmRecords(*records, kMcmModName, id);
                if (state == k2040::NativeMcmRecordState::Ambiguous) {
                    k2040::log::Warn("Ambiguous live MCM duplicate for " + std::string(id) +
                        "; preserving all entries for manual recovery.");
                    continue;
                }
                if (state != k2040::NativeMcmRecordState::StaleUnboundDuplicate) continue;
                const auto survivor = k2040::SingleBoundNativeMcmRecord(*records, kMcmModName, id);
                if (!survivor) continue;
                Scaleform::GFx::Value args[2];
                args[0] = static_cast<std::int32_t>(0);
                args[1] = static_cast<std::int32_t>(0);
                Scaleform::GFx::Value cleared;
                if (!menu->uiMovie->Invoke("root.mcm.ClearKeybind", &cleared, args, 2) ||
                    !cleared.IsBoolean() || !cleared.GetBoolean()) {
                    k2040::log::Warn("MCM refused removal of stale Unbound duplicate for " + std::string(id));
                    continue;
                }
                const auto after = readLive();
                const auto retained = after
                    ? k2040::SingleBoundNativeMcmRecord(*after, kMcmModName, id)
                    : std::nullopt;
                if (!after || k2040::ClassifyNativeMcmRecords(*after, kMcmModName, id) !=
                        k2040::NativeMcmRecordState::Unique || !retained ||
                    retained->keycode != survivor->keycode ||
                    retained->modifiers != survivor->modifiers) {
                    k2040::log::Warn("Native MCM duplicate cleanup could not be verified for " + std::string(id));
                    continue;
                }
                k2040::log::Info("Removed stale Unbound duplicate through live native MCM: " + std::string(id));
                // Defer any further action until the updated map stabilizes.
                break;
            }
        }
        if (!g_mcmSyncPending.load(std::memory_order_acquire)) { finish(); return; }
        if (attemptsThisPause >= 3u) { finish(); return; }
        ++attemptsThisPause;

        // MCM's SetKeybind *registers* a keybind, whereas RemapKeybind
        // changes the existing registration and preserves its action metadata.
        // Confirm through GetKeybind before reporting success.
        const auto query = [&](std::string_view id) -> std::optional<McmLiveBinding> {
            Scaleform::GFx::Value args[2];
            args[0] = "K2040_Quick_Attach_Menu";
            const std::string keybindId(id);
            args[1] = keybindId.c_str();
            Scaleform::GFx::Value result;
            if (!menu->uiMovie->Invoke("root.mcm.GetKeybind", &result, args, 2) ||
                !result.IsObject()) return std::nullopt;
            Scaleform::GFx::Value key, mods;
            if (!result.GetMember("keycode", &key) || !result.GetMember("modifiers", &mods) ||
                !key.IsInt() || !mods.IsInt() || key.GetInt() < 0 || mods.GetInt() < 0)
                return std::nullopt;
            return McmLiveBinding{ static_cast<std::uint32_t>(key.GetInt()),
                static_cast<std::uint32_t>(mods.GetInt()), 0, 0 };
        };
        enum class LiveSyncResult { NotReady, Applied, Superseded };
        const auto sync = [&](std::string_view id, const McmLiveBinding& binding) {
            const auto all = readLive();
            if (!all) return LiveSyncResult::NotReady;
            const auto state = k2040::ClassifyNativeMcmRecords(*all, kMcmModName, id);
            if (state == k2040::NativeMcmRecordState::Ambiguous ||
                state == k2040::NativeMcmRecordState::StaleUnboundDuplicate)
                return LiveSyncResult::Superseded;
            const auto live = query(id);
            if (!live) return LiveSyncResult::NotReady;
            if (state == k2040::NativeMcmRecordState::Unique &&
                live->keycode == binding.keycode && live->modifiers == binding.modifiers)
                return LiveSyncResult::Applied;
            // MCM's SetKeybind is registration, not remapping. Never use it for
            // an existing action; it would create another logical duplicate.
            const bool absent = state == k2040::NativeMcmRecordState::Missing;
            if (absent != binding.registerMissing &&
                !(binding.registerMissing && state == k2040::NativeMcmRecordState::Unique &&
                  live->keycode == 0 && live->modifiers == 0))
                return LiveSyncResult::Superseded;
            if (!absent && (live->keycode != binding.originalKeycode ||
                            live->modifiers != binding.originalModifiers))
                return LiveSyncResult::Superseded;
            if (k2040::NativeMcmChordOccupied(*all, binding.keycode, binding.modifiers))
                return LiveSyncResult::Superseded;
            Scaleform::GFx::Value args[4];
            args[0] = "K2040_Quick_Attach_Menu";
            const std::string keybindId(id);
            args[1] = keybindId.c_str();
            args[2] = static_cast<std::int32_t>(binding.keycode);
            args[3] = static_cast<std::int32_t>(binding.modifiers);
            Scaleform::GFx::Value result;
            const char* method = absent ? "root.mcm.SetKeybind" : "root.mcm.RemapKeybind";
            if (!menu->uiMovie->Invoke(method, &result, args, 4) ||
                !result.IsBoolean() || !result.GetBoolean()) return LiveSyncResult::NotReady;
            const auto after = readLive();
            if (after && k2040::ClassifyNativeMcmRecords(*after, kMcmModName, id) ==
                    k2040::NativeMcmRecordState::Unique &&
                !k2040::NativeMcmChordOccupied(*all, binding.keycode, binding.modifiers)) {
                for (const auto& record : *after)
                    if (record.modName == kMcmModName && record.id == id &&
                        record.keycode == binding.keycode && record.modifiers == binding.modifiers)
                        return LiveSyncResult::Applied;
            }
            return LiveSyncResult::NotReady;
        };

        // Copy while holding the mutex. Never hold it during the Scaleform call.
        std::optional<McmLiveBinding> quick;
        std::optional<McmLiveBinding> builder;
        {
            std::lock_guard lock(g_mcmSyncMutex);
            quick = g_mcmQuickPending;
            builder = g_mcmBuilderPending;
        }
        // Respect newer MCM/user edits: a pending Settings write is valid only
        // while the current on-disk binding still matches that exact request.
        const auto stillCurrent = [](std::string_view id, const McmLiveBinding& binding) {
            const auto registered = ReadSharedHotkeyBinding(id);
            if (binding.registerMissing)
                return !registered || (registered->keycode == 0 && registered->modifiers == 0);
            return registered && registered->keycode == binding.originalKeycode &&
                registered->modifiers == binding.originalModifiers;
        };
        const auto quickResult = quick && stillCurrent(kOpenMenuKeybindId, *quick)
            ? sync(kOpenMenuKeybindId, *quick) : LiveSyncResult::Superseded;
        const auto builderResult = builder && stillCurrent(kOpenMenuBuilderKeybindId, *builder)
            ? sync(kOpenMenuBuilderKeybindId, *builder) : LiveSyncResult::Superseded;
        const bool staleQuick = quick && quickResult == LiveSyncResult::Superseded;
        const bool staleBuilder = builder && builderResult == LiveSyncResult::Superseded;
        const bool quickDone = quick && quickResult == LiveSyncResult::Applied;
        const bool builderDone = builder && builderResult == LiveSyncResult::Applied;
        {
            std::lock_guard lock(g_mcmSyncMutex);
            const auto unchanged = [](const McmLiveBinding& a, const McmLiveBinding& b) {
                return a.keycode == b.keycode && a.modifiers == b.modifiers &&
                    a.originalKeycode == b.originalKeycode &&
                    a.originalModifiers == b.originalModifiers &&
                    a.registerMissing == b.registerMissing;
            };
            if ((quickDone || staleQuick) && quick && g_mcmQuickPending && unchanged(*quick, *g_mcmQuickPending))
                g_mcmQuickPending.reset();
            if ((builderDone || staleBuilder) && builder && g_mcmBuilderPending && unchanged(*builder, *g_mcmBuilderPending))
                g_mcmBuilderPending.reset();
            g_mcmSyncPending.store(g_mcmQuickPending.has_value() || g_mcmBuilderPending.has_value(),
                                   std::memory_order_release);
        }
        if (staleQuick || staleBuilder)
            k2040::log::Warn("Cancelled stale Settings MCM synchronization because the registry changed.");
        if (quickDone || builderDone)
            k2040::log::Info("MCM live registration/remap verified; awaiting native serialization on close.");
        finish();
    }

    bool McmProviderActive()
    {
        return GetModuleHandleW(L"mcm.dll") != nullptr ||
               GetModuleHandleW(L"F4SEMenuFramework.dll") != nullptr;
    }

    void RefreshMcmHotkeys(bool force)
    {
        if (!McmProviderActive()) {
            if (g_loadedMcmRegistryExists || force) {
                ApplyConfiguredHotkeys(nullptr);
                g_loadedMcmRegistryExists = false;
                g_loadedMcmWriteTime.reset();
            }
            return;
        }
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
        FocusedView = 2,
        McmChild = 3
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

    std::uint32_t HeldMenuOpenerVirtualKey()
    {
        // Check the base key even if a modifier was released first. The
        // remaining physical key must not reach a vanilla game binding after
        // the focused Prisma view is closed.
        for (const auto* key : {&g_quickOpenerVirtualKey, &g_builderOpenerVirtualKey}) {
            const auto vk = key->load(std::memory_order_acquire);
            if (vk != 0 && IsPhysicalKeyDown(vk)) return vk;
        }
        return 0;
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
        // Show pending MCM work honestly; the physical opener and MCM's file
        // remain unchanged until MCM accepts the remap and serializes it.
        if (GetModuleHandleW(L"mcm.dll")) {
            std::lock_guard lock(g_mcmSyncMutex);
            const auto& pending = id == kOpenMenuKeybindId ? g_mcmQuickPending : g_mcmBuilderPending;
            if (pending) return { pending->keycode, pending->modifiers,
                BindingDisplayName(pending->keycode, pending->modifiers) + " (pending MCM)", true };
        }
        if (McmProviderActive()) {
            if (const auto shared = ReadSharedHotkeyBinding(id)) return *shared;
        }
        if(McmProviderActive() && std::filesystem::is_regular_file(
               "Data/MCM/Settings/K2040_Quick_Attach_Menu.hotkeys-initialized") &&
           (id==kOpenMenuKeybindId || id==kOpenMenuBuilderKeybindId))
            return {0,0,"Unbound",true};

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
        if (!k2040::IsSettingsCaptureKeycode(keycode) || (modifiers & ~0x7u) != 0) {
            log::Warn("Ignored unsupported Settings keybind capture code " +
                std::to_string(keycode) + "; retained existing MCM binding.");
            return false;
        }
        if (GetModuleHandleW(L"mcm.dll")) {
            // MCM maintains a separate live manager. Writing Keybinds.json here
            // races its save-on-close and loses user edits, including Tab clear.
            return StageNativeMcmLiveSync(id, keycode, modifiers);
        }
        const bool saved = WriteSharedHotkeyBindings({ PendingSharedBinding{ id, keycode, modifiers } });
        if (saved) log::Info(std::string("Shared MCM-format keybinding saved: ") + id + ".");
        return saved;
    }

    bool ResetSharedHotkeyBindings()
    {
        constexpr auto defaultQuick = static_cast<std::uint32_t>('K');
        if (GetModuleHandleW(L"mcm.dll")) {
            // Stage both only if both registrations still exist. MCM owns the
            // live state and will persist it on close; no competing file write.
            const auto quick = ReadSharedHotkeyBinding(kOpenMenuKeybindId);
            const auto builder = ReadSharedHotkeyBinding(kOpenMenuBuilderKeybindId);
            // Explicit Reset may restore a genuinely absent shortcut. Native
            // registration occurs only after the live MCM map confirms absence.
            // A duplicate with conflicting real keys must never be guessed.
            if ((quick && quick->keycode == 0 && quick->modifiers != 0) ||
                (builder && builder->keycode == 0 && builder->modifiers != 0)) return false;
            const bool quickQueued = StageNativeMcmLiveSync(kOpenMenuKeybindId, defaultQuick, 1u);
            const bool builderQueued = StageNativeMcmLiveSync(kOpenMenuBuilderKeybindId, defaultQuick, 3u);
            return quickQueued && builderQueued;
        }
        const bool saved = WriteSharedHotkeyBindings({
            PendingSharedBinding{ kOpenMenuKeybindId, defaultQuick, 1u },
            PendingSharedBinding{ kOpenMenuBuilderKeybindId, defaultQuick, 3u }
        });
        if (saved) log::Info("MCM-format keybindings restored to fresh-install defaults.");
        return saved;
    }

    void SetHotkeyCaptureActive(bool active)
    {
        const bool wasActive = g_hotkeyCaptureActive.exchange(active);
        if (active == wasActive) {
            return;
        }

        g_hotkeyCaptureReleasePending = !active;
        g_hotkeyCaptureGeneration.fetch_add(1, std::memory_order_acq_rel);
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

    void QueueMenuHotkeyActionFromMcm(bool openBuilder)
    {
        // Both child MCM and the native physical poller may observe one
        // press. Queue through the shared cross-source deduplication gate;
        // do not bypass focus or game-thread safety.
        QueueHotkeyAction(openBuilder ? HotkeyAction::ToggleBuilderMenu
                                     : HotkeyAction::ToggleQuickMenu,
                          HotkeyActionSource::McmChild);
    }

    void InitializeHotkey()
    {
        ApplyConfiguredHotkeys(nullptr);
        SeedDefaultMcmBindings();
        RefreshMcmHotkeys(true);
        InitializeControllerShortcuts();

        log::Info("Quick-menu toggle hotkey initialized. Default: Shift+K.");
        log::Info("Menu-builder toggle hotkey initialized. Default: Ctrl+Shift+K.");
        log::Info("Either opener switches to its menu while the other menu is active; Escape closes without switching.");
    }

    void PollHotkey()
    {
        PollDeferredQuickMenuGameplayIsolationRelease();
        RefreshMcmHotkeys(false);

        // Reconcile Settings changes with native MCM's live in-memory keybind
        // registry when its Pause-menu movie next becomes available. Never
        // invoke Scaleform from this background polling thread.
        static auto nextMcmSyncAttempt = std::chrono::steady_clock::time_point{};
        const auto syncNow = std::chrono::steady_clock::now();
        if ((g_mcmSyncPending.load(std::memory_order_acquire) ||
             GetModuleHandleW(L"mcm.dll")) && syncNow >= nextMcmSyncAttempt) {
            // Recheck while MCM's picker is active: a Builder duplicate can
            // appear after the first scan, before the PauseMenu is closed.
            nextMcmSyncAttempt = syncNow + std::chrono::milliseconds(400);
            if (!g_mcmSyncTaskQueued.exchange(true, std::memory_order_acq_rel)) {
                if (const auto* tasks = F4SE::GetTaskInterface()) {
                    tasks->AddTask([] { TryNativeMcmLiveSyncOnGameThread(); });
                } else {
                    g_mcmSyncTaskQueued.store(false, std::memory_order_release);
                }
            }
        }

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

        // CEF under Proton does not reliably deliver XBUTTON1/XBUTTON2 DOM
        // events. Sample the physical buttons on this polling thread, and
        // enqueue only the Prisma callback onto the game thread.
        static k2040::SettingsNativeMouseCapture nativeMouse;
        const bool captureActive = g_hotkeyCaptureActive.load(std::memory_order_acquire);
        const auto captureSession = g_hotkeyCaptureGeneration.load(std::memory_order_acquire);
        const bool mouse4 = IsPhysicalKeyDown(VK_XBUTTON1);
        const bool mouse5 = IsPhysicalKeyDown(VK_XBUTTON2);
        const std::uint32_t capturedMouse = nativeMouse.Sample(
            captureSession, captureActive, mouse4, mouse5);
        if (captureActive && capturedMouse != 0) {
            const std::uint32_t modifiers =
                (IsPhysicalKeyDown(VK_SHIFT) ? 1u : 0u) |
                (IsPhysicalKeyDown(VK_CONTROL) ? 2u : 0u) |
                (IsPhysicalKeyDown(VK_MENU) ? 4u : 0u);
            if (const auto* tasks = F4SE::GetTaskInterface()) {
                tasks->AddTask([capturedMouse, modifiers, captureSession]() {
                    if (!g_hotkeyCaptureActive.load(std::memory_order_acquire) ||
                        g_hotkeyCaptureGeneration.load(std::memory_order_acquire) != captureSession)
                        return; // cancelled, switched views or stale session
                    GetPrismaBridge().OnNativeSettingsMouseCapture(capturedMouse, modifiers);
                });
            }
        }
        if (captureActive) {
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
