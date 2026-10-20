#include "UserSettings.h"

#include "Logger.h"
#include "Settings.h"

#include <RE/Fallout.h>
#include <RE/T/TESDataHandler.h>

#include <Windows.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
    constexpr int kSchemaVersion = 2;
    constexpr int kOldestSupportedSchemaVersion = 1;
    constexpr int kWeaponMenuProfileSchemaVersion = 1;
    constexpr std::string_view kWeaponMenuProfileFormat = "K2040QuickAttachWeaponMenu";
    constexpr std::uintmax_t kMaximumWeaponMenuProfileBytes = 1024u * 1024u;
    constexpr std::size_t kMaximumWeaponMenuProfileFiles = 256;

    enum class ExpectedFormType
    {
        Weapon,
        CategoryMessage,
        CategoryOptionList,
        CategoryAttachPoint,
        OptionMod
    };

    struct PersistedIdentity
    {
        std::string plugin;
        std::uint32_t localFormId = 0;
        std::string key;
        bool parseMalformed = false;
        bool resolved = false;

        bool IsSyntacticallyValid() const
        {
            return !parseMalformed && !plugin.empty() && localFormId != 0 && !key.empty() && key == MakeKey(plugin, localFormId);
        }

        bool IsUsable() const
        {
            return IsSyntacticallyValid() && resolved;
        }

        static std::string MakeKey(const std::string& pluginName, std::uint32_t localId)
        {
            if (pluginName.empty() || localId == 0) {
                return {};
            }
            return pluginName + ":0x" + k2040::ToHexFormId(localId);
        }
    };

    struct PersistedCategoryIdentity
    {
        PersistedIdentity categoryMessage;
        PersistedIdentity optionFormList;
        PersistedIdentity persistenceIdentity;
        bool parseMalformed = false;

        bool IsSyntacticallyValid() const
        {
            return !parseMalformed && (persistenceIdentity.IsSyntacticallyValid() ||
                optionFormList.IsSyntacticallyValid() || categoryMessage.IsSyntacticallyValid());
        }

        bool IsUsable() const
        {
            return !parseMalformed &&
                (persistenceIdentity.IsUsable() ||
                    (!persistenceIdentity.IsSyntacticallyValid() && optionFormList.IsUsable()) ||
                    (!persistenceIdentity.IsSyntacticallyValid() && !optionFormList.IsSyntacticallyValid() && categoryMessage.IsUsable()));
        }

        std::string CanonicalKey() const
        {
            if (persistenceIdentity.IsSyntacticallyValid()) {
                return persistenceIdentity.key;
            }
            if (optionFormList.IsSyntacticallyValid()) {
                return optionFormList.key;
            }
            return categoryMessage.key;
        }
    };

    struct WeaponPreferences
    {
        PersistedIdentity weapon;
        std::string diagnosticName;
        bool hidden = false;
        k2040::AuthoredMenuOverride authoredMenuOverride = k2040::AuthoredMenuOverride::Inherit;
        k2040::BracketedTextOverride bracketedTextOverride = k2040::BracketedTextOverride::Inherit;
        bool forceUnsafeSwaps = false;
        bool parseMalformed = false;
        std::vector<PersistedCategoryIdentity> hiddenCategories;
        std::vector<PersistedIdentity> hiddenOptions;
        std::vector<PersistedCategoryIdentity> categoryOrder;
        std::vector<PersistedIdentity> optionOrder;

        struct CategoryLabel
        {
            PersistedCategoryIdentity category;
            std::string label;
            bool parseMalformed = false;
        };

        struct OptionLabel
        {
            PersistedIdentity option;
            std::string label;
            bool parseMalformed = false;
        };

        std::vector<CategoryLabel> categoryLabels;
        std::vector<OptionLabel> optionLabels;
    };

    struct ParsedWeaponMenuProfile
    {
        WeaponPreferences weapon;
        std::string profileName;
        std::filesystem::path path;
    };

    struct PreferencesDocument
    {
        struct GeneralPreferences
        {
            struct Layout
            {
                std::optional<double> scale;
                std::optional<double> positionX;
                std::optional<double> positionY;

                bool HasExplicitValues() const
                {
                    return scale.has_value() || positionX.has_value() || positionY.has_value();
                }
            };

            std::optional<bool> hideBracketedPrefixes;
            std::optional<bool> hideBracketedInfixes;
            std::optional<bool> hideBracketedSuffixes;
            std::optional<std::string> presentation;
            std::optional<double> backgroundOpacity;
            std::optional<std::string> theme;
            std::optional<std::string> customAccent;
            std::optional<std::string> customText;
            std::optional<std::string> customPanel;
            std::optional<std::string> customInstalled;
            std::optional<bool> useAuthoredMenus;
            std::optional<std::string> controlHints;
            std::optional<bool> closeAfterApply;
            std::optional<bool> cheatMode;
            std::optional<bool> loggingEnabled;
            std::optional<double> menuSlowdown;
            std::optional<double> builderPanelWidth;
            std::optional<double> builderPanelHeight;
            std::optional<double> settingsPanelWidth;
            std::optional<double> settingsPanelHeight;
            Layout cascade;
            Layout radial;
            Layout hybrid;
            Layout horizontal;

            bool HasExplicitValues() const
            {
                return hideBracketedPrefixes.has_value() ||
                    hideBracketedInfixes.has_value() ||
                    hideBracketedSuffixes.has_value() ||
                    presentation.has_value() || backgroundOpacity.has_value() ||
                    theme.has_value() || customAccent.has_value() || customText.has_value() ||
                    customPanel.has_value() || customInstalled.has_value() ||
                    useAuthoredMenus.has_value() ||
                    controlHints.has_value() || closeAfterApply.has_value() ||
                    cheatMode.has_value() ||
                    loggingEnabled.has_value() || menuSlowdown.has_value() ||
                    builderPanelWidth.has_value() || builderPanelHeight.has_value() ||
                    settingsPanelWidth.has_value() || settingsPanelHeight.has_value() ||
                    cascade.HasExplicitValues() || radial.HasExplicitValues() ||
                    hybrid.HasExplicitValues() || horizontal.HasExplicitValues();
            }
        } general;

        std::vector<WeaponPreferences> weapons;
    };

    struct JsonValue;
    using JsonObject = std::map<std::string, JsonValue>;
    using JsonArray = std::vector<JsonValue>;

    struct JsonValue
    {
        using Value = std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject>;
        Value value = nullptr;

        const JsonObject* AsObject() const { return std::get_if<JsonObject>(&value); }
        const JsonArray* AsArray() const { return std::get_if<JsonArray>(&value); }
        const std::string* AsString() const { return std::get_if<std::string>(&value); }
        const bool* AsBool() const { return std::get_if<bool>(&value); }
        const double* AsNumber() const { return std::get_if<double>(&value); }
    };

    class JsonParser
    {
    public:
        explicit JsonParser(std::string text) : text_(std::move(text)) {}

        std::optional<JsonValue> Parse()
        {
            SkipWhitespace();
            auto value = ParseValue();
            if (!value) {
                return std::nullopt;
            }

            SkipWhitespace();
            return pos_ == text_.size() ? value : std::nullopt;
        }

    private:
        std::optional<JsonValue> ParseValue()
        {
            SkipWhitespace();
            if (pos_ >= text_.size()) {
                return std::nullopt;
            }

            const char c = text_[pos_];
            if (c == '{') return ParseObject();
            if (c == '[') return ParseArray();
            if (c == '"') {
                auto s = ParseString();
                if (!s) return std::nullopt;
                JsonValue v;
                v.value = *s;
                return v;
            }
            if (c == 't' && ConsumeLiteral("true")) {
                JsonValue v;
                v.value = true;
                return v;
            }
            if (c == 'f' && ConsumeLiteral("false")) {
                JsonValue v;
                v.value = false;
                return v;
            }
            if (c == 'n' && ConsumeLiteral("null")) {
                JsonValue v;
                v.value = nullptr;
                return v;
            }
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                return ParseNumber();
            }

            return std::nullopt;
        }

        std::optional<JsonValue> ParseObject()
        {
            if (!Consume('{')) return std::nullopt;

            JsonObject object;
            SkipWhitespace();
            if (Consume('}')) {
                JsonValue v;
                v.value = std::move(object);
                return v;
            }

            while (true) {
                SkipWhitespace();
                auto key = ParseString();
                if (!key) return std::nullopt;
                if (!Consume(':')) return std::nullopt;
                auto value = ParseValue();
                if (!value) return std::nullopt;
                if (object.find(*key) != object.end()) return std::nullopt;
                object.emplace(std::move(*key), std::move(*value));
                SkipWhitespace();
                if (Consume('}')) break;
                if (!Consume(',')) return std::nullopt;
            }

            JsonValue v;
            v.value = std::move(object);
            return v;
        }

        std::optional<JsonValue> ParseArray()
        {
            if (!Consume('[')) return std::nullopt;

            JsonArray array;
            SkipWhitespace();
            if (Consume(']')) {
                JsonValue v;
                v.value = std::move(array);
                return v;
            }

            while (true) {
                auto value = ParseValue();
                if (!value) return std::nullopt;
                array.push_back(std::move(*value));
                SkipWhitespace();
                if (Consume(']')) break;
                if (!Consume(',')) return std::nullopt;
            }

            JsonValue v;
            v.value = std::move(array);
            return v;
        }

        std::optional<JsonValue> ParseNumber()
        {
            const auto start = pos_;
            if (pos_ < text_.size() && text_[pos_] == '-') {
                ++pos_;
            }

            const auto intStart = pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
            }
            if (pos_ == intStart) {
                return std::nullopt;
            }

            if (pos_ < text_.size() && text_[pos_] == '.') {
                ++pos_;
                const auto fracStart = pos_;
                while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                    ++pos_;
                }
                if (pos_ == fracStart) {
                    return std::nullopt;
                }
            }

            try {
                JsonValue v;
                v.value = std::stod(text_.substr(start, pos_ - start));
                return v;
            } catch (...) {
                return std::nullopt;
            }
        }

        std::optional<std::string> ParseString()
        {
            if (!Consume('"')) return std::nullopt;

            std::string result;
            while (pos_ < text_.size()) {
                const char c = text_[pos_++];
                if (c == '"') {
                    return result;
                }

                if (static_cast<unsigned char>(c) < 0x20) {
                    return std::nullopt;
                }

                if (c == '\\') {
                    if (pos_ >= text_.size()) return std::nullopt;
                    const char escaped = text_[pos_++];
                    switch (escaped) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u':
                    {
                        if (pos_ + 4 > text_.size()) return std::nullopt;
                        const auto hex = text_.substr(pos_, 4);
                        pos_ += 4;
                        char* end = nullptr;
                        const auto codepoint = std::strtoul(hex.c_str(), &end, 16);
                        if (!end || *end != '\0' || codepoint > 0x7F) {
                            return std::nullopt;
                        }
                        result.push_back(static_cast<char>(codepoint));
                        break;
                    }
                    default:
                        return std::nullopt;
                    }
                } else {
                    result.push_back(c);
                }
            }

            return std::nullopt;
        }

        bool Consume(char expected)
        {
            SkipWhitespace();
            if (pos_ < text_.size() && text_[pos_] == expected) {
                ++pos_;
                return true;
            }
            return false;
        }

        bool ConsumeLiteral(std::string_view literal)
        {
            if (text_.compare(pos_, literal.size(), literal) == 0) {
                pos_ += literal.size();
                return true;
            }
            return false;
        }

        void SkipWhitespace()
        {
            while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
            }
        }

        std::string text_;
        std::size_t pos_ = 0;
    };

    PreferencesDocument g_document;
    k2040::UserSettingsDiagnostics g_diagnostics;
    std::size_t g_parseOnlyMalformedRecordCount = 0;
    bool g_dirty = false;

    std::string LoadStateText(k2040::UserSettingsLoadState state)
    {
        switch (state) {
        case k2040::UserSettingsLoadState::Uninitialized: return "Uninitialized";
        case k2040::UserSettingsLoadState::Missing: return "Missing";
        case k2040::UserSettingsLoadState::LoadedSupported: return "LoadedSupported";
        case k2040::UserSettingsLoadState::Malformed: return "Malformed";
        case k2040::UserSettingsLoadState::UnsupportedVersion: return "UnsupportedVersion";
        case k2040::UserSettingsLoadState::ReadError: return "ReadError";
        default: return "Unknown";
        }
    }

    bool IsSaveAllowedForState(k2040::UserSettingsLoadState state)
    {
        return state == k2040::UserSettingsLoadState::Missing ||
            state == k2040::UserSettingsLoadState::LoadedSupported;
    }

    void RecomputeSaveGate()
    {
        const bool stateAllowed = IsSaveAllowedForState(g_diagnostics.loadState);
        g_diagnostics.saveAllowed =
            stateAllowed &&
            g_diagnostics.malformedRecordCount == 0 &&
            g_diagnostics.duplicateRecordCount == 0;
        g_diagnostics.dirty = g_dirty;

        if (g_diagnostics.saveAllowed) {
            g_diagnostics.blockedSaveReason.clear();
        } else if (!stateAllowed) {
            g_diagnostics.blockedSaveReason =
                "automatic save blocked because preferences load state is " + g_diagnostics.loadStateText;
        } else if (g_diagnostics.malformedRecordCount != 0) {
            g_diagnostics.blockedSaveReason =
                "automatic save blocked because preferences contain malformed records: " +
                std::to_string(g_diagnostics.malformedRecordCount);
        } else if (g_diagnostics.duplicateRecordCount != 0) {
            g_diagnostics.blockedSaveReason =
                "automatic save blocked because preferences contain duplicate records: " +
                std::to_string(g_diagnostics.duplicateRecordCount);
        }
    }

    void SetLoadState(k2040::UserSettingsLoadState state, std::string status)
    {
        g_diagnostics.loadState = state;
        g_diagnostics.loadStateText = LoadStateText(state);
        g_diagnostics.status = std::move(status);
        g_diagnostics.loaded = state != k2040::UserSettingsLoadState::Uninitialized;
        g_diagnostics.missing = state == k2040::UserSettingsLoadState::Missing;
        RecomputeSaveGate();
    }

    bool CategoryHasMalformedSyntax(const PersistedCategoryIdentity& category)
    {
        return !category.IsSyntacticallyValid();
    }

    bool IsValidCustomLabel(const std::string& label)
    {
        return !label.empty() && label.size() <= 256 &&
            std::none_of(label.begin(), label.end(), [](unsigned char value) { return value < 0x20; });
    }

    bool WeaponHasMalformedSyntax(const WeaponPreferences& weapon)
    {
        if (!weapon.weapon.IsSyntacticallyValid()) {
            return true;
        }

        return std::any_of(weapon.hiddenCategories.begin(), weapon.hiddenCategories.end(), CategoryHasMalformedSyntax) ||
            std::any_of(weapon.hiddenOptions.begin(), weapon.hiddenOptions.end(), [](const auto& option) {
                return !option.IsSyntacticallyValid();
            }) || std::any_of(weapon.categoryOrder.begin(), weapon.categoryOrder.end(), CategoryHasMalformedSyntax) ||
            std::any_of(weapon.optionOrder.begin(), weapon.optionOrder.end(), [](const auto& option) {
                return !option.IsSyntacticallyValid();
            }) || std::any_of(weapon.categoryLabels.begin(), weapon.categoryLabels.end(), [](const auto& value) {
                return value.parseMalformed || !value.category.IsSyntacticallyValid() || !IsValidCustomLabel(value.label);
            }) || std::any_of(weapon.optionLabels.begin(), weapon.optionLabels.end(), [](const auto& value) {
                return value.parseMalformed || !value.option.IsSyntacticallyValid() || !IsValidCustomLabel(value.label);
            });
    }

    bool WeaponHasDuplicateRecords(const WeaponPreferences& weapon)
    {
        const auto hasDuplicateCategories = [](const std::vector<PersistedCategoryIdentity>& values) {
            std::set<std::string> seen;
            return std::any_of(values.begin(), values.end(), [&](const auto& value) {
                return !seen.insert(value.CanonicalKey()).second;
            });
        };
        const auto hasDuplicateIdentities = [](const std::vector<PersistedIdentity>& values) {
            std::set<std::string> seen;
            return std::any_of(values.begin(), values.end(), [&](const auto& value) {
                return !seen.insert(value.key).second;
            });
        };

        std::set<std::string> categoryLabels;
        const bool duplicateCategoryLabels = std::any_of(
            weapon.categoryLabels.begin(), weapon.categoryLabels.end(), [&](const auto& value) {
                return !categoryLabels.insert(value.category.CanonicalKey()).second;
            });
        std::set<std::string> optionLabels;
        const bool duplicateOptionLabels = std::any_of(
            weapon.optionLabels.begin(), weapon.optionLabels.end(), [&](const auto& value) {
                return !optionLabels.insert(value.option.key).second;
            });

        return hasDuplicateCategories(weapon.hiddenCategories) ||
            hasDuplicateIdentities(weapon.hiddenOptions) ||
            hasDuplicateCategories(weapon.categoryOrder) ||
            hasDuplicateIdentities(weapon.optionOrder) ||
            duplicateCategoryLabels || duplicateOptionLabels;
    }

    void RecomputeDocumentSyntaxDiagnostics()
    {
        g_diagnostics.malformedRecordCount = g_parseOnlyMalformedRecordCount;
        g_diagnostics.duplicateRecordCount = 0;

        std::set<std::string> seenWeapons;
        for (const auto& weapon : g_document.weapons) {
            if (weapon.parseMalformed || !weapon.weapon.IsSyntacticallyValid()) {
                ++g_diagnostics.malformedRecordCount;
            } else if (!seenWeapons.insert(weapon.weapon.key).second) {
                ++g_diagnostics.duplicateRecordCount;
            }

            std::set<std::string> seenCategories;
            for (const auto& category : weapon.hiddenCategories) {
                if (category.parseMalformed || !category.IsSyntacticallyValid()) {
                    ++g_diagnostics.malformedRecordCount;
                } else if (!seenCategories.insert(category.CanonicalKey()).second) {
                    ++g_diagnostics.duplicateRecordCount;
                }
            }

            std::set<std::string> seenOptions;
            for (const auto& option : weapon.hiddenOptions) {
                if (option.parseMalformed || !option.IsSyntacticallyValid()) {
                    ++g_diagnostics.malformedRecordCount;
                } else if (!seenOptions.insert(option.key).second) {
                    ++g_diagnostics.duplicateRecordCount;
                }
            }

            std::set<std::string> seenOrderedCategories;
            for (const auto& category : weapon.categoryOrder) {
                if (category.parseMalformed || !category.IsSyntacticallyValid()) {
                    ++g_diagnostics.malformedRecordCount;
                } else if (!seenOrderedCategories.insert(category.CanonicalKey()).second) {
                    ++g_diagnostics.duplicateRecordCount;
                }
            }

            std::set<std::string> seenOrderedOptions;
            for (const auto& option : weapon.optionOrder) {
                if (option.parseMalformed || !option.IsSyntacticallyValid()) {
                    ++g_diagnostics.malformedRecordCount;
                } else if (!seenOrderedOptions.insert(option.key).second) {
                    ++g_diagnostics.duplicateRecordCount;
                }
            }

            std::set<std::string> seenCategoryLabels;
            for (const auto& value : weapon.categoryLabels) {
                if (value.parseMalformed || !value.category.IsSyntacticallyValid() || !IsValidCustomLabel(value.label)) {
                    ++g_diagnostics.malformedRecordCount;
                } else if (!seenCategoryLabels.insert(value.category.CanonicalKey()).second) {
                    ++g_diagnostics.duplicateRecordCount;
                }
            }

            std::set<std::string> seenOptionLabels;
            for (const auto& value : weapon.optionLabels) {
                if (value.parseMalformed || !value.option.IsSyntacticallyValid() || !IsValidCustomLabel(value.label)) {
                    ++g_diagnostics.malformedRecordCount;
                } else if (!seenOptionLabels.insert(value.option.key).second) {
                    ++g_diagnostics.duplicateRecordCount;
                }
            }
        }

        RecomputeSaveGate();
    }

    bool CanMutatePreferences()
    {
        RecomputeSaveGate();
        return g_diagnostics.saveAllowed;
    }

    void MarkDirty()
    {
        g_dirty = true;
        g_diagnostics.dirty = true;
    }

    std::string JsonEscape(const std::string& value)
    {
        std::ostringstream out;
        for (const char c : value) {
            switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u"
                        << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(c))
                        << std::nouppercase << std::dec;
                } else {
                    out << c;
                }
                break;
            }
        }
        return out.str();
    }

    const JsonValue* FindMember(const JsonObject& object, const std::string& key)
    {
        const auto it = object.find(key);
        return it == object.end() ? nullptr : std::addressof(it->second);
    }

    bool ReadOptionalStringMember(const JsonObject& object, const std::string& key, std::string& output)
    {
        const auto* member = FindMember(object, key);
        if (!member) {
            return true;
        }

        const auto* value = member->AsString();
        if (!value) {
            return false;
        }

        output = *value;
        return true;
    }

    bool ReadOptionalStringMember(const JsonObject& object, const std::string& key, std::optional<std::string>& output)
    {
        const auto* member = FindMember(object, key);
        if (!member) {
            return true;
        }

        const auto* value = member->AsString();
        if (!value) {
            return false;
        }

        output = *value;
        return true;
    }

    bool ReadOptionalBoolMember(const JsonObject& object, const std::string& key, bool& output)
    {
        const auto* member = FindMember(object, key);
        if (!member) {
            return true;
        }

        const auto* value = member->AsBool();
        if (!value) {
            return false;
        }

        output = *value;
        return true;
    }

    bool ReadOptionalBoolMember(const JsonObject& object, const std::string& key, std::optional<bool>& output)
    {
        const auto* member = FindMember(object, key);
        if (!member) {
            return true;
        }

        const auto* value = member->AsBool();
        if (!value) {
            return false;
        }

        output = *value;
        return true;
    }

    bool ReadOptionalNumberMember(const JsonObject& object, const std::string& key, std::optional<double>& output)
    {
        const auto* member = FindMember(object, key);
        if (!member) {
            return true;
        }

        const auto* value = member->AsNumber();
        if (!value || !std::isfinite(*value)) {
            return false;
        }

        output = *value;
        return true;
    }

    bool IsSupportedSchemaVersion(const JsonObject& root)
    {
        const auto* versionValue = FindMember(root, "schemaVersion");
        const auto* version = versionValue ? versionValue->AsNumber() : nullptr;
        if (!version || !std::isfinite(*version)) {
            return false;
        }

        double integerPart = 0.0;
        if (std::modf(*version, std::addressof(integerPart)) != 0.0) {
            return false;
        }

        return integerPart >= static_cast<double>(kOldestSupportedSchemaVersion) &&
            integerPart <= static_cast<double>(kSchemaVersion);
    }

    std::optional<k2040::BracketedTextOverride> ParseBracketedTextOverride(const std::string& value)
    {
        if (value == "inherit") return k2040::BracketedTextOverride::Inherit;
        if (value == "hide") return k2040::BracketedTextOverride::Hide;
        if (value == "show") return k2040::BracketedTextOverride::Show;
        return std::nullopt;
    }

    std::optional<k2040::AuthoredMenuOverride> ParseAuthoredMenuOverride(const std::string& value)
    {
        if (value == "inherit") return k2040::AuthoredMenuOverride::Inherit;
        if (value == "authored") return k2040::AuthoredMenuOverride::ForceAuthored;
        if (value == "generated") return k2040::AuthoredMenuOverride::ForceGenerated;
        return std::nullopt;
    }

    bool IsPresentationName(const std::string& value)
    {
        return value == "cascade" || value == "radial" || value == "hybrid" || value == "horizontal";
    }

    bool IsThemeName(const std::string& value)
    {
        return value == "default" || value == "pipboy" || value == "high-contrast" || value == "custom";
    }

    bool IsControlHintsMode(const std::string& value)
    {
        return value == "off" || value == "contextual" || value == "always";
    }

    bool IsHexColour(const std::string& value)
    {
        if (value.size() != 7 || value[0] != '#') return false;
        return std::all_of(value.begin() + 1, value.end(), [](unsigned char c) {
            return std::isxdigit(c) != 0;
        });
    }

    bool ReadOptionalLayoutMember(
        const JsonObject& object,
        const std::string& key,
        PreferencesDocument::GeneralPreferences::Layout& output)
    {
        const auto* member = FindMember(object, key);
        if (!member) return true;
        const auto* layout = member->AsObject();
        return layout &&
            ReadOptionalNumberMember(*layout, "scale", output.scale) &&
            ReadOptionalNumberMember(*layout, "positionX", output.positionX) &&
            ReadOptionalNumberMember(*layout, "positionY", output.positionY);
    }

    std::uint32_t ParseLocalFormId(const std::string& text)
    {
        if (text.size() != 10 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) {
            return 0;
        }

        std::uint32_t parsed = 0;
        const auto* first = text.data() + 2;
        const auto* last = text.data() + text.size();
        const auto result = std::from_chars(first, last, parsed, 16);
        if (result.ec != std::errc{} || result.ptr != last || parsed == 0) {
            return 0;
        }

        return parsed;
    }

    PersistedIdentity ReadIdentity(const JsonValue* value)
    {
        PersistedIdentity identity;
        if (!value) {
            return identity;
        }

        const auto* object = value->AsObject();
        if (!object) {
            identity.parseMalformed = true;
            return identity;
        }

        std::string localFormIdText;
        const bool pluginTypeValid = ReadOptionalStringMember(*object, "plugin", identity.plugin);
        const bool localFormIdTypeValid = ReadOptionalStringMember(*object, "localFormId", localFormIdText);
        const bool keyTypeValid = ReadOptionalStringMember(*object, "key", identity.key);
        identity.parseMalformed = !pluginTypeValid || !localFormIdTypeValid || !keyTypeValid;
        identity.localFormId = ParseLocalFormId(localFormIdText);
        const auto derived = PersistedIdentity::MakeKey(identity.plugin, identity.localFormId);
        if (identity.key.empty()) {
            identity.key = derived;
        }
        return identity;
    }

    PersistedCategoryIdentity ReadCategoryIdentity(const JsonValue& value)
    {
        PersistedCategoryIdentity identity;
        const auto* object = value.AsObject();
        if (!object) {
            identity.parseMalformed = true;
            return identity;
        }

        identity.categoryMessage = ReadIdentity(FindMember(*object, "categoryMessage"));
        identity.optionFormList = ReadIdentity(FindMember(*object, "optionFormList"));
        identity.persistenceIdentity = ReadIdentity(FindMember(*object, "persistenceIdentity"));
        identity.parseMalformed =
            identity.categoryMessage.parseMalformed ||
            identity.optionFormList.parseMalformed ||
            identity.persistenceIdentity.parseMalformed;
        return identity;
    }

    WeaponPreferences::CategoryLabel ReadCategoryLabel(const JsonValue& value)
    {
        WeaponPreferences::CategoryLabel result;
        const auto* object = value.AsObject();
        if (!object) {
            result.parseMalformed = true;
            return result;
        }

        const auto* category = FindMember(*object, "category");
        if (!category) {
            result.parseMalformed = true;
        } else {
            result.category = ReadCategoryIdentity(*category);
        }
        result.parseMalformed = !ReadOptionalStringMember(*object, "label", result.label) ||
            !IsValidCustomLabel(result.label) || result.category.parseMalformed || result.parseMalformed;
        return result;
    }

    WeaponPreferences::OptionLabel ReadOptionLabel(const JsonValue& value)
    {
        WeaponPreferences::OptionLabel result;
        const auto* object = value.AsObject();
        if (!object) {
            result.parseMalformed = true;
            return result;
        }

        result.option = ReadIdentity(FindMember(*object, "option"));
        result.parseMalformed = !ReadOptionalStringMember(*object, "label", result.label) ||
            !IsValidCustomLabel(result.label) || result.option.parseMalformed;
        return result;
    }

    WeaponPreferences ReadWeaponPreferences(const JsonObject& weaponObject)
    {
        WeaponPreferences record;
        record.weapon = ReadIdentity(FindMember(weaponObject, "identity"));
        record.parseMalformed = record.weapon.parseMalformed;
        record.parseMalformed =
            !ReadOptionalStringMember(weaponObject, "diagnosticName", record.diagnosticName) ||
            record.parseMalformed;
        record.parseMalformed =
            !ReadOptionalBoolMember(weaponObject, "hidden", record.hidden) ||
            record.parseMalformed;

        bool legacyIgnoreAuthoredMenu = false;
        record.parseMalformed =
            !ReadOptionalBoolMember(weaponObject, "ignoreAuthoredMenu", legacyIgnoreAuthoredMenu) ||
            record.parseMalformed;
        std::string authoredMenuOverride = legacyIgnoreAuthoredMenu ? "generated" : "inherit";
        record.parseMalformed =
            !ReadOptionalStringMember(weaponObject, "authoredMenuOverride", authoredMenuOverride) ||
            record.parseMalformed;
        const auto parsedAuthoredMenuOverride = ParseAuthoredMenuOverride(authoredMenuOverride);
        if (!parsedAuthoredMenuOverride) {
            record.parseMalformed = true;
        } else {
            record.authoredMenuOverride = *parsedAuthoredMenuOverride;
        }

        std::string bracketedTextOverride = "inherit";
        record.parseMalformed =
            !ReadOptionalStringMember(weaponObject, "bracketedTextOverride", bracketedTextOverride) ||
            record.parseMalformed;
        const auto parsedBracketedTextOverride = ParseBracketedTextOverride(bracketedTextOverride);
        if (!parsedBracketedTextOverride) {
            record.parseMalformed = true;
        } else {
            record.bracketedTextOverride = *parsedBracketedTextOverride;
        }
        record.parseMalformed =
            !ReadOptionalBoolMember(weaponObject, "forceUnsafeSwaps", record.forceUnsafeSwaps) ||
            record.parseMalformed;

        const auto readCategoryArray = [&](const char* key, std::vector<PersistedCategoryIdentity>& output) {
            const auto* member = FindMember(weaponObject, key);
            if (!member) return;
            const auto* values = member->AsArray();
            if (!values) {
                record.parseMalformed = true;
                return;
            }
            for (const auto& value : *values) {
                output.push_back(ReadCategoryIdentity(value));
            }
        };
        const auto readIdentityArray = [&](const char* key, std::vector<PersistedIdentity>& output) {
            const auto* member = FindMember(weaponObject, key);
            if (!member) return;
            const auto* values = member->AsArray();
            if (!values) {
                record.parseMalformed = true;
                return;
            }
            for (const auto& value : *values) {
                output.push_back(ReadIdentity(std::addressof(value)));
            }
        };

        readCategoryArray("hiddenCategories", record.hiddenCategories);
        readIdentityArray("hiddenOptions", record.hiddenOptions);
        readCategoryArray("categoryOrder", record.categoryOrder);
        readIdentityArray("optionOrder", record.optionOrder);

        if (const auto* member = FindMember(weaponObject, "categoryLabels")) {
            const auto* values = member->AsArray();
            if (!values) {
                record.parseMalformed = true;
            } else {
                for (const auto& value : *values) {
                    record.categoryLabels.push_back(ReadCategoryLabel(value));
                }
            }
        }

        if (const auto* member = FindMember(weaponObject, "optionLabels")) {
            const auto* values = member->AsArray();
            if (!values) {
                record.parseMalformed = true;
            } else {
                for (const auto& value : *values) {
                    record.optionLabels.push_back(ReadOptionLabel(value));
                }
            }
        }

        return record;
    }

    PersistedIdentity ToPersisted(const k2040::FormRef& ref)
    {
        PersistedIdentity identity;
        identity.plugin = ref.sourcePlugin;
        identity.localFormId = ref.localFormId;
        identity.key = ref.persistentKey.empty() ?
            PersistedIdentity::MakeKey(identity.plugin, identity.localFormId) :
            ref.persistentKey;
        identity.resolved = ref.persistentIdentityValid;
        return identity;
    }

    PersistedCategoryIdentity ToPersistedCategory(const k2040::EcoMenuCategory& category)
    {
        PersistedCategoryIdentity identity;
        identity.categoryMessage = ToPersisted(category.categoryMessage);
        identity.optionFormList = ToPersisted(category.optionFormList);
        identity.persistenceIdentity = ToPersisted(category.persistenceIdentity);
        return identity;
    }

    void WriteIdentity(std::ostringstream& json, const PersistedIdentity& identity)
    {
        json << "{";
        json << "\"plugin\":\"" << JsonEscape(identity.plugin) << "\",";
        json << "\"localFormId\":\"0x" << k2040::ToHexFormId(identity.localFormId) << "\",";
        json << "\"key\":\"" << JsonEscape(identity.key) << "\"";
        json << "}";
    }

    void WriteCategoryIdentity(std::ostringstream& json, const PersistedCategoryIdentity& identity)
    {
        json << "{";
        json << "\"categoryKey\":\"" << JsonEscape(identity.CanonicalKey()) << "\",";
        json << "\"keySource\":\"" << (identity.persistenceIdentity.IsSyntacticallyValid() ? "persistenceIdentity" :
            (identity.optionFormList.IsSyntacticallyValid() ? "optionFormList" : "categoryMessage")) << "\",";
        json << "\"categoryMessage\":";
        WriteIdentity(json, identity.categoryMessage);
        json << ",\"optionFormList\":";
        WriteIdentity(json, identity.optionFormList);
        json << ",\"persistenceIdentity\":";
        WriteIdentity(json, identity.persistenceIdentity);
        json << "}";
    }

    void WriteCategoryLabel(std::ostringstream& json, const WeaponPreferences::CategoryLabel& value)
    {
        json << "{\"category\":";
        WriteCategoryIdentity(json, value.category);
        json << ",\"label\":\"" << JsonEscape(value.label) << "\"}";
    }

    void WriteOptionLabel(std::ostringstream& json, const WeaponPreferences::OptionLabel& value)
    {
        json << "{\"option\":";
        WriteIdentity(json, value.option);
        json << ",\"label\":\"" << JsonEscape(value.label) << "\"}";
    }

    void WriteWeaponPreferencesObject(
        std::ostringstream& json,
        const WeaponPreferences& weapon,
        const std::string& indent,
        bool includeSafetyPreferences)
    {
        const auto memberIndent = indent + "  ";
        json << indent << "{\n";
        json << memberIndent << "\"identity\": ";
        WriteIdentity(json, weapon.weapon);
        json << ",\n";
        json << memberIndent << "\"diagnosticName\": \"" << JsonEscape(weapon.diagnosticName) << "\",\n";
        json << memberIndent << "\"hidden\": " << (weapon.hidden ? "true" : "false") << ",\n";
        json << memberIndent << "\"ignoreAuthoredMenu\": " <<
            (weapon.authoredMenuOverride == k2040::AuthoredMenuOverride::ForceGenerated ? "true" : "false") << ",\n";
        json << memberIndent << "\"authoredMenuOverride\": \"" <<
            k2040::AuthoredMenuOverrideName(weapon.authoredMenuOverride) << "\",\n";
        json << memberIndent << "\"bracketedTextOverride\": \"" <<
            k2040::BracketedTextOverrideName(weapon.bracketedTextOverride) << "\",\n";
        if (includeSafetyPreferences) {
            json << memberIndent << "\"forceUnsafeSwaps\": " <<
                (weapon.forceUnsafeSwaps ? "true" : "false") << ",\n";
        }
        json << memberIndent << "\"hiddenCategories\": [";
        for (std::size_t index = 0; index < weapon.hiddenCategories.size(); ++index) {
            json << (index == 0 ? "" : ", ");
            WriteCategoryIdentity(json, weapon.hiddenCategories[index]);
        }
        json << "],\n";
        json << memberIndent << "\"hiddenOptions\": [";
        for (std::size_t index = 0; index < weapon.hiddenOptions.size(); ++index) {
            json << (index == 0 ? "" : ", ");
            WriteIdentity(json, weapon.hiddenOptions[index]);
        }
        json << "],\n";
        json << memberIndent << "\"categoryOrder\": [";
        for (std::size_t index = 0; index < weapon.categoryOrder.size(); ++index) {
            json << (index == 0 ? "" : ", ");
            WriteCategoryIdentity(json, weapon.categoryOrder[index]);
        }
        json << "],\n";
        json << memberIndent << "\"optionOrder\": [";
        for (std::size_t index = 0; index < weapon.optionOrder.size(); ++index) {
            json << (index == 0 ? "" : ", ");
            WriteIdentity(json, weapon.optionOrder[index]);
        }
        json << "],\n";
        json << memberIndent << "\"categoryLabels\": [";
        for (std::size_t index = 0; index < weapon.categoryLabels.size(); ++index) {
            json << (index == 0 ? "" : ", ");
            WriteCategoryLabel(json, weapon.categoryLabels[index]);
        }
        json << "],\n";
        json << memberIndent << "\"optionLabels\": [";
        for (std::size_t index = 0; index < weapon.optionLabels.size(); ++index) {
            json << (index == 0 ? "" : ", ");
            WriteOptionLabel(json, weapon.optionLabels[index]);
        }
        json << "]\n";
        json << indent << "}";
    }

    std::string SerializeWeaponMenuProfile(const WeaponPreferences& weapon)
    {
        std::ostringstream json;
        json << "{\n";
        json << "  \"format\": \"" << kWeaponMenuProfileFormat << "\",\n";
        json << "  \"schemaVersion\": " << kWeaponMenuProfileSchemaVersion << ",\n";
        json << "  \"createdWith\": \"K2040's Quick Attach Menu " K2040_QUICK_ATTACH_MENU_VERSION "\",\n";
        json << "  \"profileName\": \"" << JsonEscape(
            weapon.diagnosticName.empty() ? "Weapon menu" : weapon.diagnosticName + " menu") << "\",\n";
        json << "  \"categoryIdentityRule\": \"plugin filename plus local FormID; generated categories use their attachment-point keyword\",\n";
        json << "  \"weapon\":\n";
        WriteWeaponPreferencesObject(json, weapon, "  ", false);
        json << "\n}\n";
        return json.str();
    }

    void SortDocument()
    {
        std::sort(g_document.weapons.begin(), g_document.weapons.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.weapon.key < rhs.weapon.key;
        });

        for (auto& weapon : g_document.weapons) {
            std::sort(weapon.hiddenCategories.begin(), weapon.hiddenCategories.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.CanonicalKey() < rhs.CanonicalKey();
            });
            std::sort(weapon.hiddenOptions.begin(), weapon.hiddenOptions.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.key < rhs.key;
            });
            std::sort(weapon.categoryLabels.begin(), weapon.categoryLabels.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.category.CanonicalKey() < rhs.category.CanonicalKey();
            });
            std::sort(weapon.optionLabels.begin(), weapon.optionLabels.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.option.key < rhs.option.key;
            });
        }
    }

    std::string Serialize()
    {
        SortDocument();

        std::ostringstream json;
        json << "{\n";
        json << "  \"schemaVersion\": " << kSchemaVersion << ",\n";
        json << "  \"categoryIdentityRule\": \"persistenceIdentity is canonical when present; authored menus use their option FormList and generated menus use their attachment-point keyword\",\n";
        if (g_document.general.HasExplicitValues()) {
            json << "  \"general\": {\n";
            bool wroteValue = false;
            const auto beginValue = [&](const char* key) {
                json << (wroteValue ? ",\n" : "") << "    \"" << key << "\": ";
                wroteValue = true;
            };
            const auto writeBool = [&](const char* key, const std::optional<bool>& value) {
                if (!value.has_value()) {
                    return;
                }
                beginValue(key);
                json << (*value ? "true" : "false");
            };
            const auto writeNumber = [&](const char* key, const std::optional<double>& value) {
                if (!value.has_value()) return;
                beginValue(key);
                json << std::fixed << std::setprecision(4) << *value << std::defaultfloat;
            };
            const auto writeString = [&](const char* key, const std::optional<std::string>& value) {
                if (!value.has_value()) return;
                beginValue(key);
                json << "\"" << JsonEscape(*value) << "\"";
            };
            const auto writeLayout = [&](const char* key, const PreferencesDocument::GeneralPreferences::Layout& layout) {
                if (!layout.HasExplicitValues()) return;
                beginValue(key);
                json << "{";
                bool wroteLayoutValue = false;
                const auto member = [&](const char* memberKey, const std::optional<double>& value) {
                    if (!value.has_value()) return;
                    json << (wroteLayoutValue ? "," : "") << "\"" << memberKey << "\":"
                         << std::fixed << std::setprecision(4) << *value << std::defaultfloat;
                    wroteLayoutValue = true;
                };
                member("scale", layout.scale);
                member("positionX", layout.positionX);
                member("positionY", layout.positionY);
                json << "}";
            };
            writeBool("hideBracketedPrefixes", g_document.general.hideBracketedPrefixes);
            writeBool("hideBracketedInfixes", g_document.general.hideBracketedInfixes);
            writeBool("hideBracketedSuffixes", g_document.general.hideBracketedSuffixes);
            writeString("presentation", g_document.general.presentation);
            writeNumber("backgroundOpacity", g_document.general.backgroundOpacity);
            writeString("theme", g_document.general.theme);
            writeString("customAccent", g_document.general.customAccent);
            writeString("customText", g_document.general.customText);
            writeString("customPanel", g_document.general.customPanel);
            writeString("customInstalled", g_document.general.customInstalled);
            writeBool("useAuthoredMenus", g_document.general.useAuthoredMenus);
            writeString("controlHints", g_document.general.controlHints);
            writeBool("closeAfterApply", g_document.general.closeAfterApply);
            writeBool("cheatMode", g_document.general.cheatMode);
            writeBool("loggingEnabled", g_document.general.loggingEnabled);
            writeNumber("menuSlowdown", g_document.general.menuSlowdown);
            writeNumber("builderPanelWidth", g_document.general.builderPanelWidth);
            writeNumber("builderPanelHeight", g_document.general.builderPanelHeight);
            writeNumber("settingsPanelWidth", g_document.general.settingsPanelWidth);
            writeNumber("settingsPanelHeight", g_document.general.settingsPanelHeight);
            writeLayout("cascade", g_document.general.cascade);
            writeLayout("radial", g_document.general.radial);
            writeLayout("hybrid", g_document.general.hybrid);
            writeLayout("horizontal", g_document.general.horizontal);
            json << "\n  },\n";
        }
        json << "  \"weapons\": [";

        for (std::size_t wi = 0; wi < g_document.weapons.size(); ++wi) {
            const auto& weapon = g_document.weapons[wi];
            json << (wi == 0 ? "\n" : ",\n");
            WriteWeaponPreferencesObject(json, weapon, "    ", true);
        }

        if (!g_document.weapons.empty()) {
            json << "\n";
        }
        json << "  ]\n";
        json << "}\n";
        return json.str();
    }

    bool ResolveIdentity(PersistedIdentity& identity, ExpectedFormType expectedType)
    {
        identity.resolved = false;
        if (!identity.IsSyntacticallyValid()) {
            return false;
        }

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return false;
        }

        RE::TESForm* form = nullptr;
        switch (expectedType) {
        case ExpectedFormType::Weapon:
            form = dataHandler->LookupForm<RE::TESObjectWEAP>(identity.localFormId, identity.plugin);
            break;
        case ExpectedFormType::CategoryMessage:
            form = dataHandler->LookupForm<RE::BGSMessage>(identity.localFormId, identity.plugin);
            break;
        case ExpectedFormType::CategoryOptionList:
            form = dataHandler->LookupForm<RE::BGSListForm>(identity.localFormId, identity.plugin);
            break;
        case ExpectedFormType::CategoryAttachPoint:
            form = dataHandler->LookupForm<RE::BGSKeyword>(identity.localFormId, identity.plugin);
            break;
        case ExpectedFormType::OptionMod:
            form = dataHandler->LookupForm<RE::BGSMod::Attachment::Mod>(identity.localFormId, identity.plugin);
            break;
        default:
            break;
        }

        if (!form) {
            return false;
        }

        identity.resolved = true;
        return true;
    }

    void ValidateCategory(PersistedCategoryIdentity& category)
    {
        const bool hasMessage = category.categoryMessage.IsSyntacticallyValid();
        const bool hasList = category.optionFormList.IsSyntacticallyValid();
        const bool hasPersistenceIdentity = category.persistenceIdentity.IsSyntacticallyValid();

        if (!hasMessage && !hasList && !hasPersistenceIdentity) {
            return;
        }

        if (hasMessage) {
            ResolveIdentity(category.categoryMessage, ExpectedFormType::CategoryMessage);
        }

        if (hasList) {
            ResolveIdentity(category.optionFormList, ExpectedFormType::CategoryOptionList);
        }
        if (hasPersistenceIdentity) {
            ResolveIdentity(category.persistenceIdentity,
                hasList ? ExpectedFormType::CategoryOptionList : ExpectedFormType::CategoryAttachPoint);
        }
    }

    WeaponPreferences* FindWeapon(const k2040::FormRef& weapon)
    {
        const auto identity = ToPersisted(weapon);
        if (!identity.IsUsable()) {
            return nullptr;
        }

        WeaponPreferences* match = nullptr;
        std::size_t candidateCount = 0;
        std::size_t usableCount = 0;
        for (auto& record : g_document.weapons) {
            if (record.weapon.IsSyntacticallyValid() && record.weapon.key == identity.key) {
                ++candidateCount;
                if (!record.parseMalformed && record.weapon.resolved) {
                    match = std::addressof(record);
                    ++usableCount;
                }
            }
        }

        return candidateCount == 1 && usableCount == 1 ? match : nullptr;
    }

    const WeaponPreferences* FindWeaponConst(const k2040::FormRef& weapon)
    {
        return FindWeapon(weapon);
    }

    void EnsureLoaded()
    {
        if (g_diagnostics.loadState == k2040::UserSettingsLoadState::Uninitialized) {
            k2040::LoadUserPreferences();
        }
    }

    bool ContainsIdentity(const std::vector<PersistedIdentity>& values, const PersistedIdentity& identity)
    {
        std::size_t matches = 0;
        for (const auto& value : values) {
            if (value.key == identity.key && value.resolved && identity.resolved) {
                ++matches;
            }
        }
        return matches == 1;
    }

    bool ContainsCategoryIdentity(const std::vector<PersistedCategoryIdentity>& values, const PersistedCategoryIdentity& identity)
    {
        std::size_t matches = 0;
        for (const auto& value : values) {
            if (value.CanonicalKey() == identity.CanonicalKey() && value.IsUsable() && identity.IsUsable()) {
                ++matches;
            }
        }
        return matches == 1;
    }

    void RemoveIdentity(std::vector<PersistedIdentity>& values, const PersistedIdentity& identity)
    {
        values.erase(std::remove_if(values.begin(), values.end(), [&](const auto& value) {
            return value.key == identity.key;
        }), values.end());
    }

    void RemoveCategoryIdentity(std::vector<PersistedCategoryIdentity>& values, const PersistedCategoryIdentity& identity)
    {
        values.erase(std::remove_if(values.begin(), values.end(), [&](const auto& value) {
            return value.CanonicalKey() == identity.CanonicalKey();
        }), values.end());
    }

    bool CurrentMenuHasCategory(const k2040::EcoWeaponMenu& menu, const PersistedCategoryIdentity& identity)
    {
        for (const auto& category : menu.categories) {
            const auto current = ToPersistedCategory(category);
            if (current.CanonicalKey() == identity.CanonicalKey() && current.IsUsable() && identity.IsUsable()) {
                return true;
            }
        }
        return false;
    }

    bool CurrentMenuHasOption(const k2040::EcoWeaponMenu& menu, const PersistedIdentity& identity)
    {
        for (const auto& category : menu.categories) {
            for (const auto& option : category.options) {
                const auto current = ToPersisted(option.omod);
                if (current.key == identity.key && current.IsUsable() && identity.IsUsable()) {
                    return true;
                }
            }
        }
        return false;
    }

    bool HasUnresolvedData(const WeaponPreferences& weapon)
    {
        if (!weapon.weapon.IsUsable()) {
            return true;
        }
        return std::any_of(weapon.hiddenCategories.begin(), weapon.hiddenCategories.end(), [](const auto& category) {
            return !category.IsUsable();
        }) || std::any_of(weapon.hiddenOptions.begin(), weapon.hiddenOptions.end(), [](const auto& option) {
            return !option.IsUsable();
        }) || std::any_of(weapon.categoryOrder.begin(), weapon.categoryOrder.end(), [](const auto& category) {
            return !category.IsUsable();
        }) || std::any_of(weapon.optionOrder.begin(), weapon.optionOrder.end(), [](const auto& option) {
            return !option.IsUsable();
        }) || std::any_of(weapon.categoryLabels.begin(), weapon.categoryLabels.end(), [](const auto& value) {
            return !value.category.IsUsable();
        }) || std::any_of(weapon.optionLabels.begin(), weapon.optionLabels.end(), [](const auto& value) {
            return !value.option.IsUsable();
        });
    }

    bool ResolveWeaponPreferences(WeaponPreferences& weapon)
    {
        if (!ResolveIdentity(weapon.weapon, ExpectedFormType::Weapon)) {
            return false;
        }
        for (auto& category : weapon.hiddenCategories) ValidateCategory(category);
        for (auto& option : weapon.hiddenOptions) ResolveIdentity(option, ExpectedFormType::OptionMod);
        for (auto& category : weapon.categoryOrder) ValidateCategory(category);
        for (auto& option : weapon.optionOrder) ResolveIdentity(option, ExpectedFormType::OptionMod);
        for (auto& value : weapon.categoryLabels) ValidateCategory(value.category);
        for (auto& value : weapon.optionLabels) ResolveIdentity(value.option, ExpectedFormType::OptionMod);
        return !HasUnresolvedData(weapon);
    }

    bool IsSafeProfileName(const std::string& value)
    {
        return !value.empty() && value.size() <= 128 &&
            std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 0x20; });
    }

    bool HasWeaponMenuProfileExtension(const std::filesystem::path& path)
    {
        constexpr std::string_view suffix = ".k2040qam.json";
        std::string name = path.filename().string();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return name.size() > suffix.size() && name.ends_with(suffix);
    }

    std::optional<ParsedWeaponMenuProfile> ParseWeaponMenuProfile(
        const std::filesystem::path& path,
        std::string& error)
    {
        std::error_code sizeError;
        const auto size = std::filesystem::file_size(path, sizeError);
        if (sizeError || size == 0 || size > kMaximumWeaponMenuProfileBytes) {
            error = "file is empty, unreadable, or larger than 1 MiB";
            return std::nullopt;
        }

        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            error = "file could not be opened";
            return std::nullopt;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        if (!file.good() && !file.eof()) {
            error = "file could not be read completely";
            return std::nullopt;
        }

        auto parsed = JsonParser(buffer.str()).Parse();
        const auto* root = parsed ? parsed->AsObject() : nullptr;
        if (!root) {
            error = "JSON is malformed";
            return std::nullopt;
        }

        const auto* formatValue = FindMember(*root, "format");
        const auto* format = formatValue ? formatValue->AsString() : nullptr;
        const auto* versionValue = FindMember(*root, "schemaVersion");
        const auto* version = versionValue ? versionValue->AsNumber() : nullptr;
        if (!format || *format != kWeaponMenuProfileFormat || !version ||
            !std::isfinite(*version) || *version != static_cast<double>(kWeaponMenuProfileSchemaVersion)) {
            error = "format or schema version is unsupported";
            return std::nullopt;
        }

        ParsedWeaponMenuProfile result;
        result.path = path;
        const auto* profileNameValue = FindMember(*root, "profileName");
        const auto* profileName = profileNameValue ? profileNameValue->AsString() : nullptr;
        if (profileName) result.profileName = *profileName;

        const auto* weaponValue = FindMember(*root, "weapon");
        const auto* weaponObject = weaponValue ? weaponValue->AsObject() : nullptr;
        if (!weaponObject) {
            error = "weapon profile is missing";
            return std::nullopt;
        }
        result.weapon = ReadWeaponPreferences(*weaponObject);
        if (result.profileName.empty()) {
            result.profileName = result.weapon.diagnosticName.empty() ? path.stem().string() : result.weapon.diagnosticName + " menu";
        }

        if (!IsSafeProfileName(result.profileName) ||
            result.weapon.diagnosticName.size() > 256 ||
            std::any_of(result.weapon.diagnosticName.begin(), result.weapon.diagnosticName.end(), [](unsigned char c) { return c < 0x20; }) ||
            result.weapon.parseMalformed || WeaponHasMalformedSyntax(result.weapon) ||
            WeaponHasDuplicateRecords(result.weapon)) {
            error = "weapon profile contains malformed or duplicate records";
            return std::nullopt;
        }
        if (!ResolveWeaponPreferences(result.weapon)) {
            error = "target weapon or referenced menu forms are not loaded";
            return std::nullopt;
        }
        return result;
    }

    struct WeaponMenuProfileScan
    {
        k2040::WeaponMenuProfileCatalog catalog;
        std::vector<ParsedWeaponMenuProfile> matchingProfiles;
    };

    WeaponMenuProfileScan ScanWeaponMenuProfiles(const k2040::FormRef& weapon)
    {
        WeaponMenuProfileScan result;
        const auto target = ToPersisted(weapon);
        if (!target.IsUsable()) {
            return result;
        }

        std::vector<std::filesystem::path> paths;
        const auto collectDirectory = [&](const std::filesystem::path& directory) {
            std::error_code directoryError;
            const bool exists = std::filesystem::exists(directory, directoryError);
            if (directoryError) {
                ++result.catalog.invalidFileCount;
                return;
            }
            if (!exists) return;
            if (!std::filesystem::is_directory(directory, directoryError) || directoryError) {
                ++result.catalog.invalidFileCount;
                return;
            }

            std::filesystem::directory_iterator iterator(directory, directoryError);
            const std::filesystem::directory_iterator end;
            while (!directoryError && iterator != end && paths.size() < kMaximumWeaponMenuProfileFiles) {
                const auto entry = *iterator;
                std::error_code statusError;
                const auto status = entry.symlink_status(statusError);
                if (!statusError && std::filesystem::is_regular_file(status) && HasWeaponMenuProfileExtension(entry.path())) {
                    paths.push_back(entry.path());
                }
                iterator.increment(directoryError);
            }
            if (directoryError) ++result.catalog.invalidFileCount;
        };
        collectDirectory(k2040::GetWeaponMenuProfileDirectory());
        collectDirectory(k2040::GetWeaponMenuExportDirectory());
        std::sort(paths.begin(), paths.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.generic_string() < rhs.generic_string();
        });

        for (const auto& path : paths) {
            std::string error;
            auto parsed = ParseWeaponMenuProfile(path, error);
            if (!parsed) {
                ++result.catalog.invalidFileCount;
                k2040::log::Warn("Ignored weapon menu profile '" + path.filename().string() + "': " + error + ".");
                continue;
            }
            if (parsed->weapon.weapon.key != target.key) {
                ++result.catalog.otherWeaponCount;
                continue;
            }

            k2040::WeaponMenuProfileInfo info;
            info.index = result.matchingProfiles.size();
            info.profileName = parsed->profileName;
            const bool localExport = path.parent_path() == k2040::GetWeaponMenuExportDirectory();
            info.fileName = std::string(localExport ? "Exports/" : "WeaponMenus/") + path.filename().string();
            info.targetPlugin = parsed->weapon.weapon.plugin;
            info.targetLocalFormId = parsed->weapon.weapon.localFormId;
            result.catalog.profiles.push_back(std::move(info));
            result.matchingProfiles.push_back(std::move(*parsed));
        }
        return result;
    }

    std::string SanitizeFileComponent(const std::string& value)
    {
        std::string result;
        result.reserve(std::min<std::size_t>(value.size(), 64));
        bool previousUnderscore = false;
        for (const unsigned char c : value) {
            if (result.size() >= 64) break;
            const bool safe = std::isalnum(c) != 0 || c == '-' || c == '_';
            const char output = safe ? static_cast<char>(c) : '_';
            if (output == '_' && previousUnderscore) continue;
            result.push_back(output);
            previousUnderscore = output == '_';
        }
        while (!result.empty() && result.back() == '_') result.pop_back();
        return result.empty() ? "weapon" : result;
    }

    std::string WindowsErrorMessage(DWORD error);

    bool WriteTextFileAtomically(
        const std::filesystem::path& path,
        const std::string& text,
        std::string& error)
    {
        std::error_code directoryError;
        std::filesystem::create_directories(path.parent_path(), directoryError);
        if (directoryError) {
            error = "could not create the export folder: " + directoryError.message();
            return false;
        }

        auto temp = path;
        temp += ".tmp";
        {
            std::ofstream file(temp, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!file.is_open()) {
                error = "could not open the temporary export file";
                return false;
            }
            file << text;
            file.flush();
            if (!file.good()) {
                error = "the export did not finish writing";
                std::error_code removeError;
                std::filesystem::remove(temp, removeError);
                return false;
            }
        }

        if (!MoveFileExW(temp.native().c_str(), path.native().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            error = "could not replace the export file; " + WindowsErrorMessage(GetLastError());
            std::error_code removeError;
            std::filesystem::remove(temp, removeError);
            return false;
        }
        return true;
    }

    std::string WindowsErrorMessage(DWORD error)
    {
        return "GetLastError=" + std::to_string(static_cast<unsigned long>(error));
    }

    bool SaveUserPreferencesInternal(bool explicitRecovery)
    {
        EnsureLoaded();

        if (!explicitRecovery && !g_diagnostics.saveAllowed) {
            g_diagnostics.saved = false;
            g_diagnostics.dirty = g_dirty;
            k2040::log::Warn(g_diagnostics.blockedSaveReason);
            return false;
        }

        const auto path = k2040::GetUserSettingsPath();
        auto temp = path;
        temp += ".tmp";

        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            g_diagnostics.saved = false;
            g_diagnostics.dirty = g_dirty;
            g_diagnostics.blockedSaveReason = "save failed: could not create directory: " + ec.message();
            k2040::log::Warn(g_diagnostics.blockedSaveReason);
            return false;
        }

        const auto text = Serialize();
        {
            std::ofstream file(temp, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!file.is_open()) {
                g_diagnostics.saved = false;
                g_diagnostics.dirty = g_dirty;
                g_diagnostics.blockedSaveReason = "save failed: could not open temporary file";
                k2040::log::Warn(g_diagnostics.blockedSaveReason);
                return false;
            }

            file << text;
            file.flush();
            if (!file.good()) {
                g_diagnostics.saved = false;
                g_diagnostics.dirty = g_dirty;
                g_diagnostics.blockedSaveReason = "save failed: temporary write did not complete";
                k2040::log::Warn(g_diagnostics.blockedSaveReason);
                return false;
            }
        }

        const auto tempNative = temp.native();
        const auto pathNative = path.native();
        if (!MoveFileExW(tempNative.c_str(), pathNative.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            const auto lastError = GetLastError();
            g_diagnostics.saved = false;
            g_diagnostics.dirty = g_dirty;
            g_diagnostics.blockedSaveReason = "save failed: could not replace destination; " + WindowsErrorMessage(lastError);
            k2040::log::Warn(g_diagnostics.blockedSaveReason);
            return false;
        }

        g_diagnostics.saved = true;
        g_parseOnlyMalformedRecordCount = 0;
        g_dirty = false;
        RecomputeDocumentSyntaxDiagnostics();
        g_diagnostics.path = path;
        g_diagnostics.weaponCount = g_document.weapons.size();
        SetLoadState(k2040::UserSettingsLoadState::LoadedSupported, "saved");
        if (!explicitRecovery) {
            if (RE::TESDataHandler::GetSingleton()) {
                k2040::ValidateUserPreferencesAgainstLoadedForms();
            } else {
                g_diagnostics.gameDataValidated = false;
                g_diagnostics.validationEnvironmentAvailable = false;
                g_diagnostics.status = "saved; game-data validation pending";
            }
        }
        return true;
    }
}

