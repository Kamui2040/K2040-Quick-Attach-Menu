#pragma once

#include <algorithm>
#include <cstdint>
#include <tuple>
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
        bool operator==(const NativeMcmRecord&) const = default;
    };

    // PauseMenu stays open while the player edits MCM hotkeys. A one-shot
    // startup scan can miss duplicates created later by that UI. Recheck a
    // changed, stable native snapshot, never a moving capture in progress.
    // Order is irrelevant; native GetAllKeybinds is keyed by physical chord.
    class NativeMcmChangeGate
    {
    public:
        bool ShouldScan(std::vector<NativeMcmRecord> current)
        {
            const auto order = [](const NativeMcmRecord& a, const NativeMcmRecord& b) {
                return std::tie(a.modName, a.id, a.keycode, a.modifiers) <
                       std::tie(b.modName, b.id, b.keycode, b.modifiers);
            };
            std::sort(current.begin(), current.end(), order);
            if (!seen_ || current != lastSeen_) {
                lastSeen_ = std::move(current);
                seen_ = true;
                consecutive_ = 1;
                return false;
            }
            if (consecutive_ < 2) ++consecutive_;
            if (scanned_ && lastScanned_ == current) return false;
            lastScanned_ = std::move(current);
            scanned_ = true;
            return consecutive_ >= 2;
        }

        void Reset()
        {
            lastSeen_.clear();
            lastScanned_.clear();
            seen_ = scanned_ = false;
            consecutive_ = 0;
        }

    private:
        std::vector<NativeMcmRecord> lastSeen_;
        std::vector<NativeMcmRecord> lastScanned_;
        bool seen_ = false;
        bool scanned_ = false;
        unsigned consecutive_ = 0;
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
        if (count == 2 && real == 1 && unbound == 1) {
            // ClearKeybind(0,0) is global to the physical chord. If another
            // action also claims that chord, the target cannot be proven safe.
            for (const auto& record : records)
                if (record.keycode == 0 && record.modifiers == 0 &&
                    (record.modName != modName || record.id != id))
                    return NativeMcmRecordState::Ambiguous;
            return NativeMcmRecordState::StaleUnboundDuplicate;
        }
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
