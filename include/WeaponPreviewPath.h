#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace k2040
{
    // ModelPreview API v4 accepts a single archive-relative NIF path. Reject
    // traversal and external paths before forwarding model data to PrismaUI.
    // This path identifies the BASE weapon mesh, not an OMOD-assembled item.
    [[nodiscard]] inline std::string NormalizeWeaponPreviewNifPath(std::string_view source)
    {
        if (source.empty() || source.size() > 240) return {};
        std::string path;
        path.reserve(source.size() + 7);
        for (const unsigned char c : source) {
            if (c <= 0x20 || c == 0x7F || c == ':' || c == '*' || c == '?' ||
                c == '"' || c == '<' || c == '>' || c == '|') return {};
            path.push_back(c == '\\' ? '/' : static_cast<char>(c));
        }
        if (path.empty() || path.front() == '/') return {};
        const auto lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
        const auto startsWith = [&](std::string_view prefix) {
            return path.size() >= prefix.size() &&
                std::equal(prefix.begin(), prefix.end(), path.begin(),
                    [&](char a, char b) { return lower(a) == lower(b); });
        };
        if (startsWith("Meshes/")) path.erase(0, 7);
        else if (startsWith("Data/")) return {};
        if (path.empty()) return {};
        std::size_t begin = 0;
        while (begin < path.size()) {
            const auto end = path.find('/', begin);
            const auto part = path.substr(begin, end == std::string::npos ? end : end - begin);
            if (part.empty() || part == "." || part == "..") return {};
            if (end == std::string::npos) break;
            begin = end + 1;
            if (begin == path.size()) return {};
        }
        if (path.size() < 5 || !std::equal(path.end() - 4, path.end(), ".nif",
            [&](char a, char b) { return lower(a) == lower(b); })) return {};
        return "Meshes/" + path;
    }
}
