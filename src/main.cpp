#include <F4SE/F4SE.h>

#include "Hotkey.h"
#include "InputHandler.h"
#include "Logger.h"
#include "PrismaBridge.h"
#include "Settings.h"
#include "UserSettings.h"

// Current runtime note:
// - Exports legacy F4SEPlugin_Query explicitly for Fallout 4 1.10.163 / F4SE 0.6.23.
// - Uses CommonLibF4's F4SE_PLUGIN_LOAD macro for the load entry point.
// - Registers F4SE messaging.
// - Initializes PrismaUI at kGameDataReady.
// - Uses native polling for the two toggle/switch openers and physical Escape close.
// - Applies only attachment changes that pass the live per-operation safety gates.

namespace
{
    constexpr const char* kPluginName = "K2040_Quick_Attach_Menu";
    constexpr std::uint32_t kPluginVersionPacked = 0x000500BE; // 0.5.190 focused-view hotkey ownership test build

    void OnPluginLoaded()
    {
        k2040::log::Init();
        k2040::ReloadSettings();
        k2040::LoadUserPreferences();
        k2040::InitializeHotkey();

        k2040::log::Info("K2040's Quick Attach Menu compatibility build loaded.");
    }

    void OnGameDataReady()
    {
        k2040::log::Info("F4SE kGameDataReady received.");

        if (k2040::GetPrismaBridge().Initialize()) {
            k2040::log::Info("PrismaBridge initialized successfully at kGameDataReady.");
        } else {
            k2040::log::Warn("PrismaBridge could not initialize at kGameDataReady.");
        }

        k2040::ValidateUserPreferencesAgainstLoadedForms();

        // Current runtime note:
        // In this runtime setup, kInputLoaded was not observed in the plugin log.
        // Start hotkey polling here instead, because kGameDataReady is confirmed
        // and PrismaUI has already been requested.
        k2040::log::Info("Starting hotkey polling from kGameDataReady fallback.");
        k2040::RegisterInputHandler();
    }

    void F4SEAPI MessageHandler(F4SE::MessagingInterface::Message* message)
    {
        if (!message) {
            return;
        }

        switch (message->type) {
        case F4SE::MessagingInterface::kPostLoad:
            k2040::log::Info("F4SE kPostLoad received.");
            break;

        case F4SE::MessagingInterface::kPostPostLoad:
            k2040::log::Info("F4SE kPostPostLoad received.");
            break;

        case F4SE::MessagingInterface::kPreLoadGame:
            k2040::log::Info("F4SE kPreLoadGame received; discarding the current Prisma view before the load transition.");
            k2040::GetPrismaBridge().ResetForGameTransition("kPreLoadGame");
            break;

        case F4SE::MessagingInterface::kPostLoadGame:
            k2040::log::Info("F4SE kPostLoadGame received; refreshing the hidden Prisma Dock registration view.");
            k2040::GetPrismaBridge().ResetForGameTransition("kPostLoadGame");
            k2040::ValidateUserPreferencesAgainstLoadedForms();
            k2040::GetPrismaBridge().EnsureDockRegistration();
            break;

        case F4SE::MessagingInterface::kInputLoaded:
            k2040::log::Info("F4SE kInputLoaded received; registering input handler if not already active.");
            k2040::RegisterInputHandler();
            break;

        case F4SE::MessagingInterface::kNewGame:
            k2040::log::Info("F4SE kNewGame received; refreshing the hidden Prisma Dock registration view.");
            k2040::GetPrismaBridge().ResetForGameTransition("kNewGame");
            k2040::ValidateUserPreferencesAgainstLoadedForms();
            k2040::GetPrismaBridge().EnsureDockRegistration();
            break;

        case F4SE::MessagingInterface::kGameDataReady:
            OnGameDataReady();
            break;

        default:
            break;
        }
    }

    void RegisterMessaging()
    {
        const auto* messaging = F4SE::GetMessagingInterface();

        if (!messaging) {
            k2040::log::Error("F4SE messaging interface unavailable.");
            return;
        }

        if (!messaging->RegisterListener(MessageHandler)) {
            k2040::log::Error("Failed to register F4SE messaging listener.");
            return;
        }

        k2040::log::Info("F4SE messaging listener registered.");
    }
}

F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* f4se, F4SE::PluginInfo* pluginInfo)
{
    k2040::log::Init();
    k2040::log::Info("F4SEPlugin_Query called.");

    if (!pluginInfo) {
        k2040::log::Error("F4SEPlugin_Query received null PluginInfo.");
        return false;
    }

    pluginInfo->infoVersion = F4SE::PluginInfo::kVersion;
    pluginInfo->name = kPluginName;
    pluginInfo->version = kPluginVersionPacked;

    if (!f4se) {
        k2040::log::Error("F4SEPlugin_Query received null F4SE query interface.");
        return false;
    }

    if (f4se->IsEditor()) {
        k2040::log::Error("Creation Kit/editor detected; refusing to load.");
        return false;
    }

    k2040::log::Info("F4SEPlugin_Query accepted.");
    return true;
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* f4se)
{
    k2040::log::Init();
    k2040::log::Info("F4SEPlugin_Load called.");

    if (!f4se) {
        k2040::log::Error("F4SEPlugin_Load received null F4SE load interface.");
        return false;
    }

    F4SE::Init(f4se);

    OnPluginLoaded();
    RegisterMessaging();

    k2040::log::Info("F4SEPlugin_Load completed.");
    return true;
}
