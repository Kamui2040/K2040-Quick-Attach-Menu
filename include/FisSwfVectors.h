#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace k2040::fis
{
    // Runtime-only geometry reconstructed from a separately installed icon SWF.
    // No third-party vector paths or raster outputs are stored in the mod files.
    struct VectorPart
    {
        std::string path;
        std::array<double, 6> matrix{ 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
    };

    struct VectorIcon
    {
        // x, y, width, height in the original SWF's twip coordinate space.
        std::array<double, 4> bounds{};
        std::vector<VectorPart> parts;
    };

    struct Library
    {
        std::size_t shapeCount = 0;
        std::size_t spriteCount = 0;
        std::size_t exportCount = 0;
        std::map<std::string, VectorIcon> symbols;
    };

    // Throws on malformed or unsupported input; callers should catch and
    // fall back to a text-only menu without interrupting gameplay.
    Library Load(const std::filesystem::path& installedSwfPath);
}
