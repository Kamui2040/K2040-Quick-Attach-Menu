#pragma once

#include <cstdint>

namespace k2040::plugin
{
    // Use subtraction after checking the header, so malformed sizes cannot
    // wrap when walking an untrusted plugin record or GRUP.
    [[nodiscard]] constexpr bool FitsRecordData(
        std::uint64_t dataBytes, std::uint64_t remainingBytes) noexcept
    {
        return dataBytes <= remainingBytes;
    }

    [[nodiscard]] constexpr bool FitsGroupData(
        std::uint64_t totalGroupBytes, std::uint64_t remainingBytes,
        std::uint64_t headerBytes) noexcept
    {
        return totalGroupBytes >= headerBytes &&
            FitsRecordData(totalGroupBytes - headerBytes, remainingBytes);
    }
}
