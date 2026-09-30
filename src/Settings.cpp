#include "Settings.h"

#include "Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    k2040::Settings g_settings;

    std::string Trim(std::string value)
    {
        auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char c) {
            return !isSpace(c);
        }));

        value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char c) {
            return !isSpace(c);
        }).base(), value.end());

        return value;
    }

    bool ParseBool(const std::string& value, bool fallback)
    {
        std::string v = value;
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (v == "true" || v == "1" || v == "yes" || v == "on") {
            return true;
        }

        if (v == "false" || v == "0" || v == "no" || v == "off") {
            return false;
        }

        return fallback;
    }

    float ParseFloat(const std::string& value, float fallback)
    {
        try {
            return std::stof(value);
        } catch (...) {
            return fallback;
        }
    }

    std::string BuildDefaultIniText()
    {
        const k2040::Settings defaults;

        std::ostringstream ini;

        ini
            << "; K2040's Quick Attach Menu default configuration\n"
            << "; Existing user INIs are never overwritten by the plugin.\n"
            << "; If this file is missing, the plugin recreates these defaults on startup.\n"
            << "\n"
            << "[General]\n"
            << "ConfigVersion=1\n"
            << "\n"
            << "[Hotkeys]\n"
            << "OpenMenu=" << defaults.openMenuHotkey << "\n"
            << "OpenMenuBuilder=" << defaults.openMenuBuilderHotkey << "\n"
            << "\n"
            << "[UI]\n"
            << "UseGameUIColor=" << (defaults.useGameUIColor ? "true" : "false") << "\n"
            << "Scale=" << defaults.scale << "\n"
            << "PositionX=" << defaults.positionX << "\n"
            << "PositionY=" << defaults.positionY << "\n"
            << "ShowControlHints=" << (defaults.showControlHints ? "true" : "false") << "\n"
            << "HideBracketedText=" << (defaults.hideBracketedText ? "true" : "false") << "\n"
            << "\n"
            << "[Behavior]\n"
            << "CloseAfterApply=" << (defaults.closeAfterApply ? "true" : "false") << "\n"
            << "\n"
            << "[Runtime]\n"
            << "HideInvalidOptions=" << (defaults.hideInvalidOptions ? "true" : "false") << "\n"
            << "MenuSource=" << defaults.menuSource << "\n"
            << "AutoInstallProviderIfSafe=" << (defaults.autoInstallProviderIfSafe ? "true" : "false") << "\n"
            << "RemoveDependentChildrenFirst=" << (defaults.removeDependentChildrenFirst ? "true" : "false") << "\n"
            << "AllowNoLooseModOptions=" << (defaults.allowNoLooseModOptions ? "true" : "false") << "\n";

        return ini.str();
    }

    void EnsureDefaultIniExists()
    {
        const auto iniPath = k2040::GetIniPath();

        if (std::filesystem::exists(iniPath)) {
            k2040::log::Info(std::string("INI found: ") + iniPath.string());
            return;
        }

        std::error_code ec;
        std::filesystem::create_directories(iniPath.parent_path(), ec);

        if (ec) {
            k2040::log::Warn(std::string("Failed to create INI directory: ") + ec.message());
            return;
        }

        std::ofstream file(iniPath, std::ios::out | std::ios::trunc);

        if (!file.is_open()) {
            k2040::log::Warn(std::string("INI missing, but default INI could not be created: ") + iniPath.string());
            return;
        }

        file << BuildDefaultIniText();
        file.close();

        k2040::log::Info(std::string("INI missing; created default INI: ") + iniPath.string());
    }
}

namespace k2040
{
    std::filesystem::path GetIniPath()
    {
        return std::filesystem::path("Data") / "F4SE" / "Plugins" / "K2040_Quick_Attach_Menu.ini";
    }

    Settings LoadSettings()
    {
        Settings settings;

        EnsureDefaultIniExists();

        const auto iniPath = GetIniPath();
        std::ifstream file(iniPath);

        if (!file.is_open()) {
            log::Warn(std::string("Could not open INI; internal defaults will be used: ") + iniPath.string());
            return settings;
        }

        std::string currentSection;
        std::string line;

        while (std::getline(file, line)) {
            line = Trim(line);

            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                currentSection = Trim(line.substr(1, line.size() - 2));
                continue;
            }

            const auto equals = line.find('=');
            if (equals == std::string::npos) {
                continue;
            }

            const std::string key = Trim(line.substr(0, equals));
            const std::string value = Trim(line.substr(equals + 1));

            if (currentSection == "Hotkeys" && key == "OpenMenu") {
                settings.openMenuHotkey = value;
            } else if (currentSection == "Hotkeys" && key == "OpenMenuBuilder") {
                settings.openMenuBuilderHotkey = value;
            } else if (currentSection == "UI" && key == "UseGameUIColor") {
                settings.useGameUIColor = ParseBool(value, settings.useGameUIColor);
            } else if (currentSection == "UI" && key == "Scale") {
                settings.scale = ParseFloat(value, settings.scale);
            } else if (currentSection == "UI" && key == "PositionX") {
                settings.positionX = ParseFloat(value, settings.positionX);
            } else if (currentSection == "UI" && key == "PositionY") {
                settings.positionY = ParseFloat(value, settings.positionY);
            } else if (currentSection == "UI" && key == "ShowControlHints") {
                settings.showControlHints = ParseBool(value, settings.showControlHints);
            } else if (currentSection == "UI" && key == "HideBracketedText") {
                settings.hideBracketedText = ParseBool(value, settings.hideBracketedText);
            } else if (currentSection == "Behavior" && key == "CloseAfterApply") {
                settings.closeAfterApply = ParseBool(value, settings.closeAfterApply);
            } else if (currentSection == "Runtime" && key == "HideInvalidOptions") {
                settings.hideInvalidOptions = ParseBool(value, settings.hideInvalidOptions);
            } else if (currentSection == "Runtime" && key == "MenuSource") {
                settings.menuSource = value;
            } else if (currentSection == "Runtime" && key == "AutoInstallProviderIfSafe") {
                settings.autoInstallProviderIfSafe = ParseBool(value, settings.autoInstallProviderIfSafe);
            } else if (currentSection == "Runtime" && key == "RemoveDependentChildrenFirst") {
                settings.removeDependentChildrenFirst = ParseBool(value, settings.removeDependentChildrenFirst);
            } else if (currentSection == "Runtime" && key == "AllowNoLooseModOptions") {
                settings.allowNoLooseModOptions = ParseBool(value, settings.allowNoLooseModOptions);
            }
        }

        log::Info(std::string("INI loaded: ") + iniPath.string());
        return settings;
    }

    const Settings& GetSettings()
    {
        return g_settings;
    }

    void ReloadSettings()
    {
        g_settings = LoadSettings();
    }
}
