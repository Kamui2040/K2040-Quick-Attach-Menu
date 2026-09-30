#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace RE
{
    class TESForm;
    class TESObjectWEAP;
    class Actor;
}

namespace k2040
{
    struct FormRef
    {
        uint32_t formId = 0;
        uint32_t localFormId = 0;
        std::string editorId;
        std::string displayName;
        std::string sourcePlugin;
        std::string persistentKey;
        bool persistentIdentityValid = false;
        std::string persistentIdentityStatus;
    };

    struct OmodAttachmentInfo
    {
        FormRef omod;
        FormRef consumesAttachPoint;
        std::vector<FormRef> providesAttachParentSlots;

        uint32_t templateItemIndex = 0;
        uint32_t attachmentIndex = 0;
        uint32_t rank = 0;

        bool templateItemIsDefault = false;
        bool optional = false;
        bool childrenExclusive = false;
    };

    struct WeaponInstanceDataProbe
    {
        bool present = false;
        std::string status;

        // Read-only fields copied from TESObjectWEAP::InstanceData.
        // These describe the already-applied live weapon instance data, but
        // they do not identify exact installed OMOD FormIDs.
        FormRef ammo;

        bool hasEquipSlot = false;
        bool hasAimModel = false;
        bool hasZoomData = false;
        bool hasImpactDataSet = false;
        bool hasRangedData = false;
        bool hasKeywordData = false;

        uint32_t value = 0;
        uint16_t attackDamage = 0;
        uint16_t ammoCapacity = 0;
        uint16_t rank = 0;

        float weight = 0.0f;
        float speed = 0.0f;
        float reach = 0.0f;
        float minRange = 0.0f;
        float maxRange = 0.0f;
        float attackDelaySec = 0.0f;
        float reloadSpeed = 0.0f;
        float attackActionPointCost = 0.0f;
        float colorRemappingIndex = 0.0f;
    };

    struct EquippedWeaponInfo
    {
        bool hasWeapon = false;
        FormRef weapon;
        uint32_t equippedSlotIndex = 0;
        bool equippedInventoryStackFound = false;
        uint32_t equippedInventoryStackIndex = 0;
        uint32_t equippedInventoryStackCount = 0;
        std::vector<FormRef> baseAttachParentSlots;
        std::vector<OmodAttachmentInfo> defaultTemplateMods;
        std::vector<FormRef> defaultTemplateProvidedSlots;
        std::vector<FormRef> currentAvailableSlotsPreview;

        // Resolved live installed OMODs from BGSObjectInstanceExtra/ObjectIndexData.
        // This is the current best read-only runtime source for installed
        // attachment identity on the equipped inventory stack.
        std::vector<FormRef> installedObjectInstanceMods;
        std::string installedObjectInstanceProbeStatus;
        uint32_t installedObjectInstanceRawCount = 0;
        uint32_t installedObjectInstanceResolvedCount = 0;

        // Read-only live equipped-instance probe.
        // This is not yet a full installed-OMOD list. It verifies whether
        // Actor::GetEquippedItem returns instance data and exposes instance
        // keyword data that may help identify current live state.
        bool equippedInstanceDataPresent = false;
        uint32_t equippedInstanceKeywordCount = 0;
        std::vector<FormRef> equippedInstanceKeywords;

        // Diagnostic-only keyword buckets.
        // DN keywords are useful hints on some weapons, but they are not a final
        // installed-OMOD source of truth because many weapons do not use them.
        std::vector<FormRef> equippedInstanceDnKeywords;
        std::vector<FormRef> equippedInstanceMaKeywords;
        std::vector<FormRef> equippedInstanceAnimKeywords;
        std::vector<FormRef> equippedInstanceWeaponTypeKeywords;
        std::vector<FormRef> equippedInstanceGameplayKeywords;
        std::vector<FormRef> equippedInstanceOtherKeywords;

        std::string equippedInstanceProbeStatus;
        std::string equippedInstanceStateReliability;

        // Direct live instance-data output probe.
        // Useful for confirming applied stats/data, but not direct OMOD identity.
        WeaponInstanceDataProbe liveWeaponInstanceData;

        std::string status;
    };

    enum class EcoOptionRole
    {
        Unknown,
        LeafOption,
        ProviderOption,
        ToggleOption
    };

    enum class EcoCategoryRole
    {
        Unknown,
        RootCategory,
        DependentCategory,
        MixedCategory
    };

    struct EcoMenuOption
    {
        uint32_t optionIndex = 0;
        uint32_t messageButtonIndex = 0;

