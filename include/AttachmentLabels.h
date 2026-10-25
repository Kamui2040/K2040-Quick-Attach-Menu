#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace k2040
{
    // Presentation only. A generated option always keeps its OMOD form identity;
    // the linked MISC is still used for inventory/loose-mod transactions.
    inline std::string ChooseGeneratedAttachmentLabel(
        std::string_view omodName,
        std::string_view looseModName,
        std::string_view recordEditorName,
        std::string_view humanizedEditorName)
    {
        for (const auto name : std::array{
                 omodName, looseModName, recordEditorName, humanizedEditorName }) {
            const bool readable = std::any_of(name.begin(), name.end(), [](unsigned char c) {
                return !std::isspace(c);
            });
            if (readable) {
                return std::string(name);
            }
        }
        return "Unnamed attachment";
    }
}
