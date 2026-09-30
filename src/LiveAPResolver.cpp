#include "LiveAPResolver.h"

#include "Logger.h"
#include "RuntimeState.h"

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include <RE/B/BGSMod.h>
#include <RE/B/BGSKeyword.h>

#include <algorithm>

namespace
{
    using k2040::FormRef;

    FormRef MakeFormRefFromKeyword(RE::BGSKeyword* keyword)
    {
        FormRef ref;
        if (!keyword) return ref;
        ref.formId = keyword->GetFormID();
        const char* id = keyword->GetFormEditorID();
        ref.editorId = id ? id : std::string();
        // Keep displayName consistent with MakeFormRef usage in RuntimeState: use editorId when present, else a placeholder.
        ref.displayName = ref.editorId.empty() ? std::string("(no editor id)") : ref.editorId;
        return ref;
    }

    FormRef ResolveAttachPointKeyword(std::uint16_t keywordIndex)
    {
        const auto* keyword = RE::detail::BGSKeywordGetTypedKeywordByIndex(RE::KeywordType::kAttachPoint, keywordIndex);
        return keyword ? MakeFormRefFromKeyword(const_cast<RE::BGSKeyword*>(keyword)) : FormRef{};
    }

    std::vector<FormRef> CollectAttachParentSlots(const RE::BGSAttachParentArray& attachParents)
    {
        std::vector<FormRef> result;
        if (!attachParents.array || attachParents.size == 0) return result;
        result.reserve(attachParents.size);
        for (std::uint32_t i = 0; i < attachParents.size; ++i) {
            const auto keywordIndex = attachParents.array[i].keywordIndex;
            auto ref = ResolveAttachPointKeyword(keywordIndex);
            if (ref.formId != 0) result.push_back(ref);
        }
        return result;
    }

    bool ContainsFormId(const std::vector<FormRef>& values, std::uint32_t formId)
    {
        for (const auto& v : values) if (v.formId == formId) return true;
        return false;
    }

    void AppendUniqueFormRefs(std::vector<FormRef>& target, const std::vector<FormRef>& source)
    {
        for (const auto& v : source) {
            if (v.formId == 0) continue;
            if (!ContainsFormId(target, v.formId)) target.push_back(v);
        }
    }
}

namespace k2040
{
    LiveAPResult ResolveLiveAttachPoints(const EquippedWeaponInfo& weaponInfo)
    {
        LiveAPResult result;

        // graphPreviewAttachPoints: base WEAP APPR + default-template provided slots (diagnostic only)
        result.graphPreviewAttachPoints.clear();
        AppendUniqueFormRefs(result.graphPreviewAttachPoints, weaponInfo.baseAttachParentSlots);
        AppendUniqueFormRefs(result.graphPreviewAttachPoints, weaponInfo.defaultTemplateProvidedSlots);

        // liveReachableAttachPoints starts with base WEAP APPR only (do NOT include default-template APs)
        result.liveReachableAttachPoints.clear();
        AppendUniqueFormRefs(result.liveReachableAttachPoints, weaponInfo.baseAttachParentSlots);

        // Resolve installedObjectInstanceMods entries by FormID and read attach data; deduplicate installed OMOD FormIDs preserving first occurrence
        std::vector<std::uint32_t> seenInstalled;
        seenInstalled.reserve(weaponInfo.installedObjectInstanceMods.size());
        for (const auto& omodRef : weaponInfo.installedObjectInstanceMods) {
            if (omodRef.formId == 0) {
                // unresolved/non-OMOD entries are diagnostic-only; preserve visibility via counts in the caller
                continue;
            }

            // skip repeated nonzero installed OMOD FormIDs while preserving first occurrence
            if (std::find(seenInstalled.begin(), seenInstalled.end(), omodRef.formId) != seenInstalled.end()) {
                continue;
            }
            seenInstalled.push_back(omodRef.formId);

            OmodAttachmentInfo info;
            info.omod = omodRef;

            // Resolve TESForm by runtime FormID
            RE::TESForm* form = RE::TESForm::GetFormByID(omodRef.formId);
            if (!form) {
                info.consumesAttachPoint = FormRef{};
                info.providesAttachParentSlots.clear();
                info.templateItemIsDefault = false;
                result.installedOmods.push_back(info);
                continue;
            }

            auto* mod = form->As<RE::BGSMod::Attachment::Mod>();
            if (!mod) {
                // Not an OMOD; record diagnostic-only entry and continue
                info.consumesAttachPoint = FormRef{};
                info.providesAttachParentSlots.clear();
                result.installedOmods.push_back(info);
                continue;
            }

            // Read attach point keyword index and attach parents directly (pattern verified in repo)
            const auto keywordIndex = mod->attachPoint.keywordIndex;
            info.consumesAttachPoint = ResolveAttachPointKeyword(keywordIndex);
            info.providesAttachParentSlots = CollectAttachParentSlots(mod->attachParents);

            // Preserve other metadata minimally; installed-instance optional metadata unavailable, default to false
            info.templateItemIndex = 0;
            info.attachmentIndex = 0;
            info.rank = 0;
            info.templateItemIsDefault = false;
            info.optional = false;
            info.childrenExclusive = mod->childrenExclusive;

            // Add provided APs to live reachable set uniquely (runtime-provided APs can enable other options)
            AppendUniqueFormRefs(result.liveReachableAttachPoints, info.providesAttachParentSlots);

            result.installedOmods.push_back(info);
        }

        return result;
    }
}
