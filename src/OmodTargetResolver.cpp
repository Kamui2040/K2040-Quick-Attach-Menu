#include "OmodTargetResolver.h"

#include "Logger.h"

#include <RE/Fallout.h>
#include <RE/T/TESFile.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// The record-walking approach is adapted from BaseNPCSwapper's MNAMResolver
// (mraggi/BaseNPCSwapper, GPL-3.0). This project is GPL-3.0 as well.
// The implementation here is specialized for Quick Attach Menu and preserves
// the full MNAM keyword array plus winning-override semantics.

namespace
{
    constexpr std::uint32_t FourCC(const char (&value)[5])
    {
        return static_cast<std::uint32_t>(static_cast<std::uint8_t>(value[0])) |
            (static_cast<std::uint32_t>(static_cast<std::uint8_t>(value[1])) << 8) |
            (static_cast<std::uint32_t>(static_cast<std::uint8_t>(value[2])) << 16) |
            (static_cast<std::uint32_t>(static_cast<std::uint8_t>(value[3])) << 24);
    }

    constexpr auto kTagTES4 = FourCC("TES4");
    constexpr auto kTagGRUP = FourCC("GRUP");
    constexpr auto kTagOMOD = FourCC("OMOD");
    constexpr auto kTagMNAM = FourCC("MNAM");
    constexpr auto kTagXXXX = FourCC("XXXX");
    constexpr std::uint32_t kFlagCompressed = 0x00040000u;

    struct RecordHeader
    {
        std::uint32_t type = 0;
        std::uint32_t dataSize = 0;
        std::uint32_t flags = 0;
        std::uint32_t formID = 0;
        std::uint32_t versionControl1 = 0;
        std::uint16_t formVersion = 0;
        std::uint16_t versionControl2 = 0;
    };
    static_assert(sizeof(RecordHeader) == 24);

    struct CachedTargetMetadata
    {
        bool parsed = false;
        bool compressed = false;
        std::vector<RE::TESFormID> targetKeywordIds;
    };

    std::mutex g_cacheMutex;
    std::unordered_map<RE::TESFormID, CachedTargetMetadata> g_targetCache;
    std::unordered_set<const RE::TESFile*> g_scannedFiles;
    std::unordered_set<const RE::TESFile*> g_failedFiles;

    bool ReadHeader(std::ifstream& stream, RecordHeader& header)
    {
        stream.read(reinterpret_cast<char*>(std::addressof(header)), sizeof(header));
        return static_cast<std::size_t>(stream.gcount()) == sizeof(header);
    }

    RE::TESFormID ApplyRuntimeIndex(RE::TESFile* file, RE::TESFormID localFormID)
    {
        if (!file) {
            return 0;
        }

        if (file->IsLight()) {
            return 0xFE000000u |
                (static_cast<RE::TESFormID>(file->GetSmallFileCompileIndex()) << 12) |
                (localFormID & 0xFFFu);
        }

        return (static_cast<RE::TESFormID>(file->GetCompileIndex()) << 24) |
            (localFormID & 0x00FFFFFFu);
    }

    RE::TESFormID ResolveFileFormID(RE::TESFile* file, RE::TESFormID fileFormID)
    {
        if (!file) {
            return 0;
        }

        const auto masterIndex = static_cast<std::uint8_t>(fileFormID >> 24);
        const auto localPart = fileFormID & 0x00FFFFFFu;

        if (masterIndex < file->masterCount) {
            if (!file->masterPtrs) {
                return 0;
            }

            auto* master = file->masterPtrs[masterIndex];
            return master ? ApplyRuntimeIndex(master, localPart) : 0;
        }

        return ApplyRuntimeIndex(file, localPart);
    }

    std::filesystem::path BuildPluginPath(RE::TESFile* file)
    {
        if (!file) {
            return {};
        }

        const char* filename = file->filename;
        if (!filename || !filename[0]) {
            return {};
        }

        const char* directory = file->path;
        if (directory && directory[0]) {
            auto candidate = std::filesystem::path(directory) / filename;
            if (std::filesystem::is_regular_file(candidate)) {
                return candidate;
            }
        }

        auto dataCandidate = std::filesystem::path("Data") / filename;
        if (std::filesystem::is_regular_file(dataCandidate)) {
            return dataCandidate;
        }

        return {};
    }

    RE::TESFile* WinningFile(RE::TESForm* form)
    {
        if (!form) {
            return nullptr;
        }

        if (form->sourceFiles.array && !form->sourceFiles.array->empty()) {
            return form->sourceFiles.array->back();
        }

        return form->GetFile(0);
    }

    std::vector<RE::TESFormID> ExtractTargetKeywords(
        RE::TESFile* file,
        const std::vector<char>& payload)
    {
        std::vector<RE::TESFormID> result;
        std::size_t position = 0;
        std::uint32_t extendedSize = 0;

        while (position + 6 <= payload.size()) {
            const auto subtype =
                *reinterpret_cast<const std::uint32_t*>(payload.data() + position);
            const auto subSize =
                *reinterpret_cast<const std::uint16_t*>(payload.data() + position + 4);
            position += 6;

            const std::size_t realSize = extendedSize ? extendedSize : subSize;
            extendedSize = 0;

            if (position + realSize > payload.size()) {
                break;
            }

            if (subtype == kTagXXXX) {
                if (realSize == sizeof(std::uint32_t)) {
                    extendedSize =
                        *reinterpret_cast<const std::uint32_t*>(payload.data() + position);
                }
                position += realSize;
                continue;
            }

            if (subtype == kTagMNAM) {
                const auto count = realSize / sizeof(RE::TESFormID);
                result.reserve(result.size() + count);

                for (std::size_t index = 0; index < count; ++index) {
                    const auto fileKeywordID =
                        *reinterpret_cast<const RE::TESFormID*>(
                            payload.data() + position + index * sizeof(RE::TESFormID));
                    const auto runtimeKeywordID =
                        ResolveFileFormID(file, fileKeywordID);
                    if (runtimeKeywordID != 0) {
                        result.push_back(runtimeKeywordID);
                    }
                }
            }

            position += realSize;
        }

        return result;
    }

