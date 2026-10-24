#pragma once

#include <cctype>
#include <cstddef>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

namespace k2040
{
    // Resolve only a root-object member. A textual find could accidentally
    // accept a matching name inside another mod's nested registry metadata.
    inline std::optional<std::size_t> FindMcmObjectMemberValue(
        std::string_view json, std::string_view member)
    {
        std::size_t cursor = 0;
        while (cursor < json.size() &&
               std::isspace(static_cast<unsigned char>(json[cursor])) != 0) ++cursor;
        if (cursor == json.size() || json[cursor] != '{') return std::nullopt;
        unsigned int braces = 0;
        unsigned int arrays = 0;
        bool inString = false;
        bool escaped = false;
        for (; cursor < json.size(); ++cursor) {
            const char c = json[cursor];
            if (inString) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') inString = false;
                continue;
            }
            if (c == '"') {
                // Keys, not string values, are immediately after '{' or ','.
                // Only root-object keys qualify; nested data is not searched.
                std::size_t prior = cursor;
                while (prior > 0 &&
                       std::isspace(static_cast<unsigned char>(json[prior - 1])) != 0) --prior;
                const bool key = braces == 1 && arrays == 0 && prior > 0 &&
                    (json[prior - 1] == '{' || json[prior - 1] == ',');
                if (key) {
                    std::size_t end = cursor + 1;
                    bool escapedKey = false;
                    for (; end < json.size(); ++end) {
                        if (escapedKey) { escapedKey = false; continue; }
                        if (json[end] == '\\') { escapedKey = true; continue; }
                        if (json[end] == '"') break;
                    }
                    if (end >= json.size()) return std::nullopt;
                    if (json.substr(cursor + 1, end - cursor - 1) == member) {
                        std::size_t value = end + 1;
                        while (value < json.size() &&
                               std::isspace(static_cast<unsigned char>(json[value])) != 0) ++value;
                        if (value >= json.size() || json[value] != ':') return std::nullopt;
                        ++value;
                        while (value < json.size() &&
                               std::isspace(static_cast<unsigned char>(json[value])) != 0) ++value;
                        return value < json.size() ? std::optional<std::size_t>{value} : std::nullopt;
                    }
                    cursor = end;
                } else {
                    inString = true;
                }
            } else if (c == '{') {
                ++braces;
            } else if (c == '}' && braces != 0) {
                --braces;
                if (braces == 0) break;
            } else if (c == '[') {
                ++arrays;
            } else if (c == ']' && arrays != 0) {
                --arrays;
            }
        }
        return std::nullopt;
    }

    struct McmKeybindArray
    {
        std::size_t closingBracket = 0;
        std::vector<std::pair<std::size_t, std::size_t>> objects;
    };

    // One shared traversal for MCM registry reads, native polling, collision
    // checks and repair. No records are modified here. Fail closed for an
    // incomplete array, a missing separator or a non-object array member.
    inline std::optional<McmKeybindArray> ScanMcmKeybindArray(
        std::string_view json, std::size_t openingBracket)
    {
        if (openingBracket >= json.size() || json[openingBracket] != '[')
            return std::nullopt;

        McmKeybindArray result;
        std::size_t objectStart = 0;
        unsigned int depth = 0;
        bool inString = false;
        bool escaped = false;
        bool expectObject = true;
        for (std::size_t cursor = openingBracket + 1; cursor < json.size(); ++cursor) {
            const char c = json[cursor];
            if (inString) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') inString = false;
                continue;
            }
            if (c == '"') {
                if (depth == 0) return std::nullopt;
                inString = true;
            } else if (depth != 0) {
                if (c == '{') ++depth;
                else if (c == '}' && --depth == 0) {
                    result.objects.emplace_back(objectStart, cursor + 1);
                    expectObject = false;
                }
            } else if (std::isspace(static_cast<unsigned char>(c)) != 0) {
                continue;
            } else if (c == ']' && (!expectObject || result.objects.empty())) {
                result.closingBracket = cursor;
                return result;
            } else if (expectObject && c == '{') {
                objectStart = cursor;
                depth = 1;
            } else if (!expectObject && c == ',') {
                expectObject = true;
            } else {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

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