namespace k2040
{
    std::filesystem::path GetUserSettingsPath()
    {
        return std::filesystem::path("Data") / "F4SE" / "Plugins" / "K2040_Quick_Attach_Menu_UserSettings.json";
    }

    std::filesystem::path GetWeaponMenuProfileDirectory()
    {
        return std::filesystem::path("Data") / "F4SE" / "Plugins" /
            "K2040_Quick_Attach_Menu" / "WeaponMenus";
    }

    std::filesystem::path GetWeaponMenuExportDirectory()
    {
        return std::filesystem::path("Data") / "F4SE" / "Plugins" /
            "K2040_Quick_Attach_Menu" / "Exports";
    }

    void LoadUserPreferences()
    {
        g_document = {};
        g_diagnostics = {};
        g_parseOnlyMalformedRecordCount = 0;
        g_dirty = false;
        g_diagnostics.path = GetUserSettingsPath();
        SetLoadState(UserSettingsLoadState::Uninitialized, "not loaded");

        std::error_code existsError;
        const bool preferencesFileExists = std::filesystem::exists(g_diagnostics.path, existsError);
        if (existsError) {
            SetLoadState(UserSettingsLoadState::ReadError, "could not query preferences file: " + existsError.message() + "; automatic save blocked");
            log::Warn(std::string("User preferences existence query failed; automatic save blocked: ") + existsError.message());
            return;
        }

        if (!preferencesFileExists) {
            SetLoadState(UserSettingsLoadState::Missing, "missing; using empty defaults");
            log::Info(std::string("User preferences missing; using empty defaults: ") + g_diagnostics.path.string());
            return;
        }

        std::ifstream file(g_diagnostics.path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            SetLoadState(UserSettingsLoadState::ReadError, "could not open existing preferences file; automatic save blocked");
            log::Warn(std::string("User preferences could not be opened; automatic save blocked: ") + g_diagnostics.path.string());
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        if (!file.good() && !file.eof()) {
            SetLoadState(UserSettingsLoadState::ReadError, "could not read complete preferences file; automatic save blocked");
            log::Warn("User preferences read failed; automatic save blocked.");
            return;
        }

        auto parsed = JsonParser(buffer.str()).Parse();
        const auto* root = parsed ? parsed->AsObject() : nullptr;
        if (!root) {
            ++g_diagnostics.malformedRecordCount;
            SetLoadState(UserSettingsLoadState::Malformed, "malformed JSON; automatic save blocked");
            log::Warn("User preferences JSON was malformed; automatic save blocked.");
            return;
        }

        if (!IsSupportedSchemaVersion(*root)) {
            ++g_diagnostics.unsupportedVersionCount;
            SetLoadState(UserSettingsLoadState::UnsupportedVersion, "unsupported schema version; automatic save blocked");
            log::Warn("User preferences schema version is missing, non-integer, or unsupported; automatic save blocked.");
            return;
        }

        if (const auto* generalValue = FindMember(*root, "general")) {
            const auto* general = generalValue->AsObject();
            if (!general ||
                !ReadOptionalBoolMember(*general, "hideBracketedPrefixes", g_document.general.hideBracketedPrefixes) ||
                !ReadOptionalBoolMember(*general, "hideBracketedInfixes", g_document.general.hideBracketedInfixes) ||
                !ReadOptionalBoolMember(*general, "hideBracketedSuffixes", g_document.general.hideBracketedSuffixes) ||
                !ReadOptionalStringMember(*general, "presentation", g_document.general.presentation) ||
                !ReadOptionalNumberMember(*general, "backgroundOpacity", g_document.general.backgroundOpacity) ||
                !ReadOptionalStringMember(*general, "theme", g_document.general.theme) ||
                !ReadOptionalStringMember(*general, "customAccent", g_document.general.customAccent) ||
                !ReadOptionalStringMember(*general, "customText", g_document.general.customText) ||
                !ReadOptionalStringMember(*general, "customPanel", g_document.general.customPanel) ||
                !ReadOptionalStringMember(*general, "customInstalled", g_document.general.customInstalled) ||
                !ReadOptionalBoolMember(*general, "useAuthoredMenus", g_document.general.useAuthoredMenus) ||
                !ReadOptionalStringMember(*general, "controlHints", g_document.general.controlHints) ||
                !ReadOptionalBoolMember(*general, "closeAfterApply", g_document.general.closeAfterApply) ||
                !ReadOptionalBoolMember(*general, "cheatMode", g_document.general.cheatMode) ||
                !ReadOptionalBoolMember(*general, "loggingEnabled", g_document.general.loggingEnabled) ||
                !ReadOptionalNumberMember(*general, "menuSlowdown", g_document.general.menuSlowdown) ||
                !ReadOptionalNumberMember(*general, "builderPanelWidth", g_document.general.builderPanelWidth) ||
                !ReadOptionalNumberMember(*general, "builderPanelHeight", g_document.general.builderPanelHeight) ||
                !ReadOptionalNumberMember(*general, "settingsPanelWidth", g_document.general.settingsPanelWidth) ||
                !ReadOptionalNumberMember(*general, "settingsPanelHeight", g_document.general.settingsPanelHeight) ||
                !ReadOptionalLayoutMember(*general, "cascade", g_document.general.cascade) ||
                !ReadOptionalLayoutMember(*general, "radial", g_document.general.radial) ||
                !ReadOptionalLayoutMember(*general, "hybrid", g_document.general.hybrid) ||
                !ReadOptionalLayoutMember(*general, "horizontal", g_document.general.horizontal) ||
                (g_document.general.presentation && !IsPresentationName(*g_document.general.presentation)) ||
                (g_document.general.theme && !IsThemeName(*g_document.general.theme)) ||
                (g_document.general.controlHints && !IsControlHintsMode(*g_document.general.controlHints)) ||
                (g_document.general.customAccent && !IsHexColour(*g_document.general.customAccent)) ||
                (g_document.general.customText && !IsHexColour(*g_document.general.customText)) ||
                (g_document.general.customPanel && !IsHexColour(*g_document.general.customPanel)) ||
                (g_document.general.customInstalled && !IsHexColour(*g_document.general.customInstalled))) {
                ++g_diagnostics.malformedRecordCount;
                SetLoadState(UserSettingsLoadState::Malformed, "general settings are malformed; automatic save blocked");
                log::Warn("User preferences general settings were malformed; automatic save blocked.");
                return;
            }
        }

        const auto* weaponsValue = FindMember(*root, "weapons");
        const auto* weapons = weaponsValue ? weaponsValue->AsArray() : nullptr;
        if (!weapons) {
            ++g_diagnostics.malformedRecordCount;
            SetLoadState(UserSettingsLoadState::Malformed, "weapons array missing; automatic save blocked");
            log::Warn("User preferences weapons array missing; automatic save blocked.");
            return;
        }

        for (const auto& weaponValue : *weapons) {
            const auto* weaponObject = weaponValue.AsObject();
            if (!weaponObject) {
                ++g_parseOnlyMalformedRecordCount;
                continue;
            }
            g_document.weapons.push_back(ReadWeaponPreferences(*weaponObject));
        }

        RecomputeDocumentSyntaxDiagnostics();
        g_diagnostics.weaponCount = g_document.weapons.size();
        SetLoadState(UserSettingsLoadState::LoadedSupported, "loaded; game-data validation pending");
        log::Info(std::string("User preferences parsed: ") + g_diagnostics.path.string());
    }

    void ValidateUserPreferencesAgainstLoadedForms()
    {
        EnsureLoaded();

        g_diagnostics.gameDataValidated = false;
        g_diagnostics.validationEnvironmentAvailable = false;
        g_diagnostics.unresolvedRecordCount = 0;
        g_diagnostics.staleDocumentRecordCount = 0;

        if (g_diagnostics.loadState != UserSettingsLoadState::LoadedSupported) {
            return;
        }

        if (!RE::TESDataHandler::GetSingleton()) {
            g_diagnostics.status = "loaded; game-data validation could not run because TESDataHandler is unavailable";
            g_diagnostics.blockedSaveReason = "cleanup blocked because loaded-form validation environment is unavailable";
            log::Warn("User preferences loaded-form validation skipped: TESDataHandler unavailable.");
            return;
        }

        g_diagnostics.validationEnvironmentAvailable = true;
        RecomputeDocumentSyntaxDiagnostics();

        for (auto& weapon : g_document.weapons) {
            const bool weaponResolved = ResolveIdentity(weapon.weapon, ExpectedFormType::Weapon);
            if (!weaponResolved) {
                if (weapon.weapon.IsSyntacticallyValid()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }

            for (auto& category : weapon.hiddenCategories) {
                ValidateCategory(category);
                if (category.IsSyntacticallyValid() && !category.IsUsable()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                } else if (!weaponResolved && category.IsUsable()) {
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }

            for (auto& option : weapon.hiddenOptions) {
                ResolveIdentity(option, ExpectedFormType::OptionMod);
                if (option.IsSyntacticallyValid() && !option.IsUsable()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                } else if (!weaponResolved && option.IsUsable()) {
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }

            for (auto& category : weapon.categoryOrder) {
                ValidateCategory(category);
                if (category.IsSyntacticallyValid() && !category.IsUsable()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }

            for (auto& option : weapon.optionOrder) {
                ResolveIdentity(option, ExpectedFormType::OptionMod);
                if (option.IsSyntacticallyValid() && !option.IsUsable()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }

            for (auto& value : weapon.categoryLabels) {
                ValidateCategory(value.category);
                if (value.category.IsSyntacticallyValid() && !value.category.IsUsable()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }

            for (auto& value : weapon.optionLabels) {
                ResolveIdentity(value.option, ExpectedFormType::OptionMod);
                if (value.option.IsSyntacticallyValid() && !value.option.IsUsable()) {
                    ++g_diagnostics.unresolvedRecordCount;
                    ++g_diagnostics.staleDocumentRecordCount;
                }
            }
        }

        g_diagnostics.gameDataValidated = true;
        g_diagnostics.status = "loaded and game-data validated";
        log::Info("User preferences validated against loaded forms.");
    }

    bool SaveUserPreferences()
    {
        return SaveUserPreferencesInternal(false);
    }

    bool RecoverUserPreferencesWithDefaults()
    {
        g_document = {};
        g_diagnostics = {};
        g_parseOnlyMalformedRecordCount = 0;
        g_dirty = true;
        g_diagnostics.path = GetUserSettingsPath();
        g_diagnostics.gameDataValidated = true;
        SetLoadState(UserSettingsLoadState::LoadedSupported, "explicit recovery requested; replacing preferences with defaults");
        return SaveUserPreferencesInternal(true);
    }

    bool CleanupUnresolvedUserPreferences()
    {
        EnsureLoaded();
        if (g_diagnostics.loadState != UserSettingsLoadState::LoadedSupported) {
            g_diagnostics.blockedSaveReason = "cleanup blocked because preferences load state is " + g_diagnostics.loadStateText;
            return false;
        }

        if (!g_diagnostics.gameDataValidated) {
            g_diagnostics.blockedSaveReason = "cleanup blocked because loaded-form validation has not completed";
            return false;
        }

        if (!g_diagnostics.validationEnvironmentAvailable) {
            g_diagnostics.blockedSaveReason = "cleanup blocked because loaded-form validation environment was unavailable";
            return false;
        }

        if (!CanMutatePreferences()) {
            return false;
        }

        bool changed = false;
        for (auto& weapon : g_document.weapons) {
            const auto categoryCountBefore = weapon.hiddenCategories.size();
            weapon.hiddenCategories.erase(std::remove_if(weapon.hiddenCategories.begin(), weapon.hiddenCategories.end(), [](const auto& category) {
                return category.IsSyntacticallyValid() && !category.IsUsable();
            }), weapon.hiddenCategories.end());
            changed = changed || weapon.hiddenCategories.size() != categoryCountBefore;

            const auto optionCountBefore = weapon.hiddenOptions.size();
            weapon.hiddenOptions.erase(std::remove_if(weapon.hiddenOptions.begin(), weapon.hiddenOptions.end(), [](const auto& option) {
                return option.IsSyntacticallyValid() && !option.IsUsable();
            }), weapon.hiddenOptions.end());
            changed = changed || weapon.hiddenOptions.size() != optionCountBefore;

            const auto orderedCategoryCountBefore = weapon.categoryOrder.size();
            weapon.categoryOrder.erase(std::remove_if(weapon.categoryOrder.begin(), weapon.categoryOrder.end(), [](const auto& category) {
                return category.IsSyntacticallyValid() && !category.IsUsable();
            }), weapon.categoryOrder.end());
            changed = changed || weapon.categoryOrder.size() != orderedCategoryCountBefore;

            const auto orderedOptionCountBefore = weapon.optionOrder.size();
            weapon.optionOrder.erase(std::remove_if(weapon.optionOrder.begin(), weapon.optionOrder.end(), [](const auto& option) {
                return option.IsSyntacticallyValid() && !option.IsUsable();
            }), weapon.optionOrder.end());
            changed = changed || weapon.optionOrder.size() != orderedOptionCountBefore;

            const auto categoryLabelCountBefore = weapon.categoryLabels.size();
            weapon.categoryLabels.erase(std::remove_if(weapon.categoryLabels.begin(), weapon.categoryLabels.end(), [](const auto& value) {
                return value.category.IsSyntacticallyValid() && !value.category.IsUsable();
            }), weapon.categoryLabels.end());
            changed = changed || weapon.categoryLabels.size() != categoryLabelCountBefore;

            const auto optionLabelCountBefore = weapon.optionLabels.size();
            weapon.optionLabels.erase(std::remove_if(weapon.optionLabels.begin(), weapon.optionLabels.end(), [](const auto& value) {
                return value.option.IsSyntacticallyValid() && !value.option.IsUsable();
            }), weapon.optionLabels.end());
            changed = changed || weapon.optionLabels.size() != optionLabelCountBefore;
        }

        const auto weaponCountBefore = g_document.weapons.size();
        g_document.weapons.erase(std::remove_if(g_document.weapons.begin(), g_document.weapons.end(), [](const auto& weapon) {
            return weapon.weapon.IsSyntacticallyValid() && !weapon.weapon.IsUsable();
        }), g_document.weapons.end());
        changed = changed || g_document.weapons.size() != weaponCountBefore;

        if (!changed) {
            return false;
        }

        RecomputeDocumentSyntaxDiagnostics();
        g_diagnostics.weaponCount = g_document.weapons.size();
        MarkDirty();
        const bool saved = SaveUserPreferencesInternal(false);
        if (saved) {
            ValidateUserPreferencesAgainstLoadedForms();
        }
        return saved;
    }

    const UserSettingsDiagnostics& GetUserSettingsDiagnostics()
    {
        EnsureLoaded();
        return g_diagnostics;
    }

    bool RegisterOrUpdateWeapon(const EcoWeaponMenu& menu)
    {
        EnsureLoaded();
        if (!menu.valid) {
            return false;
        }

        if (!CanMutatePreferences()) {
            return false;
        }

        const auto identity = ToPersisted(menu.weapon);
        if (!identity.IsUsable()) {
            return false;
        }

        if (auto* existing = FindWeapon(menu.weapon)) {
            if (existing->diagnosticName == menu.weapon.displayName) {
                return false;
            }
            existing->diagnosticName = menu.weapon.displayName;
            MarkDirty();
            return true;
        }

        for (const auto& record : g_document.weapons) {
            if (record.weapon.key == identity.key) {
                return false;
            }
        }

        WeaponPreferences record;
        record.weapon = identity;
        record.diagnosticName = menu.weapon.displayName;
        g_document.weapons.push_back(std::move(record));
        RecomputeDocumentSyntaxDiagnostics();
        g_diagnostics.weaponCount = g_document.weapons.size();
        MarkDirty();
        return true;
    }

    bool IsWeaponRegistered(const FormRef& weapon)
    {
        EnsureLoaded();
        return FindWeaponConst(weapon) != nullptr;
    }

    bool IsWeaponHidden(const FormRef& weapon)
    {
        EnsureLoaded();
        const auto* record = FindWeaponConst(weapon);
        return record ? record->hidden : false;
    }

    bool IsCategoryHidden(const FormRef& weapon, const EcoMenuCategory& category)
    {
        EnsureLoaded();
        const auto* record = FindWeaponConst(weapon);
        if (!record) {
            return false;
        }

        const auto identity = ToPersistedCategory(category);
        if (!identity.IsUsable()) {
            return false;
        }

        return ContainsCategoryIdentity(record->hiddenCategories, identity);
    }

    bool IsOptionHidden(const FormRef& weapon, const FormRef& omod)
    {
        EnsureLoaded();
        const auto* record = FindWeaponConst(weapon);
        if (!record) {
            return false;
        }

        const auto identity = ToPersisted(omod);
        if (!identity.IsUsable()) {
            return false;
        }

        return ContainsIdentity(record->hiddenOptions, identity);
    }

    bool SetWeaponHidden(const FormRef& weapon, bool hidden)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }
        auto* record = FindWeapon(weapon);
        if (!record) {
            return false;
        }
        if (record->hidden == hidden) {
            return false;
        }
        record->hidden = hidden;
        MarkDirty();
        return true;
    }

    bool SetCategoryHidden(const FormRef& weapon, const EcoMenuCategory& category, bool hidden)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }
        auto* record = FindWeapon(weapon);
        if (!record) {
            return false;
        }

        const auto identity = ToPersistedCategory(category);
        if (!identity.IsUsable()) {
            return false;
        }

        if (hidden) {
            if (!ContainsCategoryIdentity(record->hiddenCategories, identity)) {
                record->hiddenCategories.push_back(identity);
                RecomputeDocumentSyntaxDiagnostics();
                MarkDirty();
                return true;
            }
        } else {
            const auto before = record->hiddenCategories.size();
            RemoveCategoryIdentity(record->hiddenCategories, identity);
            if (record->hiddenCategories.size() != before) {
                RecomputeDocumentSyntaxDiagnostics();
                MarkDirty();
                return true;
            }
        }
        return false;
    }

    bool SetOptionHidden(const FormRef& weapon, const FormRef& omod, bool hidden)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }
        auto* record = FindWeapon(weapon);
        if (!record) {
            return false;
        }

        const auto identity = ToPersisted(omod);
        if (!identity.IsUsable()) {
            return false;
        }

        if (hidden) {
            if (!ContainsIdentity(record->hiddenOptions, identity)) {
                record->hiddenOptions.push_back(identity);
                RecomputeDocumentSyntaxDiagnostics();
                MarkDirty();
                return true;
            }
        } else {
            const auto before = record->hiddenOptions.size();
            RemoveIdentity(record->hiddenOptions, identity);
            if (record->hiddenOptions.size() != before) {
                RecomputeDocumentSyntaxDiagnostics();
                MarkDirty();
                return true;
            }
        }
        return false;
    }

    bool ResetWeaponVisibility(const FormRef& weapon)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }
        auto* record = FindWeapon(weapon);
        if (!record) {
            return false;
        }

