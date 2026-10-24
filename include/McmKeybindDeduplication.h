#pragma once

#include <cctype>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace k2040
{
    // Called only with parsed object ranges for one mod+hotkey id. Preserve the
    // final assignment (also the native parser's effective assignment) and
    // every other mod's bytes. The earlier entries must have trailing commas.
    inline bool RemoveEarlierMcmKeybindObjects(
        std::string& registry,
        const std::vector<std::pair<std::size_t, std::size_t>>& matches)
    {
        if (matches.size() < 2) return true;
        std::string cleaned = registry;
        for (std::size_t index = matches.size() - 1; index > 0; --index) {
            const auto [start, end] = matches[index - 1];
            if (start >= end || end > cleaned.size() ||
                cleaned[start] != '{' || cleaned[end - 1] != '}') return false;
            std::size_t after = end;
            while (after < cleaned.size() &&
                   std::isspace(static_cast<unsigned char>(cleaned[after])) != 0) ++after;
            if (after >= cleaned.size() || cleaned[after] != ',') return false;
            cleaned.erase(start, after - start + 1);
        }
        registry = std::move(cleaned);
        return true;
    }
}
