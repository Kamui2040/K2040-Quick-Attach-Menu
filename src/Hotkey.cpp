#include "SettingsNativeMouseCapture.h"
#include "Hotkey.h"
#include "GameplayKeybindPolicy.h"
#include "ControllerShortcuts.h"

#include <F4SE/F4SE.h>
#include <RE/U/UI.h>
#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

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
    // Framework owns MCM settings; QAM gameplay shortcuts never write MCM.
    std::atomic<std::uint8_t> g_lastQuickActionSource = 0;
    std::atomic<std::uint8_t> g_lastBuilderActionSource = 0;
    constexpr std::string_view kOpenMenuKeybindId = "openQuickMenu";
    constexpr std::string_view kOpenMenuBuilderKeybindId = "openMenuBuilder";

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

    void ResetHotkeyEdgeState()
    {
        g_wasOpenPressedLastPoll.store(false);
        g_wasBuilderPressedLastPoll.store(false);
        g_wasEscapePressedLastPoll.store(false);
    }

    void ApplyConfiguredHotkeys()
    {
        // Gameplay bindings are mod-owned user preferences, not native MCM.
        // A legacy INI value is used only while the user has no saved value.
        const auto prefs=k2040::GetQuickMenuPreferences();
        g_openMenuHotkey=k2040::ParseHotkey(prefs.quickMenuHotkey);
        g_openMenuBuilderHotkey=k2040::ParseHotkey(prefs.menuBuilderHotkey);

        g_quickOpenerVirtualKey.store(g_openMenuHotkey.virtualKey, std::memory_order_release);
        g_builderOpenerVirtualKey.store(g_openMenuBuilderHotkey.virtualKey, std::memory_order_release);
        ResetHotkeyEdgeState();
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
        if (key == "MOUSE3") return VK_MBUTTON;
        if (key == "MOUSE4") return VK_XBUTTON1;
        if (key == "MOUSE5") return VK_XBUTTON2;

        return 0;
    }

    bool UnifiedEditorIsOpen()
    {
        const auto hub=GetModuleHandleW(L"K2040_Unified_Settings.dll");
        if(!hub)return false;
        using QueryFn=bool(__cdecl*)() noexcept;
        const auto proc=GetProcAddress(hub,"K2040_IsSettingsEditorOpenV1");
        if(!proc)return false; // Older optional cores: normal fallback.
        return reinterpret_cast<QueryFn>(proc)();
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
        if (id != kOpenMenuKeybindId && id != kOpenMenuBuilderKeybindId)
            return {0,0,"Unbound",false};
        const auto p=GetQuickMenuPreferences();
        const auto state=ParseHotkey(id == kOpenMenuBuilderKeybindId
            ? p.menuBuilderHotkey : p.quickMenuHotkey);
        std::uint32_t keycode=state.virtualKey;
        if (keycode==VK_MBUTTON)keycode=258;
        else if(keycode==VK_XBUTTON1)keycode=259;
        else if(keycode==VK_XBUTTON2)keycode=260;
        const std::uint32_t modifiers=(state.shiftRequired?1u:0u)|
            (state.ctrlRequired?2u:0u)|(state.altRequired?4u:0u);
        return {keycode,modifiers,keycode?BindingDisplayName(keycode,modifiers):
            "Unbound",false};
    }

    bool SetSharedHotkeyBinding(const std::string& id,
                                std::uint32_t keycode,std::uint32_t modifiers)
    {
        if(id!=kOpenMenuKeybindId&&id!=kOpenMenuBuilderKeybindId)return false;
        std::string chord;
        if(keycode==0&&modifiers==0)chord="Unbound";
        else chord=gameplay_keys::FromKeycode(keycode,modifiers);
        if(chord.empty()||!gameplay_keys::Valid(chord))return false;
        if(!GetUserSettingsDiagnostics().saveAllowed)return false;
        auto p=GetQuickMenuPreferences();
        const auto prior=p;
        if(id==kOpenMenuKeybindId)p.quickMenuHotkey=chord;
        else p.menuBuilderHotkey=chord;
        if(p.quickMenuHotkey!="Unbound"&&
           p.quickMenuHotkey==p.menuBuilderHotkey)return false;
        if(!SetQuickMenuPreferences(p)) {
            if(!GetUserSettingsDiagnostics().saveAllowed)return false;
            return true;
        }
        if(!SaveUserPreferences()){
            (void)SetQuickMenuPreferences(prior);
            return false;
        }
        ApplyConfiguredHotkeys();
        log::Info("Independent Quick Attach gameplay binding saved.");
        return true;
    }

    bool ResetSharedHotkeyBindings()
    {
        if(!GetUserSettingsDiagnostics().saveAllowed)return false;
        auto p=GetQuickMenuPreferences();
        const auto prior=p;
        p.quickMenuHotkey="Shift+K";
        p.menuBuilderHotkey="Ctrl+Shift+K";
        if(!SetQuickMenuPreferences(p)){
            if(!GetUserSettingsDiagnostics().saveAllowed)return false;
            return true;
        }
        if(!SaveUserPreferences()){
            (void)SetQuickMenuPreferences(prior);
            return false;
        }
        ApplyConfiguredHotkeys();
        return true;
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

    void InitializeHotkey()
    {
        ApplyConfiguredHotkeys();
        InitializeControllerShortcuts();
        log::Info("Quick Attach gameplay bindings are configured in K2040 Framework, independently of MCM.");
    }

    void PollHotkey()
    {
        PollDeferredQuickMenuGameplayIsolationRelease();

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
        // A key captured by the shared settings editor must not also open
        // gameplay views. Preserve edge state to avoid replay after close.
        if (UnifiedEditorIsOpen()) {
            g_wasOpenPressedLastPoll.store(openPressedNow);
            g_wasBuilderPressedLastPoll.store(builderPressedNow);
            return;
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
