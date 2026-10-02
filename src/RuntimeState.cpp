#include "RuntimeState.h"
#include "LiveAPResolver.h"
#include "OmodTargetResolver.h"

#include "Logger.h"
#include "Settings.h"
#include "UserSettings.h"

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <RE/B/BGSObjectInstanceExtra.h>
#include <RE/E/ExtraDataList.h>
#include <RE/M/MESSAGEBOX_BUTTON.h>
#include <RE/T/TESDataHandler.h>
#include <RE/T/TESFile.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
    std::string SafeEditorId(const RE::TESForm* form)
    {
        if (!form) {
            return {};
        }

        const char* editorId = form->GetFormEditorID();
        return editorId ? editorId : "";
    }

    std::string SafeDisplayName(const RE::TESObjectWEAP* weapon)
    {
        if (!weapon) {
            return {};
        }

        const char* name = weapon->GetFullName();
        return name ? name : "";
    }

    template <class T>
    std::string SafeFullName(const T* form)
    {
        if (!form) return {};
        const char* name = form->GetFullName();
        return name ? name : "";
    }

    std::string HumanizeOmodEditorId(std::string value)
    {
        if (value.empty()) {
            return {};
        }

        std::string spaced;
        spaced.reserve(value.size() + 8);

        for (std::size_t i = 0; i < value.size(); ++i) {
            const unsigned char current = static_cast<unsigned char>(value[i]);
            if (value[i] == '_' || value[i] == '-') {
                if (!spaced.empty() && spaced.back() != ' ') {
                    spaced.push_back(' ');
                }
                continue;
            }

            if (!spaced.empty() &&
                std::isupper(current) &&
                (std::islower(static_cast<unsigned char>(value[i - 1])) ||
                 std::isdigit(static_cast<unsigned char>(value[i - 1])))) {
                spaced.push_back(' ');
            }

            spaced.push_back(value[i]);
        }

        std::istringstream input(spaced);
        std::vector<std::string> words;
        std::string word;
        while (input >> word) {
            words.push_back(word);
        }

        const auto isGenericPrefix = [](const std::string& token) {
            std::string lower = token;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            return lower == "mod" ||
                lower == "omod" ||
                lower == "legendary" ||
                lower == "weapon" ||
                lower == "weap";
        };

        while (!words.empty() && isGenericPrefix(words.front())) {
            words.erase(words.begin());
        }

        std::ostringstream output;
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i != 0) {
                output << ' ';
            }
            output << words[i];
        }

        auto result = output.str();
        return result.empty() ? value : result;
    }

    std::string HumanizeAttachPoint(const k2040::FormRef& attachPoint)
    {
        std::string value = attachPoint.editorId;
        const auto lower = [&]() {
            std::string result = value;
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return result;
        }();

        if (lower.find("receiver") != std::string::npos) return "Receiver";
        if (lower.find("caliber") != std::string::npos || lower.find("ammo") != std::string::npos) return "Caliber";
        if (lower.find("muzzle") != std::string::npos) return "Muzzle";
        if (lower.find("barrel") != std::string::npos) return "Barrel";
        if (lower.find("mag") != std::string::npos) return "Magazine";
        if (lower.find("scope") != std::string::npos || lower.find("sight") != std::string::npos) return "Sights";
        if (lower.find("stock") != std::string::npos) return "Stock";
        if (lower.find("grip") != std::string::npos) return "Grip";
        if (lower.find("rail") != std::string::npos) return "Rail";
        if (lower.find("laser") != std::string::npos) return "Laser";
        if (lower.find("material") != std::string::npos) return "Material";

        const auto prefix = value.find("ap_");
        if (prefix != std::string::npos) value.erase(0, prefix + 3);
        const auto lastUnderscore = value.rfind('_');
        if (lastUnderscore != std::string::npos && lastUnderscore + 1 < value.size()) {
            value = value.substr(lastUnderscore + 1);
        }
        std::replace(value.begin(), value.end(), '_', ' ');
        if (!value.empty()) value.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(value.front())));
        return value.empty() ? "Attachments" : value;
    }

    k2040::FormRef MakeFormRef(const RE::TESForm* form)
    {
        k2040::FormRef ref;

        if (!form) {
            return ref;
        }

        ref.formId = form->GetFormID();
        ref.editorId = SafeEditorId(form);
        ref.displayName = ref.editorId.empty() ? "(no editor id)" : ref.editorId;

        auto* nonConstForm = const_cast<RE::TESForm*>(form);
        auto* file = nonConstForm->GetFile(0);
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (file && dataHandler) {
            ref.sourcePlugin = std::string(file->GetFilename());
            ref.localFormId = nonConstForm->GetLocalFormID();
            if (!ref.sourcePlugin.empty() && ref.localFormId != 0) {
                auto* roundTrip = dataHandler->LookupForm(ref.localFormId, ref.sourcePlugin);
                if (roundTrip == form) {
                    ref.persistentKey = ref.sourcePlugin + ":0x" + k2040::ToHexFormId(ref.localFormId);
                    ref.persistentIdentityValid = true;
                    ref.persistentIdentityStatus = "plugin/local FormID round-trip matched loaded form";
                } else {
                    ref.sourcePlugin.clear();
                    ref.localFormId = 0;
                    ref.persistentKey.clear();
                    ref.persistentIdentityValid = false;
                    ref.persistentIdentityStatus = "plugin/local FormID round-trip did not match loaded form";
                }
            }
        } else {
            ref.persistentIdentityStatus = "TESDataHandler or source file unavailable for persistent identity";
        }

        return ref;
    }

    k2040::FormRef ResolveAttachPointKeyword(std::uint16_t keywordIndex)
    {
        const auto* keyword = RE::detail::BGSKeywordGetTypedKeywordByIndex(
            RE::KeywordType::kAttachPoint,
            keywordIndex);

        return keyword ? MakeFormRef(keyword) : k2040::FormRef{};
    }

    std::vector<k2040::FormRef> CollectAttachParentSlots(const RE::BGSAttachParentArray& attachParents)
    {
        std::vector<k2040::FormRef> result;

        if (!attachParents.array || attachParents.size == 0) {
            return result;
        }

        result.reserve(attachParents.size);

        for (std::uint32_t i = 0; i < attachParents.size; ++i) {
            const auto keywordIndex = attachParents.array[i].keywordIndex;
            auto ref = ResolveAttachPointKeyword(keywordIndex);

            if (ref.formId != 0) {
                result.push_back(ref);
            }
        }

        return result;
    }

    bool ContainsFormId(const std::vector<k2040::FormRef>& values, std::uint32_t formId)
    {
        for (const auto& value : values) {
            if (value.formId == formId) {
                return true;
            }
        }

        return false;
    }

    bool TryGetAttachmentContainerData(
        const RE::BGSMod::Attachment::Mod* mod,
        RE::BGSMod::Container::Data& data)
    {
        if (!mod) {
            return false;
        }

        // Attachment::Mod::GetData is inlined on NG/AE and has no callable
        // relocation there. The inherited Container::GetData exposes the
        // attachment/property container fields we need on both OG and NG/AE.
        const auto* container = static_cast<const RE::BGSMod::Container*>(mod);
        return container->GetData(std::addressof(data)) != nullptr;
    }

    bool IsAttachmentCollectionOmod(const RE::BGSMod::Attachment::Mod* mod)
    {
        RE::BGSMod::Container::Data data{};
        return TryGetAttachmentContainerData(mod, data) &&
            data.attachments &&
            data.attachmentCount > 0;
    }

    bool IsAttachmentCollectionOmod(std::uint32_t formId)
    {
        auto* form = formId != 0 ? RE::TESForm::GetFormByID(formId) : nullptr;
        auto* mod = form ? form->As<RE::BGSMod::Attachment::Mod>() : nullptr;
        return IsAttachmentCollectionOmod(mod);
    }

    void AppendUniqueFormRefs(std::vector<k2040::FormRef>& target, const std::vector<k2040::FormRef>& source)
    {
        for (const auto& value : source) {
            if (value.formId == 0) {
                continue;
            }

            if (!ContainsFormId(target, value.formId)) {
                target.push_back(value);
            }
        }
    }

    std::string JoinFormRefEditorIds(const std::vector<k2040::FormRef>& values)
    {
        if (values.empty()) {
            return "(none)";
        }

        std::string text;

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                text += ", ";
            }

            text += values[i].editorId.empty() ? k2040::ToHexFormId(values[i].formId) : values[i].editorId;
        }

        return text;
    }

    std::vector<k2040::OmodAttachmentInfo> CollectDefaultTemplateMods(
        const RE::BGSMod::Template::Items& objectTemplate,
        std::vector<k2040::FormRef>& providedSlots)
    {
        std::vector<k2040::OmodAttachmentInfo> result;

        const auto templateItemCount = objectTemplate.items.size();

        for (std::uint32_t itemIndex = 0; itemIndex < templateItemCount; ++itemIndex) {
            const auto* item = objectTemplate.items[itemIndex];

            if (!item || !item->isDefault) {
                continue;
            }

            RE::BGSMod::Container::Data data{};
            const auto* dataPtr = item->GetData(std::addressof(data));

            if (!dataPtr || !dataPtr->attachments || dataPtr->attachmentCount == 0) {
                continue;
            }

            for (std::uint32_t attachmentIndex = 0; attachmentIndex < dataPtr->attachmentCount; ++attachmentIndex) {
                const auto& attachment = dataPtr->attachments[attachmentIndex];

                if (!attachment.mod) {
                    continue;
                }

                k2040::OmodAttachmentInfo info;
                info.omod = MakeFormRef(attachment.mod);
                info.consumesAttachPoint = ResolveAttachPointKeyword(attachment.mod->attachPoint.keywordIndex);
                info.providesAttachParentSlots = CollectAttachParentSlots(attachment.mod->attachParents);

                info.templateItemIndex = itemIndex;
                info.attachmentIndex = attachmentIndex;
                info.rank = attachment.index;
                info.templateItemIsDefault = item->isDefault;
                info.optional = attachment.optional;
                info.childrenExclusive = attachment.childrenExclusive;

                AppendUniqueFormRefs(providedSlots, info.providesAttachParentSlots);
                result.push_back(info);
            }
        }

        return result;
    }

    std::string JoinOmodSummary(const std::vector<k2040::OmodAttachmentInfo>& mods)
    {
        if (mods.empty()) {
            return "(none)";
        }

        std::string text;

        for (std::size_t i = 0; i < mods.size(); ++i) {
            if (i > 0) {
                text += " | ";
            }

            const auto& mod = mods[i];
            const auto omodName = mod.omod.editorId.empty() ? k2040::ToHexFormId(mod.omod.formId) : mod.omod.editorId;
            const auto consumes = mod.consumesAttachPoint.editorId.empty() ? "(none)" : mod.consumesAttachPoint.editorId;

            text += omodName;
            text += " consumes ";
            text += consumes;
            text += " provides ";
            text += JoinFormRefEditorIds(mod.providesAttachParentSlots);
        }

        return text;
    }


bool StartsWith(const std::string& value, const std::string& prefix)
{
    return value.size() >= prefix.size() &&
        value.compare(0, prefix.size(), prefix) == 0;
}

void PushKeywordBucketUnique(std::vector<k2040::FormRef>& bucket, const k2040::FormRef& ref)
{
    if (ref.formId == 0) {
        return;
    }

    if (!ContainsFormId(bucket, ref.formId)) {
        bucket.push_back(ref);
    }
}

void ClassifyInstanceKeyword(const k2040::FormRef& ref, k2040::EquippedWeaponInfo& info)
{
    const auto& id = ref.editorId;

    if (StartsWith(id, "dn_") || id.find("_dn_") != std::string::npos) {
        PushKeywordBucketUnique(info.equippedInstanceDnKeywords, ref);
        return;
    }

    if (StartsWith(id, "ma_")) {
        PushKeywordBucketUnique(info.equippedInstanceMaKeywords, ref);
        return;
    }

    if (StartsWith(id, "Anims") || StartsWith(id, "s_")) {
        PushKeywordBucketUnique(info.equippedInstanceAnimKeywords, ref);
        return;
    }

    if (StartsWith(id, "WeaponType") || id == "ObjectTypeWeapon" || id == "QuickkeyGun") {
        PushKeywordBucketUnique(info.equippedInstanceWeaponTypeKeywords, ref);
        return;
    }

    if (StartsWith(id, "ECO_") || StartsWith(id, "modskill_") || id == "remapNode") {
        PushKeywordBucketUnique(info.equippedInstanceGameplayKeywords, ref);
        return;
    }

    PushKeywordBucketUnique(info.equippedInstanceOtherKeywords, ref);
}

void ClassifyInstanceKeywords(k2040::EquippedWeaponInfo& info)
{
    info.equippedInstanceDnKeywords.clear();
    info.equippedInstanceMaKeywords.clear();
    info.equippedInstanceAnimKeywords.clear();
    info.equippedInstanceWeaponTypeKeywords.clear();
    info.equippedInstanceGameplayKeywords.clear();
    info.equippedInstanceOtherKeywords.clear();

    for (const auto& ref : info.equippedInstanceKeywords) {
        ClassifyInstanceKeyword(ref, info);
    }

    info.equippedInstanceStateReliability =
        "Diagnostic only. Live instance keywords can expose state hints such as DN keywords, "
        "but not all weapons use DN keywords. Installed attachment identity should come from "
        "the resolved object-instance OMOD vector when available.";
}

    std::vector<k2040::FormRef> CollectInstanceKeywords(RE::TBO_InstanceData* instanceData)
    {
        std::vector<k2040::FormRef> result;

        if (!instanceData) {
            return result;
        }

        auto* keywordData = instanceData->GetKeywordData();

        if (!keywordData) {
            return result;
        }

        result.reserve(keywordData->GetNumKeywords());

        keywordData->ForEachKeyword([&](RE::BGSKeyword* keyword) {
            if (keyword) {
                result.push_back(MakeFormRef(keyword));
            }

            return RE::BSContainer::ForEachResult::kContinue;
        });

        return result;
    }