        std::string label;
        std::string sourceLabel;

        FormRef omod;
        FormRef looseMod;

        FormRef consumesAttachPoint;
        std::vector<FormRef> providesAttachParentSlots;
        std::vector<FormRef> targetKeywords;

        EcoOptionRole role = EcoOptionRole::Unknown;

        bool hasLooseMod = false;
        bool looseModRequired = true;
        bool isInstalled = false;
        bool isAvailableInInventory = false;

        // Structural and UI state derived from live APs and providers
        bool isStructurallyValid = false;
        bool isVisible = true;
        bool isSelectable = false;
        bool userHidden = false;
        bool labelCustomized = false;

        // Deterministic status/block reason (explicit string, not encoded only in booleans)
        std::string status;
    };

    struct EcoMenuCategory
    {
        uint32_t categoryIndex = 0;
        uint32_t messageButtonIndex = 0;

        std::string label;
        std::string sourceLabel;

        FormRef categoryMessage;
        FormRef optionFormList;
        // Stable source-neutral identity used by persisted visibility rules.
        // Authored menus prefer their option FormList; generated menus use the
        // consumed attachment-point keyword.
        FormRef persistenceIdentity;

        std::vector<EcoMenuOption> options;

        EcoCategoryRole role = EcoCategoryRole::Unknown;

        std::vector<FormRef> requiredAttachPoints;
        std::vector<FormRef> providerCreatedAttachPoints;

        bool providerRequired = false;
        bool hasVisibleOptions = false;
        bool userHidden = false;
        bool labelCustomized = false;
    };

    struct EcoDependencyEdge
    {
        size_t providerCategoryIndex = 0;
        size_t providerOptionIndex = 0;

        size_t childCategoryIndex = 0;
        size_t childOptionIndex = 0;

        FormRef providedAttachPoint;

        bool providerInstalled = false;
        bool childCurrentlyValid = false;

        // Diagnostic explanation for the edge state
        std::string status;
    };

    struct EcoWeaponMenu
    {
        FormRef weapon;
        FormRef weaponKeyword;

        FormRef rootFormList;
        FormRef weaponMenuMessage;
        FormRef categoryMessageList;
        FormRef categoryOptionList;

        std::vector<EcoMenuCategory> categories;
        std::vector<EcoDependencyEdge> dependencyEdges;

        // Resolved live attach-point previews and installed OMOD attachment metadata
        std::vector<FormRef> graphPreviewAttachPoints;
        std::vector<FormRef> liveReachableAttachPoints;
        std::vector<OmodAttachmentInfo> installedOmodAttachmentInfo;

        bool valid = false;
        bool runtimeGenerated = false;
        bool userHidden = false;
        bool registeredInUserSettings = false;
        std::string status;
    };

    struct AttachmentLooseReturn
    {
        uint32_t looseFormId = 0;
        uint32_t countBefore = 0;
        uint32_t quantity = 0;
    };

    struct AttachmentMutationRequest
    {
        uint32_t expectedWeaponFormId = 0;
        uint32_t targetOmodFormId = 0;
        uint32_t consumedAttachPointFormId = 0;
        uint32_t expectedPreviousOmodFormId = 0;
        std::vector<uint32_t> dependentRemovalOmodFormIds;
        std::vector<AttachmentLooseReturn> preparedLooseReturns;
    };

    struct AttachmentReturnPreparation
    {
        bool success = false;
        bool required = false;
        std::string status;
        std::string message;
        uint32_t previousOmodFormId = 0;
        std::vector<uint32_t> dependentRemovalOmodFormIds;
        std::vector<AttachmentLooseReturn> looseReturns;
    };

    struct AttachmentMutationResult
    {
        bool success = false;
        std::string status;
        std::string message;
        EquippedWeaponInfo weaponInfo;
        EcoWeaponMenu menu;
    };

    EquippedWeaponInfo GetEquippedWeaponInfo();
    EcoWeaponMenu BuildEcoWeaponMenu_ReadOnly(
        const EquippedWeaponInfo& weaponInfo,
        bool includeInventoryUnavailableGeneratedOptions = false);
    bool ReturnLooseModThroughPickup(uint32_t looseModFormId);
    AttachmentReturnPreparation PrepareAttachmentReturn(const AttachmentMutationRequest& request);
    bool CancelPreparedAttachmentReturn(const AttachmentMutationRequest& request);
    AttachmentMutationResult ApplySimpleAttachmentChange(const AttachmentMutationRequest& request);

    std::string ToHexFormId(uint32_t formId);
}
