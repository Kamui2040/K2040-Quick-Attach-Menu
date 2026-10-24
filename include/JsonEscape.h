#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace k2040
{
    // Shared by the Prisma payload, saved preferences and diagnostic JSON.
    // Escaping all C0 controls prevents user/mod-provided names from breaking
    // JSON documents, while preserving non-ASCII UTF-8 input bytes.
    inline std::string JsonEscape(std::string_view value)
    {
        std::ostringstream out;
        for (const unsigned char c : value) {
            switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    out << "\\u" << std::uppercase << std::hex
                        << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c) << std::nouppercase << std::dec;
                } else {
                    out << static_cast<char>(c);
                }
                break;
            }
        }
        return out.str();
    }
}