std::string FormatFloat(float value)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << value;
    return ss.str();
}

std::string ProbeAmmoName(const k2040::FormRef& ammo)
{
    if (ammo.formId == 0) {
        return "(none)";
    }

    if (!ammo.editorId.empty()) {
        return ammo.editorId;
    }

    return k2040::ToHexFormId(ammo.formId);
}

void CaptureWeaponInstanceDataProbe(RE::TBO_InstanceData* instanceData, k2040::EquippedWeaponInfo& info)
{
    auto& probe = info.liveWeaponInstanceData;

    if (!instanceData) {
        probe.present = false;
        probe.status = "No TBO_InstanceData was available for TESObjectWEAP::InstanceData probing.";
        return;
    }

    // The equipped object has already been confirmed to be a TESObjectWEAP.
    // In this branch, TESObjectWEAP::InstanceData derives from TBO_InstanceData.
    // This probe is read-only and copies plain already-applied instance fields.
    const auto* weaponData = static_cast<const RE::TESObjectWEAP::InstanceData*>(instanceData);

    if (!weaponData) {
        probe.present = false;
        probe.status = "TBO_InstanceData could not be treated as TESObjectWEAP::InstanceData.";
        return;
    }

    probe.present = true;
    probe.ammo = MakeFormRef(weaponData->ammo);

    probe.hasEquipSlot = weaponData->equipSlot != nullptr;
    probe.hasAimModel = weaponData->aimModel != nullptr;
    probe.hasZoomData = weaponData->zoomData != nullptr;
    probe.hasImpactDataSet = weaponData->impactDataSet != nullptr;
    probe.hasRangedData = weaponData->rangedData != nullptr;
    probe.hasKeywordData = weaponData->keywords != nullptr;

    probe.value = weaponData->value;
    probe.attackDamage = weaponData->attackDamage;
    probe.ammoCapacity = weaponData->ammoCapacity;
    probe.rank = weaponData->rank;

    probe.weight = weaponData->weight;
    probe.speed = weaponData->speed;
    probe.reach = weaponData->reach;
    probe.minRange = weaponData->minRange;
    probe.maxRange = weaponData->maxRange;
    probe.attackDelaySec = weaponData->attackDelaySec;
    probe.reloadSpeed = weaponData->reloadSpeed;
    probe.attackActionPointCost = weaponData->attackActionPointCost;
    probe.colorRemappingIndex = weaponData->colorRemappingIndex;

    probe.status =
        "TESObjectWEAP::InstanceData probe succeeded. These are applied live instance fields, "
        "not exact installed OMOD FormIDs.";
}

std::string BuildWeaponInstanceDataSummary(const k2040::WeaponInstanceDataProbe& probe)
{
    if (!probe.present) {
        return probe.status.empty() ? "(not present)" : probe.status;
    }

    std::string text;
    text += "ammo=" + ProbeAmmoName(probe.ammo);
    text += ", damage=" + std::to_string(probe.attackDamage);
    text += ", ammoCapacity=" + std::to_string(probe.ammoCapacity);
    text += ", value=" + std::to_string(probe.value);
    text += ", weight=" + FormatFloat(probe.weight);
    text += ", speed=" + FormatFloat(probe.speed);
    text += ", range=" + FormatFloat(probe.minRange) + "-" + FormatFloat(probe.maxRange);
    text += ", AP=" + FormatFloat(probe.attackActionPointCost);
    text += ", hasZoomData=" + std::string(probe.hasZoomData ? "true" : "false");
    text += ", hasAimModel=" + std::string(probe.hasAimModel ? "true" : "false");
    text += ", hasKeywordData=" + std::string(probe.hasKeywordData ? "true" : "false");

    return text;
}


void CaptureObjectInstanceExtraProbe(RE::PlayerCharacter* player, RE::TESObjectWEAP* weapon, k2040::EquippedWeaponInfo& info)
{
    if (!player || !weapon || !player->inventoryList) {
        k2040::log::Info("Live object-instance extra status: player, weapon, or inventoryList unavailable; probe skipped.");
        k2040::log::Info("Live object-instance extra summary: present=false");
        info.installedObjectInstanceProbeStatus = "player, weapon, or inventoryList unavailable; probe skipped.";
        return;
    }

    bool equippedStackFound = false;
    bool extraDataPresent = false;
    bool objectInstanceExtraPresent = false;
    bool valuesPresent = false;

    std::uint32_t matchingWeaponStackIndex = 0;
    std::uint32_t matchedStackIndex = 0;
    std::uint32_t matchedStackCount = 0;
    std::uint16_t itemIndex = 0;
    std::uint32_t indexDataCount = 0;

    std::string rawIndexData;
    std::string resolvedIndexData;
    std::string status = "No equipped inventory stack matching the equipped weapon was found.";

    player->inventoryList->ForEachStack(
        [&](RE::BGSInventoryItem& item) {
            return item.object == weapon;
        },
        [&](RE::BGSInventoryItem&, RE::BGSInventoryItem::Stack& stack) {
            const auto currentStackIndex = matchingWeaponStackIndex++;

            if (!stack.IsEquipped()) {
                return true;
            }

            equippedStackFound = true;
            matchedStackIndex = currentStackIndex;
            matchedStackCount = stack.GetCount();
            info.equippedInventoryStackFound = true;
            info.equippedInventoryStackIndex = currentStackIndex;
            info.equippedInventoryStackCount = matchedStackCount;

            auto* extra = stack.extra.get();
            extraDataPresent = extra != nullptr;

            if (!extra) {
                status = "Equipped weapon inventory stack was found, but it had no ExtraDataList.";
                return false;
            }

            auto* objectInstanceExtra = extra->GetByType<RE::BGSObjectInstanceExtra>();
            objectInstanceExtraPresent = objectInstanceExtra != nullptr;

            if (!objectInstanceExtra) {
                status = "Equipped weapon inventory stack had ExtraDataList, but no BGSObjectInstanceExtra.";
                return false;
            }

            itemIndex = objectInstanceExtra->itemIndex;
            valuesPresent = objectInstanceExtra->values != nullptr;

            if (!valuesPresent) {
                status = "BGSObjectInstanceExtra was present, but its values buffer was null.";
                return false;
            }

            const auto indexData = objectInstanceExtra->GetIndexData();
            indexDataCount = static_cast<std::uint32_t>(indexData.size());

            for (const auto& entry : indexData) {
                if (!rawIndexData.empty()) {
                    rawIndexData += " | ";
                }

                rawIndexData += "objectID=" + k2040::ToHexFormId(entry.objectID);
                rawIndexData += "/index=" + std::to_string(static_cast<std::uint32_t>(entry.index));
                rawIndexData += "/rank=" + std::to_string(static_cast<std::uint32_t>(entry.rank));
                rawIndexData += "/disabled=" + std::to_string(static_cast<std::uint32_t>(entry.disabled));

                if (!resolvedIndexData.empty()) {
                    resolvedIndexData += " | ";
                }

                auto* resolvedForm = RE::TESForm::GetFormByID(entry.objectID);
                if (resolvedForm) {
                    const auto resolvedRef = MakeFormRef(resolvedForm);

                    if (std::string(resolvedForm->GetFormTypeString()) == "OMOD" &&
                        entry.disabled == 0) {
                        info.installedObjectInstanceMods.push_back(resolvedRef);
                    }

                    resolvedIndexData += k2040::ToHexFormId(entry.objectID);
                    resolvedIndexData += "->";
                    resolvedIndexData += resolvedRef.editorId.empty() ? "(no editor id)" : resolvedRef.editorId;
                    resolvedIndexData += "/type=";
                    resolvedIndexData += resolvedForm->GetFormTypeString();
                    resolvedIndexData += "/index=" + std::to_string(static_cast<std::uint32_t>(entry.index));
                    resolvedIndexData += "/rank=" + std::to_string(static_cast<std::uint32_t>(entry.rank));
                    resolvedIndexData += "/disabled=" + std::to_string(static_cast<std::uint32_t>(entry.disabled));
                } else {
                    resolvedIndexData += k2040::ToHexFormId(entry.objectID);
                    resolvedIndexData += "->(unresolved)";
                    resolvedIndexData += "/index=" + std::to_string(static_cast<std::uint32_t>(entry.index));
                    resolvedIndexData += "/rank=" + std::to_string(static_cast<std::uint32_t>(entry.rank));
                    resolvedIndexData += "/disabled=" + std::to_string(static_cast<std::uint32_t>(entry.disabled));
                }
            }

            status = "BGSObjectInstanceExtra probe succeeded on the equipped inventory stack. Raw ObjectIndexData entries were resolved through the runtime form table; only active resolved OMOD refs are stored in installedObjectInstanceMods.";
            return false;
        });

    std::string summary;
    summary += "present=" + std::string(objectInstanceExtraPresent && valuesPresent ? "true" : "false");
    summary += ", equippedStackFound=" + std::string(equippedStackFound ? "true" : "false");
    summary += ", extraDataPresent=" + std::string(extraDataPresent ? "true" : "false");
    summary += ", objectInstanceExtraPresent=" + std::string(objectInstanceExtraPresent ? "true" : "false");
    summary += ", valuesPresent=" + std::string(valuesPresent ? "true" : "false");
    summary += ", matchedStackIndex=" + std::to_string(matchedStackIndex);
    summary += ", matchedStackCount=" + std::to_string(matchedStackCount);
    summary += ", itemIndex=" + std::to_string(itemIndex);
    summary += ", indexDataCount=" + std::to_string(indexDataCount);

    if (!rawIndexData.empty()) {
        summary += ", rawIndexData=" + rawIndexData;
    }

    if (!resolvedIndexData.empty()) {
        summary += ", resolvedIndexData=" + resolvedIndexData;
    }

    info.installedObjectInstanceProbeStatus = status;
    info.installedObjectInstanceRawCount = indexDataCount;
    info.installedObjectInstanceResolvedCount = static_cast<std::uint32_t>(info.installedObjectInstanceMods.size());

    k2040::log::Info("Live object-instance extra status: " + status);
    k2040::log::Info("Live object-instance extra summary: " + summary);
    k2040::log::Info("Live object-instance resolved mods: " + (resolvedIndexData.empty() ? std::string("(none)") : resolvedIndexData));
    k2040::log::Info("Live object-instance installed OMOD refs: " + JoinFormRefEditorIds(info.installedObjectInstanceMods));

    // Diagnostic-only MODCOL/container probe. Some weapon mods use OMOD
    // collections that can cause multiple OMOD records from one attachment
    // point to appear in the live object-instance vector. Record the OMOD's
    // embedded attachment container without changing installed-state logic.
    for (const auto& omodRef : info.installedObjectInstanceMods) {
        auto* form = RE::TESForm::GetFormByID(omodRef.formId);
        auto* mod = form ? form->As<RE::BGSMod::Attachment::Mod>() : nullptr;
        if (!mod) {
            continue;
        }

        RE::BGSMod::Container::Data modData{};
        if (!TryGetAttachmentContainerData(mod, modData)) {
            k2040::log::Warn(
                "Installed OMOD container diagnostic skipped because container data was unavailable: form=" +
                k2040::ToHexFormId(omodRef.formId));
            continue;
        }

        auto* looseMod = mod->GetLooseMod();
        const auto consumes = ResolveAttachPointKeyword(mod->attachPoint.keywordIndex);

        std::string nested;
        if (modData.attachments && modData.attachmentCount > 0) {
            for (std::uint32_t i = 0; i < modData.attachmentCount; ++i) {
                const auto& attachment = modData.attachments[i];
                if (!attachment.mod) {
                    continue;
                }
                if (!nested.empty()) {
                    nested += " | ";
                }

                const auto childRef = MakeFormRef(attachment.mod);
                const auto childConsumes = ResolveAttachPointKeyword(attachment.mod->attachPoint.keywordIndex);
                nested += "form=" + k2040::ToHexFormId(childRef.formId);
                if (!childRef.editorId.empty()) {
                    nested += "/editor=" + childRef.editorId;
                }
                nested += "/index=" + std::to_string(static_cast<std::uint32_t>(attachment.index));
                nested += "/optional=" + std::string(attachment.optional ? "true" : "false");
                nested += "/childrenExclusive=" + std::string(attachment.childrenExclusive ? "true" : "false");
                nested += "/consumes=" +
                    (childConsumes.editorId.empty()
                        ? k2040::ToHexFormId(childConsumes.formId)
                        : childConsumes.editorId);
                nested += "/hasLooseMod=" +
                    std::string(attachment.mod->GetLooseMod() ? "true" : "false");
            }
        }

        k2040::log::Info(
            "Installed OMOD container diagnostic: form=" + k2040::ToHexFormId(omodRef.formId) +
            ", editor=" + (omodRef.editorId.empty() ? std::string("(none)") : omodRef.editorId) +
            ", source=" + (omodRef.sourcePlugin.empty() ? std::string("(unknown)") : omodRef.sourcePlugin) +
            ", consumes=" + (consumes.editorId.empty() ? k2040::ToHexFormId(consumes.formId) : consumes.editorId) +
            ", hasLooseMod=" + std::string(looseMod ? "true" : "false") +
            ", containerAttachmentCount=" + std::to_string(modData.attachmentCount) +
            ", propertyModCount=" + std::to_string(modData.propertyModCount) +
            ", collectionLike=" + std::string(modData.attachmentCount > 0 ? "true" : "false") +
            ", nested={" + (nested.empty() ? std::string("(none)") : nested) + "}");
    }
}
    void CaptureInstanceProbe(const RE::BGSObjectInstance& equipped, k2040::EquippedWeaponInfo& info)
    {
        auto* instanceData = equipped.instanceData.get();

        info.equippedInstanceDataPresent = instanceData != nullptr;

        if (!instanceData) {
            info.equippedInstanceProbeStatus = "Actor::GetEquippedItem returned no TBO_InstanceData for the equipped weapon.";
            info.equippedInstanceStateReliability =
                "No live instance keyword data was available; direct OMOD / object index probing is still required.";
            return;
        }

        info.equippedInstanceKeywords = CollectInstanceKeywords(instanceData);
        info.equippedInstanceKeywordCount = static_cast<std::uint32_t>(info.equippedInstanceKeywords.size());
        ClassifyInstanceKeywords(info);

        if (info.equippedInstanceKeywordCount == 0) {
            info.equippedInstanceProbeStatus =
                "TBO_InstanceData is present, but GetKeywordData returned no readable instance keywords. "
                "This does not yet expose installed OMODs directly.";
        } else {
            info.equippedInstanceProbeStatus =
                "TBO_InstanceData is present and exposed " +
                std::to_string(info.equippedInstanceKeywordCount) +
                " instance keyword(s). This is a live-instance probe, not yet a full installed-OMOD list.";
        }
    }

    RE::TESObjectWEAP* TryGetEquippedWeapon(
        RE::PlayerCharacter* player,
        std::uint32_t equipIndexValue,
        std::string& diagnostic,
        RE::BGSObjectInstance& equippedOut)
    {
        if (!player) {
            diagnostic = "PlayerCharacter singleton unavailable.";
            return nullptr;
        }

        RE::BGSEquipIndex equipIndex{};
        equipIndex.index = equipIndexValue;

        auto* result = player->GetEquippedItem(std::addressof(equippedOut), equipIndex);

        if (!result) {
            diagnostic = "GetEquippedItem returned null for equip index " + std::to_string(equipIndexValue) + ".";
            return nullptr;
        }

        if (!result->object) {
            diagnostic = "GetEquippedItem returned an empty object for equip index " + std::to_string(equipIndexValue) + ".";
            return nullptr;
        }

        auto* weapon = result->object->As<RE::TESObjectWEAP>();

        if (!weapon) {
            diagnostic =
                "Equipped object at equip index " +
                std::to_string(equipIndexValue) +
                " is not a weapon. FormID " +
                k2040::ToHexFormId(result->object->GetFormID()) +
                " / EditorID " +
                (SafeEditorId(result->object).empty() ? "(empty)" : SafeEditorId(result->object)) +
                ".";
            return nullptr;
        }

        diagnostic = "Weapon found at equip index " + std::to_string(equipIndexValue) + ".";
        return weapon;
    }
}

