#pragma once
// Validate the optional hub snapshot before any appearance data reaches CEF.
#include "JsonPreferencesParser.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace k2040::appearance {
inline bool IsHexColor(std::string_view value) {
    if (value.size() != 7 || value.front() != '#') return false;
    for (const char c : value.substr(1)) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}
inline bool ValidSnapshot(std::string_view input) {
    if (input.empty() || input.size() > 4095) return false;
    const auto parsed = json::JsonParser(std::string(input)).Parse();
    const auto* root = parsed ? parsed->AsObject() : nullptr;
    if (!root) return false;
    const auto schema = root->find("schema");
    const auto theme = root->find("theme");
    const auto palette = root->find("palette");
    const auto colors = root->find("colors");
    if (schema == root->end() || theme == root->end() ||
        palette == root->end() || colors == root->end()) return false;
    const auto* version = schema->second.AsNumber();
    const auto* style = theme->second.AsString();
    const auto* paletteName = palette->second.AsString();
    const auto* roles = colors->second.AsObject();
    if (!version || *version != 1 || !style || !paletteName || !roles) return false;
    if (*style != "fallout4" && *style != "fallout76" &&
        *style != "doom-eternal" && *style != "fallout3-newvegas") return false;
    if (paletteName->empty() || paletteName->size() > 48 ||
        !std::all_of(paletteName->begin(), paletteName->end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        })) return false;
    constexpr std::array<std::string_view, 7> keys {
        "background", "panel", "text", "accent", "muted", "border", "highlight"
    };
    if (roles->size() != keys.size()) return false;
    for (auto key : keys) {
        const auto it = roles->find(std::string(key));
        if (it == roles->end()) return false;
        const auto* value = it->second.AsString();
        if (!value || !IsHexColor(*value)) return false;
    }
    return true;
}
}