    void ProcessOmodGroup(
        RE::TESFile* file,
        std::ifstream& stream,
        std::uint32_t bodySize)
    {
        std::uint32_t consumed = 0;

        while (consumed + sizeof(RecordHeader) <= bodySize) {
            RecordHeader header{};
            if (!ReadHeader(stream, header)) {
                break;
            }
            consumed += sizeof(RecordHeader);

            if (header.type == kTagGRUP) {
                if (header.dataSize < sizeof(RecordHeader)) {
                    break;
                }

                const auto nestedBody =
                    header.dataSize - static_cast<std::uint32_t>(sizeof(RecordHeader));
                stream.seekg(nestedBody, std::ios::cur);
                consumed += nestedBody;
                continue;
            }

            if (header.dataSize > bodySize - consumed) {
                break;
            }

            if (header.type != kTagOMOD) {
                stream.seekg(header.dataSize, std::ios::cur);
                consumed += header.dataSize;
                continue;
            }

            const auto runtimeOmodID = ResolveFileFormID(file, header.formID);
            CachedTargetMetadata metadata;
            metadata.parsed = true;

            if (header.flags & kFlagCompressed) {
                metadata.compressed = true;
                g_targetCache[runtimeOmodID] = std::move(metadata);
                stream.seekg(header.dataSize, std::ios::cur);
                consumed += header.dataSize;
                continue;
            }

            std::vector<char> payload(header.dataSize);
            stream.read(payload.data(), static_cast<std::streamsize>(header.dataSize));
            if (static_cast<std::uint32_t>(stream.gcount()) != header.dataSize) {
                break;
            }

            metadata.targetKeywordIds = ExtractTargetKeywords(file, payload);
            g_targetCache[runtimeOmodID] = std::move(metadata);
            consumed += header.dataSize;
        }
    }

    bool ScanPlugin(RE::TESFile* file)
    {
        if (!file) {
            return false;
        }

        const auto path = BuildPluginPath(file);
        if (path.empty()) {
            k2040::log::Warn(
                std::string("OMOD target resolver could not locate plugin file: ") +
                std::string(file->GetFilename()));
            return false;
        }

        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            k2040::log::Warn(
                std::string("OMOD target resolver could not open plugin file: ") +
                path.string());
            return false;
        }

        RecordHeader tes4{};
        if (!ReadHeader(stream, tes4) || tes4.type != kTagTES4) {
            k2040::log::Warn(
                std::string("OMOD target resolver rejected invalid plugin header: ") +
                path.string());
            return false;
        }

        stream.seekg(tes4.dataSize, std::ios::cur);

        std::size_t omodCountBefore = g_targetCache.size();

        RecordHeader header{};
        while (ReadHeader(stream, header)) {
            if (header.type != kTagGRUP ||
                header.dataSize < sizeof(RecordHeader)) {
                break;
            }

            const auto bodySize =
                header.dataSize - static_cast<std::uint32_t>(sizeof(RecordHeader));

            if (header.flags == kTagOMOD && header.formID == 0) {
                ProcessOmodGroup(file, stream, bodySize);
            } else {
                stream.seekg(bodySize, std::ios::cur);
            }
        }

        k2040::log::Info(
            std::string("OMOD target resolver scanned ") +
            std::string(file->GetFilename()) +
            ": " +
            std::to_string(g_targetCache.size() - omodCountBefore) +
            " OMOD record(s) cached.");

        return true;
    }
}

namespace k2040
{
    OmodTargetMetadata ResolveOmodTargetMetadata(
        RE::BGSMod::Attachment::Mod* omod)
    {
        OmodTargetMetadata result;
        if (!omod) {
            return result;
        }

        std::scoped_lock lock(g_cacheMutex);

        auto* winningFile = WinningFile(omod);
        if (!winningFile) {
            return result;
        }

        if (!g_scannedFiles.contains(winningFile) &&
            !g_failedFiles.contains(winningFile)) {
            if (ScanPlugin(winningFile)) {
                g_scannedFiles.insert(winningFile);
            } else {
                g_failedFiles.insert(winningFile);
            }
        }

        const auto it = g_targetCache.find(omod->GetFormID());
        if (it == g_targetCache.end() ||
            !it->second.parsed ||
            it->second.compressed) {
            result.status = OmodTargetMetadataStatus::Unavailable;
            return result;
        }

        if (it->second.targetKeywordIds.empty()) {
            result.status = OmodTargetMetadataStatus::NoTargetKeywords;
            return result;
        }

        for (const auto keywordID : it->second.targetKeywordIds) {
            auto* keyword = RE::TESForm::GetFormByID<RE::BGSKeyword>(keywordID);
            if (keyword) {
                result.targetKeywords.push_back(keyword);
            }
        }

        result.status = result.targetKeywords.empty()
            ? OmodTargetMetadataStatus::Unavailable
            : OmodTargetMetadataStatus::Resolved;
        return result;
    }
}
