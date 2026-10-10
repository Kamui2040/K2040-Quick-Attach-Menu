#include "InstalledIconBridge.h"

#include "FisSwfVectors.h"
#include "Logger.h"

#include <Windows.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    constexpr std::size_t kMaxIconJsonSize = 128 * 1024;
    constexpr std::size_t kMaxPathChars = 32768;

    bool ValidPath(std::string_view path)
    {
        if (path.empty() || path.size() > kMaxPathChars) return false;
        for (const char character : path) {
            if ((character >= '0' && character <= '9') ||
                character == 'M' || character == 'L' || character == 'Q' ||
                character == ' ' || character == '-') {
                continue;
            }
            return false;
        }
        return true;
    }

    bool ValidIcon(const k2040::fis::VectorIcon& icon)
    {
        if (icon.parts.empty() || icon.parts.size() > 16) return false;
        for (const double value : icon.bounds) {
            if (!std::isfinite(value) || std::abs(value) > 10000000.0) return false;
        }
        if (icon.bounds[2] <= 0 || icon.bounds[3] <= 0) return false;
        for (const auto& part : icon.parts) {
            if (!ValidPath(part.path)) return false;
            for (const double value : part.matrix) {
                if (!std::isfinite(value) || std::abs(value) > 10000000.0) return false;
            }
        }
        return true;
    }

    std::filesystem::path ActiveLibraryPath()
    {
        std::array<wchar_t, 32768> moduleName{};
        const auto length = GetModuleFileNameW(nullptr, moduleName.data(),
            static_cast<DWORD>(moduleName.size()));
        if (length == 0 || length >= moduleName.size()) return {};
        return std::filesystem::path(std::wstring_view(moduleName.data(), length)).parent_path()
            / "Data" / "Interface" / "FallUI_IconLib.swf";
    }

    std::string ReadInstalledVectors()
    {
        const auto path = ActiveLibraryPath();
        if (path.empty()) return "{}";

        try {
            if (!std::filesystem::is_regular_file(path)) return "{}";
            const auto library = k2040::fis::Load(path);
            // Only stable, manually reviewed class-to-symbol matches. Do not
            // infer attachment compatibility or use whole-weapon art for parts.
            constexpr std::array<std::pair<std::string_view, std::string_view>, 2> mappings{{
                { "attachment.generic", "m_M8r.Repo.Mod" },
                { "ammo_caliber.generic", "m_M8r.Fo4Wpn.Ammo" }
            }};
            std::ostringstream json;
            json.imbue(std::locale::classic());
            json << "{";
            bool wroteOne = false;
            for (const auto& [classId, swfSymbol] : mappings) {
                const auto found = library.symbols.find(std::string(swfSymbol));
                if (found == library.symbols.end() || !ValidIcon(found->second)) continue;
                const auto& icon = found->second;
                if (wroteOne) json << ",";
                wroteOne = true;
                json << "\"" << classId << "\":{\"bounds\":[";
                for (std::size_t n = 0; n < icon.bounds.size(); ++n) {
                    if (n) json << ",";
                    json << icon.bounds[n];
                }
                json << "],\"parts\":[";
                for (std::size_t n = 0; n < icon.parts.size(); ++n) {
                    if (n) json << ",";
                    const auto& part = icon.parts[n];
                    json << "{\"d\":\"" << part.path << "\",\"matrix\":[";
                    for (std::size_t j = 0; j < part.matrix.size(); ++j) {
                        if (j) json << ",";
                        json << part.matrix[j];
                    }
                    json << "]}";
                }
                json << "]}";
                if (json.tellp() > static_cast<std::streampos>(kMaxIconJsonSize)) return "{}";
            }
            json << "}";
            if (wroteOne) {
                k2040::log::Info("Optional FIS icon geometry resolved from the deployed library.");
            }
            return json.str();
        } catch (const std::exception& exc) {
            k2040::log::Warn(std::string("Optional FIS icon reader could not use installed library: ") +
                exc.what());
            return "{}";
        }
    }
}

namespace k2040
{
    const std::string& InstalledIconVectorsJson()
    {
        // The installed library is immutable during the running game session.
        // Read it once; refresh after a normal Fallout 4 restart if mods change.
        static const std::string resources = ReadInstalledVectors();
        return resources;
    }
}
