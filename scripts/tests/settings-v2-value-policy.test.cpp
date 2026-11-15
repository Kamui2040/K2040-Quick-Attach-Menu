#include "SettingsV2ValuePolicy.h"

#include <cassert>
#include <string_view>

namespace
{
    k2040::json::JsonValue Parse(std::string_view text)
    {
        const auto parsed = k2040::json::JsonParser(std::string(text)).Parse();
        assert(parsed);
        return *parsed;
    }
    bool Allowed(std::string_view field, std::string_view json)
    {
        return k2040::settings_v2::ValidFieldValue(field, Parse(json));
    }
}

int main()
{
    // Exercise all declared editable field types; never touch live settings.
    for (const auto field : {
        "hideBracketedPrefixes", "hideBracketedInfixes", "hideBracketedSuffixes",
        "shortenClockLabels", "useAuthoredMenus", "closeAfterApply",
        "allowMovementWhileOpen", "cheatMode", "loggingEnabled"
    }) {
        assert(Allowed(field, "true"));
        assert(Allowed(field, "false"));
        assert(!Allowed(field, "1"));
        assert(!Allowed(field, R"("true")"));
    }
    for (const auto field : {
        "customAccent", "customText", "customPanel", "customInstalled"
    }) {
        assert(Allowed(field, R"("#AaBb09")"));
        assert(!Allowed(field, R"("#AABBCCDD")"));
        assert(!Allowed(field, R"("#GGCCDD")"));
        assert(!Allowed(field, R"("purple")"));
        assert(!Allowed(field, "true"));
    }
    for (const auto entry : {
        "cascade","radial","hybrid","horizontal","orbit","clock"
    }) {
        assert(Allowed("presentation", "\"" + std::string(entry) + "\""));
    }
    for (const auto entry : {
        "default","pipboy","high-contrast","fallout76","custom"
    }) {
        assert(Allowed("theme", "\"" + std::string(entry) + "\""));
    }
    for (const auto entry : {"off","contextual","always"})
        assert(Allowed("controlHints", "\"" + std::string(entry) + "\""));

    assert(!Allowed("presentation", R"("settings")"));
    assert(!Allowed("theme", R"("midnight")")); // Shared theme is not a saved native theme.
    assert(!Allowed("controlHints", R"("hidden")"));
    assert(!Allowed("presentation", "false"));
    assert(Allowed("backgroundOpacity", "0.25"));
    assert(Allowed("backgroundOpacity", "1"));
    assert(Allowed("backgroundOpacity", "0.73"));
    assert(!Allowed("backgroundOpacity", "0.249"));
    assert(!Allowed("backgroundOpacity", "0.255"));
    assert(!Allowed("backgroundOpacity", "1.01"));
    assert(Allowed("menuSlowdown", "0"));
    assert(Allowed("menuSlowdown", "1"));
    assert(Allowed("menuSlowdown", "0.82"));
    assert(!Allowed("menuSlowdown", "0.825"));
    assert(!Allowed("menuSlowdown", "-0.01"));
    assert(Allowed("scale", "0.5"));
    assert(Allowed("scale", "1.5"));
    assert(!Allowed("scale", "1.005"));
    assert(!Allowed("scale", "1.51"));
    for (const auto field : {"positionX","positionY"}) {
        assert(Allowed(field, "0.05"));
        assert(Allowed(field, "0.95"));
        assert(!Allowed(field, "0.005"));
        assert(!Allowed(field, "0.055"));
    }
    for (const auto field : {
        "resetAll", "openQuickMenu", "openMenuBuilder",
        "controllerQuickShortcut", "hideInvalidOptions", "hideBracketedText",
        "builderPanelWidth", "settingsPanelWidth", "weaponProfiles"
    }) {
        assert(!Allowed(field, "true"));
        assert(!Allowed(field, "0"));
        assert(!Allowed(field, R"("value")"));
    }
}
