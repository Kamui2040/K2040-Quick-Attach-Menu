#pragma once

#include <filesystem>
#include <string>

namespace k2040
{
    struct Settings
    {
        std::string openMenuHotkey = "Shift+K";
        std::string openMenuBuilderHotkey = "Ctrl+Shift+K";

        bool useGameUIColor = true;
        float scale = 1.0f;
        float positionX = 0.5f;
        float positionY = 0.5f;
        bool showControlHints = false;
        bool hideBracketedText = false;

        bool closeAfterApply = false;

        bool hideInvalidOptions = true;
        std::string menuSource = "Auto";
        bool autoInstallProviderIfSafe = false;
        bool removeDependentChildrenFirst = true;
        bool allowNoLooseModOptions = true;

    };

    Settings LoadSettings();
    const Settings& GetSettings();
    void ReloadSettings();

    std::filesystem::path GetIniPath();
}