namespace k2040
{
    std::string ToHexFormId(std::uint32_t formId)
    {
        std::ostringstream ss;
        ss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << formId;
        return ss.str();
    }

    bool ReturnLooseModThroughPickup(std::uint32_t looseModFormId)
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* looseForm = RE::TESForm::GetFormByID(looseModFormId);
        auto* looseObject = looseForm ? looseForm->As<RE::TESBoundObject>() : nullptr;
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        auto* cell = player ? player->GetParentCell() : nullptr;
        if (!player || !player->inventoryList || !looseObject || !dataHandler || !cell) {
            return false;
        }

        const auto before = player->inventoryList->GetItemCount(looseObject);

        RE::NEW_REFR_DATA referenceData;
        referenceData.location = player->data.location;
        referenceData.direction = player->data.angle;
        referenceData.object = looseObject;
        referenceData.initializeScripts = true;
        referenceData.initiallyDisabled = true;
        if (cell->IsInterior()) {
            referenceData.interior = cell;
        } else {
            referenceData.world = cell->worldSpace;
        }

        auto handle = dataHandler->CreateReferenceAtLocation(referenceData);
        auto reference = handle.get();
        if (!reference) {
            log::Warn("Fallout could not create the temporary loose-mod pickup reference.");
            return false;
        }

        reference->Enable(false);
        const bool activated = reference->ActivateRef(player, nullptr, 1, true, true, false);
        const bool returned = player->inventoryList->GetItemCount(looseObject) == before + 1;

        if (!returned) {
            if (!reference->IsDeleted()) {
                reference->Disable();
                reference->SetDelete(true);
            }
            log::Warn(
                "Fallout's pickup path did not return the loose mod: activated=" +
                std::string(activated ? "true" : "false") + ".");
            return false;
        }

