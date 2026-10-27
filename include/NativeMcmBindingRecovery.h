#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace k2040
{
    struct NativeMcmRecord
    {
        std::string modName;
        std::string id;
        std::uint32_t keycode = 0;
        std::uint32_t modifiers = 0;
    };

    enum class NativeMcmRecordState
    {
        Missing,
        Unique,
        StaleUnboundDuplicate,
        Ambiguous
    };

    // MCM indexes bindings by (keycode, modifiers), NOT by mod/action ID.
    // Do not invent extra (0,0) records, or automatically pick between two
    // different real keys. Only a 0/0 record shadowing exactly one real key
    // can safely be removed from MCM's live manager.
    inline NativeMcmRecordState ClassifyNativeMcmRecords(
        const std::vector<NativeMcmRecord>& records,
        std::string_view modName, std::string_view id)
    {
        unsigned count = 0;
        unsigned real = 0;
        unsigned unbound = 0;
        for (const auto& record : records) {
            if (record.modName != modName || record.id != id) continue;
            ++count;
            if (record.modifiers > 7 || (record.keycode == 0 && record.modifiers != 0))
                return NativeMcmRecordState::Ambiguous;
            if (record.keycode == 0) ++unbound;
            else ++real;
        }
        if (count == 0) return NativeMcmRecordState::Missing;
        if (count == 1) return NativeMcmRecordState::Unique;
        if (count == 2 && real == 1 && unbound == 1)
            return NativeMcmRecordState::StaleUnboundDuplicate;
        return NativeMcmRecordState::Ambiguous;
    }

    inline std::optional<NativeMcmRecord> SingleBoundNativeMcmRecord(
        const std::vector<NativeMcmRecord>& records,
        std::string_view modName, std::string_view id)
    {
        std::optional<NativeMcmRecord> found;
        for (const auto& record : records) {
            if (record.modName != modName || record.id != id || record.keycode == 0) continue;
            if (found) return std::nullopt;
            found = record;
        }
        return found;
    }

    inline bool NativeMcmChordOccupied(
        const std::vector<NativeMcmRecord>& records, std::uint32_t keycode,
        std::uint32_t modifiers)
    {
        for (const auto& record : records)
            if (record.keycode == keycode && record.modifiers == modifiers) return true;
        return false;
    }
}
