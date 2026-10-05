#pragma once

#include "RuntimeState.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace k2040
{
    enum class BracketedTextOverride
    {
        Inherit,
        Hide,
        Show
    };

    enum class AuthoredMenuOverride
    {
        Inherit,
        ForceAuthored,
        ForceGenerated
    };

    struct BracketedTextPreferences
    {
        bool hidePrefixes = false;
        bool hideInfixes = false;
        bool hideSuffixes = false;
    };

    struct QuickMenuLayoutSettings
    {
        double scale = 1.0;
        double positionX = 0.5;
        double positionY = 0.5;
    };

    struct QuickMenuPreferences
    {
        std::string presentation = "cascade";
        double backgroundOpacity = 0.92;
        std::string theme = "default";
        std::string customAccent = "#66c4ff";
        std::string customText = "#f4faff";
        std::string customPanel = "#0b1218";
        std::string customInstalled = "#74e398";
        bool useAuthoredMenus = true;
        std::string controlHints = "contextual";
        bool closeAfterApply = false;
        bool loggingEnabled = true;
        double menuSlowdown = 1.0;
        double builderPanelWidth = 0.0;
        double builderPanelHeight = 0.0;
        double settingsPanelWidth = 0.0;
        double settingsPanelHeight = 0.0;
        QuickMenuLayoutSettings cascade;
        QuickMenuLayoutSettings radial;
        QuickMenuLayoutSettings hybrid;
        QuickMenuLayoutSettings horizontal;
    };

    enum class UserSettingsLoadState
    {
        Uninitialized,
        Missing,
        LoadedSupported,
        Malformed,
        UnsupportedVersion,
        ReadError
    };

    struct UserSettingsDiagnostics
    {
        std::filesystem::path path;
        UserSettingsLoadState loadState = UserSettingsLoadState::Uninitialized;
        bool gameDataValidated = false;
        bool validationEnvironmentAvailable = false;
        bool saveAllowed = false;
        bool dirty = false;
        bool loaded = false;
        bool missing = false;
        bool saved = false;
        std::string status;
        std::string loadStateText;
        std::string blockedSaveReason;
        std::size_t weaponCount = 0;
        std::size_t malformedRecordCount = 0;
        std::size_t unsupportedVersionCount = 0;
        std::size_t duplicateRecordCount = 0;
        std::size_t unresolvedRecordCount = 0;
        std::size_t staleDocumentRecordCount = 0;
        std::size_t currentMenuStaleRecordCount = 0;
    };

    struct WeaponMenuProfileInfo
    {
        std::size_t index = 0;
        std::string profileName;
        std::string fileName;
        std::string targetPlugin;
        std::uint32_t targetLocalFormId = 0;
    };

    struct WeaponMenuProfileCatalog
    {
        std::vector<WeaponMenuProfileInfo> profiles;
        std::size_t invalidFileCount = 0;
        std::size_t otherWeaponCount = 0;
    };

    struct WeaponMenuProfileResult
    {
        bool success = false;
        std::string status;
        std::string message;
        std::string fileName;
    };

    std::filesystem::path GetUserSettingsPath();
    std::filesystem::path GetWeaponMenuProfileDirectory();
    std::filesystem::path GetWeaponMenuExportDirectory();

    void LoadUserPreferences();
    void ValidateUserPreferencesAgainstLoadedForms();
    bool SaveUserPreferences();
    bool RecoverUserPreferencesWithDefaults();
    bool CleanupUnresolvedUserPreferences();
    const UserSettingsDiagnostics& GetUserSettingsDiagnostics();

    bool RegisterOrUpdateWeapon(const EcoWeaponMenu& menu);
    bool IsWeaponRegistered(const FormRef& weapon);

    bool IsWeaponHidden(const FormRef& weapon);
    bool IsCategoryHidden(const FormRef& weapon, const EcoMenuCategory& category);
    bool IsOptionHidden(const FormRef& weapon, const FormRef& omod);

    bool SetWeaponHidden(const FormRef& weapon, bool hidden);
    bool SetCategoryHidden(const FormRef& weapon, const EcoMenuCategory& category, bool hidden);
    bool SetOptionHidden(const FormRef& weapon, const FormRef& omod, bool hidden);
    bool ResetWeaponVisibility(const FormRef& weapon);
    bool ResetAllVisibilityPreferences();
    bool SaveMenuOrder(const EcoWeaponMenu& menu);
    bool ResetWeaponOrder(const FormRef& weapon);
    bool SetCategoryLabel(const FormRef& weapon, const EcoMenuCategory& category, const std::string& label);
    bool SetOptionLabel(const FormRef& weapon, const EcoMenuOption& option, const std::string& label);
    bool ResetWeaponLabels(const FormRef& weapon);
    AuthoredMenuOverride GetAuthoredMenuOverride(const FormRef& weapon);
    bool SetAuthoredMenuOverride(const FormRef& weapon, AuthoredMenuOverride value);
    const char* AuthoredMenuOverrideName(AuthoredMenuOverride value);
    bool IsAuthoredMenuIgnored(const FormRef& weapon);
    bool SetAuthoredMenuIgnored(const FormRef& weapon, bool ignored);
    BracketedTextOverride GetBracketedTextOverride(const FormRef& weapon);
    bool SetBracketedTextOverride(const FormRef& weapon, BracketedTextOverride value);
    const char* BracketedTextOverrideName(BracketedTextOverride value);
    bool GetForceUnsafeSwaps(const FormRef& weapon);
    bool SetForceUnsafeSwaps(const FormRef& weapon, bool enabled);
    BracketedTextPreferences GetBracketedTextPreferences();
    bool SetBracketedTextPreferences(const BracketedTextPreferences& value);
    QuickMenuPreferences GetQuickMenuPreferences();
    bool SetQuickMenuPreferences(const QuickMenuPreferences& value);
    bool ResetGeneralPreferences();
    bool ResetAllUserPreferences();

    WeaponMenuProfileCatalog FindWeaponMenuProfiles(const FormRef& weapon);
    WeaponMenuProfileResult ExportWeaponMenuProfile(const EcoWeaponMenu& menu);
    WeaponMenuProfileResult ImportWeaponMenuProfile(const EcoWeaponMenu& menu, std::size_t profileIndex);

    void ApplyVisibilityPreferences(EcoWeaponMenu& menu);
}
