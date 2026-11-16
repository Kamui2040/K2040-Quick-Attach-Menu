#pragma once

// Independent, fail-closed input validation for this module's V2 adapter.
// The hub validates the schema too, but a directly callable native export
// must never rely on the caller having performed that validation.
#include "JsonPreferencesParser.h"
#include "GameplayKeybindPolicy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace k2040::settings_v2
{
    template <std::size_t N>
    [[nodiscard]] inline bool OneOf(std::string_view value,
                                    const std::array<std::string_view, N>& accepted) noexcept
    {
        return std::find(accepted.begin(), accepted.end(), value) != accepted.end();
    }

    [[nodiscard]] inline bool WithinStep(double number, double minimum,
                                         double maximum, double step) noexcept
    {
        if (!std::isfinite(number) || number < minimum || number > maximum) return false;
        const double count = (number - minimum) / step;
        return std::isfinite(count) && std::abs(count - std::round(count)) < 1e-7;
    }

    [[nodiscard]] inline bool ValidColor(std::string_view value) noexcept
    {
        if (value.size() != 7 || value[0] != '#') return false;
        return std::all_of(value.begin() + 1, value.end(), [](unsigned char ch) {
            return (ch >= '0' && ch <= '9') ||
                   (ch >= 'a' && ch <= 'f') ||
                   (ch >= 'A' && ch <= 'F');
        });
    }

    [[nodiscard]] inline bool ValidFieldValue(std::string_view field,
                                              const json::JsonValue& input) noexcept
    {
        using namespace std::literals;
        constexpr std::array toggles{
            "hideBracketedPrefixes"sv, "hideBracketedInfixes"sv,
            "hideBracketedSuffixes"sv, "shortenClockLabels"sv,
            "useAuthoredMenus"sv, "closeAfterApply"sv,
            "allowMovementWhileOpen"sv, "cheatMode"sv, "loggingEnabled"sv
        };
        if (OneOf(field, toggles)) return input.AsBool() != nullptr;

        constexpr std::array colors{
            "customAccent"sv, "customText"sv, "customPanel"sv, "customInstalled"sv
        };
        if (OneOf(field, colors))
            return input.AsString() && ValidColor(*input.AsString());

        const auto* value = input.AsString();
        if (field == "controllerQuickShortcut") {
            constexpr std::array values{"None"sv,"A"sv,"B"sv,"X"sv,"Y"sv,
                "DUp"sv,"DDown"sv,"DLeft"sv,"DRight"sv,"LB"sv,"RB"sv,
                "LT"sv,"RT"sv,"Start"sv,"Back"sv,"LS"sv,"RS"sv,
                "LB+RB"sv,"LT+DUp"sv,"LB+Y"sv};
            return value && OneOf(*value,values);
        }
        if (field == "quickMenuHotkey" || field == "menuBuilderHotkey")
            return value && gameplay_keys::Valid(*value);
        if (field == "presentation") {
            constexpr std::array entries{"cascade"sv, "radial"sv, "hybrid"sv,
                                         "horizontal"sv, "orbit"sv, "clock"sv};
            return value && OneOf(*value, entries);
        }
        if (field == "controlHints") {
            constexpr std::array entries{"off"sv, "contextual"sv, "always"sv};
            return value && OneOf(*value, entries);
        }
        const auto* number = input.AsNumber();
        if (!number) return false;
        if (field == "backgroundOpacity") return WithinStep(*number, .25, 1., .01);
        if (field == "menuSlowdown") return WithinStep(*number, 0., 1., .01);
        if (field == "scale") return WithinStep(*number, .5, 1.5, .01);
        if (field == "positionX" || field == "positionY")
            return WithinStep(*number, .05, .95, .01);
        // Actions and INI-only/legacy fields can never be written via V2.
        return false;
    }
}