        log::Info(
            "Fallout returned the installed loose mod through a temporary pickup reference: form=" +
            ToHexFormId(looseModFormId) + ".");
        return true;
    }

    EquippedWeaponInfo GetEquippedWeaponInfo()
    {
        EquippedWeaponInfo info;

        auto* player = RE::PlayerCharacter::GetSingleton();

        if (!player) {
            info.status = "PlayerCharacter singleton unavailable.";
            log::Warn(std::string("Equipped weapon probe failed: ") + info.status);
            return info;
        }

        std::string diagnostic0;
        std::string diagnostic1;

        RE::BGSObjectInstance equipped0(nullptr, nullptr);
        RE::BGSObjectInstance equipped1(nullptr, nullptr);

        auto* weapon = TryGetEquippedWeapon(player, 0, diagnostic0, equipped0);
        const RE::BGSObjectInstance* matchedInstance = std::addressof(equipped0);
        std::uint32_t matchedEquipIndex = 0;

        if (!weapon) {
            weapon = TryGetEquippedWeapon(player, 1, diagnostic1, equipped1);
            matchedInstance = std::addressof(equipped1);
            matchedEquipIndex = 1;
        }

        if (!weapon) {
            info.status =
                "No equipped weapon detected through Actor::GetEquippedItem. "
                "Index 0: " + diagnostic0 + " Index 1: " + diagnostic1;

            log::Warn(std::string("Equipped weapon probe failed: ") + info.status);
            return info;
        }

        info.hasWeapon = true;
        info.weapon = MakeFormRef(weapon);
        info.equippedSlotIndex = matchedEquipIndex;
        info.weapon.displayName = SafeDisplayName(weapon);
        info.baseAttachParentSlots = CollectAttachParentSlots(weapon->attachParents);
        info.defaultTemplateMods = CollectDefaultTemplateMods(weapon->objectTemplate, info.defaultTemplateProvidedSlots);
        info.currentAvailableSlotsPreview = info.baseAttachParentSlots;
        AppendUniqueFormRefs(info.currentAvailableSlotsPreview, info.defaultTemplateProvidedSlots);

        if (matchedInstance) {
            CaptureInstanceProbe(*matchedInstance, info);
            CaptureWeaponInstanceDataProbe(matchedInstance->instanceData.get(), info);
            CaptureObjectInstanceExtraProbe(player, weapon, info);
        }

        if (info.weapon.displayName.empty()) {
            info.weapon.displayName = "(unnamed weapon)";
        }

        info.status =
            "Equipped weapon detected read-only via Actor::GetEquippedItem index " +
            std::to_string(matchedEquipIndex) +
            ". FormID " +
            ToHexFormId(info.weapon.formId) +
            " / EditorID " +
            (info.weapon.editorId.empty() ? "(empty)" : info.weapon.editorId) +
            ". Base WEAP APPR count " +
            std::to_string(info.baseAttachParentSlots.size()) +
            ". Default template OMOD count " +
            std::to_string(info.defaultTemplateMods.size()) +
            ". Available AP preview count " +
            std::to_string(info.currentAvailableSlotsPreview.size()) +
            ". Installed object-instance OMOD count " +
            std::to_string(info.installedObjectInstanceMods.size()) +
            ". Live instance data present " +
            (info.equippedInstanceDataPresent ? "true" : "false") +
            ". Live instance keyword count " +
            std::to_string(info.equippedInstanceKeywordCount) +
            ". Live instance keywords diagnostic only true."
            ". Live WEAP instance data present " +
            (info.liveWeaponInstanceData.present ? "true" : "false") +
            ".";

        k2040::log::Info("Equipped weapon probe succeeded.");
        k2040::log::Info(std::string("Equipped weapon name: ") + info.weapon.displayName);
        k2040::log::Info(std::string("Equipped weapon EditorID: ") + (info.weapon.editorId.empty() ? "(empty)" : info.weapon.editorId));
        k2040::log::Info(std::string("Equipped weapon FormID: ") + ToHexFormId(info.weapon.formId));
        k2040::log::Info(std::string("Equipped weapon status: ") + info.status);
        k2040::log::Info(std::string("Base WEAP APPR slots: ") + JoinFormRefEditorIds(info.baseAttachParentSlots));
        k2040::log::Info(std::string("Default template OMODs: ") + JoinOmodSummary(info.defaultTemplateMods));
        k2040::log::Info(std::string("Default template provided AP slots: ") + JoinFormRefEditorIds(info.defaultTemplateProvidedSlots));
        k2040::log::Info(std::string("Available AP preview slots: ") + JoinFormRefEditorIds(info.currentAvailableSlotsPreview));
        k2040::log::Info(std::string("Installed object-instance OMODs: ") + JoinFormRefEditorIds(info.installedObjectInstanceMods));
        k2040::log::Info(std::string("Live equipped instance data present: ") + (info.equippedInstanceDataPresent ? "true" : "false"));
        k2040::log::Info(std::string("Live equipped instance keyword count: ") + std::to_string(info.equippedInstanceKeywordCount));
        k2040::log::Info(std::string("Live equipped instance keywords: ") + JoinFormRefEditorIds(info.equippedInstanceKeywords));
        k2040::log::Info(std::string("Live DN/state hint keywords: ") + JoinFormRefEditorIds(info.equippedInstanceDnKeywords));
        k2040::log::Info(std::string("Live MA/compatibility keywords: ") + JoinFormRefEditorIds(info.equippedInstanceMaKeywords));
        k2040::log::Info(std::string("Live animation/sight keywords: ") + JoinFormRefEditorIds(info.equippedInstanceAnimKeywords));
        k2040::log::Info(std::string("Live weapon-type keywords: ") + JoinFormRefEditorIds(info.equippedInstanceWeaponTypeKeywords));
        k2040::log::Info(std::string("Live gameplay/ECO keywords: ") + JoinFormRefEditorIds(info.equippedInstanceGameplayKeywords));
        k2040::log::Info(std::string("Live other keywords: ") + JoinFormRefEditorIds(info.equippedInstanceOtherKeywords));
        k2040::log::Info(std::string("Live equipped instance probe status: ") + info.equippedInstanceProbeStatus);
        k2040::log::Info(std::string("Live equipped instance state reliability: ") + info.equippedInstanceStateReliability);
        k2040::log::Info(std::string("Live WEAP instance data status: ") + info.liveWeaponInstanceData.status);
        k2040::log::Info(std::string("Live WEAP instance data summary: ") + BuildWeaponInstanceDataSummary(info.liveWeaponInstanceData));

        return info;
    }

    EcoWeaponMenu BuildGenericWeaponMenu_ReadOnly(
        const EquippedWeaponInfo& weaponInfo,
        bool includeInventoryUnavailableOptions)
    {
        EcoWeaponMenu menu;
        menu.weapon = weaponInfo.weapon;
        menu.valid = false;
        menu.runtimeGenerated = true;
        menu.status = "Runtime-generated menu unavailable.";

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler || !weaponInfo.hasWeapon || weaponInfo.weapon.sourcePlugin.empty()) {
            return menu;
        }

        struct Candidate
        {
            RE::BGSMod::Attachment::Mod* mod = nullptr;
            FormRef omod;
            FormRef looseMod;
            FormRef consumes;
            std::vector<FormRef> provides;
            std::vector<FormRef> filters;
            std::string fallbackLabel;
            bool installed = false;
            bool inventoryAvailable = false;
        };

        auto* equippedForm = RE::TESForm::GetFormByID(weaponInfo.weapon.formId);
        auto* equippedWeapon = equippedForm ? equippedForm->As<RE::TESObjectWEAP>() : nullptr;
        if (!equippedWeapon) {
            menu.status = "Equipped weapon could not be reacquired for generated-menu compatibility checks.";
            return menu;
        }

        std::vector<Candidate> candidates;
        const auto& allMods = dataHandler->GetFormArray<RE::BGSMod::Attachment::Mod>();
        candidates.reserve(allMods.size());

        // CommonLibF4 exposes FNAM/filter keywords but not the OMOD record's
        // raw MNAM Target OMOD Keywords used by weapon mod-association checks.
        // Read MNAM from the winning plugin record instead of approximating
        // compatibility from source plugin or the wrong runtime keyword array.
        //
        // Workbench-compatible candidate rule:
        // - consumed AP must belong to the weapon/provider graph;
        // - OMODs with no MNAM target are generic for that AP;
        // - OMODs with MNAM targets require at least one matching keyword on
        //   the equipped base WEAP;
        // - installed OMODs remain visible even if metadata cannot be resolved.
        //
        // Inventory affects Quick Menu visibility only; Builder enumeration is
        // independent of inventory.
        for (auto* mod : allMods) {
            if (!mod || mod->targetFormType != RE::ENUM_FORM_ID::kWEAP) continue;

            Candidate candidate;
            candidate.mod = mod;
            candidate.omod = MakeFormRef(mod);
            candidate.installed = ContainsFormId(weaponInfo.installedObjectInstanceMods, candidate.omod.formId);

            if (IsAttachmentCollectionOmod(mod)) {
                if (candidate.installed) {
                    log::Info(
                        "Generated menu ignored installed attachment collection container: OMOD=" +
                        ToHexFormId(candidate.omod.formId) +
                        ", source=" +
                        (candidate.omod.sourcePlugin.empty() ? std::string("(unknown)") : candidate.omod.sourcePlugin));
                }
                continue;
            }

            auto* looseMod = mod->GetLooseMod();
            candidate.looseMod = MakeFormRef(looseMod);
            const bool noLooseOptionAllowed =
                !looseMod && GetSettings().allowNoLooseModOptions;

            if (looseMod) {
                auto* player = RE::PlayerCharacter::GetSingleton();
                candidate.inventoryAvailable = player && player->inventoryList &&
                    player->inventoryList->GetItemCount(looseMod) > 0;
            } else if (!candidate.installed && !noLooseOptionAllowed) {
                log::Info(
                    "Generated candidate rejected because no loose mod is linked and no-loose options are disabled: OMOD=" +
                    ToHexFormId(candidate.omod.formId) +
                    ", source=" +
                    (candidate.omod.sourcePlugin.empty() ? std::string("(unknown)") : candidate.omod.sourcePlugin));
                continue;
            }

            // Builder enumeration is inventory-independent. Gameplay Quick Menu
            // additionally allows explicitly enabled no-loose OMOD actions,
            // because those options do not require a carried MISC item.
            if (!candidate.installed &&
                !candidate.inventoryAvailable &&
                !noLooseOptionAllowed &&
                !includeInventoryUnavailableOptions) continue;

            candidate.consumes = ResolveAttachPointKeyword(mod->attachPoint.keywordIndex);
            if (candidate.consumes.formId == 0) continue;
            candidate.provides = CollectAttachParentSlots(mod->attachParents);

            const auto targetMetadata = ResolveOmodTargetMetadata(mod);
            candidate.fallbackLabel =
                HumanizeOmodEditorId(targetMetadata.recordEditorId);
            bool targetMatches = candidate.installed ||
                targetMetadata.status == OmodTargetMetadataStatus::NoTargetKeywords;

            bool explicitTargetMatched = false;
            if (targetMetadata.status == OmodTargetMetadataStatus::Resolved) {
                for (auto* keyword : targetMetadata.targetKeywords) {
                    auto target = MakeFormRef(keyword);
                    if (target.formId == 0) {
                        continue;
                    }

                    candidate.filters.push_back(target);
                    if (equippedWeapon->HasKeyword(keyword)) {
                        targetMatches = true;
                        explicitTargetMatched = true;
                    }
                }
            }

            // Generated no-loose choices need an explicit weapon-family match.
            // This supports player-facing damage tiers and similar actions while
            // keeping generic/internal no-loose helpers out of the catalog.
            if (!candidate.installed && !looseMod &&
                GetSettings().allowNoLooseModOptions &&
                !explicitTargetMatched) {
                log::Info(
                    "Generated no-loose candidate rejected without explicit matching MNAM target: OMOD=" +
                    ToHexFormId(candidate.omod.formId) +
                    ", source=" +
                    (candidate.omod.sourcePlugin.empty() ? std::string("(unknown)") : candidate.omod.sourcePlugin));
                continue;
            }

            if (!candidate.installed && !targetMatches) {
                const char* metadataStatus =
                    targetMetadata.status == OmodTargetMetadataStatus::Resolved
                        ? "resolved-no-match"
                        : (targetMetadata.status == OmodTargetMetadataStatus::NoTargetKeywords
                            ? "no-mnam"
                            : "mnam-unavailable");

                log::Info(
                    "Generated candidate rejected by OMOD MNAM compatibility: OMOD=" +
                    ToHexFormId(candidate.omod.formId) +
                    ", source=" +
                    (candidate.omod.sourcePlugin.empty() ? std::string("(unknown)") : candidate.omod.sourcePlugin) +
                    ", targetMetadata=" + metadataStatus +
                    ", targetKeywords=" + JoinFormRefEditorIds(candidate.filters));
                continue;
            }

            if (!candidate.installed && candidate.inventoryAvailable) {
                log::Info(
                    "Generated carried candidate accepted: OMOD=" + ToHexFormId(candidate.omod.formId) +
                    ", looseMod=" + ToHexFormId(candidate.looseMod.formId) +
                    ", label="" + SafeFullName(looseMod) + """ +
                    ", consumes=" +
                        (candidate.consumes.editorId.empty() ? ToHexFormId(candidate.consumes.formId) : candidate.consumes.editorId) +
                    ", targetKeywords=" + JoinFormRefEditorIds(candidate.filters));
            } else if (!candidate.installed && !looseMod && explicitTargetMatched) {
                log::Info(
                    "Generated no-loose candidate accepted: OMOD=" + ToHexFormId(candidate.omod.formId) +
                    ", label="" + SafeFullName(candidate.mod) + """ +
                    ", consumes=" +
                        (candidate.consumes.editorId.empty() ? ToHexFormId(candidate.consumes.formId) : candidate.consumes.editorId) +
                    ", targetKeywords=" + JoinFormRefEditorIds(candidate.filters));
            }

            candidates.push_back(std::move(candidate));
        }

        // Retain only candidates connected to a base attach point, directly or
        // through another compatible provider OMOD.
        std::vector<FormRef> graphAttachPoints = weaponInfo.baseAttachParentSlots;
        bool expanded = true;
        while (expanded) {
            expanded = false;
            for (const auto& candidate : candidates) {
                if (!ContainsFormId(graphAttachPoints, candidate.consumes.formId)) continue;
                const auto before = graphAttachPoints.size();
                AppendUniqueFormRefs(graphAttachPoints, candidate.provides);
                expanded = expanded || graphAttachPoints.size() != before;
            }
        }

        const auto live = ResolveLiveAttachPoints(weaponInfo);
        menu.graphPreviewAttachPoints = graphAttachPoints;
        menu.liveReachableAttachPoints = live.liveReachableAttachPoints;
        menu.installedOmodAttachmentInfo = live.installedOmods;

        for (const auto& candidate : candidates) {
            if (!ContainsFormId(graphAttachPoints, candidate.consumes.formId)) continue;

            auto categoryIt = std::find_if(
                menu.categories.begin(), menu.categories.end(),
                [&](const EcoMenuCategory& category) {
                    return !category.requiredAttachPoints.empty() &&
                        category.requiredAttachPoints.front().formId == candidate.consumes.formId;
                });

            if (categoryIt == menu.categories.end()) {
                EcoMenuCategory category;
                category.categoryIndex = static_cast<std::uint32_t>(menu.categories.size());
                category.label = HumanizeAttachPoint(candidate.consumes);
                category.persistenceIdentity = candidate.consumes;
                category.requiredAttachPoints.push_back(candidate.consumes);
                category.providerRequired = !ContainsFormId(weaponInfo.baseAttachParentSlots, candidate.consumes.formId);
                category.role = category.providerRequired ? EcoCategoryRole::DependentCategory : EcoCategoryRole::RootCategory;
                menu.categories.push_back(std::move(category));
                categoryIt = std::prev(menu.categories.end());
            }

            EcoMenuOption option;
            option.optionIndex = static_cast<std::uint32_t>(categoryIt->options.size());
            option.omod = candidate.omod;
            option.looseMod = candidate.looseMod;
            option.hasLooseMod = candidate.looseMod.formId != 0;
            option.looseModRequired = option.hasLooseMod;
            option.isAvailableInInventory = candidate.inventoryAvailable;
            option.consumesAttachPoint = candidate.consumes;
            option.providesAttachParentSlots = candidate.provides;
            option.targetKeywords = candidate.filters;
            option.role = !candidate.provides.empty() ? EcoOptionRole::ProviderOption :
                (option.hasLooseMod ? EcoOptionRole::LeafOption : EcoOptionRole::ToggleOption);
            option.isInstalled = candidate.installed;
            option.isStructurallyValid = ContainsFormId(live.liveReachableAttachPoints, candidate.consumes.formId);
            option.isVisible = true;
            option.isSelectable = option.isStructurallyValid &&
                (option.isInstalled ||
                    (option.hasLooseMod && option.isAvailableInInventory) ||
                    (!option.hasLooseMod && GetSettings().allowNoLooseModOptions));

            option.label = SafeFullName(candidate.mod->GetLooseMod());
            if (option.label.empty()) option.label = SafeFullName(candidate.mod);
            if (option.label.empty()) option.label = candidate.fallbackLabel;
            if (option.label.empty()) {
                option.label = HumanizeOmodEditorId(candidate.omod.editorId);
            }
            if (option.label.empty()) option.label = "Unnamed attachment";

            if (option.isInstalled) option.status = "installed";
            else if (!option.isStructurallyValid) option.status = "provider-not-installed";
            else if (option.hasLooseMod && !option.isAvailableInInventory) option.status = "inventory-unavailable";
            else if (!option.hasLooseMod && !GetSettings().allowNoLooseModOptions) option.status = "no-loose-mod-disallowed";
            else option.status = "ready";

            categoryIt->hasVisibleOptions = categoryIt->hasVisibleOptions ||
                (option.isVisible && (option.isInstalled || (option.isSelectable && option.isStructurallyValid)));
            categoryIt->options.push_back(std::move(option));
        }

        for (const auto& providerCategory : menu.categories) {
            for (const auto& provider : providerCategory.options) {
                for (const auto& provided : provider.providesAttachParentSlots) {
                    for (const auto& childCategory : menu.categories) {
                        if (childCategory.requiredAttachPoints.empty() ||
                            childCategory.requiredAttachPoints.front().formId != provided.formId) continue;
                        for (const auto& child : childCategory.options) {
                            EcoDependencyEdge edge;
                            edge.providerCategoryIndex = providerCategory.categoryIndex;
                            edge.providerOptionIndex = provider.optionIndex;
                            edge.childCategoryIndex = childCategory.categoryIndex;
                            edge.childOptionIndex = child.optionIndex;
                            edge.providedAttachPoint = provided;
                            edge.providerInstalled = provider.isInstalled;
                            edge.childCurrentlyValid = child.isStructurallyValid;
                            edge.status = provider.isInstalled ? "provider-installed" : "provider-missing";
                            menu.dependencyEdges.push_back(std::move(edge));
                        }
                    }
                }
            }
        }

        menu.valid = !menu.categories.empty();
        menu.status = menu.valid
            ? "Runtime-generated weapon menu built from compatible OMOD/AP data."
            : "No compatible runtime OMOD graph found for the equipped weapon.";

        log::Info(
            "Generic menu finished: valid=" + std::string(menu.valid ? "true" : "false") +
            ", candidates=" + std::to_string(candidates.size()) +
            ", categories=" + std::to_string(menu.categories.size()) +
            ", status=" + menu.status);
        return menu;
    }

    static EcoWeaponMenu BuildEcoAuthoredWeaponMenu_ReadOnly(const EquippedWeaponInfo& weaponInfo)
    {
        EcoWeaponMenu menu;
        menu.weapon = weaponInfo.weapon;
        menu.valid = false;
        menu.status = "ECO parser not run yet.";

        constexpr const char* kECOPlugin = "Dank_ECO.esp";
        constexpr std::uint32_t kECORegistryLocalFormId = 0x000219u;

        k2040::log::Info(std::string("ECO registry lookup: plugin=") + kECOPlugin + ", local=0x000219");

        auto* dataHandler = RE::TESDataHandler::GetSingleton();

        if (!dataHandler) {
            menu.status = "TESDataHandler unavailable.";
            k2040::log::Warn("ECO parser: TESDataHandler singleton unavailable.");
            return menu;
        }

        // Typed plugin+local lookup. Do not hardcode load-order form IDs.
        RE::BGSListForm* registry = nullptr;

        // Preferred typed lookup; signatures vary between runtimes but the common
        // pattern is LookupForm<T>(localFormId, modName). If not available, this
        // call will be null and parsing will report unavailable.
        registry = dataHandler->LookupForm<RE::BGSListForm>(kECORegistryLocalFormId, kECOPlugin);

        if (!registry) {
            menu.status = "ECO registry (Dank_ECO.esp:0x000219) not found via typed lookup.";
            k2040::log::Info("ECO registry lookup failed for Dank_ECO.esp:0x000219");
            return menu;
        }

        k2040::log::Info("ECO registry found via typed lookup.");

        // Collect candidate roots (the registry is a FLST of FLST roots).
        std::vector<RE::BGSListForm*> roots;
        registry->ForEachForm([&](RE::TESForm* f) {
            if (f) {
                if (auto* lf = f->As<RE::BGSListForm>()) {
                    roots.push_back(lf);
                }
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });

        k2040::log::Info(std::string("ECO registry roots inspected: ") + std::to_string(roots.size()));

        // Reacquire the equipped weapon TESObjectWEAP from the supplied weaponInfo.
        RE::TESForm* weaponForm = RE::TESForm::GetFormByID(weaponInfo.weapon.formId);
        RE::TESObjectWEAP* equipped = weaponForm ? weaponForm->As<RE::TESObjectWEAP>() : nullptr;

        if (!equipped) {
            menu.status = "Equipped weapon could not be reacquired from runtime form table.";
            k2040::log::Warn("ECO parser: equipped weapon reacquire failed.");
            return menu;
        }

        // Helper to collect MESSAGEBOX_BUTTON pointers from a BGSMessage.
        auto CollectMessageButtons = [&](const RE::BGSMessage* msg) -> std::vector<const RE::MESSAGEBOX_BUTTON*> {
            std::vector<const RE::MESSAGEBOX_BUTTON*> out;
            if (!msg) {
                return out;
            }

            // BGSMessage::buttonList is a BSSimpleList<MESSAGEBOX_BUTTON*>
            // Use the provided iterators to traverse the list in authored order.
            auto* nonConstMsg = const_cast<RE::BGSMessage*>(msg);
            for (const auto& btn : nonConstMsg->buttonList) {
                out.push_back(btn);
            }

            return out;
        };

        // Parser outcome tracking
        bool anyKeywordMatched = false;
        std::string lastRejectionReason;

        for (auto* root : roots) {
            if (!root) {
                continue;
            }

            // Use authored arrayOfForms for exact authored ordering and counts.
            const auto& rootArr = root->arrayOfForms;
            const auto rootCount = rootArr.size();
            if (rootCount == 0) {
                // Empty root; skip.
                continue;
            }

            // Inspect entry 0 only to determine the root keyword (do not validate full shape yet).
            auto* entry0 = rootArr.data()[0];
            if (!entry0) {
                // No usable entry 0; skip.
                continue;
            }

            auto* rootKeyword = entry0->As<RE::BGSKeyword>();
            if (!rootKeyword) {
                // First entry is not a keyword; skip this root.
                continue;
            }

            // Check if this root targets the equipped weapon.
            if (!equipped->HasKeyword(rootKeyword)) {
                // Root keyword does not match this weapon; skip.
                continue;
            }

            // Keyword matched: record for diagnostics and mark that a keyword matched.
            anyKeywordMatched = true;
            menu.weaponKeyword = MakeFormRef(rootKeyword);
            menu.rootFormList = MakeFormRef(root);

            // Clear candidate-specific metadata so a rejected earlier root cannot
            // leave stale message/list references attached to this matched root.
            menu.weaponMenuMessage = {};
            menu.categoryMessageList = {};
            menu.categoryOptionList = {};
            menu.categories.clear();

            // Now validate that the matched root has the exact expected 4 authored entries.
            if (rootCount != 4) {
                lastRejectionReason = "Matching ECO root malformed: expected 4 authored entries but found " + std::to_string(rootCount) + ".";
                k2040::log::Warn(std::string("ECO parser: candidate root rejected: ") + lastRejectionReason);
                continue;
            }

            // Require entries 1..3 to be present and typed correctly before proceeding.
            auto* entry1 = rootArr.data()[1];
            auto* entry2 = rootArr.data()[2];
            auto* entry3 = rootArr.data()[3];

            if (!entry1 || !entry2 || !entry3) {
                lastRejectionReason = "Matching ECO root malformed: null entry among entries 1-3.";
                k2040::log::Warn(std::string("ECO parser: candidate root rejected: ") + lastRejectionReason);
                continue;
            }

            auto* rootMessage = entry1->As<RE::BGSMessage>();
            auto* categoryMessageList = entry2->As<RE::BGSListForm>();
            auto* categoryOptionList = entry3->As<RE::BGSListForm>();

            if (!rootMessage || !categoryMessageList || !categoryOptionList) {
                lastRejectionReason = "Matching ECO root malformed: unexpected typed entries in positions 1-3.";
                k2040::log::Warn(std::string("ECO parser: candidate root rejected: ") + lastRejectionReason);
                continue;
            }

            // Typed validation succeeded; record message and category lists for parsing.
            menu.weaponMenuMessage = MakeFormRef(rootMessage);
            menu.categoryMessageList = MakeFormRef(categoryMessageList);
            menu.categoryOptionList = MakeFormRef(categoryOptionList);

            k2040::log::Info("ECO parser: matching root found and typed validation succeeded; attempting to parse candidate.");

            // Candidate-local state
            std::vector<EcoMenuCategory> candidateCategories;
            std::size_t candidateTotalOptions = 0;
            std::size_t candidateInstalledOptions = 0;
            std::string candidateRejectReason;
            bool candidateFailed = false;

            // Use authored arrays for category lists
            const auto& catMsgArr = categoryMessageList->arrayOfForms;
            const auto& catOptArr = categoryOptionList->arrayOfForms;

            const auto catMsgCount = catMsgArr.size();
            const auto catOptCount = catOptArr.size();

            if (catMsgCount == 0 || catOptCount == 0) {
                candidateFailed = true;
                candidateRejectReason = "Category lists are empty.";
            } else if (catMsgCount != catOptCount) {
                candidateFailed = true;
                candidateRejectReason = "Category list count mismatch between messages and option FLSTs.";
            }

            if (candidateFailed) {
                lastRejectionReason = candidateRejectReason;
                k2040::log::Warn(std::string("ECO parser: candidate root rejected: ") + candidateRejectReason);
                // continue to next root; leave menu.categories empty for now
                continue;
            }

            const std::size_t categoryCount = static_cast<std::size_t>(catMsgCount);

            // Collect root message buttons (category labels begin at index 2)
            const auto rootButtons = CollectMessageButtons(rootMessage);
            if (rootButtons.size() < 2 + categoryCount) {
                candidateFailed = true;
                candidateRejectReason = "Root message button count insufficient for category labels.";
            }

            if (candidateFailed) {
                lastRejectionReason = candidateRejectReason;
                k2040::log::Warn(std::string("ECO parser: candidate root rejected: ") + candidateRejectReason);
                continue;
            }

            // Iterate categories by authored index without filtering
            for (std::size_t ci = 0; ci < categoryCount && !candidateFailed; ++ci) {
                RE::BGSMessage* catMsg = nullptr;
                RE::BGSListForm* catOptFLST = nullptr;

                auto* msgForm = catMsgArr.data()[ci];
                auto* optForm = catOptArr.data()[ci];

                if (!msgForm || !optForm) {
                    candidateFailed = true;
                    candidateRejectReason = "Null entry found in category lists at index " + std::to_string(ci) + ".";
                    break;
                }

                catMsg = msgForm->As<RE::BGSMessage>();
                catOptFLST = optForm->As<RE::BGSListForm>();

                if (!catMsg || !catOptFLST) {
                    candidateFailed = true;
                    candidateRejectReason = "Category entry type mismatch at index " + std::to_string(ci) + ".";
                    break;
                }

                // Pair category label from root message buttons at offset 2
                const std::size_t rootButtonIndex = 2 + ci;
                const auto rootBtn = rootButtons[rootButtonIndex];
                if (!rootBtn) {
                    candidateFailed = true;
                    candidateRejectReason = "Missing root message button for category index " + std::to_string(ci) + ".";
                    break;
                }

                const char* rootLabelC = rootBtn->text.c_str();
                const std::string categoryLabel = rootLabelC ? rootLabelC : std::string();

                // Collect category message buttons for option labels (option labels begin at index 3)
                const auto catButtons = CollectMessageButtons(catMsg);
                if (catButtons.size() < 3) {
                    candidateFailed = true;
                    candidateRejectReason = "Category message button count insufficient for option labels at category index " + std::to_string(ci) + ".";
                    break;
                }

                // Use authored option entries via arrayOfForms
                const auto& optEntriesArr = catOptFLST->arrayOfForms;
                const auto optCount = optEntriesArr.size();
                if (optCount == 0) {
                    // allow empty option list; only require buttons minimum
                }

                const std::size_t optionCount = static_cast<std::size_t>(optCount);
                // Validate catButtons has enough labels for options
                if (catButtons.size() < 3 + optionCount) {
                    candidateFailed = true;
                    candidateRejectReason = "Category option/button count mismatch at category index " + std::to_string(ci) + ".";
                    break;
                }

                EcoMenuCategory category;
                category.categoryIndex = static_cast<uint32_t>(ci);
                category.messageButtonIndex = static_cast<uint32_t>(rootButtonIndex);
                category.label = categoryLabel;
                category.categoryMessage = MakeFormRef(catMsg);
                category.optionFormList = MakeFormRef(catOptFLST);
                category.persistenceIdentity = category.optionFormList.formId != 0 ?
                    category.optionFormList : category.categoryMessage;

                // For each option entry in the FLST, require an OMOD and populate option fields
                for (std::size_t oi = 0; oi < optionCount; ++oi) {
                    auto* optEntryForm = optEntriesArr.data()[oi];
                    if (!optEntryForm) {
                        candidateFailed = true;
                        candidateRejectReason = "Null option entry at category " + std::to_string(ci) + " index " + std::to_string(oi) + ".";
                        break;
                    }

                    auto* omod = optEntryForm->As<RE::BGSMod::Attachment::Mod>();
                    if (!omod) {
                        candidateFailed = true;
                        candidateRejectReason = "Non-OMOD option entry at category " + std::to_string(ci) + " index " + std::to_string(oi) + ".";
                        break;
                    }

                    EcoMenuOption option;
                    option.optionIndex = static_cast<uint32_t>(oi);
                    option.messageButtonIndex = static_cast<uint32_t>(3 + oi);

                    const auto catBtn = catButtons[3 + oi];
                    if (!catBtn) {
                        candidateFailed = true;
                        candidateRejectReason = "Missing category button for option " + std::to_string(oi) + " in category " + std::to_string(ci) + ".";
                        break;
                    }

                    const char* optLabelC = catBtn->text.c_str();
                    option.label = optLabelC ? optLabelC : std::string();

                    // Populate OMOD identity from typed OMOD
                    option.omod = MakeFormRef(omod);

                    // Resolve the loose-mod inventory item associated with this
                    // OMOD. OMOD forms themselves are not inventory objects.
                    auto* looseMod = omod->GetLooseMod();
                    option.hasLooseMod = looseMod != nullptr;
                    option.looseModRequired = option.hasLooseMod;
                    option.looseMod = MakeFormRef(looseMod);
                    if (looseMod) {
                        auto* player = RE::PlayerCharacter::GetSingleton();
                        option.isAvailableInInventory = player && player->inventoryList &&
                            player->inventoryList->GetItemCount(looseMod) > 0;
                    }

                    // Populate authored attachment metadata from the typed OMOD
                    option.consumesAttachPoint = ResolveAttachPointKeyword(omod->attachPoint.keywordIndex);
                    option.providesAttachParentSlots = CollectAttachParentSlots(omod->attachParents);
                    if (!option.providesAttachParentSlots.empty()) {
                        option.role = EcoOptionRole::ProviderOption;
                    } else if (option.consumesAttachPoint.formId != 0) {
                        option.role = option.hasLooseMod ? EcoOptionRole::LeafOption : EcoOptionRole::ToggleOption;
                    } else {
                        option.role = EcoOptionRole::Unknown;
                    }

                    // Installed-state by exact runtime FormID identity
                    ++candidateTotalOptions;
                    option.isInstalled = ContainsFormId(weaponInfo.installedObjectInstanceMods, option.omod.formId);
                    if (option.isInstalled) ++candidateInstalledOptions;

                    // authored options remain visible by default
                    option.isVisible = true;

                    category.options.push_back(option);
                }

                if (candidateFailed) break;

                candidateCategories.push_back(category);
            }

            if (candidateFailed) {
                lastRejectionReason = candidateRejectReason;
                k2040::log::Warn(std::string("ECO parser: candidate root rejected: ") + candidateRejectReason);
                // Leave menu.categories untouched and continue searching other roots
                continue;
            }

            // Candidate succeeded: commit to menu
            menu.categories = std::move(candidateCategories);
            menu.valid = true;
            menu.status = "ECO gun-specific menu parsed (read-only).";

            k2040::log::Info(std::string("ECO parser matched root. Categories=") + std::to_string(menu.categories.size()) + ", total options=" + std::to_string(candidateTotalOptions) + ", installed options=" + std::to_string(candidateInstalledOptions));

            // Enrich parsed menu with live AP resolution and dependency edges (read-only)
            {
                auto live = ResolveLiveAttachPoints(weaponInfo);
                // Persist resolver results into the menu so callers can inspect them.
                menu.graphPreviewAttachPoints = live.graphPreviewAttachPoints;
                menu.liveReachableAttachPoints = live.liveReachableAttachPoints;
                menu.installedOmodAttachmentInfo = live.installedOmods;

                // Populate option-level consumes/provides and structural validity
                for (std::size_t ci = 0; ci < menu.categories.size(); ++ci) {
                    auto& category = menu.categories[ci];

                    // Reset per-category diagnostics
                    category.requiredAttachPoints.clear();
                    category.providerCreatedAttachPoints.clear();
                    category.hasVisibleOptions = false;
                    category.providerRequired = false;
                    category.role = EcoCategoryRole::Unknown;

                    for (std::size_t oi = 0; oi < category.options.size(); ++oi) {
                        auto& opt = category.options[oi];



                        // If default-template data supplies consumes/provides, preserve; otherwise leave empty until resolver fills
                        // Resolver currently populates installedOmods with info when available.

                        // Derive structural validity: option consumesAttachPoint must be non-zero and present in live.liveReachableAttachPoints
                        if (opt.consumesAttachPoint.formId != 0) {
                            opt.isStructurallyValid = ContainsFormId(live.liveReachableAttachPoints, opt.consumesAttachPoint.formId);
                        } else {
                            opt.isStructurallyValid = false;
                        }

                        // Visibility: authored options visible by default unless internal (not implemented here)
                        opt.isVisible = true;

                        // Installed entries remain visible. New selections require
                        // both a structurally reachable AP and either their linked
                        // loose-mod MISC in inventory or an explicitly allowed
                        // no-loose-mod action.
                        const bool inventoryAllowsSelection = opt.isInstalled ||
                            (opt.hasLooseMod ? opt.isAvailableInInventory : GetSettings().allowNoLooseModOptions);
                        opt.isSelectable = opt.isStructurallyValid && inventoryAllowsSelection;

                        // Deterministic status precedence (explicit mapping)
                        // 1) installed + structurally valid => "installed"
                        // 2) installed + missing consumed AP => "installed-missing-consumed-ap"
                        // 3) installed + consumed AP unavailable => "installed-missing-live-attach-point"
                        // 4) not installed + missing consumed AP => "missing-consumed-ap"
                        // 5) missing AP with an authored provider but no installed matching provider => "provider-not-installed"
                        // 6) missing AP without an authored provider => "missing-live-attach-point"
                        // 7) structurally valid but required loose mod absent => "inventory-unavailable"
                        // 8) structurally valid no-loose-mod action disabled => "no-loose-mod-disallowed"
                        // 9) valid and inventory-available => "ready"

                        const bool consumesZero = (opt.consumesAttachPoint.formId == 0);
                        const bool consumesAvailable = !consumesZero && ContainsFormId(live.liveReachableAttachPoints, opt.consumesAttachPoint.formId);

                        // helper: authored provider exists for this consumed AP?
                        auto authoredProviderExists = [&](std::uint32_t apFormId) -> bool {
                            if (apFormId == 0) return false;
                            for (const auto& c : menu.categories) {
                                for (const auto& o : c.options) {
                                    for (const auto& p : o.providesAttachParentSlots) {
                                        if (p.formId == apFormId) return true;
                                    }
                                }
                            }
                            return false;
                        };

                        auto authoredInstalledProviderExists = [&](std::uint32_t apFormId) -> bool {
                            if (apFormId == 0) return false;
                            for (const auto& c : menu.categories) {
                                for (const auto& o : c.options) {
                                    if (!o.isInstalled) continue;
                                    for (const auto& p : o.providesAttachParentSlots) {
                                        if (p.formId == apFormId) return true;
                                    }
                                }
                            }
                            return false;
                        };

                        if (opt.isInstalled && opt.isStructurallyValid) {
                            opt.status = "installed";
                        } else if (opt.isInstalled && consumesZero) {
                            opt.status = "installed-missing-consumed-ap";
                        } else if (opt.isInstalled && !consumesAvailable) {
                            opt.status = "installed-missing-live-attach-point";
                        } else if (!opt.isInstalled && consumesZero) {
                            opt.status = "missing-consumed-ap";
                        } else if (!opt.isInstalled && !consumesAvailable) {
                            // consumed AP missing live; check for authored provider presence
                            if (authoredProviderExists(opt.consumesAttachPoint.formId)) {
                                // authored provider exists but none installed to satisfy it
                                if (!authoredInstalledProviderExists(opt.consumesAttachPoint.formId)) {
                                    opt.status = "provider-not-installed";
                                } else {
                                    opt.status = "missing-live-attach-point"; // defensive fallback
                                }
                            } else {
                                opt.status = "missing-live-attach-point";
                            }
                        } else if (opt.isStructurallyValid && !opt.isInstalled &&
                                   opt.hasLooseMod && !opt.isAvailableInInventory) {
                            opt.status = "inventory-unavailable";
                        } else if (opt.isStructurallyValid && !opt.isInstalled &&
                                   !opt.hasLooseMod && !GetSettings().allowNoLooseModOptions) {
                            opt.status = "no-loose-mod-disallowed";
                        } else if (opt.isStructurallyValid && !opt.isInstalled) {
                            opt.status = "ready";
                        } else {
                            opt.status = "Unknown";
                        }

                        // After computing final option state, add consumed AP to category diagnostics and update visibility summary
                        if (opt.consumesAttachPoint.formId != 0 &&
                            !ContainsFormId(category.requiredAttachPoints, opt.consumesAttachPoint.formId)) {
                            category.requiredAttachPoints.push_back(opt.consumesAttachPoint);
                        }

                        if (opt.isVisible &&
                            (opt.isInstalled || (opt.isSelectable && opt.isStructurallyValid))) {
                            category.hasVisibleOptions = true;
                        }

                        // Emit a deterministic runtime log line for diagnostic convenience
                        std::ostringstream line;
                        line << "OPT_LOG cat=" << ci << " opt=" << oi << " label=\"" << opt.label << "\" ";
                        line << "OMOD=" << (opt.omod.editorId.empty() ? ToHexFormId(opt.omod.formId) : opt.omod.editorId) << " ";
                        line << "looseMod=" << (opt.looseMod.editorId.empty() ? ToHexFormId(opt.looseMod.formId) : opt.looseMod.editorId) << " ";
                        line << "inventoryAvailable=" << (opt.isAvailableInInventory ? "true" : "false") << " ";
                        line << "consumes=" << (opt.consumesAttachPoint.editorId.empty() ? ToHexFormId(opt.consumesAttachPoint.formId) : opt.consumesAttachPoint.editorId) << " ";
                        line << "provides=" << JoinFormRefEditorIds(opt.providesAttachParentSlots) << " ";
                        line << "installed=" << (opt.isInstalled ? "true" : "false") << " ";
                        line << "structValid=" << (opt.isStructurallyValid ? "true" : "false") << " ";
                        line << "visible=" << (opt.isVisible ? "true" : "false") << " ";
                        line << "selectable=" << (opt.isSelectable ? "true" : "false") << " ";
                        line << "status=\"" << opt.status << "\"";

                        k2040::log::Info(line.str());
                    }
                }

                // After processing categories, derive provider-created APs per category using requiredAttachPoints (preserve required order)
                for (std::size_t cidx = 0; cidx < menu.categories.size(); ++cidx) {
                    auto& cat = menu.categories[cidx];
                    cat.providerCreatedAttachPoints.clear();

                    for (const auto& req : cat.requiredAttachPoints) {
                        if (req.formId == 0) continue;
                        bool provided = false;
                        for (const auto& otherCat : menu.categories) {
                            for (const auto& otherOpt : otherCat.options) {
                                for (const auto& p : otherOpt.providesAttachParentSlots) {
                                    if (p.formId == req.formId) { provided = true; break; }
                                }
                                if (provided) break;
                            }
                            if (provided) break;
                        }

                        if (provided && !ContainsFormId(cat.providerCreatedAttachPoints, req.formId)) {
                            cat.providerCreatedAttachPoints.push_back(req);
                        }
                    }

                    // Compute providerRequired and classify role using FormID identity only and base AP membership
                    std::size_t baseCount = 0;
                    std::size_t providerRequiredCount = 0;
                    std::size_t unresolvedCount = 0;

                    for (const auto& req : cat.requiredAttachPoints) {
                        if (req.formId == 0) continue;
                        const bool isBase = ContainsFormId(weaponInfo.baseAttachParentSlots, req.formId);
                        const bool hasAuthProvider = ContainsFormId(cat.providerCreatedAttachPoints, req.formId);
                        if (isBase) ++baseCount;
                        else if (hasAuthProvider) ++providerRequiredCount;
                        else ++unresolvedCount;
                    }

                    cat.providerRequired = (providerRequiredCount > 0);

                    if (cat.requiredAttachPoints.empty()) {
                        cat.role = EcoCategoryRole::Unknown;
                    } else if (unresolvedCount > 0) {
                        cat.role = EcoCategoryRole::Unknown;
                    } else if (baseCount == cat.requiredAttachPoints.size()) {
                        cat.role = EcoCategoryRole::RootCategory;
                    } else if (providerRequiredCount == cat.requiredAttachPoints.size()) {
                        cat.role = EcoCategoryRole::DependentCategory;
                    } else if (baseCount > 0 && providerRequiredCount > 0) {
                        cat.role = EcoCategoryRole::MixedCategory;
                    } else {
                        cat.role = EcoCategoryRole::Unknown;
                    }
                }

                // Build dependency edges between authored provider options and authored child options.
                menu.dependencyEdges.clear();
                std::vector<std::tuple<size_t,size_t,size_t,size_t,uint32_t>> seenEdges; // providerCat,provOpt,childCat,childOpt,apFormId
                for (std::size_t pci = 0; pci < menu.categories.size(); ++pci) {
                    const auto& pcat = menu.categories[pci];
                    for (std::size_t poi = 0; poi < pcat.options.size(); ++poi) {
                        const auto& prov = pcat.options[poi];
                        if (prov.providesAttachParentSlots.empty()) continue; // only authored providers
                        for (std::size_t cci = 0; cci < menu.categories.size(); ++cci) {
                            const auto& ccat = menu.categories[cci];
                            for (std::size_t coi = 0; coi < ccat.options.size(); ++coi) {
                                const auto& child = ccat.options[coi];
                                if (child.consumesAttachPoint.formId == 0) continue; // child must consume a nonzero AP
                                if (pci == cci && poi == coi) continue; // no self-edges

                                for (const auto& provided : prov.providesAttachParentSlots) {
                                    if (provided.formId == 0) continue;
                                    if (provided.formId != child.consumesAttachPoint.formId) continue;

                                    auto key = std::make_tuple(pci, poi, cci, coi, provided.formId);
                                    if (std::find(seenEdges.begin(), seenEdges.end(), key) != seenEdges.end()) continue; // dedupe exact relationship
                                    seenEdges.push_back(key);

                                    EcoDependencyEdge edge;
                                    edge.providerCategoryIndex = pci;
                                    edge.providerOptionIndex = poi;
                                    edge.childCategoryIndex = cci;
                                    edge.childOptionIndex = coi;
                                    edge.providedAttachPoint = provided;
                                    edge.providerInstalled = prov.isInstalled;
                                    edge.childCurrentlyValid = child.isStructurallyValid;
                                    edge.status = edge.providerInstalled ? std::string("provider-installed") : std::string("provider-missing");
                                    menu.dependencyEdges.push_back(edge);
                                }
                            }
                        }
                    }
                }
            }

            // Successful parse; stop searching further roots.
            break;
        }

        // Final parser status handling
        if (menu.valid) {
            // already set status
            k2040::log::Info(std::string("ECO parser finished: valid=true, status=") + menu.status);
        } else {
            if (!anyKeywordMatched) {
                menu.status = "No matching ECO root found for the equipped weapon.";
            } else if (!lastRejectionReason.empty()) {
                menu.status = lastRejectionReason;
            } else {
                menu.status = "Matching ECO root was found but parsing failed.";
            }

            k2040::log::Info(std::string("ECO parser finished: valid=") + (menu.valid ? "true" : "false") + ", status=" + menu.status);
        }

        return menu;
    }

    EcoWeaponMenu BuildEcoWeaponMenu_ReadOnly(
        const EquippedWeaponInfo& weaponInfo,
        bool includeInventoryUnavailableGeneratedOptions)
    {
        std::string sourceMode = GetSettings().menuSource;
        std::transform(sourceMode.begin(), sourceMode.end(), sourceMode.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        const auto quickMenuPreferences = GetQuickMenuPreferences();
        const auto weaponSourceOverride = GetAuthoredMenuOverride(weaponInfo.weapon);
        const bool forceGenerated = weaponSourceOverride == AuthoredMenuOverride::ForceGenerated;
        const bool forceAuthored = weaponSourceOverride == AuthoredMenuOverride::ForceAuthored;
        const bool globalGenerated = weaponSourceOverride == AuthoredMenuOverride::Inherit &&
            !quickMenuPreferences.useAuthoredMenus;
        if (forceGenerated || (!forceAuthored && (sourceMode == "generatedonly" || globalGenerated))) {
            log::Info(forceGenerated
                ? "Per-weapon preference forces the runtime-generated menu."
                : (globalGenerated
                    ? "Global preference ignores authored ECO menus; using the runtime-generated menu."
                    : "Menu source mode GeneratedOnly: skipping authored ECO lookup."));
            return BuildGenericWeaponMenu_ReadOnly(
                weaponInfo,
                includeInventoryUnavailableGeneratedOptions);
        }

        auto authored = BuildEcoAuthoredWeaponMenu_ReadOnly(weaponInfo);
        if (authored.valid) return authored;

        if (sourceMode == "authoredonly" || forceAuthored) {
            log::Info("Menu source mode AuthoredOnly: runtime-generated fallback disabled.");
            return authored;
        }

        log::Info("No usable authored ECO menu found; attempting runtime-generated fallback.");
        return BuildGenericWeaponMenu_ReadOnly(
            weaponInfo,
            includeInventoryUnavailableGeneratedOptions);
    }

    AttachmentReturnPreparation PrepareAttachmentReturn(const AttachmentMutationRequest& request)
    {
        AttachmentReturnPreparation preparation;

        const auto fail = [&](std::string status, std::string message) {
            preparation.success = false;
            preparation.status = std::move(status);
            preparation.message = std::move(message);
            return preparation;
        };

        const auto weaponInfo = GetEquippedWeaponInfo();
        if (!weaponInfo.hasWeapon || weaponInfo.weapon.formId != request.expectedWeaponFormId) {
            return fail("weapon-changed", "The equipped weapon changed before the attachment could be applied.");
        }

        const auto menu = BuildEcoWeaponMenu_ReadOnly(weaponInfo);
        if (!menu.valid) {
            return fail("menu-revalidation-failed", "The weapon menu is no longer valid.");
        }

        const auto categoryIt = std::find_if(
            menu.categories.begin(), menu.categories.end(),
            [&](const EcoMenuCategory& category) {
                return ContainsFormId(category.requiredAttachPoints, request.consumedAttachPointFormId);
            });
        if (categoryIt == menu.categories.end() || categoryIt->userHidden || !categoryIt->hasVisibleOptions) {
            return fail("category-changed", "The selected attachment category is no longer available.");
        }

        const auto targetIt = std::find_if(
            categoryIt->options.begin(), categoryIt->options.end(),
            [&](const EcoMenuOption& option) {
                return option.omod.formId == request.targetOmodFormId;
            });
        if (targetIt == categoryIt->options.end() || targetIt->userHidden || !targetIt->isVisible ||
            targetIt->isInstalled || !targetIt->isSelectable || !targetIt->isStructurallyValid) {
            return fail("option-changed", "The selected attachment is no longer available.");
        }

        if (!ContainsFormId(menu.liveReachableAttachPoints, request.consumedAttachPointFormId)) {
            return fail("attachment-point-changed", "The attachment point is no longer reachable on the equipped weapon.");
        }

        std::vector<const EcoMenuOption*> installedOptions;
        for (const auto& option : categoryIt->options) {
            if (option.isInstalled && option.consumesAttachPoint.formId == request.consumedAttachPointFormId) {
                installedOptions.push_back(std::addressof(option));
            }
        }
        std::vector<const OmodAttachmentInfo*> liveInstalledAtPoint;
        for (const auto& installed : menu.installedOmodAttachmentInfo) {
            if (installed.consumesAttachPoint.formId != request.consumedAttachPointFormId) {
                continue;
            }

            if (menu.runtimeGenerated && IsAttachmentCollectionOmod(installed.omod.formId)) {
                log::Info(
                    "Ignoring installed attachment collection container for replacement identity: OMOD=" +
                    ToHexFormId(installed.omod.formId) +
                    ", AP=" + ToHexFormId(installed.consumesAttachPoint.formId));
                continue;
            }

            liveInstalledAtPoint.push_back(std::addressof(installed));
        }
        const bool menuInstalledCountAmbiguous = installedOptions.size() > 1;
        const bool liveInstalledCountAmbiguous = liveInstalledAtPoint.size() > 1;
        const bool installedCountMismatch = installedOptions.size() != liveInstalledAtPoint.size();
        const bool installedIdentityMismatch =
            !installedCountMismatch &&
            !installedOptions.empty() &&
            installedOptions.front()->omod.formId != liveInstalledAtPoint.front()->omod.formId;

        if (menuInstalledCountAmbiguous || liveInstalledCountAmbiguous ||
            installedCountMismatch || installedIdentityMismatch) {
            const auto describeRef = [](const FormRef& ref) {
                std::string value = "form=" + ToHexFormId(ref.formId);
                if (!ref.editorId.empty()) {
                    value += "/editor=" + ref.editorId;
                }
                if (!ref.sourcePlugin.empty()) {
                    value += "/source=" + ref.sourcePlugin;
                }
                if (ref.localFormId != 0) {
                    value += "/local=" + ToHexFormId(ref.localFormId);
                }
                return value;
            };

            std::string reasons;
            const auto appendReason = [&](const char* reason) {
                if (!reasons.empty()) {
                    reasons += ",";
                }
                reasons += reason;
            };
            if (menuInstalledCountAmbiguous) appendReason("menu-installed-count>1");
            if (liveInstalledCountAmbiguous) appendReason("live-installed-count>1");
            if (installedCountMismatch) appendReason("menu/live-count-mismatch");
            if (installedIdentityMismatch) appendReason("menu/live-identity-mismatch");

            log::Warn(
                "Attachment ambiguity diagnostic: weapon={" + describeRef(weaponInfo.weapon) +
                "}, category=\"" + categoryIt->label +
                "\", target={" + describeRef(targetIt->omod) +
                "}, selectedAP={" + describeRef(targetIt->consumesAttachPoint) +
                "}, requestAP=" + ToHexFormId(request.consumedAttachPointFormId) +
                ", menuInstalledCount=" + std::to_string(installedOptions.size()) +
                ", liveInstalledCount=" + std::to_string(liveInstalledAtPoint.size()) +
                ", reasons=" + reasons);

            if (installedOptions.empty()) {
                log::Warn("Attachment ambiguity menu-installed candidates: (none)");
            } else {
                for (std::size_t i = 0; i < installedOptions.size(); ++i) {
                    const auto* option = installedOptions[i];
                    log::Warn(
                        "Attachment ambiguity menu-installed[" + std::to_string(i) +
                        "]: omod={" + describeRef(option->omod) +
                        "}, consumesAP={" + describeRef(option->consumesAttachPoint) +
                        "}, label=\"" + option->label +
                        "\", status=" + option->status);
                }
            }

            if (liveInstalledAtPoint.empty()) {
                log::Warn("Attachment ambiguity live-installed candidates: (none)");
            } else {
                for (std::size_t i = 0; i < liveInstalledAtPoint.size(); ++i) {
                    const auto* installed = liveInstalledAtPoint[i];
                    log::Warn(
                        "Attachment ambiguity live-installed[" + std::to_string(i) +
                        "]: omod={" + describeRef(installed->omod) +
                        "}, consumesAP={" + describeRef(installed->consumesAttachPoint) +
                        "}, providesAPCount=" + std::to_string(installed->providesAttachParentSlots.size()));
                }
            }

            return fail("installed-state-ambiguous", "The current attachment state is not safe to replace automatically.");
        }

        const auto* previousOption = installedOptions.empty() ? nullptr : installedOptions.front();
        preparation.previousOmodFormId = previousOption ? previousOption->omod.formId : 0;

        if (!previousOption && !targetIt->providesAttachParentSlots.empty()) {
            return fail("provider-install-locked", "Installing a new attachment provider is not enabled in this build.");
        }

        if (!weaponInfo.equippedInventoryStackFound || weaponInfo.equippedInventoryStackCount != 1) {
            return fail("equipped-stack-ambiguous", "The equipped weapon is not stored as one unambiguous inventory stack.");
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* targetForm = RE::TESForm::GetFormByID(request.targetOmodFormId);
        auto* targetMod = targetForm ? targetForm->As<RE::BGSMod::Attachment::Mod>() : nullptr;
        auto* targetLoose = targetMod ? targetMod->GetLooseMod() : nullptr;
        if (!player || !player->inventoryList || !targetMod) {
            return fail("runtime-form-unavailable", "The live attachment forms could not be resolved safely.");
        }
        if (targetLoose && player->inventoryList->GetItemCount(targetLoose) == 0) {
            return fail("inventory-changed", "The required loose mod is no longer in the inventory.");
        }
        if (!targetLoose && !GetSettings().allowNoLooseModOptions) {
            return fail("loose-mod-required", "This attachment has no loose mod and such actions are disabled.");
        }

        // Rebuild the live attachment graph with the selected provider in place
        // of the current option. Any installed OMOD that was reachable before
        // but is no longer reachable must be removed before its provider.
        std::vector<OmodAttachmentInfo> hypotheticalInstalled;
        for (const auto& installed : menu.installedOmodAttachmentInfo) {
            if (previousOption && installed.omod.formId == previousOption->omod.formId) continue;
            hypotheticalInstalled.push_back(installed);
        }

        OmodAttachmentInfo targetInfo;
        targetInfo.omod = targetIt->omod;
        targetInfo.consumesAttachPoint = targetIt->consumesAttachPoint;
        targetInfo.providesAttachParentSlots = targetIt->providesAttachParentSlots;
        hypotheticalInstalled.push_back(std::move(targetInfo));

        std::vector<FormRef> hypotheticalReachable = weaponInfo.baseAttachParentSlots;
        std::vector<std::uint32_t> admittedOmods;
        bool expanded = true;
        while (expanded) {
            expanded = false;
            for (const auto& installed : hypotheticalInstalled) {
                if (std::find(admittedOmods.begin(), admittedOmods.end(), installed.omod.formId) != admittedOmods.end()) {
                    continue;
                }
                if (!ContainsFormId(hypotheticalReachable, installed.consumesAttachPoint.formId)) continue;
                admittedOmods.push_back(installed.omod.formId);
                const auto before = hypotheticalReachable.size();
                AppendUniqueFormRefs(hypotheticalReachable, installed.providesAttachParentSlots);
                expanded = expanded || hypotheticalReachable.size() != before;
            }
        }

        std::vector<OmodAttachmentInfo> removals;
        for (const auto& installed : menu.installedOmodAttachmentInfo) {
            if (previousOption && installed.omod.formId == previousOption->omod.formId) continue;
            if (menu.runtimeGenerated && IsAttachmentCollectionOmod(installed.omod.formId)) {
                continue;
            }
            const bool reachableNow = ContainsFormId(
                menu.liveReachableAttachPoints,
                installed.consumesAttachPoint.formId);
            const bool reachableAfter = ContainsFormId(
                hypotheticalReachable,
                installed.consumesAttachPoint.formId);
            if (reachableNow && !reachableAfter) {
                removals.push_back(installed);
            }
        }

        // Remove deepest children first. A provider can be removed only after
        // every removal that consumes one of its provided points.
        while (!removals.empty()) {
            const auto leaf = std::find_if(removals.begin(), removals.end(), [&](const OmodAttachmentInfo& candidate) {
                return std::none_of(removals.begin(), removals.end(), [&](const OmodAttachmentInfo& other) {
                    if (other.omod.formId == candidate.omod.formId) return false;
                    return ContainsFormId(candidate.providesAttachParentSlots, other.consumesAttachPoint.formId);
                });
            });
            if (leaf == removals.end()) {
                return fail("dependency-cycle", "The installed attachment dependencies could not be ordered safely.");
            }
            preparation.dependentRemovalOmodFormIds.push_back(leaf->omod.formId);
            removals.erase(leaf);
        }

        const auto addLooseReturn = [&](RE::BGSMod::Attachment::Mod* mod) -> bool {
            if (!mod) return false;
            auto* loose = mod->GetLooseMod();
            if (!loose) return true;
            if (loose == targetLoose) return false;

            const auto looseFormId = loose->GetFormID();
            auto existing = std::find_if(
                preparation.looseReturns.begin(),
                preparation.looseReturns.end(),
                [&](const AttachmentLooseReturn& value) { return value.looseFormId == looseFormId; });
            if (existing != preparation.looseReturns.end()) {
                ++existing->quantity;
            } else {
                AttachmentLooseReturn value;
                value.looseFormId = looseFormId;
                value.countBefore = player->inventoryList->GetItemCount(loose);
                value.quantity = 1;
                preparation.looseReturns.push_back(value);
            }
            return true;
        };

        for (const auto omodFormId : preparation.dependentRemovalOmodFormIds) {
            auto* form = RE::TESForm::GetFormByID(omodFormId);
            auto* mod = form ? form->As<RE::BGSMod::Attachment::Mod>() : nullptr;
            if (!addLooseReturn(mod)) {
                return fail("dependent-return-unavailable", "An installed dependent attachment could not be returned safely.");
            }
        }

        if (previousOption) {
            auto* previousForm = RE::TESForm::GetFormByID(previousOption->omod.formId);
            auto* previousMod = previousForm ? previousForm->As<RE::BGSMod::Attachment::Mod>() : nullptr;
            if (!addLooseReturn(previousMod)) {
                return fail("previous-return-unavailable", "The installed attachment could not be returned safely.");
            }
        }

        preparation.success = true;
        preparation.required = !preparation.looseReturns.empty();
        preparation.status = preparation.required ? "return-required" : "no-return-required";
        preparation.message = preparation.required
            ? "Installed attachments must be returned before the provider changes."
            : "No loose mod needs to be returned before the attachment changes.";
        return preparation;
    }

    bool CancelPreparedAttachmentReturn(const AttachmentMutationRequest& request)
    {
        if (request.preparedLooseReturns.empty()) return true;

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->inventoryList) {
            log::Warn("Could not cancel prepared loose-mod returns because the player inventory was unavailable.");
            return false;
        }

        for (const auto& returned : request.preparedLooseReturns) {
            if (returned.quantity == 0) continue;
            auto* looseForm = RE::TESForm::GetFormByID(returned.looseFormId);
            auto* looseObject = looseForm ? looseForm->As<RE::TESBoundObject>() : nullptr;
            if (!looseObject ||
                player->inventoryList->GetItemCount(looseObject) != returned.countBefore + returned.quantity) {
                log::Warn("Could not cancel prepared loose-mod returns because an inventory count changed unexpectedly.");
                return false;
            }
        }

        bool restored = true;
        for (auto it = request.preparedLooseReturns.rbegin(); it != request.preparedLooseReturns.rend(); ++it) {
            if (it->quantity == 0) continue;
            auto* looseForm = RE::TESForm::GetFormByID(it->looseFormId);
            auto* looseObject = looseForm ? looseForm->As<RE::TESBoundObject>() : nullptr;
            RE::TESObjectREFR::RemoveItemData removeData(looseObject, it->quantity);
            removeData.reason = RE::ITEM_REMOVE_REASON::kNone;
            player->RemoveItem(removeData);
            restored = restored &&
                player->inventoryList->GetItemCount(looseObject) == it->countBefore;
        }
        log::Info(std::string("Prepared loose-mod return cancellation: ") + (restored ? "restored" : "failed") + ".");
        return restored;
    }

    AttachmentMutationResult ApplySimpleAttachmentChange(const AttachmentMutationRequest& request)
    {
        AttachmentMutationResult result;

        auto fail = [&](std::string status, std::string message, bool cancelPreparedReturn = true) {
            if (cancelPreparedReturn && !request.preparedLooseReturns.empty() &&
                !CancelPreparedAttachmentReturn(request)) {
                log::Warn("The prepared loose-mod returns could not be cancelled after the attachment transaction failed.");
            }
            result.success = false;
            result.status = std::move(status);
            result.message = std::move(message);
            return result;
        };

        result.weaponInfo = GetEquippedWeaponInfo();
        if (!result.weaponInfo.hasWeapon ||
            result.weaponInfo.weapon.formId != request.expectedWeaponFormId) {
            return fail("weapon-changed", "The equipped weapon changed before the attachment could be applied.");
        }

        result.menu = BuildEcoWeaponMenu_ReadOnly(result.weaponInfo);
        if (!result.menu.valid) {
            return fail("menu-revalidation-failed", "The weapon menu is no longer valid.");
        }

        const auto categoryIt = std::find_if(
            result.menu.categories.begin(), result.menu.categories.end(),
            [&](const EcoMenuCategory& category) {
                return ContainsFormId(category.requiredAttachPoints, request.consumedAttachPointFormId);
            });
        if (categoryIt == result.menu.categories.end() || categoryIt->userHidden || !categoryIt->hasVisibleOptions) {
            return fail("category-changed", "The selected attachment category is no longer available.");
        }

        const auto targetIt = std::find_if(
            categoryIt->options.begin(), categoryIt->options.end(),
            [&](const EcoMenuOption& option) {
                return option.omod.formId == request.targetOmodFormId;
            });
        if (targetIt == categoryIt->options.end() || targetIt->userHidden || !targetIt->isVisible ||
            targetIt->isInstalled || !targetIt->isSelectable || !targetIt->isStructurallyValid) {
            return fail("option-changed", "The selected attachment is no longer available.");
        }

        if (!ContainsFormId(result.menu.liveReachableAttachPoints, request.consumedAttachPointFormId)) {
            return fail(
                "attachment-point-changed",
                "The attachment point is no longer reachable on the equipped weapon.");
        }

        if (!result.weaponInfo.equippedInventoryStackFound ||
            result.weaponInfo.equippedInventoryStackCount != 1) {
            return fail(
                "equipped-stack-ambiguous",
                "The equipped weapon is not stored as one unambiguous inventory stack.");
        }

        const auto revalidatedPlan = PrepareAttachmentReturn(request);
        if (!revalidatedPlan.success) {
            return fail(revalidatedPlan.status, revalidatedPlan.message);
        }
        if (revalidatedPlan.previousOmodFormId != request.expectedPreviousOmodFormId ||
            revalidatedPlan.dependentRemovalOmodFormIds != request.dependentRemovalOmodFormIds ||
            revalidatedPlan.looseReturns.size() != request.preparedLooseReturns.size()) {
            return fail("dependency-state-changed", "The installed attachment dependencies changed before the weapon could be updated.");
        }
        for (std::size_t index = 0; index < revalidatedPlan.looseReturns.size(); ++index) {
            const auto& expected = revalidatedPlan.looseReturns[index];
            const auto& prepared = request.preparedLooseReturns[index];
            if (prepared.quantity == 0 || expected.looseFormId != prepared.looseFormId ||
                expected.quantity != prepared.quantity) {
                return fail("prepared-return-changed", "The prepared loose-mod returns changed before the weapon could be updated.");
            }
        }

        const std::uint32_t previousOmodFormId = request.expectedPreviousOmodFormId;

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* weaponForm = RE::TESForm::GetFormByID(request.expectedWeaponFormId);
        auto* targetForm = RE::TESForm::GetFormByID(request.targetOmodFormId);
        auto* previousForm = previousOmodFormId != 0 ? RE::TESForm::GetFormByID(previousOmodFormId) : nullptr;
        auto* targetMod = targetForm ? targetForm->As<RE::BGSMod::Attachment::Mod>() : nullptr;
        auto* previousMod = previousForm ? previousForm->As<RE::BGSMod::Attachment::Mod>() : nullptr;
        auto* weapon = weaponForm ? weaponForm->As<RE::TESObjectWEAP>() : nullptr;
        auto* targetLoose = targetMod ? targetMod->GetLooseMod() : nullptr;

        if (!player || !player->inventoryList || !weapon || !targetMod ||
            (previousOmodFormId != 0 && !previousMod)) {
            return fail("runtime-form-unavailable", "The live weapon or attachment forms could not be resolved safely.");
        }
        if (!targetLoose && !GetSettings().allowNoLooseModOptions) {
            return fail("loose-mod-required", "This attachment has no loose mod and such actions are disabled.");
        }

        const auto inventoryCount = [&](RE::TESBoundObject* object) -> std::uint32_t {
            return object && player->inventoryList ? player->inventoryList->GetItemCount(object) : 0;
        };

        const std::uint32_t targetLooseBefore = inventoryCount(targetLoose);
        if (targetLoose && targetLooseBefore == 0) {
            return fail("inventory-changed", "The required loose mod is no longer in the inventory.");
        }

        for (const auto& returned : request.preparedLooseReturns) {
            auto* returnedForm = RE::TESForm::GetFormByID(returned.looseFormId);
            auto* returnedObject = returnedForm ? returnedForm->As<RE::TESBoundObject>() : nullptr;
            if (!returnedObject || returnedObject == targetLoose ||
                inventoryCount(returnedObject) != returned.countBefore + returned.quantity) {
                return fail("prepared-return-changed", "A returned loose mod changed before the attachment could be applied.");
            }
        }

        std::vector<RE::BGSMod::Attachment::Mod*> dependentMods;
        dependentMods.reserve(request.dependentRemovalOmodFormIds.size());
        for (const auto omodFormId : request.dependentRemovalOmodFormIds) {
            auto* form = RE::TESForm::GetFormByID(omodFormId);
            auto* mod = form ? form->As<RE::BGSMod::Attachment::Mod>() : nullptr;
            if (!mod) {
                return fail("dependent-form-unavailable", "An installed dependent attachment could not be resolved safely.");
            }
            dependentMods.push_back(mod);
        }

        const auto modifyEquippedStack = [&](RE::BGSMod::Attachment::Mod* mod, bool attach) {
            if (!mod) return;
            RE::BGSInventoryItem::CheckStackIDFunctor compare(result.weaponInfo.equippedInventoryStackIndex);
            bool engineSuccess = false;
            RE::BGSInventoryItem::ModifyModDataFunctor write(
                mod,
                static_cast<std::int8_t>(result.weaponInfo.equippedSlotIndex),
                attach,
                std::addressof(engineSuccess));
            write.shouldSplitStacks = false;
            player->FindAndWriteStackDataForInventoryItem(weapon, compare, write);
            log::Info(
                "ModifyModDataFunctor completed: operation=" + std::string(attach ? "attach" : "detach") +
                ", OMOD=" + ToHexFormId(mod->GetFormID()) +
                ", engineSuccess=" + (engineSuccess ? std::string("true") : std::string("false")) +
                ", foundObject=" + (write.foundObj ? ToHexFormId(write.foundObj->GetFormID()) : std::string("none")) + ".");
        };

        const auto containsInstalled = [](const EquippedWeaponInfo& info, std::uint32_t formId) {
            return ContainsFormId(info.installedObjectInstanceMods, formId);
        };

        const auto removeOne = [&](RE::TESBoundObject* object) {
            if (!object || inventoryCount(object) == 0) return false;
            const auto before = inventoryCount(object);
            RE::TESObjectREFR::RemoveItemData removeData(object, 1);
            removeData.reason = RE::ITEM_REMOVE_REASON::kNone;
            player->RemoveItem(removeData);
            return inventoryCount(object) + 1 == before;
        };

        const auto addOne = [&](RE::TESBoundObject* object) {
            if (!object) return true;
            const auto before = inventoryCount(object);
            return ReturnLooseModThroughPickup(object->GetFormID()) &&
                inventoryCount(object) == before + 1;
        };

        const auto restoreInventoryCount = [&](RE::TESBoundObject* object, std::uint32_t expected) {
            if (!object) return expected == 0;
            auto current = inventoryCount(object);
            if (current == expected) return true;
            if (current + 1 == expected) return addOne(object);
            if (current == expected + 1) return removeOne(object);
            return false;
        };

        const auto rollback = [&](const char* reason) {
            log::Warn(std::string("Rolling back guarded attachment mutation: ") + reason + ".");
            if (previousMod) {
                modifyEquippedStack(previousMod, true);
            } else {
                modifyEquippedStack(targetMod, false);
            }
            for (auto it = dependentMods.rbegin(); it != dependentMods.rend(); ++it) {
                modifyEquippedStack(*it, true);
            }
            const bool targetInventoryRestored = restoreInventoryCount(targetLoose, targetLooseBefore);
            const auto rollbackInfo = GetEquippedWeaponInfo();
            bool attachmentRestored = !containsInstalled(rollbackInfo, request.targetOmodFormId);
            if (previousOmodFormId != 0) {
                attachmentRestored = attachmentRestored && containsInstalled(rollbackInfo, previousOmodFormId);
            } else {
                const auto rollbackMenu = BuildEcoWeaponMenu_ReadOnly(rollbackInfo);
                attachmentRestored = attachmentRestored && rollbackMenu.valid && std::none_of(
                    rollbackMenu.installedOmodAttachmentInfo.begin(),
                    rollbackMenu.installedOmodAttachmentInfo.end(),
                    [&](const OmodAttachmentInfo& installed) {
                        return installed.consumesAttachPoint.formId == request.consumedAttachPointFormId;
                    });
            }
            const bool dependentsRestored = std::all_of(
                request.dependentRemovalOmodFormIds.begin(),
                request.dependentRemovalOmodFormIds.end(),
                [&](std::uint32_t formId) { return containsInstalled(rollbackInfo, formId); });
            log::Info(
                "Attachment rollback verification: attachment=" + std::string(attachmentRestored ? "restored" : "failed") +
                ", targetInventory=" + (targetInventoryRestored ? std::string("restored") : std::string("failed")) +
                ", dependents=" + (dependentsRestored ? std::string("restored") : std::string("failed")) + ".");
            return attachmentRestored && targetInventoryRestored && dependentsRestored;
        };

        for (auto* dependent : dependentMods) {
            modifyEquippedStack(dependent, false);
        }
        modifyEquippedStack(targetMod, true);

        auto changedInfo = GetEquippedWeaponInfo();
        const bool targetInstalled = containsInstalled(changedInfo, request.targetOmodFormId);
        const bool previousRemoved = previousOmodFormId == 0 || !containsInstalled(changedInfo, previousOmodFormId);
        const bool dependentsRemoved = std::none_of(
            request.dependentRemovalOmodFormIds.begin(),
            request.dependentRemovalOmodFormIds.end(),
            [&](std::uint32_t formId) { return containsInstalled(changedInfo, formId); });
        if (!targetInstalled || !previousRemoved || !dependentsRemoved) {
            const bool rollbackPassed = rollback("post-change attachment identity did not match the requested replacement");
            return fail(
                rollbackPassed ? "attachment-change-rejected" : "rollback-failed",
                rollbackPassed
                    ? "The engine rejected the attachment change and the original state was restored."
                    : "The attachment change failed and its rollback could not be fully verified. Reload the test save before continuing.",
                rollbackPassed);
        }

        const std::uint32_t expectedTargetLoose = targetLoose ? targetLooseBefore - 1 : 0;
        if (targetLoose) {
            const auto targetAfterEngine = inventoryCount(targetLoose);
            if (targetAfterEngine == targetLooseBefore) {
                if (!removeOne(targetLoose)) {
                    const bool rollbackPassed = rollback("the selected loose mod could not be consumed");
                    return fail(
                        rollbackPassed ? "inventory-change-rejected" : "rollback-failed",
                        rollbackPassed
                            ? "The loose mod could not be consumed and the original state was restored."
                            : "Inventory adjustment failed and rollback could not be fully verified. Reload the test save before continuing.",
                        rollbackPassed);
                }
            } else if (targetAfterEngine != expectedTargetLoose) {
                const bool rollbackPassed = rollback("the removed loose-mod count changed unexpectedly");
                return fail(
                    rollbackPassed ? "inventory-change-rejected" : "rollback-failed",
                    rollbackPassed
                        ? "The inventory changed unexpectedly and the original state was restored."
                        : "The inventory changed unexpectedly and rollback could not be fully verified. Reload the test save before continuing.",
                    rollbackPassed);
            }
        }

        const bool returnedInventoryPreserved = std::all_of(
            request.preparedLooseReturns.begin(),
            request.preparedLooseReturns.end(),
            [&](const AttachmentLooseReturn& returned) {
                auto* form = RE::TESForm::GetFormByID(returned.looseFormId);
                auto* object = form ? form->As<RE::TESBoundObject>() : nullptr;
                return object && inventoryCount(object) == returned.countBefore + returned.quantity;
            });
        if (!returnedInventoryPreserved) {
            const bool rollbackPassed = rollback("a returned loose-mod count changed unexpectedly");
            return fail(
                rollbackPassed ? "inventory-change-rejected" : "rollback-failed",
                rollbackPassed
                    ? "The inventory changed unexpectedly and the original state was restored."
                    : "The inventory changed unexpectedly and rollback could not be fully verified. Reload the test save before continuing.",
                rollbackPassed);
        }

        result.weaponInfo = GetEquippedWeaponInfo();
        const bool finalTargetInstalled = containsInstalled(result.weaponInfo, request.targetOmodFormId);
        const bool finalPreviousRemoved = previousOmodFormId == 0 || !containsInstalled(result.weaponInfo, previousOmodFormId);
        const bool finalDependentsRemoved = std::none_of(
            request.dependentRemovalOmodFormIds.begin(),
            request.dependentRemovalOmodFormIds.end(),
            [&](std::uint32_t formId) { return containsInstalled(result.weaponInfo, formId); });
        const bool finalTargetInventory = !targetLoose || inventoryCount(targetLoose) == expectedTargetLoose;
        const bool finalReturnedInventory = std::all_of(
            request.preparedLooseReturns.begin(),
            request.preparedLooseReturns.end(),
            [&](const AttachmentLooseReturn& returned) {
                auto* form = RE::TESForm::GetFormByID(returned.looseFormId);
                auto* object = form ? form->As<RE::TESBoundObject>() : nullptr;
                return object && inventoryCount(object) == returned.countBefore + returned.quantity;
            });

        if (!finalTargetInstalled || !finalPreviousRemoved || !finalDependentsRemoved ||
            !finalTargetInventory || !finalReturnedInventory) {
            const bool rollbackPassed = rollback("final attachment or inventory verification failed");
            return fail(
                rollbackPassed ? "final-verification-failed" : "rollback-failed",
                rollbackPassed
                    ? "The final check failed and the original state was restored."
                    : "The final check failed and rollback could not be fully verified. Reload the test save before continuing.",
                rollbackPassed);
        }

        result.menu = BuildEcoWeaponMenu_ReadOnly(result.weaponInfo);
        if (!result.menu.valid) {
            const bool rollbackPassed = rollback("the refreshed menu could not be rebuilt");
            return fail(
                rollbackPassed ? "refresh-failed" : "rollback-failed",
                rollbackPassed
                    ? "The refreshed menu could not be rebuilt and the original state was restored."
                    : "The refreshed menu failed and rollback could not be fully verified. Reload the test save before continuing.",
                rollbackPassed);
        }

        result.success = true;
        result.status = "attachment-applied";
        result.message = "Attachment equipped.";
        log::Info(
            "Guarded attachment transaction verified: weapon=" + ToHexFormId(request.expectedWeaponFormId) +
            ", previousOMOD=" + (previousOmodFormId != 0 ? ToHexFormId(previousOmodFormId) : std::string("none")) +
            ", targetOMOD=" + ToHexFormId(request.targetOmodFormId) +
            ", targetLoose=" + (targetLoose
                ? std::to_string(targetLooseBefore) + "->" + std::to_string(expectedTargetLoose)
                : std::string("none")) +
            ", removedDependents=" + std::to_string(request.dependentRemovalOmodFormIds.size()) +
            ", returnedLooseGroups=" + std::to_string(request.preparedLooseReturns.size()) + ".");
        return result;
    }
}