        if (!record->hidden && record->hiddenCategories.empty() && record->hiddenOptions.empty()) {
            return false;
        }
        record->hidden = false;
        record->hiddenCategories.clear();
        record->hiddenOptions.clear();
        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    bool ResetAllVisibilityPreferences()
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        bool changed = false;
        for (auto& record : g_document.weapons) {
            if (record.hidden || !record.hiddenCategories.empty() || !record.hiddenOptions.empty()) {
                changed = true;
            }
            record.hidden = false;
            record.hiddenCategories.clear();
            record.hiddenOptions.clear();
        }
        if (changed) {
            RecomputeDocumentSyntaxDiagnostics();
            MarkDirty();
        }
        return changed;
    }

    bool SaveMenuOrder(const EcoWeaponMenu& menu)
    {
        EnsureLoaded();
        if (!menu.valid || !CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(menu.weapon);
        if (!record) {
            return false;
        }

        std::vector<PersistedCategoryIdentity> categoryOrder;
        std::vector<PersistedIdentity> optionOrder;
        categoryOrder.reserve(menu.categories.size());

        for (const auto& category : menu.categories) {
            auto categoryIdentity = ToPersistedCategory(category);
            if (!categoryIdentity.IsUsable()) {
                return false;
            }
            categoryOrder.push_back(std::move(categoryIdentity));

            for (const auto& option : category.options) {
                auto optionIdentity = ToPersisted(option.omod);
                if (!optionIdentity.IsUsable()) {
                    return false;
                }
                optionOrder.push_back(std::move(optionIdentity));
            }
        }

        const auto sameCategoryOrder = record->categoryOrder.size() == categoryOrder.size() &&
            std::equal(record->categoryOrder.begin(), record->categoryOrder.end(), categoryOrder.begin(),
                [](const auto& left, const auto& right) { return left.CanonicalKey() == right.CanonicalKey(); });
        const auto sameOptionOrder = record->optionOrder.size() == optionOrder.size() &&
            std::equal(record->optionOrder.begin(), record->optionOrder.end(), optionOrder.begin(),
                [](const auto& left, const auto& right) { return left.key == right.key; });

        if (sameCategoryOrder && sameOptionOrder) {
            return false;
        }

        record->categoryOrder = std::move(categoryOrder);
        record->optionOrder = std::move(optionOrder);
        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    bool ResetWeaponOrder(const FormRef& weapon)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        if (!record || (record->categoryOrder.empty() && record->optionOrder.empty())) {
            return false;
        }

        record->categoryOrder.clear();
        record->optionOrder.clear();
        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    bool SetCategoryLabel(const FormRef& weapon, const EcoMenuCategory& category, const std::string& label)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        const auto identity = ToPersistedCategory(category);
        if (!record || !identity.IsUsable()) {
            return false;
        }

        const auto found = std::find_if(record->categoryLabels.begin(), record->categoryLabels.end(), [&](const auto& value) {
            return value.category.CanonicalKey() == identity.CanonicalKey();
        });
        const bool restoreSource = label.empty() || label == category.sourceLabel;
        if (restoreSource) {
            if (found == record->categoryLabels.end()) {
                return false;
            }
            record->categoryLabels.erase(found);
        } else {
            if (!IsValidCustomLabel(label)) {
                return false;
            }
            if (found != record->categoryLabels.end()) {
                if (found->label == label) {
                    return false;
                }
                found->label = label;
            } else {
                record->categoryLabels.push_back({ identity, label, false });
            }
        }

        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    bool SetOptionLabel(const FormRef& weapon, const EcoMenuOption& option, const std::string& label)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        const auto identity = ToPersisted(option.omod);
        if (!record || !identity.IsUsable()) {
            return false;
        }

        const auto found = std::find_if(record->optionLabels.begin(), record->optionLabels.end(), [&](const auto& value) {
            return value.option.key == identity.key;
        });
        const bool restoreSource = label.empty() || label == option.sourceLabel;
        if (restoreSource) {
            if (found == record->optionLabels.end()) {
                return false;
            }
            record->optionLabels.erase(found);
        } else {
            if (!IsValidCustomLabel(label)) {
                return false;
            }
            if (found != record->optionLabels.end()) {
                if (found->label == label) {
                    return false;
                }
                found->label = label;
            } else {
                record->optionLabels.push_back({ identity, label, false });
            }
        }

        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    bool ResetWeaponLabels(const FormRef& weapon)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        if (!record || (record->categoryLabels.empty() && record->optionLabels.empty())) {
            return false;
        }

        record->categoryLabels.clear();
        record->optionLabels.clear();
        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    AuthoredMenuOverride GetAuthoredMenuOverride(const FormRef& weapon)
    {
        EnsureLoaded();
        const auto* record = FindWeaponConst(weapon);
        return record ? record->authoredMenuOverride : AuthoredMenuOverride::Inherit;
    }

    bool SetAuthoredMenuOverride(const FormRef& weapon, AuthoredMenuOverride value)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        if (!record || record->authoredMenuOverride == value) {
            return false;
        }

        record->authoredMenuOverride = value;
        if (value == AuthoredMenuOverride::ForceGenerated) {
            record->hidden = false;
            record->hiddenCategories.clear();
            record->hiddenOptions.clear();
            record->categoryOrder.clear();
            record->optionOrder.clear();
            record->categoryLabels.clear();
            record->optionLabels.clear();
        }

        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    const char* AuthoredMenuOverrideName(AuthoredMenuOverride value)
    {
        switch (value) {
        case AuthoredMenuOverride::ForceAuthored: return "authored";
        case AuthoredMenuOverride::ForceGenerated: return "generated";
        case AuthoredMenuOverride::Inherit:
        default: return "inherit";
        }
    }

    bool IsAuthoredMenuIgnored(const FormRef& weapon)
    {
        return GetAuthoredMenuOverride(weapon) == AuthoredMenuOverride::ForceGenerated;
    }

    bool SetAuthoredMenuIgnored(const FormRef& weapon, bool ignored)
    {
        return SetAuthoredMenuOverride(
            weapon,
            ignored ? AuthoredMenuOverride::ForceGenerated : AuthoredMenuOverride::Inherit);
    }

    BracketedTextOverride GetBracketedTextOverride(const FormRef& weapon)
    {
        EnsureLoaded();
        const auto* record = FindWeaponConst(weapon);
        return record ? record->bracketedTextOverride : BracketedTextOverride::Inherit;
    }

    bool SetBracketedTextOverride(const FormRef& weapon, BracketedTextOverride value)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        if (!record || record->bracketedTextOverride == value) {
            return false;
        }

        record->bracketedTextOverride = value;
        MarkDirty();
        return true;
    }

    const char* BracketedTextOverrideName(BracketedTextOverride value)
    {
        switch (value) {
        case BracketedTextOverride::Hide: return "hide";
        case BracketedTextOverride::Show: return "show";
        case BracketedTextOverride::Inherit:
        default: return "inherit";
        }
    }

    bool GetForceUnsafeSwaps(const FormRef& weapon)
    {
        EnsureLoaded();
        const auto* record = FindWeaponConst(weapon);
        return record ? record->forceUnsafeSwaps : false;
    }

    bool SetForceUnsafeSwaps(const FormRef& weapon, bool enabled)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        auto* record = FindWeapon(weapon);
        if (!record || record->forceUnsafeSwaps == enabled) {
            return false;
        }

        record->forceUnsafeSwaps = enabled;
        MarkDirty();
        return true;
    }

    BracketedTextPreferences GetBracketedTextPreferences()
    {
        EnsureLoaded();
        const bool legacyDefault = GetSettings().hideBracketedText;
        return {
            g_document.general.hideBracketedPrefixes.value_or(legacyDefault),
            g_document.general.hideBracketedInfixes.value_or(legacyDefault),
            g_document.general.hideBracketedSuffixes.value_or(legacyDefault)
        };
    }

    bool SetBracketedTextPreferences(const BracketedTextPreferences& value)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) {
            return false;
        }

        const auto current = GetBracketedTextPreferences();
        if (current.hidePrefixes == value.hidePrefixes &&
            current.hideInfixes == value.hideInfixes &&
            current.hideSuffixes == value.hideSuffixes) {
            return false;
        }

        g_document.general.hideBracketedPrefixes = value.hidePrefixes;
        g_document.general.hideBracketedInfixes = value.hideInfixes;
        g_document.general.hideBracketedSuffixes = value.hideSuffixes;
        MarkDirty();
        return true;
    }

    QuickMenuPreferences GetQuickMenuPreferences()
    {
        EnsureLoaded();
        const auto& legacy = GetSettings();
        std::string sourceMode = legacy.menuSource;
        std::transform(sourceMode.begin(), sourceMode.end(), sourceMode.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        const QuickMenuLayoutSettings fallbackLayout{
            std::clamp(static_cast<double>(legacy.scale), 0.5, 1.5),
            std::clamp(static_cast<double>(legacy.positionX), 0.05, 0.95),
            std::clamp(static_cast<double>(legacy.positionY), 0.05, 0.95)
        };
        const auto resolveLayout = [&](const PreferencesDocument::GeneralPreferences::Layout& layout) {
            return QuickMenuLayoutSettings{
                std::clamp(layout.scale.value_or(fallbackLayout.scale), 0.5, 1.5),
                std::clamp(layout.positionX.value_or(fallbackLayout.positionX), 0.05, 0.95),
                std::clamp(layout.positionY.value_or(fallbackLayout.positionY), 0.05, 0.95)
            };
        };

        QuickMenuPreferences result;
        result.presentation = g_document.general.presentation.value_or("cascade");
        result.backgroundOpacity = std::clamp(g_document.general.backgroundOpacity.value_or(0.92), 0.25, 1.0);
        result.theme = g_document.general.theme.value_or("default");
        result.customAccent = g_document.general.customAccent.value_or("#66c4ff");
        result.customText = g_document.general.customText.value_or("#f4faff");
        result.customPanel = g_document.general.customPanel.value_or("#0b1218");
        result.customInstalled = g_document.general.customInstalled.value_or("#74e398");
        result.useAuthoredMenus = g_document.general.useAuthoredMenus.value_or(sourceMode != "generatedonly");
        result.controlHints = g_document.general.controlHints.value_or(legacy.showControlHints ? "always" : "contextual");
        result.closeAfterApply = g_document.general.closeAfterApply.value_or(legacy.closeAfterApply);
        result.cheatMode = g_document.general.cheatMode.value_or(false);
        result.loggingEnabled = g_document.general.loggingEnabled.value_or(true);
        result.menuSlowdown = std::clamp(g_document.general.menuSlowdown.value_or(1.0), 0.0, 1.0);
        const auto resolvePanelDimension = [](const std::optional<double>& dimension) {
            const double value = dimension.value_or(0.0);
            return !std::isfinite(value) || value <= 0.0 ? 0.0 : std::clamp(value, 0.25, 0.98);
        };
        result.builderPanelWidth = resolvePanelDimension(g_document.general.builderPanelWidth);
        result.builderPanelHeight = resolvePanelDimension(g_document.general.builderPanelHeight);
        result.settingsPanelWidth = resolvePanelDimension(g_document.general.settingsPanelWidth);
        result.settingsPanelHeight = resolvePanelDimension(g_document.general.settingsPanelHeight);
        result.cascade = resolveLayout(g_document.general.cascade);
        result.radial = resolveLayout(g_document.general.radial);
        result.hybrid = resolveLayout(g_document.general.hybrid);
        result.horizontal = resolveLayout(g_document.general.horizontal);
        return result;
    }

    bool SetQuickMenuPreferences(const QuickMenuPreferences& requested)
    {
        EnsureLoaded();
        if (!CanMutatePreferences()) return false;

        QuickMenuPreferences value = requested;
        if (!IsPresentationName(value.presentation)) value.presentation = "cascade";
        if (!IsThemeName(value.theme)) value.theme = "default";
        if (!IsControlHintsMode(value.controlHints)) value.controlHints = "contextual";
        if (!IsHexColour(value.customAccent)) value.customAccent = "#66c4ff";
        if (!IsHexColour(value.customText)) value.customText = "#f4faff";
        if (!IsHexColour(value.customPanel)) value.customPanel = "#0b1218";
        if (!IsHexColour(value.customInstalled)) value.customInstalled = "#74e398";
        value.backgroundOpacity = std::clamp(value.backgroundOpacity, 0.25, 1.0);
        if (!std::isfinite(value.menuSlowdown)) value.menuSlowdown = 1.0;
        value.menuSlowdown = std::clamp(value.menuSlowdown, 0.0, 1.0);
        const auto clampPanelDimension = [](double value) {
            if (!std::isfinite(value) || value <= 0.0) return 0.0;
            return std::clamp(value, 0.25, 0.98);
        };
        value.builderPanelWidth = clampPanelDimension(value.builderPanelWidth);
        value.builderPanelHeight = clampPanelDimension(value.builderPanelHeight);
        value.settingsPanelWidth = clampPanelDimension(value.settingsPanelWidth);
        value.settingsPanelHeight = clampPanelDimension(value.settingsPanelHeight);
        const auto clampLayout = [](QuickMenuLayoutSettings& layout) {
            layout.scale = std::clamp(layout.scale, 0.5, 1.5);
            layout.positionX = std::clamp(layout.positionX, 0.05, 0.95);
            layout.positionY = std::clamp(layout.positionY, 0.05, 0.95);
        };
        clampLayout(value.cascade);
        clampLayout(value.radial);
        clampLayout(value.hybrid);
        clampLayout(value.horizontal);

        const auto current = GetQuickMenuPreferences();
        const auto sameLayout = [](const QuickMenuLayoutSettings& lhs, const QuickMenuLayoutSettings& rhs) {
            return lhs.scale == rhs.scale && lhs.positionX == rhs.positionX && lhs.positionY == rhs.positionY;
        };
        if (current.presentation == value.presentation &&
            current.backgroundOpacity == value.backgroundOpacity &&
            current.theme == value.theme && current.customAccent == value.customAccent &&
            current.customText == value.customText && current.customPanel == value.customPanel &&
            current.customInstalled == value.customInstalled &&
            current.useAuthoredMenus == value.useAuthoredMenus &&
            current.controlHints == value.controlHints &&
            current.closeAfterApply == value.closeAfterApply &&
            current.cheatMode == value.cheatMode &&
            current.loggingEnabled == value.loggingEnabled &&
            current.menuSlowdown == value.menuSlowdown &&
            current.builderPanelWidth == value.builderPanelWidth &&
            current.builderPanelHeight == value.builderPanelHeight &&
            current.settingsPanelWidth == value.settingsPanelWidth &&
            current.settingsPanelHeight == value.settingsPanelHeight &&
            sameLayout(current.cascade, value.cascade) && sameLayout(current.radial, value.radial) &&
            sameLayout(current.hybrid, value.hybrid) && sameLayout(current.horizontal, value.horizontal)) {
            return false;
        }

        auto& general = g_document.general;
        general.presentation = value.presentation;
        general.backgroundOpacity = value.backgroundOpacity;
        general.theme = value.theme;
        general.customAccent = value.customAccent;
        general.customText = value.customText;
        general.customPanel = value.customPanel;
        general.customInstalled = value.customInstalled;
        general.useAuthoredMenus = value.useAuthoredMenus;
        general.controlHints = value.controlHints;
        general.closeAfterApply = value.closeAfterApply;
        general.cheatMode = value.cheatMode;
        general.loggingEnabled = value.loggingEnabled;
        general.menuSlowdown = value.menuSlowdown;
        general.builderPanelWidth = value.builderPanelWidth;
        general.builderPanelHeight = value.builderPanelHeight;
        general.settingsPanelWidth = value.settingsPanelWidth;
        general.settingsPanelHeight = value.settingsPanelHeight;
        const auto storeLayout = [](PreferencesDocument::GeneralPreferences::Layout& target, const QuickMenuLayoutSettings& source) {
            target.scale = source.scale;
            target.positionX = source.positionX;
            target.positionY = source.positionY;
        };
        storeLayout(general.cascade, value.cascade);
        storeLayout(general.radial, value.radial);
        storeLayout(general.hybrid, value.hybrid);
        storeLayout(general.horizontal, value.horizontal);
        MarkDirty();
        return true;
    }

    bool ResetGeneralPreferences()
    {
        EnsureLoaded();
        if (!CanMutatePreferences() || !g_document.general.HasExplicitValues()) return false;
        g_document.general = {};
        MarkDirty();
        return true;
    }

    bool ResetAllUserPreferences()
    {
        EnsureLoaded();
        if (!CanMutatePreferences() || (!g_document.general.HasExplicitValues() && g_document.weapons.empty())) return false;
        g_document = {};
        RecomputeDocumentSyntaxDiagnostics();
        MarkDirty();
        return true;
    }

    WeaponMenuProfileCatalog FindWeaponMenuProfiles(const FormRef& weapon)
    {
        EnsureLoaded();
        return ScanWeaponMenuProfiles(weapon).catalog;
    }

    WeaponMenuProfileResult ExportWeaponMenuProfile(const EcoWeaponMenu& menu)
    {
        EnsureLoaded();
        WeaponMenuProfileResult result;
        result.status = "export-rejected";

        if (!menu.valid || !menu.weapon.persistentIdentityValid) {
            result.message = "This weapon does not have a stable identity and cannot be exported.";
            return result;
        }
        const auto* record = FindWeaponConst(menu.weapon);
        if (!record || WeaponHasMalformedSyntax(*record) || WeaponHasDuplicateRecords(*record) || HasUnresolvedData(*record)) {
            result.message = "This weapon's menu settings are not complete enough to export safely.";
            return result;
        }

        const auto fileName = SanitizeFileComponent(
            menu.weapon.displayName.empty() ? record->diagnosticName : menu.weapon.displayName) + "__" +
            SanitizeFileComponent(record->weapon.plugin) + "__" +
            ToHexFormId(record->weapon.localFormId) + ".k2040qam.json";
        const auto path = GetWeaponMenuExportDirectory() / fileName;
        std::string error;
        if (!WriteTextFileAtomically(path, SerializeWeaponMenuProfile(*record), error)) {
            result.status = "export-failed";
            result.message = "The menu profile could not be exported: " + error + ".";
            log::Warn(result.message);
            return result;
        }

        result.success = true;
        result.status = "exported";
        result.fileName = fileName;
        result.message = "Exported " + fileName + " to the Quick Attach Menu Exports folder.";
        log::Info(result.message);
        return result;
    }

    WeaponMenuProfileResult ImportWeaponMenuProfile(const EcoWeaponMenu& menu, std::size_t profileIndex)
    {
        EnsureLoaded();
        WeaponMenuProfileResult result;
        result.status = "import-rejected";

        if (!menu.valid || !menu.weapon.persistentIdentityValid || !CanMutatePreferences()) {
            result.message = "This weapon's menu settings cannot be changed safely right now.";
            return result;
        }

        auto scan = ScanWeaponMenuProfiles(menu.weapon);
        if (profileIndex >= scan.matchingProfiles.size()) {
            result.message = "That menu profile is no longer available for this weapon.";
            return result;
        }

        auto imported = std::move(scan.matchingProfiles[profileIndex]);
        const auto currentIdentity = ToPersisted(menu.weapon);
        if (!currentIdentity.IsUsable() || imported.weapon.weapon.key != currentIdentity.key) {
            result.message = "The selected profile targets a different weapon.";
            return result;
        }

        auto* existing = FindWeapon(menu.weapon);
        if (!existing) {
            result.message = "This weapon is not registered in the menu builder.";
            return result;
        }

        const auto previous = *existing;
        const bool wasDirty = g_dirty;
        const bool forceUnsafeSwaps = existing->forceUnsafeSwaps;
        imported.weapon.weapon = currentIdentity;
        imported.weapon.diagnosticName = menu.weapon.displayName;
        imported.weapon.forceUnsafeSwaps = forceUnsafeSwaps;
        imported.weapon.parseMalformed = false;
        *existing = std::move(imported.weapon);
        RecomputeDocumentSyntaxDiagnostics();
        if (!CanMutatePreferences()) {
            *existing = previous;
            g_dirty = wasDirty;
            RecomputeDocumentSyntaxDiagnostics();
            result.message = "The selected profile failed the final settings check.";
            return result;
        }

        MarkDirty();
        if (!SaveUserPreferencesInternal(false)) {
            const auto failure = g_diagnostics.blockedSaveReason;
            const auto key = currentIdentity.key;
            const auto restore = std::find_if(g_document.weapons.begin(), g_document.weapons.end(), [&](const auto& value) {
                return value.weapon.key == key;
            });
            if (restore != g_document.weapons.end()) {
                *restore = previous;
            } else {
                g_document.weapons.push_back(previous);
            }
            g_dirty = wasDirty;
            RecomputeDocumentSyntaxDiagnostics();
            result.status = "import-failed";
            result.message = failure.empty() ?
                "The imported profile could not be saved; the previous settings were restored." :
                "The imported profile could not be saved; the previous settings were restored. " + failure;
            log::Warn(result.message);
            return result;
        }

        result.success = true;
        result.status = "imported";
        result.fileName = imported.path.filename().string();
        result.message = "Imported " + result.fileName + " for " + menu.weapon.displayName + ".";
        log::Info(result.message);
        return result;
    }

    void ApplyVisibilityPreferences(EcoWeaponMenu& menu)
    {
        EnsureLoaded();
        menu.registeredInUserSettings = IsWeaponRegistered(menu.weapon);
        menu.userHidden = IsWeaponHidden(menu.weapon);

        for (auto& category : menu.categories) {
            if (category.sourceLabel.empty()) {
                category.sourceLabel = category.label;
            }
            category.label = category.sourceLabel;
            category.labelCustomized = false;
            for (auto& option : category.options) {
                if (option.sourceLabel.empty()) {
                    option.sourceLabel = option.label;
                }
                option.label = option.sourceLabel;
                option.labelCustomized = false;
            }
        }

        g_diagnostics.currentMenuStaleRecordCount = 0;
        const auto* record = FindWeaponConst(menu.weapon);
        if (record) {
            for (auto& category : menu.categories) {
                const auto categoryIdentity = ToPersistedCategory(category);
                const auto categoryLabel = std::find_if(record->categoryLabels.begin(), record->categoryLabels.end(), [&](const auto& value) {
                    return value.category.CanonicalKey() == categoryIdentity.CanonicalKey() &&
                        value.category.IsUsable() && categoryIdentity.IsUsable();
                });
                if (categoryLabel != record->categoryLabels.end()) {
                    category.label = categoryLabel->label;
                    category.labelCustomized = true;
                }

                for (auto& option : category.options) {
                    const auto optionIdentity = ToPersisted(option.omod);
                    const auto optionLabel = std::find_if(record->optionLabels.begin(), record->optionLabels.end(), [&](const auto& value) {
                        return value.option.key == optionIdentity.key && value.option.IsUsable() && optionIdentity.IsUsable();
                    });
                    if (optionLabel != record->optionLabels.end()) {
                        option.label = optionLabel->label;
                        option.labelCustomized = true;
                    }
                }
            }

            const auto categoryRank = [&](const EcoMenuCategory& category) {
                const auto identity = ToPersistedCategory(category);
                const auto found = std::find_if(record->categoryOrder.begin(), record->categoryOrder.end(), [&](const auto& ordered) {
                    return ordered.CanonicalKey() == identity.CanonicalKey() && ordered.IsUsable() && identity.IsUsable();
                });
                return found == record->categoryOrder.end() ? record->categoryOrder.size() :
                    static_cast<std::size_t>(std::distance(record->categoryOrder.begin(), found));
            };

            std::stable_sort(menu.categories.begin(), menu.categories.end(), [&](const auto& left, const auto& right) {
                return categoryRank(left) < categoryRank(right);
            });

            for (auto& category : menu.categories) {
                const auto optionRank = [&](const EcoMenuOption& option) {
                    const auto identity = ToPersisted(option.omod);
                    const auto found = std::find_if(record->optionOrder.begin(), record->optionOrder.end(), [&](const auto& ordered) {
                        return ordered.key == identity.key && ordered.IsUsable() && identity.IsUsable();
                    });
                    return found == record->optionOrder.end() ? record->optionOrder.size() :
                        static_cast<std::size_t>(std::distance(record->optionOrder.begin(), found));
                };

                std::stable_sort(category.options.begin(), category.options.end(), [&](const auto& left, const auto& right) {
                    return optionRank(left) < optionRank(right);
                });
            }

            for (const auto& category : record->hiddenCategories) {
                if (!CurrentMenuHasCategory(menu, category)) {
                    ++g_diagnostics.currentMenuStaleRecordCount;
                }
            }
            for (const auto& option : record->hiddenOptions) {
                if (!CurrentMenuHasOption(menu, option)) {
                    ++g_diagnostics.currentMenuStaleRecordCount;
                }
            }
        }

        for (auto& category : menu.categories) {
            category.userHidden = menu.userHidden || IsCategoryHidden(menu.weapon, category);
            category.hasVisibleOptions = false;

            for (auto& option : category.options) {
                option.userHidden = category.userHidden || IsOptionHidden(menu.weapon, option.omod);
                if (option.userHidden) {
                    option.isVisible = false;
                    option.isSelectable = false;
                }

                if (option.isVisible) {
                    category.hasVisibleOptions = true;
                }
            }
        }
    }
}
