#pragma once

#include <string>
#include <vector>

namespace RE
{
    class BGSKeyword;

    namespace BGSMod::Attachment
    {
        class Mod;
    }
}

namespace k2040
{
    enum class OmodTargetMetadataStatus
    {
        Resolved,
        NoTargetKeywords,
        Unavailable
    };

    struct OmodTargetMetadata
    {
        OmodTargetMetadataStatus status = OmodTargetMetadataStatus::Unavailable;
        std::string recordEditorId;
        std::vector<RE::BGSKeyword*> targetKeywords;
    };

    // CommonLibF4 does not expose an OMOD record's raw MNAM Target OMOD
    // Keywords. Resolve them from the winning loaded plugin record and cache
    // the result for the rest of the session.
    OmodTargetMetadata ResolveOmodTargetMetadata(RE::BGSMod::Attachment::Mod* omod);
}
