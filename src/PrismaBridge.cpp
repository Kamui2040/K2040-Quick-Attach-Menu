#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include "PrismaBridge.h"

#include "AttachmentRuntimeModel.h"
#include "Hotkey.h"
#include "ControllerShortcuts.h"
#include "Logger.h"
#include "Settings.h"
#include "UserSettings.h"

#include <algorithm>
#include <cstddef>
#include <charconv>
#include <cctype>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// Include Windows macros only after all CommonLib/RE headers have parsed.
#include <Windows.h>
#include <Xinput.h>

namespace
{
    constexpr const char* kMenuViewPath = "K2040_Quick_Attach_Menu/menu.html";
    constexpr const char* kMenuBuilderViewPath = "K2040_Quick_Attach_Menu/builder.html";
    constexpr const char* kSettingsViewPath = "K2040_Quick_Attach_Menu/settings.html";
    constexpr std::string_view kPauseHoldMenuName = "PrismaUI_PauseHold";

    class PauseHoldMenuEventSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            const RE::MenuOpenCloseEvent& event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (!event.opening && event.menuName == kPauseHoldMenuName) {
                k2040::GetPrismaBridge().OnPauseHoldClosed();
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    k2040::PrismaBridge g_prismaBridge;
    PauseHoldMenuEventSink g_pauseHoldMenuEventSink;
    bool g_pauseHoldMenuEventSinkRegistered = false;
    RE::BSTSmartPointer<RE::BSInputEnableLayer> g_quickMenuInputLayer;
    bool g_quickMenuGameplayIsolationActive = false;
    bool g_quickMenuTimeBaselineCaptured = false;
    bool g_quickMenuTimeAdjusted = false;
    bool g_builderMenuModeGuard = false;
    float g_previousTimeMultiplier = 1.0F;
    float g_appliedMenuTimeMultiplier = 1.0F;

    void RegisterMenuCursorForRuntime(RE::MenuCursor* cursor)
    {
        // CommonLibF4's current NG ID names the internal visibility update
        // helper (2287475), not the public register wrapper (2287485). The OG
        // ID is correct. Keep this focused relocation local until upstream's
        // cross-runtime MenuCursor ID is corrected.
        using func_t = void (*)(RE::MenuCursor*);
        static REL::Relocation<func_t> registerCursor{ REL::VariantID(1318193, 2287485) };
        registerCursor(cursor);
    }

    void UnregisterMenuCursorForRuntime(RE::MenuCursor* cursor)
    {
        using func_t = void (*)(RE::MenuCursor*);
        static REL::Relocation<func_t> unregisterCursor{ REL::VariantID(1225249, 2287486) };
        unregisterCursor(cursor);
    }

    bool TimeMultiplierMatches(float value, float expected)
    {
        return std::abs(value - expected) < 0.0001F;
    }

    bool UpdateQuickMenuTimeAdjustment(double requestedSlowdown)
    {
        if (!g_quickMenuGameplayIsolationActive) {
            return true;
        }

        auto* timer = RE::BSTimer::GetSingleton();
        if (!timer) {
            k2040::log::Warn("Could not update menu slowdown because the game timer is unavailable.");
            return false;
        }

        const float currentMultiplier = RE::BSTimer::QGlobalTimeMultiplier();
        const float currentTarget = RE::BSTimer::QGlobalTimeMultiplierTarget();
        if (g_quickMenuTimeAdjusted &&
            (!TimeMultiplierMatches(currentMultiplier, g_appliedMenuTimeMultiplier) ||
                !TimeMultiplierMatches(currentTarget, g_appliedMenuTimeMultiplier))) {
            k2040::log::Warn("Game time changed while a mod menu was open; preserving the newer multiplier instead of applying a menu slowdown change.");
            g_quickMenuTimeAdjusted = false;
            g_quickMenuTimeBaselineCaptured = false;
            return false;
        }

        const bool hadMenuAdjustment = g_quickMenuTimeAdjusted;
        if (!g_quickMenuTimeBaselineCaptured) {
            g_previousTimeMultiplier = currentTarget;
            g_quickMenuTimeBaselineCaptured = true;
        } else if (!hadMenuAdjustment && !TimeMultiplierMatches(currentTarget, g_previousTimeMultiplier)) {
            g_previousTimeMultiplier = currentTarget;
        }

        const float slowdown = static_cast<float>(std::clamp(requestedSlowdown, 0.0, 1.0));
        g_appliedMenuTimeMultiplier = g_previousTimeMultiplier * (1.0F - slowdown);
        if (hadMenuAdjustment || slowdown > 0.0001F) {
            timer->SetGlobalTimeMultiplier(g_appliedMenuTimeMultiplier, true);
        }
        g_quickMenuTimeAdjusted = !TimeMultiplierMatches(g_appliedMenuTimeMultiplier, g_previousTimeMultiplier);

        std::ostringstream message;
        message << "Menu slowdown set to " << std::lround(slowdown * 100.0F)
                << "% (time multiplier " << g_appliedMenuTimeMultiplier << ").";
        k2040::log::Info(message.str());
        return true;
    }

    bool RefreshModifiedEquippedItem(RE::TESObjectREFR* container, RE::TESBoundObject* item)
    {
        if (!container || !item) {
            return false;
        }

        // Address Library ID 1153963 is ABSENT on Fallout 4 1.11.240.
        // Its unsuccessful lookup resolves to adjacent ID 1153964, RVA
        // 0x24E2BE8 in non-executable .rdata, and crashes after a valid
        // attachment transaction. Never use a guessed adjacent ID.
        //
        // The legacy path is limited to the supported original 1.10.163
        // runtime until a real AE replacement is identified and validated.
        const auto gameModule = REX::FModule::GetExecutingModule();
        if (gameModule.GetFileVersion() != REL::Version{ 1, 10, 163, 0 }) {
            k2040::log::Warn(
                "Equipped-weapon immediate visual refresh skipped: relocation ID 1153963 "
                "is not validated for this Fallout 4 runtime. The attachment "
                "transaction succeeded; switch/re-equip weapons if the model "
                "does not update immediately.");
            return false;
        }

        // Even on the original runtime, reject relocation targets outside
        // executable .text rather than invoking an unknown pointer.
        const auto textSection = gameModule.GetSection(".text");
        const auto address = REL::ID(1153963).address();
        const auto textStart = textSection.GetAddress();
        if (!textStart || address < textStart ||
            address - textStart >= textSection.GetSize()) {
            k2040::log::Warn(
                "Equipped-weapon immediate visual refresh skipped: relocation "
                "ID 1153963 does not resolve within Fallout 4 executable .text.");
            return false;
        }

        using func_t = void (*)(RE::TESObjectREFR*, RE::TESBoundObject*, bool);
        reinterpret_cast<func_t>(address)(container, item, true);
        return true;
    }

    // Test-only alternative for exactly AE 1.11.240. Legacy OG refresh is intact.
    // Called on the game thread after the real OMOD/inventory transaction.
    bool TryAutoReequipModifiedWeaponAE(
        RE::PlayerCharacter* player,
        RE::TESObjectWEAP* weapon,
        const k2040::EquippedWeaponInfo& verified)
    {
        if (REX::FModule::GetExecutingModule().GetFileVersion() != REL::Version{ 1, 11, 240, 0 } ||
            !k2040::GetSettings().aeAutoReequipAfterApply) {
            return false;
        }
        if (!player || !weapon || !player->inventoryList || !player->currentProcess ||
            !player->currentProcess->middleHigh || !verified.hasWeapon ||
            verified.weapon.formId != weapon->GetFormID() ||
            !verified.equippedInventoryStackFound || verified.equippedInventoryStackCount != 1 ||
            verified.equippedSlotIndex > 1) {
            k2040::log::Warn("AE auto re-equip skipped: equipped stack is ambiguous or process state is unavailable.");
            return false;
        }

        // Capture the actual instance; never select another weapon of the same base form.
        RE::BGSEquipIndex index{};
        index.index = verified.equippedSlotIndex;
        RE::BGSObjectInstance instance(nullptr, nullptr);
        auto* current = player->GetEquippedItem(std::addressof(instance), index);
        if (!current || current->object != weapon || !current->instanceData) {
            k2040::log::Warn("AE auto re-equip skipped: equipped instance changed.");
            return false;
        }

        const RE::BGSEquipSlot* slot = nullptr;
        std::uint32_t loadedAmmo = 0;
        {
            auto* state = player->currentProcess->middleHigh;
            RE::BSAutoLock locker(state->equippedItemsLock);
            for (const auto& entry : state->equippedItems) {
                if (entry.equipIndex.index != index.index || entry.item.object != weapon) {
                    continue;
                }
                if (!entry.data || !entry.equipSlot || slot) {
                    k2040::log::Warn("AE auto re-equip skipped: duplicated or incomplete equipped weapon state.");
                    return false;
                }
                auto* data = RE::fallout_cast<RE::EquippedWeaponData*>(entry.data.get());
                if (!data) {
                    k2040::log::Warn("AE auto re-equip skipped: equipped data does not identify a weapon.");
                    return false;
                }
                slot = entry.equipSlot;
                loadedAmmo = data->ammoCount;
            }
        }
        if (!slot) {
            k2040::log::Warn("AE auto re-equip skipped: no matching equipment slot.");
            return false;
        }

        const auto before = k2040::GetEquippedWeaponInfo();
        if (!before.hasWeapon || before.weapon.formId != verified.weapon.formId ||
            before.equippedSlotIndex != verified.equippedSlotIndex ||
            !before.equippedInventoryStackFound ||
            before.equippedInventoryStackIndex != verified.equippedInventoryStackIndex ||
            before.equippedInventoryStackCount != 1) {
            k2040::log::Warn("AE auto re-equip skipped: inventory stack changed.");
            return false;
        }

        // A script or another mod might have changed the same equipped stack
        // after this menu's last successful OMOD transaction. Do not execute
        // a delayed re-equip using stale attachment state.
        const auto installedIds = [](const k2040::EquippedWeaponInfo& info) {
            std::vector<std::uint32_t> ids;
            ids.reserve(info.installedObjectInstanceMods.size());
            for (const auto& installed : info.installedObjectInstanceMods) {
                ids.push_back(installed.formId);
            }
            std::sort(ids.begin(), ids.end());
            return ids;
        };
        if (installedIds(before) != installedIds(verified)) {
            k2040::log::Warn("AE auto re-equip skipped: installed OMOD identities changed after the queued refresh.");
            return false;
        }

        auto* manager = RE::ActorEquipManager::GetSingleton();
        if (!manager) {
            k2040::log::Warn("AE auto re-equip skipped: actor equip manager unavailable.");
            return false;
        }
        auto* ammo = player->GetCurrentAmmo(index);
        k2040::log::Info("AE auto re-equip: beginning exact-stack unequip.");
        // The equip manager's boolean is not a reliable postcondition on AE:
        // in the 0.5.207 live test the unequip returned false, while the
        // weapon nevertheless became unequipped. Always attempt to restore
        // the same instance/stack, then inspect the real equipped state.
        const bool unequipReturned = manager->UnequipObject(
            player, std::addressof(instance), 1, slot,
            verified.equippedInventoryStackIndex, false, false, false, true, nullptr);
        if (!unequipReturned) {
            k2040::log::Warn(
                "AE auto re-equip: unequip returned false; still issuing the "
                "matching exact-stack equip to avoid stranding the weapon.");
        }
        k2040::log::Info("AE auto re-equip: beginning exact-stack re-equip.");
        const bool equipReturned = manager->EquipObject(
            player, instance, verified.equippedInventoryStackIndex, 1, slot,
            false, false, false, true, false);
        if (!equipReturned) {
            k2040::log::Warn(
                "AE auto re-equip: equip returned false; checking live equipped state "
                "rather than assuming it failed.");
        }

        const auto after = k2040::GetEquippedWeaponInfo();
        if (!after.hasWeapon || after.weapon.formId != verified.weapon.formId ||
            after.equippedSlotIndex != verified.equippedSlotIndex ||
            !after.equippedInventoryStackFound ||
            after.equippedInventoryStackIndex != verified.equippedInventoryStackIndex ||
            after.equippedInventoryStackCount != 1) {
            k2040::log::Warn("AE auto re-equip: stack changed; ammunition cannot be restored safely. Reload test save.");
            return false;
        }

        // Re-equip can refill the magazine. Restore only if the same ammo type
        // remains equipped and the previous load fits the new weapon capacity.
        if (ammo && player->GetCurrentAmmo(index) == ammo &&
            after.liveWeaponInstanceData.present &&
            loadedAmmo <= after.liveWeaponInstanceData.ammoCapacity) {
            player->SetCurrentAmmoCount(index, loadedAmmo);
            k2040::log::Info("AE auto re-equip: original loaded ammunition count restored.");
        } else if (ammo) {
            k2040::log::Warn("AE auto re-equip: ammo type/capacity changed; restoration skipped.");
        }
        k2040::log::Info("AE auto re-equip completed; visual update requires runtime QA.");
        return true;
    }

    bool ActivateQuickMenuGameplayIsolation()
    {
        if (g_quickMenuInputLayer && g_quickMenuGameplayIsolationActive) {
            return true;
        }

        auto* inputManager = RE::BSInputEnableManager::GetSingleton();
        if (!inputManager) {
            k2040::log::Warn("Could not activate quick-menu gameplay isolation because an engine service is unavailable.");
            return false;
        }

        if (!g_quickMenuInputLayer &&
            !inputManager->AllocateNewLayer(g_quickMenuInputLayer, "K2040's Quick Attach Menu")) {
            k2040::log::Warn("Could not allocate the quick-menu input layer.");
            return false;
        }

        constexpr auto otherGameplayEvents = static_cast<RE::OtherInputEvents::OTHER_EVENT_FLAG>(
            static_cast<std::uint32_t>(RE::OtherInputEvents::OTHER_EVENT_FLAG::kAll) &
            ~static_cast<std::uint32_t>(RE::OtherInputEvents::OTHER_EVENT_FLAG::kCursor));

        const bool userEventsDisabled = inputManager->EnableUserEvent(
            g_quickMenuInputLayer->layerID,
            RE::UserEvents::USER_EVENT_FLAG::kAll,
            false,
            RE::UserEvents::SENDER_ID::kMenu);
        const bool otherEventsDisabled = inputManager->EnableOtherEvent(
            g_quickMenuInputLayer->layerID,
            otherGameplayEvents,
            false,
            RE::UserEvents::SENDER_ID::kMenu);

        if (!userEventsDisabled || !otherEventsDisabled) {
            k2040::log::Warn("Could not disable all gameplay input for the quick menu.");
            g_quickMenuInputLayer.reset();
            return false;
        }

        g_quickMenuGameplayIsolationActive = true;
        if (!UpdateQuickMenuTimeAdjustment(k2040::GetQuickMenuPreferences().menuSlowdown)) {
            g_quickMenuGameplayIsolationActive = false;
            g_quickMenuInputLayer.reset();
            return false;
        }

        k2040::log::Info("Mod-menu gameplay input disabled and the configured menu slowdown applied without opening Prisma's pause-holder menu.");
        return true;
    }

    void ReleaseQuickMenuGameplayIsolation()
    {
        if (g_quickMenuTimeAdjusted) {
            if (auto* timer = RE::BSTimer::GetSingleton()) {
                const float currentMultiplier = RE::BSTimer::QGlobalTimeMultiplier();
                const float currentTarget = RE::BSTimer::QGlobalTimeMultiplierTarget();
                if (TimeMultiplierMatches(currentMultiplier, g_appliedMenuTimeMultiplier) &&
                    TimeMultiplierMatches(currentTarget, g_appliedMenuTimeMultiplier)) {
                    timer->SetGlobalTimeMultiplier(g_previousTimeMultiplier, true);
                    k2040::log::Info("Restored the game-time multiplier captured before opening the mod menu.");
                } else {
                    k2040::log::Warn("Game time changed while a mod menu was open; preserving the newer multiplier.");
                }
            }
        }

        g_quickMenuGameplayIsolationActive = false;
        g_quickMenuTimeBaselineCaptured = false;
        g_quickMenuTimeAdjusted = false;
        g_previousTimeMultiplier = 1.0F;
        g_appliedMenuTimeMultiplier = 1.0F;

        if (g_quickMenuInputLayer) {
            g_quickMenuInputLayer.reset();
            k2040::log::Info("Released the quick-menu gameplay input layer.");
        }
    }

    void ScheduleQuickMenuGameplayIsolationRelease()
    {
        if (const auto* taskInterface = F4SE::GetTaskInterface()) {
            taskInterface->AddTask([]() {
                ReleaseQuickMenuGameplayIsolation();
            });
        } else {
            k2040::log::Warn("F4SE task interface unavailable; releasing quick-menu gameplay isolation immediately.");
            ReleaseQuickMenuGameplayIsolation();
        }
    }

    bool ActivateBuilderMenuModeGuard()
    {
        if (g_builderMenuModeGuard) {
            return true;
        }

        auto* ui = RE::UI::GetSingleton();
        if (!ui) {
            k2040::log::Warn("Could not activate the menu-builder hotkey guard because the game UI is unavailable.");
            return false;
        }

        if (ui->menuMode == std::numeric_limits<std::uint32_t>::max()) {
            k2040::log::Warn("Could not activate the menu-builder hotkey guard because the menu counter is full.");
            return false;
        }

        ++ui->menuMode;
        g_builderMenuModeGuard = true;
        k2040::log::Info("Menu-builder menu-state guard activated for MCM hotkey isolation.");
        return true;
    }

    void ReleaseBuilderMenuModeGuard()
    {
        if (!g_builderMenuModeGuard) {
            return;
        }

        if (auto* ui = RE::UI::GetSingleton()) {
            if (ui->menuMode > 0) {
                --ui->menuMode;
                k2040::log::Info("Menu-builder menu-state guard released.");
            } else {
                k2040::log::Warn("Menu-builder menu-state guard found an empty menu counter during release.");
            }
        } else {
            k2040::log::Warn("Could not release the menu-builder menu-state guard because the game UI is unavailable.");
        }

        g_builderMenuModeGuard = false;
    }

    std::string JsonEscape(const std::string& value)
    {
        std::ostringstream out;

        for (const char c : value) {
            switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                out << c;
                break;
            }
        }

        return out.str();
    }

    std::string ToCssColour(const RE::NiColor& colour)
    {
        static constexpr char digits[] = "0123456789abcdef";
        const auto channel = [](float value) {
            return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0F, 1.0F) * 255.0F));
        };
        const std::uint8_t red = channel(colour.r);
        const std::uint8_t green = channel(colour.g);
        const std::uint8_t blue = channel(colour.b);
        std::string result = "#000000";
        result[1] = digits[(red >> 4) & 0x0F];
        result[2] = digits[red & 0x0F];
        result[3] = digits[(green >> 4) & 0x0F];
        result[4] = digits[green & 0x0F];
        result[5] = digits[(blue >> 4) & 0x0F];
        result[6] = digits[blue & 0x0F];
        return result;
    }

    const char* ConsoleLevelToString(PRISMA_UI_API::ConsoleMessageLevel level)
    {
        using PRISMA_UI_API::ConsoleMessageLevel;

        switch (level) {
        case ConsoleMessageLevel::Log:
            return "Log";
        case ConsoleMessageLevel::Warning:
            return "Warning";
        case ConsoleMessageLevel::Error:
            return "Error";
        case ConsoleMessageLevel::Debug:
            return "Debug";
        case ConsoleMessageLevel::Info:
            return "Info";
        default:
            return "Unknown";
        }
    }

    void WriteFormRefJson(std::ostringstream& json, const k2040::FormRef& value)
    {
        json
            << "{"
            << "\"formId\":\"" << k2040::ToHexFormId(value.formId) << "\","
            << "\"sourcePlugin\":\"" << JsonEscape(value.sourcePlugin) << "\","
            << "\"localFormId\":\"" << k2040::ToHexFormId(value.localFormId) << "\","
            << "\"persistentKey\":\"" << JsonEscape(value.persistentKey) << "\","
            << "\"persistentIdentityValid\":" << (value.persistentIdentityValid ? "true" : "false") << ","
            << "\"persistentIdentityStatus\":\"" << JsonEscape(value.persistentIdentityStatus) << "\","
            << "\"editorId\":\"" << JsonEscape(value.editorId) << "\","
            << "\"displayName\":\"" << JsonEscape(value.displayName) << "\""
            << "}";
    }

    void WriteFormRefArrayJson(std::ostringstream& json, const std::vector<k2040::FormRef>& values)
    {
        json << "[";

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                json << ",";
            }

            WriteFormRefJson(json, values[i]);
        }

        json << "]";
    }


void WriteWeaponInstanceDataProbeJson(std::ostringstream& json, const k2040::WeaponInstanceDataProbe& value)
{
    json
        << "{"
        << "\"present\":" << (value.present ? "true" : "false") << ","
        << "\"status\":\"" << JsonEscape(value.status) << "\","
        << "\"ammo\":";

    WriteFormRefJson(json, value.ammo);

    json
        << ",\"hasEquipSlot\":" << (value.hasEquipSlot ? "true" : "false")
        << ",\"hasAimModel\":" << (value.hasAimModel ? "true" : "false")
        << ",\"hasZoomData\":" << (value.hasZoomData ? "true" : "false")
        << ",\"hasImpactDataSet\":" << (value.hasImpactDataSet ? "true" : "false")
        << ",\"hasRangedData\":" << (value.hasRangedData ? "true" : "false")
        << ",\"hasKeywordData\":" << (value.hasKeywordData ? "true" : "false")
        << ",\"value\":" << value.value
        << ",\"attackDamage\":" << value.attackDamage
        << ",\"ammoCapacity\":" << value.ammoCapacity
        << ",\"rank\":" << value.rank
        << ",\"weight\":" << value.weight
        << ",\"speed\":" << value.speed
        << ",\"reach\":" << value.reach
        << ",\"minRange\":" << value.minRange
        << ",\"maxRange\":" << value.maxRange
        << ",\"attackDelaySec\":" << value.attackDelaySec
        << ",\"reloadSpeed\":" << value.reloadSpeed
        << ",\"attackActionPointCost\":" << value.attackActionPointCost
        << ",\"colorRemappingIndex\":" << value.colorRemappingIndex
        << "}";
}

    void WriteOmodAttachmentInfoArrayJson(std::ostringstream& json, const std::vector<k2040::OmodAttachmentInfo>& values)
    {
        json << "[";

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                json << ",";
            }

            const auto& value = values[i];

            json << "{";

            json << "\"omod\":";
            WriteFormRefJson(json, value.omod);

            json << ",\"consumesAttachPoint\":";
            WriteFormRefJson(json, value.consumesAttachPoint);

            json << ",\"providesAttachParentSlots\":";
            WriteFormRefArrayJson(json, value.providesAttachParentSlots);

            json
                << ",\"templateItemIndex\":" << value.templateItemIndex
                << ",\"attachmentIndex\":" << value.attachmentIndex
                << ",\"rank\":" << value.rank
                << ",\"templateItemIsDefault\":" << (value.templateItemIsDefault ? "true" : "false")
                << ",\"optional\":" << (value.optional ? "true" : "false")
                << ",\"childrenExclusive\":" << (value.childrenExclusive ? "true" : "false")
                << "}";
        }

        json << "]";
    }

    bool ParseSelectionKey(const char* argument, std::uint32_t& categoryIndex, std::uint32_t& optionIndex)
    {
        if (!argument) {
            return false;
        }

        const std::string_view value(argument);
        if (value.empty() || value.size() > 31) {
            return false;
        }

        const auto separator = value.find(':');
        if (separator == std::string_view::npos || separator == 0 || separator + 1 >= value.size()) {
            return false;
        }

        const auto categoryText = value.substr(0, separator);
        const auto optionText = value.substr(separator + 1);
        const auto categoryResult = std::from_chars(
            categoryText.data(), categoryText.data() + categoryText.size(), categoryIndex);
        const auto optionResult = std::from_chars(
            optionText.data(), optionText.data() + optionText.size(), optionIndex);

        return categoryResult.ec == std::errc{} &&
            categoryResult.ptr == categoryText.data() + categoryText.size() &&
            optionResult.ec == std::errc{} &&
            optionResult.ptr == optionText.data() + optionText.size();
    }

    bool ParseIndexList(std::string_view text, std::vector<std::uint32_t>& values)
    {
        values.clear();
        if (text.empty() || text.size() > 2048) {
            return false;
        }

        std::size_t start = 0;
        while (start < text.size()) {
            const auto separator = text.find(',', start);
            const auto end = separator == std::string_view::npos ? text.size() : separator;
            const auto token = text.substr(start, end - start);
            std::uint32_t value = 0;
            const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
            if (token.empty() || result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
                std::find(values.begin(), values.end(), value) != values.end()) {
                values.clear();
                return false;
            }
            values.push_back(value);
            if (separator == std::string_view::npos) {
                break;
            }
            start = separator + 1;
        }

        return !values.empty();
    }

    std::optional<double> ParseFiniteDouble(std::string_view text)
    {
        try {
            std::size_t consumed = 0;
            const auto value = std::stod(std::string(text), &consumed);
            if (consumed != text.size() || !std::isfinite(value)) return std::nullopt;
            return value;
        } catch (...) {
            return std::nullopt;
        }
    }

    std::vector<std::string_view> SplitCommand(std::string_view text, char separator = ':')
    {
        std::vector<std::string_view> parts;
        std::size_t start = 0;
        while (start <= text.size()) {
            const auto end = text.find(separator, start);
            parts.push_back(text.substr(start, end == std::string_view::npos ? text.size() - start : end - start));
            if (end == std::string_view::npos) break;
            start = end + 1;
        }
        return parts;
    }

    bool ParseCustomLabel(std::string_view text, std::string& label)
    {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
            text.remove_prefix(1);
        }
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
            text.remove_suffix(1);
        }
        if (text.size() > 256 || std::any_of(text.begin(), text.end(), [](unsigned char value) { return value < 0x20; })) {
            return false;
        }
        label.assign(text.begin(), text.end());
        return true;
    }

    void OnMenuViewDomReady(PrismaView view)
    {
        k2040::GetPrismaBridge().OnDomReady(view);
    }

    void OnMenuCloseRequested(const char* argument)
    {
        k2040::GetPrismaBridge().OnCloseRequested(argument);
    }

    void OnMenuHotkeyActionRequested(const char* argument)
    {
        k2040::GetPrismaBridge().OnHotkeyActionRequested(argument);
    }

    void OnMenuOptionPreviewRequested(const char* argument)
    {
        k2040::GetPrismaBridge().OnOptionPreviewRequested(argument);
    }

    void OnMenuBuilderChangeRequested(const char* argument)
    {
        k2040::GetPrismaBridge().OnBuilderChangeRequested(argument);
    }

    void OnMenuSettingsChangeRequested(const char* argument)
    {
        k2040::GetPrismaBridge().OnSettingsChangeRequested(argument);
    }

    void OnMenuConsoleMessage(PrismaView view, PRISMA_UI_API::ConsoleMessageLevel level, const char* message)
    {
        std::string text = "Prisma menu view console [";
        text += ConsoleLevelToString(level);
        text += "] view ";
        text += k2040::ToHexFormId(static_cast<std::uint32_t>(view & 0xFFFFFFFFu));
        text += ": ";
        text += message ? message : "(null)";

        if (level == PRISMA_UI_API::ConsoleMessageLevel::Error) {
            k2040::log::Warn(text);
        } else {
            k2040::log::Info(text);
        }
    }
}

namespace k2040
{
    PrismaBridge& GetPrismaBridge()
    {
        return g_prismaBridge;
    }

    bool PrismaBridge::Initialize()
    {
        // PrismaUI 2.1.1 assigns reserved numeric IDs 136-139 to V9-V12. The
        // earlier 2.1.0.2 provider used sequential IDs, so try the released
        // 2.1.1 contract first and retain raw ID 9 as the old V10 fallback.
        // The menu only uses the inherited V10 surface.
        if (auto* api12 = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI12>()) {
            controllerApi_ = api12;
            api_ = api12;
            log::Info("PrismaUI IVPrismaUI12 API acquired with native controller action support.");
        } else {
            controllerApi_ = nullptr;
            api_ = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI10>();
            if (api_) {
                log::Info("PrismaUI IVPrismaUI10 API acquired with the released 2.1.1 interface ID.");
            } else {
                constexpr auto kSequentialV10 = static_cast<PRISMA_UI_API::InterfaceVersion>(9);
                api_ = static_cast<PRISMA_UI_API::IVPrismaUI10*>(
                    PRISMA_UI_API::RequestPluginAPI(kSequentialV10));
                if (api_) {
                    log::Info("PrismaUI IVPrismaUI10 API acquired with the 2.1.0.2 sequential interface ID fallback.");
                }
            }
        }

        if (!api_) {
            log::Warn("PrismaUI IVPrismaUI12 and both IVPrismaUI10 interface IDs are not available yet.");
            return false;
        }

        log::Info("PrismaUI 2.1.1 compatibility API acquired.");

        if (!g_pauseHoldMenuEventSinkRegistered) {
            if (auto* ui = RE::UI::GetSingleton()) {
                ui->RegisterSink<RE::MenuOpenCloseEvent>(&g_pauseHoldMenuEventSink);
                g_pauseHoldMenuEventSinkRegistered = true;
                log::Info("Registered for Prisma pause-holder menu close events.");
            } else {
                log::Warn("Could not register for Prisma pause-holder menu close events because the game UI is unavailable.");
            }
        }

        return true;
    }

    void PrismaBridge::EnsureDockRegistration()
    {
        if (menuOpen_) {
            return;
        }

        if (!api_ && !Initialize()) {
            log::Warn("Prisma Dock registration deferred because the PrismaUI API is unavailable.");
            return;
        }

        viewMode_ = ViewMode::QuickMenu;
        CreateMenuViewIfNeeded();

        if (menuView_ == 0 || !api_->IsValid(menuView_)) {
            log::Warn("Prisma Dock registration could not prepare a hidden Quick Attach Menu view.");
            return;
        }

        api_->Hide(menuView_);
        log::Info("Hidden Quick Attach Menu view prepared for Prisma Dock discovery.");
    }

    bool PrismaBridge::IsAvailable() const
    {
        return api_ != nullptr;
    }

    bool PrismaBridge::IsMenuFocused() const
    {
        // Older PrismaUI focus tests returned Focus() true while HasFocus() was
        // false. Keep internal state until the 2.1 runtime path has been
        // retested in-game.
        return menuOpen_;
    }

    bool PrismaBridge::IsMenuBuilderOpen() const
    {
        return menuOpen_ && viewMode_ != ViewMode::QuickMenu;
    }

    bool PrismaBridge::IsQuickControllerInputActive() const
    {
        return quickControllerInputActive_.load(std::memory_order_relaxed);
    }

    void PrismaBridge::OnControllerStickSector(int sector)
    {
        // Called only by a queued F4SE game-thread task. Never call PrismaUI
        // from the physical controller polling thread.
        if (sector < -1 || sector >= 72 ||
            !quickControllerInputActive_.load(std::memory_order_relaxed) ||
            !menuOpen_ || viewMode_ != ViewMode::QuickMenu ||
            !viewDomReady_ || !api_ || !api_->IsValid(menuView_)) {
            return;
        }
        // -1 is a neutral-release signal, not an input or confirmation.
        const auto value = std::to_string(sector);
        api_->InteropCall(menuView_, "k2040ControllerStickSector", value.c_str());
    }

    bool PrismaBridge::CanOpenFromHotkey() const
    {
        if (const auto* ui = RE::UI::GetSingleton()) {
            if (ui->GetMenuOpen(RE::BSFixedString(RE::DialogueMenu::MENU_NAME.data()))) {
                log::Info("Open-menu hotkey ignored because DialogueMenu is active.");
                return false;
            }

            if (ui->menuMode != 0) {
                return false;
            }
        }

        if (!api_) {
            return true;
        }

        if (IsMenuFocused()) {
            return true;
        }

        // Hotkeys should yield only while another Prisma view actually owns
        // input. PrismaUI 2.1.1 keeps the Dock view visible while collapsed,
        // so treating every visible panel as active blocks this menu forever.
        const PrismaView focusedView = api_->GetFocusedView();
        return focusedView == 0 || focusedView == menuView_;
    }

    bool PrismaBridge::BeginOpenFromHotkey()
    {
        if (!CanOpenFromHotkey()) {
            return false;
        }

        // Isolate gameplay before weapon probing and payload construction so
        // the opening K press cannot reach the Pip-Boy during that work.
        return ActivateQuickMenuGameplayIsolation();
    }

    std::string PrismaBridge::BuildMenuPayload(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu) const
    {
        std::ostringstream json;
        const auto& settings = GetSettings();
        const auto bracketedText = GetBracketedTextPreferences();
        const auto quickMenu = GetQuickMenuPreferences();
        const auto quickMenuHotkey = GetSharedHotkeyBinding("openQuickMenu");
        const auto builderHotkey = GetSharedHotkeyBinding("openMenuBuilder");
        const auto hudColour = ToCssColour(RE::HUDMenuUtils::GetGameplayHUDColor());
        const auto hudBackgroundColour = ToCssColour(RE::HUDMenuUtils::GetGameplayHUDBackgroundColor());
        const auto menuProfiles = viewMode_ == ViewMode::MenuBuilder ?
            FindWeaponMenuProfiles(menu.weapon) : WeaponMenuProfileCatalog{};
        json
            << "{"
            << "\"title\":\"K2040's Quick Attach Menu\","
            << "\"mode\":\"cascade-quick-mod\","
            << "\"menuSource\":\"" << (menu.runtimeGenerated ? "runtime-generated" : "ECO-authored") << "\","
            << "\"codeBatch\":\"CascadeMenu\","
            << "\"pluginVersion\":\"" K2040_QUICK_ATTACH_MENU_VERSION "\","
            << "\"prismaApi\":\"" << (controllerApi_ ? "IVPrismaUI12" : "IVPrismaUI10") << "\","
            << "\"prismaReview\":\"2.1.1 base runtime validated; controller path pending focused QA\","
            << "\"exporterReference\":\"v0.38 internal reference only; runtime does not depend on JSON\","
            << "\"weapon\":{"
                << "\"hasWeapon\":" << (weaponInfo.hasWeapon ? "true" : "false") << ","
                << "\"name\":\"" << JsonEscape(weaponInfo.weapon.displayName) << "\","
                << "\"editorId\":\"" << JsonEscape(weaponInfo.weapon.editorId) << "\","
                << "\"formId\":\"" << ToHexFormId(weaponInfo.weapon.formId) << "\","
                << "\"sourcePlugin\":\"" << JsonEscape(weaponInfo.weapon.sourcePlugin) << "\","
                << "\"localFormId\":\"" << ToHexFormId(weaponInfo.weapon.localFormId) << "\","
                << "\"persistentKey\":\"" << JsonEscape(weaponInfo.weapon.persistentKey) << "\","
                << "\"persistentIdentityValid\":" << (weaponInfo.weapon.persistentIdentityValid ? "true" : "false") << ","
                << "\"persistentIdentityStatus\":\"" << JsonEscape(weaponInfo.weapon.persistentIdentityStatus) << "\","
                << "\"status\":\"" << JsonEscape(weaponInfo.status) << "\","
                << "\"baseAttachParentSlotCount\":" << weaponInfo.baseAttachParentSlots.size() << ","
                << "\"baseAttachParentSlots\":";

        WriteFormRefArrayJson(json, weaponInfo.baseAttachParentSlots);

        json
                << ",\"defaultTemplateModCount\":" << weaponInfo.defaultTemplateMods.size()
                << ",\"defaultTemplateMods\":";

        WriteOmodAttachmentInfoArrayJson(json, weaponInfo.defaultTemplateMods);

        json
                << ",\"defaultTemplateProvidedSlotCount\":" << weaponInfo.defaultTemplateProvidedSlots.size()
                << ",\"defaultTemplateProvidedSlots\":";

        WriteFormRefArrayJson(json, weaponInfo.defaultTemplateProvidedSlots);

        json
                << ",\"availableAttachParentSlotPreviewCount\":" << weaponInfo.currentAvailableSlotsPreview.size()
                << ",\"availableAttachParentSlotPreview\":";

        WriteFormRefArrayJson(json, weaponInfo.currentAvailableSlotsPreview);

        json
                << ",\"installedObjectInstanceModCount\":" << weaponInfo.installedObjectInstanceMods.size()
                << ",\"installedObjectInstanceRawCount\":" << weaponInfo.installedObjectInstanceRawCount
                << ",\"installedObjectInstanceResolvedCount\":" << weaponInfo.installedObjectInstanceResolvedCount
                << ",\"installedObjectInstanceProbeStatus\":\"" << JsonEscape(weaponInfo.installedObjectInstanceProbeStatus) << "\""
                << ",\"installedObjectInstanceMods\":";

        WriteFormRefArrayJson(json, weaponInfo.installedObjectInstanceMods);

        json
                << ",\"equippedInstanceDataPresent\":" << (weaponInfo.equippedInstanceDataPresent ? "true" : "false")
                << ",\"equippedInstanceKeywordCount\":" << weaponInfo.equippedInstanceKeywordCount
                << ",\"equippedInstanceKeywords\":";

        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceKeywords);

        json << ",\"equippedInstanceDnKeywords\":";
        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceDnKeywords);

        json << ",\"equippedInstanceMaKeywords\":";
        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceMaKeywords);

        json << ",\"equippedInstanceAnimKeywords\":";
        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceAnimKeywords);

        json << ",\"equippedInstanceWeaponTypeKeywords\":";
        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceWeaponTypeKeywords);

        json << ",\"equippedInstanceGameplayKeywords\":";
        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceGameplayKeywords);

        json << ",\"equippedInstanceOtherKeywords\":";
        WriteFormRefArrayJson(json, weaponInfo.equippedInstanceOtherKeywords);

        json
                << ",\"equippedInstanceProbeStatus\":\"" << JsonEscape(weaponInfo.equippedInstanceProbeStatus) << "\""
                << ",\"equippedInstanceStateReliability\":\"" << JsonEscape(weaponInfo.equippedInstanceStateReliability) << "\""
                << ",\"liveWeaponInstanceData\":";

        WriteWeaponInstanceDataProbeJson(json, weaponInfo.liveWeaponInstanceData);

        json
            << "},"
            << "\"parser\":{";

        json << "\"ecoRootFound\":" << (menu.rootFormList.formId != 0 ? "true" : "false") << ",";
        json << "\"valid\":" << (menu.valid ? "true" : "false") << ",";
        json << "\"registeredInUserSettings\":" << (menu.registeredInUserSettings ? "true" : "false") << ",";
        json << "\"userHidden\":" << (menu.userHidden ? "true" : "false") << ",";
        json << "\"ignoreAuthoredMenu\":" << (IsAuthoredMenuIgnored(menu.weapon) ? "true" : "false") << ",";
        json << "\"authoredMenuOverride\":\"" << AuthoredMenuOverrideName(GetAuthoredMenuOverride(menu.weapon)) << "\",";
        json << "\"bracketedTextOverride\":\"" <<
            BracketedTextOverrideName(GetBracketedTextOverride(menu.weapon)) << "\",";
        json << "\"forceUnsafeSwaps\":" << (GetForceUnsafeSwaps(menu.weapon) ? "true" : "false") << ",";
        json << "\"status\":\"" << JsonEscape(menu.status) << "\",";

        // Root and menu form refs
        json << "\"rootFormList\":";
        WriteFormRefJson(json, menu.rootFormList);
        json << ",\"weaponKeyword\":";
        WriteFormRefJson(json, menu.weaponKeyword);
        json << ",\"weaponMenuMessage\":";
        WriteFormRefJson(json, menu.weaponMenuMessage);
        json << ",\"categoryMessageList\":";
        WriteFormRefJson(json, menu.categoryMessageList);
        json << ",\"categoryOptionList\":";
        WriteFormRefJson(json, menu.categoryOptionList);

        // Compute counts and list categories/options
        std::size_t optionCount = 0;
        std::size_t installedCount = 0;

        for (const auto& cat : menu.categories) {
            optionCount += cat.options.size();
            for (const auto& opt : cat.options) {
                if (opt.isInstalled) ++installedCount;
            }
        }

        json << ",\"categoryCount\":" << menu.categories.size()
             << ",\"optionCount\":" << optionCount
             << ",\"installedOptionCount\":" << installedCount
             << ",\"categories\":[";

        for (std::size_t ci = 0; ci < menu.categories.size(); ++ci) {
            if (ci > 0) json << ",";
            const auto& cat = menu.categories[ci];

            json << "{"
                 << "\"categoryIndex\":" << cat.categoryIndex
                 << ",\"label\":\"" << JsonEscape(cat.label) << "\""
                 << ",\"sourceLabel\":\"" << JsonEscape(cat.sourceLabel) << "\""
                 << ",\"labelCustomized\":" << (cat.labelCustomized ? "true" : "false")
                 << ",\"categoryMessage\":";

            WriteFormRefJson(json, cat.categoryMessage);

            json << ",\"messageButtonIndex\":" << cat.messageButtonIndex
                 << ",\"optionFormList\":";

            WriteFormRefJson(json, cat.optionFormList);

            // Category diagnostic fields
            json << ",\"role\":\"" << (cat.role == EcoCategoryRole::RootCategory ? "RootCategory" : (cat.role == EcoCategoryRole::DependentCategory ? "DependentCategory" : (cat.role == EcoCategoryRole::MixedCategory ? "MixedCategory" : "Unknown"))) << "\"";
            json << ",\"requiredAttachPoints\":";
            WriteFormRefArrayJson(json, cat.requiredAttachPoints);
            json << ",\"providerCreatedAttachPoints\":";
            WriteFormRefArrayJson(json, cat.providerCreatedAttachPoints);
            json << ",\"providerRequired\":" << (cat.providerRequired ? "true" : "false");
            json << ",\"hasVisibleOptions\":" << (cat.hasVisibleOptions ? "true" : "false");
            json << ",\"userHidden\":" << (cat.userHidden ? "true" : "false");
            json << ",\"preferenceHidden\":" << (IsCategoryHidden(menu.weapon, cat) ? "true" : "false");

            json << ",\"options\":[";

            for (std::size_t oi = 0; oi < cat.options.size(); ++oi) {
                if (oi > 0) json << ",";
                const auto& opt = cat.options[oi];

                json << "{"
                     << "\"optionIndex\":" << opt.optionIndex
                     << ",\"messageButtonIndex\":" << opt.messageButtonIndex
                     << ",\"label\":\"" << JsonEscape(opt.label) << "\""
                     << ",\"sourceLabel\":\"" << JsonEscape(opt.sourceLabel) << "\""
                     << ",\"labelCustomized\":" << (opt.labelCustomized ? "true" : "false")
                     << ",\"omod\":";

                WriteFormRefJson(json, opt.omod);

                json << ",\"consumesAttachPoint\":";
                WriteFormRefJson(json, opt.consumesAttachPoint);
                json << ",\"providesAttachParentSlots\":";
                WriteFormRefArrayJson(json, opt.providesAttachParentSlots);
                json << ",\"role\":\"" << (opt.role == EcoOptionRole::ProviderOption ? "ProviderOption" : (opt.role == EcoOptionRole::LeafOption ? "LeafOption" : (opt.role == EcoOptionRole::ToggleOption ? "ToggleOption" : "Unknown"))) << "\"";
                json << ",\"looseMod\":";
                WriteFormRefJson(json, opt.looseMod);
                json << ",\"hasLooseMod\":" << (opt.hasLooseMod ? "true" : "false");
                json << ",\"looseModRequired\":" << (opt.looseModRequired ? "true" : "false");
                json << ",\"isAvailableInInventory\":" << (opt.isAvailableInInventory ? "true" : "false");
                json << ",\"isInstalled\":" << ((opt.isInstalled || opt.isDefaultApplied) ? "true" : "false");
                json << ",\"isDefaultApplied\":" << (opt.isDefaultApplied ? "true" : "false");
                json << ",\"isStructurallyValid\":" << (opt.isStructurallyValid ? "true" : "false");
                json << ",\"isVisible\":" << (opt.isVisible ? "true" : "false");
                json << ",\"isSelectable\":" << (opt.isSelectable ? "true" : "false");
                json << ",\"userHidden\":" << (opt.userHidden ? "true" : "false");
                json << ",\"preferenceHidden\":" << (IsOptionHidden(menu.weapon, opt.omod) ? "true" : "false");
                json << ",\"status\":\"" << JsonEscape(opt.status) << "\"";

                json << "}";
            }

            json << "]}";
        }

        // close categories array and then emit resolver results and edges in the parser object
        json << "],";
        json << "\"graphPreviewAttachPoints\":";
        WriteFormRefArrayJson(json, menu.graphPreviewAttachPoints);
        json << ",\"liveReachableAttachPoints\":";
        WriteFormRefArrayJson(json, menu.liveReachableAttachPoints);
        json << ",\"installedOmodAttachmentInfo\":";
        WriteOmodAttachmentInfoArrayJson(json, menu.installedOmodAttachmentInfo);

        json << ",\"dependencyEdges\": [";
        for (std::size_t ei = 0; ei < menu.dependencyEdges.size(); ++ei) {
            if (ei > 0) json << ",";
            const auto& e = menu.dependencyEdges[ei];
            json << "{";
            json << "\"providerCategoryIndex\":" << e.providerCategoryIndex;
            json << ",\"providerOptionIndex\":" << e.providerOptionIndex;
            json << ",\"childCategoryIndex\":" << e.childCategoryIndex;
            json << ",\"childOptionIndex\":" << e.childOptionIndex;
            json << ",\"providedAttachPoint\":";
            WriteFormRefJson(json, e.providedAttachPoint);
            json << ",\"providerInstalled\":" << (e.providerInstalled ? "true" : "false");
            json << ",\"childCurrentlyValid\":" << (e.childCurrentlyValid ? "true" : "false");
            json << ",\"status\":\"" << JsonEscape(e.status) << "\"";
            json << "}";
        }
        json << "]";

        json << "},";

        const auto& userSettings = GetUserSettingsDiagnostics();
        json << "\"userPreferences\":{"
             << "\"path\":\"" << JsonEscape(userSettings.path.string()) << "\","
             << "\"loadState\":\"" << JsonEscape(userSettings.loadStateText) << "\","
             << "\"loaded\":" << (userSettings.loaded ? "true" : "false") << ","
             << "\"missing\":" << (userSettings.missing ? "true" : "false") << ","
             << "\"gameDataValidated\":" << (userSettings.gameDataValidated ? "true" : "false") << ","
             << "\"validationEnvironmentAvailable\":" << (userSettings.validationEnvironmentAvailable ? "true" : "false") << ","
             << "\"saveAllowed\":" << (userSettings.saveAllowed ? "true" : "false") << ","
             << "\"dirty\":" << (userSettings.dirty ? "true" : "false") << ","
             << "\"saved\":" << (userSettings.saved ? "true" : "false") << ","
             << "\"status\":\"" << JsonEscape(userSettings.status) << "\","
             << "\"blockedSaveReason\":\"" << JsonEscape(userSettings.blockedSaveReason) << "\","
             << "\"weaponCount\":" << userSettings.weaponCount << ","
             << "\"malformedRecordCount\":" << userSettings.malformedRecordCount << ","
             << "\"unsupportedVersionCount\":" << userSettings.unsupportedVersionCount << ","
             << "\"duplicateRecordCount\":" << userSettings.duplicateRecordCount << ","
             << "\"unresolvedRecordCount\":" << userSettings.unresolvedRecordCount << ","
             << "\"staleDocumentRecordCount\":" << userSettings.staleDocumentRecordCount << ","
             << "\"currentMenuStaleRecordCount\":" << userSettings.currentMenuStaleRecordCount
             << "},";
        json << "\"menuProfiles\":{"
             << "\"status\":\"" << JsonEscape(builderProfileStatus_) << "\","
             << "\"message\":\"" << JsonEscape(builderProfileMessage_) << "\","
             << "\"fileName\":\"" << JsonEscape(builderProfileFileName_) << "\","
             << "\"importDirectory\":\"" << JsonEscape(GetWeaponMenuProfileDirectory().generic_string()) << "\","
             << "\"exportDirectory\":\"" << JsonEscape(GetWeaponMenuExportDirectory().generic_string()) << "\","
             << "\"invalidFileCount\":" << menuProfiles.invalidFileCount << ","
             << "\"otherWeaponCount\":" << menuProfiles.otherWeaponCount << ","
             << "\"profiles\":[";
        for (std::size_t index = 0; index < menuProfiles.profiles.size(); ++index) {
            if (index > 0) json << ",";
            const auto& profile = menuProfiles.profiles[index];
            json << "{"
                 << "\"index\":" << profile.index << ","
                 << "\"profileName\":\"" << JsonEscape(profile.profileName) << "\","
                 << "\"fileName\":\"" << JsonEscape(profile.fileName) << "\","
                 << "\"targetPlugin\":\"" << JsonEscape(profile.targetPlugin) << "\","
                 << "\"targetLocalFormId\":\"" << ToHexFormId(profile.targetLocalFormId) << "\""
                 << "}";
        }
        json << "]},";
        json << "\"attachmentModel\":" << BuildAttachmentRuntimeModelDebugJson() << ","
             << "\"settings\":{"
                 << "\"openMenuHotkey\":\"" << JsonEscape(quickMenuHotkey.displayName) << "\","
                 << "\"openMenuHotkeyKeycode\":" << quickMenuHotkey.keycode << ","
                 << "\"openMenuHotkeyModifiers\":" << quickMenuHotkey.modifiers << ","
                 << "\"openMenuBuilderHotkey\":\"" << JsonEscape(builderHotkey.displayName) << "\","
                 << "\"openMenuBuilderHotkeyKeycode\":" << builderHotkey.keycode << ","
                 << "\"openMenuBuilderHotkeyModifiers\":" << builderHotkey.modifiers << ","
                 << "\"closeAfterApply\":" << (quickMenu.closeAfterApply ? "true" : "false") << ","
                 << "\"controllerSupported\":" << (controllerApi_ ? "true" : "false") << ","
                 << "\"controllerQuickShortcut\":\"" << JsonEscape(GetControllerShortcut("quick")) << "\","
                 << "\"loggingEnabled\":" << (quickMenu.loggingEnabled ? "true" : "false") << ","
                 << "\"menuSlowdown\":" << quickMenu.menuSlowdown << ","
                 << "\"hideInvalidOptions\":" << (settings.hideInvalidOptions ? "true" : "false") << ","
                 << "\"menuSource\":\"" << JsonEscape(settings.menuSource) << "\","
                 << "\"autoInstallProviderIfSafe\":" << (settings.autoInstallProviderIfSafe ? "true" : "false") << ","
                 << "\"removeDependentChildrenFirst\":" << (settings.removeDependentChildrenFirst ? "true" : "false") << ","
                 << "\"allowNoLooseModOptions\":" << (settings.allowNoLooseModOptions ? "true" : "false") << ","
                 << "\"useGameUIColor\":" << (settings.useGameUIColor ? "true" : "false") << ","
                 << "\"presentation\":\"" << JsonEscape(quickMenu.presentation) << "\","
                 << "\"backgroundOpacity\":" << quickMenu.backgroundOpacity << ","
                 << "\"theme\":\"" << JsonEscape(quickMenu.theme) << "\","
                 << "\"customAccent\":\"" << JsonEscape(quickMenu.customAccent) << "\","
                 << "\"customText\":\"" << JsonEscape(quickMenu.customText) << "\","
                 << "\"customPanel\":\"" << JsonEscape(quickMenu.customPanel) << "\","
                 << "\"customInstalled\":\"" << JsonEscape(quickMenu.customInstalled) << "\","
                 << "\"hudColor\":\"" << hudColour << "\","
                 << "\"hudBackgroundColor\":\"" << hudBackgroundColour << "\","
                 << "\"useAuthoredMenus\":" << (quickMenu.useAuthoredMenus ? "true" : "false") << ","
                 << "\"controlHints\":\"" << JsonEscape(quickMenu.controlHints) << "\","
                 << "\"builderPanelWidth\":" << quickMenu.builderPanelWidth << ","
                 << "\"builderPanelHeight\":" << quickMenu.builderPanelHeight << ","
                 << "\"settingsPanelWidth\":" << quickMenu.settingsPanelWidth << ","
                 << "\"settingsPanelHeight\":" << quickMenu.settingsPanelHeight << ","
                 << "\"scale\":" << (quickMenu.presentation == "radial" ? quickMenu.radial.scale : quickMenu.presentation == "hybrid" ? quickMenu.hybrid.scale : quickMenu.presentation == "horizontal" ? quickMenu.horizontal.scale : quickMenu.cascade.scale) << ","
                 << "\"positionX\":" << (quickMenu.presentation == "radial" ? quickMenu.radial.positionX : quickMenu.presentation == "hybrid" ? quickMenu.hybrid.positionX : quickMenu.presentation == "horizontal" ? quickMenu.horizontal.positionX : quickMenu.cascade.positionX) << ","
                 << "\"positionY\":" << (quickMenu.presentation == "radial" ? quickMenu.radial.positionY : quickMenu.presentation == "hybrid" ? quickMenu.hybrid.positionY : quickMenu.presentation == "horizontal" ? quickMenu.horizontal.positionY : quickMenu.cascade.positionY) << ","
                 << "\"layouts\":{"
                     << "\"cascade\":{\"scale\":" << quickMenu.cascade.scale << ",\"positionX\":" << quickMenu.cascade.positionX << ",\"positionY\":" << quickMenu.cascade.positionY << "},"
                     << "\"radial\":{\"scale\":" << quickMenu.radial.scale << ",\"positionX\":" << quickMenu.radial.positionX << ",\"positionY\":" << quickMenu.radial.positionY << "},"
                     << "\"hybrid\":{\"scale\":" << quickMenu.hybrid.scale << ",\"positionX\":" << quickMenu.hybrid.positionX << ",\"positionY\":" << quickMenu.hybrid.positionY << "},"
                     << "\"horizontal\":{\"scale\":" << quickMenu.horizontal.scale << ",\"positionX\":" << quickMenu.horizontal.positionX << ",\"positionY\":" << quickMenu.horizontal.positionY << "}"
                 << "},"
                 << "\"hideBracketedText\":" << (settings.hideBracketedText ? "true" : "false") << ","
                 << "\"hideBracketedPrefixes\":" << (bracketedText.hidePrefixes ? "true" : "false") << ","
                 << "\"hideBracketedInfixes\":" << (bracketedText.hideInfixes ? "true" : "false") << ","
                 << "\"hideBracketedSuffixes\":" << (bracketedText.hideSuffixes ? "true" : "false") << ","
                 << "\"showControlHints\":" << (quickMenu.controlHints != "off" ? "true" : "false")
             << "}"
             << "}";

        return json.str();
    }

    void PrismaBridge::CreateMenuViewIfNeeded()
    {
        if (!api_) {
            return;
        }

        if (menuView_ != 0 && api_->IsValid(menuView_)) {
            return;
        }

        viewDomReady_ = false;
        pendingPayload_ = false;
        pendingFocus_ = false;

        const char* viewPath = kMenuViewPath;
        if (viewMode_ == ViewMode::MenuBuilder) {
            viewPath = kMenuBuilderViewPath;
        } else if (viewMode_ == ViewMode::Settings) {
            viewPath = kSettingsViewPath;
        }
        menuView_ = api_->CreateView(viewPath, OnMenuViewDomReady);

        if (menuView_ == 0 || !api_->IsValid(menuView_)) {
            log::Error("Failed to create Prisma menu view.");
            menuView_ = 0;
            return;
        }

        // Capture framework/browser diagnostics during asynchronous loading.
        api_->RegisterConsoleCallback(menuView_, OnMenuConsoleMessage);
        api_->SetOrder(menuView_, 500);
        api_->SetViewRole(menuView_, PRISMA_UI_API::ViewRole::kPanel);
        api_->SetViewOwnsEscape(menuView_, true);
        api_->Hide(menuView_);

        log::Info("Prisma menu view created as an interactive panel with Escape ownership.");
    }

    // Fallout 4 stores gamepad mappings as XInput button bits. Read the
    // currently loaded control map rather than assuming A is always Activate.
    // This is game-thread only, and does not change any user input settings.
    const char* CanonicalGamepadButton(std::uint32_t code)
    {
        switch (code) {
        case XINPUT_GAMEPAD_A: return "A";
        case XINPUT_GAMEPAD_B: return "B";
        case XINPUT_GAMEPAD_X: return "X";
        case XINPUT_GAMEPAD_Y: return "Y";
        case XINPUT_GAMEPAD_DPAD_UP: return "DUp";
        case XINPUT_GAMEPAD_DPAD_DOWN: return "DDown";
        case XINPUT_GAMEPAD_DPAD_LEFT: return "DLeft";
        case XINPUT_GAMEPAD_DPAD_RIGHT: return "DRight";
        case XINPUT_GAMEPAD_LEFT_SHOULDER: return "LB";
        case XINPUT_GAMEPAD_RIGHT_SHOULDER: return "RB";
        case XINPUT_GAMEPAD_BACK: return "Back";
        case XINPUT_GAMEPAD_START: return "Start";
        case XINPUT_GAMEPAD_LEFT_THUMB: return "LS";
        case XINPUT_GAMEPAD_RIGHT_THUMB: return "RS";
        default: return nullptr;
        }
    }

    const char* ReadMappedControllerButton(
        std::string_view event,
        RE::UserEvents::INPUT_CONTEXT_ID context)
    {
        if (const auto* controls = RE::ControlMap::GetSingleton()) {
            return CanonicalGamepadButton(
                controls->GetMappedKey(event, RE::INPUT_DEVICE::kGamepad, context));
        }
        return nullptr;
    }

    void PrismaBridge::BindControllerActions()
    {
        if (!controllerApi_ || menuView_ == 0 || !api_->IsValid(menuView_)) {
            return;
        }

        controllerApi_->ClearControllerActions(menuView_);
        if (viewMode_ != ViewMode::QuickMenu) {
            log::Info("Controller actions intentionally disabled for Builder/Settings.");
            return;
        }
        // Remapped gameplay Activate takes priority. Menu Accept is the
        // fallback for installs that do not expose a usable gameplay binding.
        const char* confirm = ReadMappedControllerButton(
            "Activate", RE::UserEvents::INPUT_CONTEXT_ID::kMainGameplay);
        if (!confirm) {
            confirm = ReadMappedControllerButton(
                "Accept", RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
        }
        if (!confirm) {
            confirm = "A";
            log::Warn("Could not resolve a supported mapped controller confirmation; using default A.");
        }

        const char* cancel = ReadMappedControllerButton(
            "Cancel", RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
        if (!cancel) cancel = "B";
        if (std::string_view(cancel) == confirm) {
            cancel = std::string_view(confirm) == "A" ? "B" : "A";
            log::Warn("Controller accept/cancel mappings overlapped; selected the other face button for cancel.");
        }

        const std::pair<const char*, const char*> bindings[] = {
            { confirm, "accept" },
            { cancel, "cancel" },
            { "LB", "previous" },
            { "RB", "next" },
            { "DUp", "up" },
            { "DDown", "down" },
            { "DLeft", "left" },
            { "DRight", "right" }
        };

        std::size_t bound = 0;
        for (const auto& [button, action] : bindings) {
            // A remapped confirm/cancel button must never also navigate.
            if (std::string_view(action) != "accept" &&
                std::string_view(action) != "cancel" &&
                (std::string_view(button) == confirm || std::string_view(button) == cancel)) {
                log::Warn(std::string("Controller navigation binding skipped because it overlaps confirmation/cancel: ") + button + ".");
                continue;
            }
            if (controllerApi_->BindControllerAction(menuView_, button, action)) {
                ++bound;
            } else {
                log::Warn(std::string("Prisma controller action could not be bound: ") + button + ".");
            }
        }

        // Keep browser-side dispatch tied to the exact buttons bound above.
        // A stick or D-pad navigation event can never submit an attachment.
        api_->InteropCall(menuView_, "k2040ControllerConfirmButton", confirm);
        api_->InteropCall(menuView_, "k2040ControllerCancelButton", cancel);
        log::Info(
            "Prisma controller actions bound: " + std::to_string(bound) +
            " (confirm=" + confirm + ", cancel=" + cancel + ").");
    }

    void PrismaBridge::PushPayloadToView()
    {
        if (!api_ || menuView_ == 0 || !api_->IsValid(menuView_)) {
            return;
        }

        if (!viewDomReady_) {
            pendingPayload_ = true;
            return;
        }

        api_->InteropCall(menuView_, "k2040SetPayload", lastPayload_.c_str());
        pendingPayload_ = false;

        log::Info("Prisma menu payload sent to view.");
    }

    void PrismaBridge::FocusAndShowMenuView()
    {
        if (!api_ || menuView_ == 0 || !api_->IsValid(menuView_)) {
            log::Warn("FocusAndShowMenuView skipped: API or menu view is invalid.");
            return;
        }

        api_->Show(menuView_);
        log::Info("Prisma menu view Show() called.");

        if (!viewDomReady_) {
            pendingFocus_ = true;
            log::Info("Prisma menu view focus deferred until DOM ready.");
            return;
        }

        // Prisma's PauseHold menu hides a lowered-but-drawn weapon as soon as it
        // opens. Freeze game time and disable gameplay input through engine
        // services instead, then focus the browser without a vanilla menu.
        constexpr bool requestedPauseGame = false;
        constexpr bool requestedDisableFocusMenu = true;

        if (!ActivateQuickMenuGameplayIsolation()) {
            api_->Hide(menuView_);
            menuOpen_ = false;
            log::Warn("Prisma menu focus cancelled because gameplay isolation could not be established.");
            return;
        }

        std::uint32_t cursorOwnerCountBeforeFocus = 0;
        if (const auto* cursor = RE::MenuCursor::GetSingleton()) {
            cursorOwnerCountBeforeFocus = cursor->registeredCursors;
        }

        const bool focused = api_->Focus(menuView_, requestedPauseGame, requestedDisableFocusMenu);
        const bool hasFocus = api_->HasFocus(menuView_);
        const bool anyFocus = api_->HasAnyActiveFocus();

        if (focused && hasFocus) {
            EnsureMenuCursorAfterFocus(cursorOwnerCountBeforeFocus);
        }

        pendingFocus_ = false;

        log::Info("Prisma menu view Focus() requested with pauseGame=false, disableFocusMenu=true.");
        log::Info(std::string("Prisma menu view Focus() returned: ") + (focused ? "true" : "false"));
        log::Info(std::string("Prisma menu view HasFocus() after Focus: ") + (hasFocus ? "true" : "false"));
        log::Info(std::string("Prisma HasAnyActiveFocus() after Focus: ") + (anyFocus ? "true" : "false"));

        if (!focused || !hasFocus) {
            SetMenuHotkeyUiForwardingActive(false);
            ReleaseBuilderMenuModeGuard();
            UnregisterMenuCursorFallback();
            ReleaseQuickMenuGameplayIsolation();
            menuOpen_ = false;
            log::Warn("Prisma menu focus failed; any plugin fallback cursor ownership was released.");
            return;
        }
        if (viewMode_ != ViewMode::QuickMenu && !ActivateBuilderMenuModeGuard()) {
            SetMenuHotkeyUiForwardingActive(false);
            api_->Unfocus(menuView_);
            api_->Hide(menuView_);
            UnregisterMenuCursorFallback();
            ReleaseQuickMenuGameplayIsolation();
            menuOpen_ = false;
            log::Warn("Prisma menu-builder focus cancelled because MCM hotkeys could not be isolated.");
            return;
        }

        SetMenuHotkeyUiForwardingActive(true);
        log::Info("Focused Prisma view enabled supplemental opener-hotkey forwarding.");

        // Browser-side focus is a supplement for current and future HTML
        // controls. The view owns a tiny focus-only heartbeat that dirties two
        // pixels twice per second for PrismaUI 2.1's composite watchdog.
        api_->Invoke(menuView_, "try { if (window.k2040StartFocusHeartbeat) { window.k2040StartFocusHeartbeat(); } window.focus(); if (document.body) { document.body.setAttribute('tabindex','-1'); document.body.focus(); } if (window.k2040FocusMenuInitial) { window.k2040FocusMenuInitial(); } } catch(e) { console.error(e); }");
        log::Info("Prisma menu view browser focus and focus-only paint heartbeat requested.");
    }

    void PrismaBridge::CaptureWeaponPresentationState()
    {
        weaponDrawStateCaptured_ = false;
        weaponWasDrawnBeforeOpen_ = false;
        menuOpenedInFirstPerson_ = false;
        firstPersonGeometryWasHiddenBeforeOpen_ = false;

        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            log::Warn("Could not capture the player's weapon presentation before opening the menu.");
            return;
        }

        weaponWasDrawnBeforeOpen_ = player->GetWeaponMagicDrawn();
        firstPersonGeometryWasHiddenBeforeOpen_ = player->hideFirstPersonGeometry;

        if (auto* camera = RE::PlayerCamera::GetSingleton()) {
            menuOpenedInFirstPerson_ = camera->QCameraEquals(RE::CameraState::kFirstPerson);
        }

        weaponDrawStateCaptured_ = true;
        log::Info(std::string("Weapon presentation captured before menu focus: state=") +
            (weaponWasDrawnBeforeOpen_ ? "drawn" : "holstered") +
            ", camera=" + (menuOpenedInFirstPerson_ ? "first-person" : "other") +
            ", firstPersonGeometryHidden=" + (firstPersonGeometryWasHiddenBeforeOpen_ ? "true" : "false"));
    }

    void PrismaBridge::PrepareFirstPersonPresentationRestore()
    {
        const bool hadCapturedState = weaponDrawStateCaptured_;
        const bool wasDrawnBeforeOpen = weaponWasDrawnBeforeOpen_;
        const bool wasFirstPersonBeforeOpen = menuOpenedInFirstPerson_;
        const bool geometryWasHiddenBeforeOpen = firstPersonGeometryWasHiddenBeforeOpen_;
        weaponDrawStateCaptured_ = false;
        weaponWasDrawnBeforeOpen_ = false;
        menuOpenedInFirstPerson_ = false;
        firstPersonGeometryWasHiddenBeforeOpen_ = false;

        if (!hadCapturedState) {
            log::Info("No pre-menu weapon presentation was available for close recovery.");
            pendingFirstPersonPresentationRefresh_ = false;
            return;
        }

        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            log::Warn("Could not restore first-person presentation after menu close because the player is unavailable.");
            pendingFirstPersonPresentationRefresh_ = false;
            return;
        }

        const bool isDrawnAfterClose = player->GetWeaponMagicDrawn();
        log::Info(std::string("Weapon presentation across menu close: before=") +
            (wasDrawnBeforeOpen ? "drawn" : "holstered") +
            ", after=" + (isDrawnAfterClose ? "drawn" : "holstered") +
            ", firstPersonBefore=" + (wasFirstPersonBeforeOpen ? "true" : "false") +
            ", geometryHiddenBefore=" + (geometryWasHiddenBeforeOpen ? "true" : "false") +
            ", geometryHiddenAfter=" + (player->hideFirstPersonGeometry ? "true" : "false"));

        // Prisma pause/focus can leave the first-person model visually hidden
        // even though the actor still reports a drawn weapon. Restore only the
        // presentation that was active before the menu; do not toggle weapon
        // state or force first-person geometry that was already hidden.
        if (!wasDrawnBeforeOpen || !wasFirstPersonBeforeOpen || geometryWasHiddenBeforeOpen) {
            log::Info("First-person presentation refresh was not needed after menu close.");
            pendingFirstPersonPresentationRefresh_ = false;
            return;
        }

        pendingFirstPersonPresentationRefresh_ = true;
        log::Info("First-person presentation refresh armed for the Prisma pause-holder close event.");
    }

    void PrismaBridge::CompleteFirstPersonPresentationRestore(const char* reason)
    {
        const bool restoreFirstPerson = pendingFirstPersonPresentationRefresh_.exchange(false);

        // Coalesce all successful AE OMOD changes from the open Quick Menu
        // into one latest-stack re-equip after menu/Prisma teardown. This avoids
        // running an equip cycle for each rapid change while animations and
        // model loads from prior changes may still be outstanding.
        std::optional<EquippedWeaponInfo> deferredAE;
        if (!menuOpen_ && pendingAEReequipInfo_) {
            deferredAE = std::move(pendingAEReequipInfo_);
            pendingAEReequipInfo_.reset();
        }

        if (!restoreFirstPerson && !deferredAE) {
            return;
        }

        const auto* taskInterface = F4SE::GetTaskInterface();
        if (!taskInterface) {
            log::Warn("Post-close weapon restore skipped because the F4SE task interface is unavailable.");
            return;
        }

        const bool hasDeferredAE = deferredAE.has_value();
        const auto expectedGeneration = menuGeneration_;
        taskInterface->AddTask([this, expectedGeneration, restoreFirstPerson,
                                deferredAE = std::move(deferredAE)]() {
            // A new menu or save transition invalidates both the stored exact
            // stack and any queued post-close work. Never replay an old equip.
            if (menuGeneration_ != expectedGeneration || menuOpen_) {
                k2040::log::Info("Stale post-close weapon task skipped after menu/game transition.");
                return;
            }

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                k2040::log::Warn("Post-close weapon restore skipped because the player is unavailable.");
                return;
            }

            if (deferredAE) {
                auto* weapon = RE::TESForm::GetFormByID<RE::TESObjectWEAP>(deferredAE->weapon.formId);
                if (weapon && TryAutoReequipModifiedWeaponAE(player, weapon, *deferredAE)) {
                    k2040::log::Info("One post-close AE re-equip completed for the latest modified weapon stack.");
                } else {
                    k2040::log::Warn("Post-close AE re-equip was skipped or could not verify the equipped stack.");
                }
            }

            if (!restoreFirstPerson) {
                return;
            }

            const bool weaponIsDrawn = player->GetWeaponMagicDrawn();
            k2040::log::Info(std::string("Post-close game task observed weapon state: ") +
                (weaponIsDrawn ? "drawn" : "holstered"));

            if (!weaponIsDrawn) {
                player->DrawWeaponMagicHands(true);
                k2040::log::Info("Restored the weapon draw state captured before opening the quick menu.");
            }

            if (auto* taskQueue = RE::TaskQueueInterface::GetSingleton()) {
                taskQueue->QueueShow1stPerson(true);
                k2040::log::Info("Queued first-person presentation refresh after post-close weapon work.");
            } else {
                k2040::log::Warn("First-person presentation refresh skipped because the game task queue is unavailable.");
            }
        });

        std::string message = "Scheduled post-close weapon update";
        if (reason && reason[0] != '\0') {
            message += " ";
            message += reason;
        }
        message += hasDeferredAE ? " with deferred AE re-equip." : " with presentation-only recovery.";
        log::Info(message);
    }

    void PrismaBridge::OnPauseHoldClosed()
    {
        CompleteFirstPersonPresentationRestore("after the Prisma pause-holder closed");
    }

    void PrismaBridge::EnsureMenuCursorAfterFocus(std::uint32_t ownerCountBeforeFocus)
    {
        auto* cursor = RE::MenuCursor::GetSingleton();
        if (!cursor) {
            log::Warn("The game cursor is unavailable after Prisma focus.");
            return;
        }

        // Current Prisma providers own the game cursor for focused views. Older
        // providers did not always register one when FocusMenu was disabled, so
        // retain a fallback only when focus did not add a cursor owner. This
        // avoids double ownership and stale constraints across view switches.
        if (!cursorFallbackRegistered_ && cursor->registeredCursors <= ownerCountBeforeFocus) {
            const auto ownerCountBeforeFallback = cursor->registeredCursors;
            RegisterMenuCursorForRuntime(cursor);
            if (cursor->registeredCursors <= ownerCountBeforeFallback) {
                log::Warn("Prisma focus and the plugin fallback both failed to register the game cursor.");
            } else {
                cursorFallbackRegistered_ = true;
                log::Info(
                    "Prisma focus did not register the game cursor; plugin fallback cursor ownership activated (owners " +
                    std::to_string(ownerCountBeforeFallback) + "->" + std::to_string(cursor->registeredCursors) + ").");
            }
        } else if (!cursorFallbackRegistered_) {
            log::Info("Prisma owns the game cursor for the focused mod view.");
        }

        cursor->ClearConstraints();
        log::Info("Game cursor constraints cleared for the focused mod view.");
    }

    void PrismaBridge::UnregisterMenuCursorFallback()
    {
        if (!cursorFallbackRegistered_) {
            return;
        }

        if (auto* cursor = RE::MenuCursor::GetSingleton()) {
            const auto ownerCountBeforeRelease = cursor->registeredCursors;
            UnregisterMenuCursorForRuntime(cursor);
            log::Info(
                "Plugin fallback cursor ownership released after Prisma menu focus (owners " +
                std::to_string(ownerCountBeforeRelease) + "->" + std::to_string(cursor->registeredCursors) + ").");
        } else {
            log::Warn("The game cursor was unavailable while releasing plugin fallback cursor ownership.");
        }

        cursorFallbackRegistered_ = false;
    }

    void PrismaBridge::OpenView(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu, ViewMode mode)
    {
        // Invalidate any previous session's queued post-close refresh. Keep
        // pending OMOD changes across Quick Menu -> Builder hotkey switches.
        ++menuGeneration_;
        pendingFirstPersonPresentationRefresh_ = false;

        // A hidden Prisma 2.1 view can remain valid while its underlying render
        // state is no longer safe to show again. The same stale-view condition
        // was observed across load transitions. Retire a previously closed view
        // before rebuilding the menu so every reopen gets a fresh page and focus
        // lifecycle. This runs outside page callbacks; CloseMenu only hides the
        // current view and the next native hotkey open performs the destruction.
        if (!menuOpen_ && menuView_ != 0) {
            const PrismaView previousView = menuView_;

            menuView_ = 0;
            viewDomReady_ = false;
            pendingPayload_ = false;
            pendingFocus_ = false;

            if (api_ && api_->IsValid(previousView)) {
                if (api_->HasFocus(previousView)) {
                    api_->Unfocus(previousView);
                }

                if (controllerApi_) controllerApi_->ClearControllerActions(previousView);
                api_->Hide(previousView);
                api_->Destroy(previousView);
                log::Info("Closed Prisma menu view destroyed before reopen.");
            }
        }

        quickControllerInputActive_ = false;
        viewMode_ = mode;
        currentWeaponInfo_ = weaponInfo;
        currentMenu_ = menu;
        CaptureWeaponPresentationState();

        if (!api_) {
            Initialize();
        }

        lastPayload_ = BuildMenuPayload(weaponInfo, menu);
        log::Info("Prisma menu payload built.");
        log::Info(std::string("Prisma menu payload size: ") + std::to_string(lastPayload_.size()) + " bytes.");

        if (!api_) {
            log::Warn("PrismaUI API is not available. Payload logged only.");
            ScheduleQuickMenuGameplayIsolationRelease();
            return;
        }

        CreateMenuViewIfNeeded();
        PushPayloadToView();

        menuOpen_ = true;
        log::Info("Prisma menu internal open state set to true.");

        FocusAndShowMenuView();
    }

    void PrismaBridge::OpenMenu(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu)
    {
        OpenView(weaponInfo, menu, ViewMode::QuickMenu);
    }

    void PrismaBridge::OpenMenuBuilder(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu)
    {
        builderProfileStatus_.clear();
        builderProfileMessage_.clear();
        builderProfileFileName_.clear();
        OpenView(weaponInfo, menu, ViewMode::MenuBuilder);
    }

    void PrismaBridge::RequestViewSwitch(ViewMode mode)
    {
        if (!menuOpen_ || mode == viewMode_) {
            return;
        }

        bool expected = false;
        if (!viewSwitchPending_.compare_exchange_strong(expected, true)) {
            log::Info("Prisma internal view switch ignored because another switch is pending.");
            return;
        }

        const auto* taskInterface = F4SE::GetTaskInterface();
        if (!taskInterface) {
            viewSwitchPending_ = false;
            log::Warn("Prisma internal view switch could not be scheduled because the F4SE task interface is unavailable.");
            return;
        }

        taskInterface->AddTask([this, mode]() {
            SwitchView(mode);
        });
    }

    void PrismaBridge::SwitchView(ViewMode mode)
    {
        SetHotkeyCaptureActive(false);
        if (!menuOpen_ || !api_) {
            viewSwitchPending_ = false;
            return;
        }

        const PrismaView previousView = menuView_;
        menuView_ = 0;
        viewDomReady_ = false;
        pendingPayload_ = false;
        pendingFocus_ = false;

        if (previousView != 0 && api_->IsValid(previousView)) {
            api_->Invoke(previousView, "try { if (window.k2040StopFocusHeartbeat) { window.k2040StopFocusHeartbeat(); } } catch(e) { console.error(e); }");
            if (api_->HasFocus(previousView)) {
                api_->Unfocus(previousView);
            }
            if (controllerApi_) controllerApi_->ClearControllerActions(previousView);
            api_->Hide(previousView);
            api_->Destroy(previousView);
        }
        // Prisma releases the active engine cursor owner during Unfocus. Clear
        // the matching fallback state before focusing the replacement view so
        // Builder <-> Settings switches can acquire a new owner.
        UnregisterMenuCursorFallback();

        quickControllerInputActive_ = false;
        viewMode_ = mode;
        lastPayload_ = BuildMenuPayload(currentWeaponInfo_, currentMenu_);
        CreateMenuViewIfNeeded();
        if (menuView_ == 0) {
            viewSwitchPending_ = false;
            log::Warn("Prisma internal view switch failed while creating the requested page.");
            CloseMenuInternal(false);
            return;
        }

        PushPayloadToView();
        viewSwitchPending_ = false;
        FocusAndShowMenuView();
        log::Info(mode == ViewMode::Settings ?
            "Prisma menu switched from the builder to general settings." :
            "Prisma menu returned from general settings to the builder.");
    }

    void PrismaBridge::CloseMenu()
    {
        CloseMenuInternal(false);
    }

    void PrismaBridge::CloseMenuForSwitch()
    {
        CloseMenuInternal(true);
    }

    void PrismaBridge::CloseMenuInternal(bool preserveGameplayIsolation)
    {
        SetHotkeyCaptureActive(false);
        SetMenuHotkeyUiForwardingActive(false);
        quickControllerInputActive_ = false;
        menuOpen_ = false;
        pendingFocus_ = false;
        log::Info("Prisma menu internal open state set to false and pending focus cancelled.");

        ReleaseBuilderMenuModeGuard();

        if (!preserveGameplayIsolation) {
            PrepareFirstPersonPresentationRestore();
        }

        if (!api_ || menuView_ == 0 || !api_->IsValid(menuView_)) {
            UnregisterMenuCursorFallback();
            currentWeaponInfo_ = {};
            currentMenu_ = {};
            attachmentMutationPending_ = false;
            viewSwitchPending_ = false;
            if (preserveGameplayIsolation) {
                log::Info("Prisma menu closed for a hotkey switch; gameplay isolation remains active.");
            } else {
                ScheduleQuickMenuGameplayIsolationRelease();
                CompleteFirstPersonPresentationRestore("without an active Prisma view");
            }
            return;
        }

        bool pauseHoldWasOpen = false;
        if (!preserveGameplayIsolation) {
            if (const auto* ui = RE::UI::GetSingleton()) {
                pauseHoldWasOpen = ui->GetMenuOpen(RE::BSFixedString(kPauseHoldMenuName));
            }
        }

        api_->Invoke(menuView_, "try { if (window.k2040StopFocusHeartbeat) { window.k2040StopFocusHeartbeat(); } } catch(e) { console.error(e); }");
        api_->Unfocus(menuView_);
        log::Info("Prisma menu view Unfocus() called.");

        api_->Hide(menuView_);
        log::Info("Prisma menu view Hide() called.");
        UnregisterMenuCursorFallback();
        currentWeaponInfo_ = {};
        currentMenu_ = {};
        attachmentMutationPending_ = false;
        viewSwitchPending_ = false;

        if (preserveGameplayIsolation) {
            log::Info("Prisma menu closed for a hotkey switch; gameplay isolation remains active.");
        } else {
            ScheduleQuickMenuGameplayIsolationRelease();

            if (pauseHoldWasOpen) {
                log::Info("Waiting for the Prisma pause-holder to close before refreshing first-person presentation.");
            } else {
                CompleteFirstPersonPresentationRestore("after menu close without an active Prisma pause-holder");
            }
        }
    }

    void PrismaBridge::ResetForGameTransition(const char* reason)
    {
        // A task queued for a prior save or game load must never re-equip
        // a weapon after the player/inventory context has changed.
        ++menuGeneration_;
        pendingAEReequipInfo_.reset();
        quickControllerInputActive_ = false;
        const PrismaView previousView = menuView_;

        SetMenuHotkeyUiForwardingActive(false);
        menuOpen_ = false;
        pendingPayload_ = false;
        pendingFocus_ = false;
        viewDomReady_ = false;
        lastPayload_.clear();
        builderProfileStatus_.clear();
        builderProfileMessage_.clear();
        builderProfileFileName_.clear();
        currentWeaponInfo_ = {};
        currentMenu_ = {};
        attachmentMutationPending_ = false;
        viewSwitchPending_ = false;
        menuView_ = 0;
        weaponDrawStateCaptured_ = false;
        weaponWasDrawnBeforeOpen_ = false;
        menuOpenedInFirstPerson_ = false;
        firstPersonGeometryWasHiddenBeforeOpen_ = false;
        pendingFirstPersonPresentationRefresh_ = false;

        ReleaseBuilderMenuModeGuard();

        if (api_ && previousView != 0 && api_->IsValid(previousView)) {
            if (api_->HasFocus(previousView)) {
                api_->Unfocus(previousView);
            }

            if (controllerApi_) controllerApi_->ClearControllerActions(previousView);
            api_->Hide(previousView);
            api_->Destroy(previousView);
            log::Info("Prisma menu view destroyed for game transition.");
        }

        UnregisterMenuCursorFallback();
        ReleaseQuickMenuGameplayIsolation();

        std::string message = "Prisma menu view state reset for game transition";
        if (reason && reason[0] != '\0') {
            message += ": ";
            message += reason;
        }
        message += ".";
        log::Info(message);
    }

    void PrismaBridge::OnDomReady(PrismaView view)
    {
        if (view != menuView_) {
            return;
        }

        viewDomReady_ = true;
        quickControllerInputActive_ = menuOpen_ && viewMode_ == ViewMode::QuickMenu && controllerApi_;
        log::Info("Prisma menu view DOM ready.");

        // Page-facing listeners are safest once the JavaScript context exists.
        // This view currently uses literal text and ships no translation table.
        api_->BindUIEvent(menuView_, "k2040CloseRequested", OnMenuCloseRequested);
        api_->BindUIEvent(menuView_, "k2040HotkeyActionRequested", OnMenuHotkeyActionRequested);
        api_->BindUIEvent(menuView_, "k2040OptionPreviewRequested", OnMenuOptionPreviewRequested);
        api_->BindUIEvent(menuView_, "k2040BuilderChangeRequested", OnMenuBuilderChangeRequested);
        api_->BindUIEvent(menuView_, "k2040SettingsChangeRequested", OnMenuSettingsChangeRequested);
        BindControllerActions();
        if (pendingPayload_ || !lastPayload_.empty()) {
            PushPayloadToView();
        }

        if (pendingFocus_) {
            log::Info("Retrying Prisma menu view focus after DOM ready.");
            FocusAndShowMenuView();
        }
    }

    void PrismaBridge::OnCloseRequested(const char*)
    {
        log::Info("Prisma menu view requested close; queuing it on the game thread.");
        const auto* taskInterface = F4SE::GetTaskInterface();
        if (!taskInterface) {
            log::Warn("Prisma menu close ignored because the F4SE game-thread task interface is unavailable.");
            return;
        }

        taskInterface->AddTask([]() {
            GetPrismaBridge().CloseMenu();
        });
    }

    void PrismaBridge::OnHotkeyActionRequested(const char* argument)
    {
        if (!argument) {
            log::Warn("Focused Prisma view sent an empty hotkey action.");
            return;
        }

        const std::string_view action(argument);
        if (action == "quick") {
            QueueMenuHotkeyActionFromUi(false);
        } else if (action == "builder") {
            QueueMenuHotkeyActionFromUi(true);
        } else {
            log::Warn("Focused Prisma view sent an unknown hotkey action.");
        }
    }

    void PrismaBridge::RefreshBuilderPayload(bool rebuildMenu)
    {
        EquippedWeaponInfo weaponInfo;
        if (rebuildMenu) {
            weaponInfo = GetEquippedWeaponInfo();
            if (weaponInfo.hasWeapon && weaponInfo.weapon.formId == currentMenu_.weapon.formId) {
                currentWeaponInfo_ = weaponInfo;
                currentMenu_ = BuildEcoWeaponMenu_ReadOnly(weaponInfo, true);
                if (currentMenu_.valid) {
                    RegisterOrUpdateWeapon(currentMenu_);
                }
            } else {
                log::Warn("Menu-builder refresh could not rebuild because the equipped weapon changed.");
            }
        }

        for (auto& category : currentMenu_.categories) {
            category.userHidden = false;
            for (auto& option : category.options) {
                option.userHidden = false;
            }
        }

        currentMenu_.userHidden = false;
        ApplyVisibilityPreferences(currentMenu_);

        if (!rebuildMenu) {
            weaponInfo = currentWeaponInfo_;
            if (!weaponInfo.hasWeapon) {
                weaponInfo.hasWeapon = currentMenu_.weapon.formId != 0;
                weaponInfo.weapon = currentMenu_.weapon;
            }
        }
        lastPayload_ = BuildMenuPayload(weaponInfo, currentMenu_);
        PushPayloadToView();
    }

    void PrismaBridge::OnBuilderChangeRequested(const char* argument)
    {
        if (!menuOpen_ || viewMode_ != ViewMode::MenuBuilder || !argument) {
            log::Warn("Menu-builder change rejected because the builder is not ready.");
            return;
        }

        const std::string_view command(argument);
        bool changed = false;

        if (command == "open-settings") {
            RequestViewSwitch(ViewMode::Settings);
            return;
        }

        if (command == "export-profile") {
            const auto result = ExportWeaponMenuProfile(currentMenu_);
            builderProfileStatus_ = result.status;
            builderProfileMessage_ = result.message;
            builderProfileFileName_ = result.fileName;
            RefreshBuilderPayload();
            return;
        }

        constexpr std::string_view importProfilePrefix = "import-profile:";
        if (command.rfind(importProfilePrefix, 0) == 0) {
            const auto value = command.substr(importProfilePrefix.size());
            std::size_t profileIndex = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), profileIndex);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
                builderProfileStatus_ = "import-rejected";
                builderProfileMessage_ = "The selected menu profile was invalid.";
                builderProfileFileName_.clear();
                RefreshBuilderPayload();
                return;
            }

            const auto result = ImportWeaponMenuProfile(currentMenu_, profileIndex);
            builderProfileStatus_ = result.status;
            builderProfileMessage_ = result.message;
            builderProfileFileName_ = result.fileName;
            RefreshBuilderPayload(result.success);
            return;
        }

        const auto builderParts = SplitCommand(command);
        if (builderParts.size() == 4 && builderParts[0] == "set" && builderParts[1] == "builder-panel-size") {
            const auto width = ParseFiniteDouble(builderParts[2]);
            const auto height = ParseFiniteDouble(builderParts[3]);
            if (!width || !height) {
                log::Warn("Menu-builder panel size was malformed.");
                return;
            }
            auto preferences = GetQuickMenuPreferences();
            preferences.builderPanelWidth = *width;
            preferences.builderPanelHeight = *height;
            SetQuickMenuPreferences(preferences);
            const auto& diagnostics = GetUserSettingsDiagnostics();
            if (diagnostics.dirty && !SaveUserPreferences()) {
                log::Warn("Menu-builder panel size could not be saved.");
            }
            RefreshBuilderPayload();
            return;
        }

        constexpr std::string_view menuSourcePrefix = "menu-source:";
        if (command.rfind(menuSourcePrefix, 0) == 0) {
            const auto value = command.substr(menuSourcePrefix.size());
            AuthoredMenuOverride source = AuthoredMenuOverride::Inherit;
            if (value == "authored") source = AuthoredMenuOverride::ForceAuthored;
            else if (value == "generated") source = AuthoredMenuOverride::ForceGenerated;
            else if (value != "inherit") {
                log::Warn("Menu-builder ECO source preference was invalid.");
                return;
            }
            changed = SetAuthoredMenuOverride(currentMenu_.weapon, source);
            const auto& diagnostics = GetUserSettingsDiagnostics();
            const bool saved = !diagnostics.dirty || SaveUserPreferences();
            if (changed && !saved) {
                log::Warn("Menu-builder ECO source preference could not be saved.");
            }
            RefreshBuilderPayload(changed);
            if (!changed) {
                log::Info("Per-weapon ECO source preference already matched the request.");
            } else if (saved) {
                log::Info("Per-weapon ECO source preference saved.");
            }
            return;
        }

        constexpr std::string_view bracketedTextPrefix = "bracketed-text:";
        if (command.rfind(bracketedTextPrefix, 0) == 0) {
            const auto value = command.substr(bracketedTextPrefix.size());
            BracketedTextOverride mode = BracketedTextOverride::Inherit;
            if (value == "hide") mode = BracketedTextOverride::Hide;
            else if (value == "show") mode = BracketedTextOverride::Show;
            else if (value != "inherit") {
                log::Warn("Menu-builder bracketed-text preference was invalid.");
                return;
            }
            changed = SetBracketedTextOverride(currentMenu_.weapon, mode);
            const auto& diagnostics = GetUserSettingsDiagnostics();
            const bool saved = !diagnostics.dirty || SaveUserPreferences();
            if (changed && !saved) {
                log::Warn("Menu-builder bracketed-text preference could not be saved.");
            }
            RefreshBuilderPayload();
            if (!changed) {
                log::Info("Per-weapon bracketed-text preference already matched the request.");
            } else if (saved) {
                log::Info("Per-weapon bracketed-text preference saved.");
            }
            return;
        }

        constexpr std::string_view forceUnsafeSwapsPrefix = "force-unsafe-swaps:";
        if (command.rfind(forceUnsafeSwapsPrefix, 0) == 0) {
            const auto value = command.substr(forceUnsafeSwapsPrefix.size());
            if (value != "0" && value != "1") {
                log::Warn("Menu-builder Force Unsafe Swaps preference was invalid.");
                return;
            }
            const bool enabled = value == "1";
            changed = SetForceUnsafeSwaps(currentMenu_.weapon, enabled);
            const auto& diagnostics = GetUserSettingsDiagnostics();
            const bool saved = !diagnostics.dirty || SaveUserPreferences();
            if (changed && !saved) {
                log::Warn("Menu-builder Force Unsafe Swaps preference could not be saved.");
            }
            RefreshBuilderPayload();
            if (!changed) {
                log::Info("Per-weapon Force Unsafe Swaps preference already matched the request.");
            } else if (saved) {
                log::Info(std::string("Per-weapon Force Unsafe Swaps ") + (enabled ? "enabled." : "disabled; safe swaps restored."));
            }
            return;
        }

        if (!currentMenu_.valid) {
            log::Warn("Menu-builder change rejected because no editable menu is available.");
            return;
        }

        constexpr std::string_view categoryOrderPrefix = "category-order:";
        constexpr std::string_view optionOrderPrefix = "option-order:";

        if (command.rfind(categoryOrderPrefix, 0) == 0) {
            std::vector<std::uint32_t> order;
            if (!ParseIndexList(command.substr(categoryOrderPrefix.size()), order) ||
                order.size() != currentMenu_.categories.size()) {
                log::Warn("Menu-builder category order rejected because it was incomplete or malformed.");
                return;
            }

            std::vector<EcoMenuCategory> reordered;
            reordered.reserve(currentMenu_.categories.size());
            for (const auto categoryIndex : order) {
                const auto found = std::find_if(currentMenu_.categories.begin(), currentMenu_.categories.end(),
                    [categoryIndex](const auto& category) { return category.categoryIndex == categoryIndex; });
                if (found == currentMenu_.categories.end()) {
                    log::Warn("Menu-builder category order rejected because it referenced an unknown category.");
                    return;
                }
                reordered.push_back(*found);
            }
            currentMenu_.categories = std::move(reordered);
            changed = SaveMenuOrder(currentMenu_);
        } else if (command.rfind(optionOrderPrefix, 0) == 0) {
            const auto body = command.substr(optionOrderPrefix.size());
            const auto separator = body.find(':');
            if (separator == std::string_view::npos) {
                return;
            }

            std::uint32_t categoryIndex = 0;
            const auto categoryText = body.substr(0, separator);
            const auto categoryResult = std::from_chars(
                categoryText.data(), categoryText.data() + categoryText.size(), categoryIndex);
            std::vector<std::uint32_t> order;
            if (categoryResult.ec != std::errc{} || categoryResult.ptr != categoryText.data() + categoryText.size() ||
                !ParseIndexList(body.substr(separator + 1), order)) {
                log::Warn("Menu-builder entry order rejected because it was malformed.");
                return;
            }

            const auto categoryIt = std::find_if(currentMenu_.categories.begin(), currentMenu_.categories.end(),
                [categoryIndex](const auto& category) { return category.categoryIndex == categoryIndex; });
            if (categoryIt == currentMenu_.categories.end() || order.size() != categoryIt->options.size()) {
                log::Warn("Menu-builder entry order rejected because its category or list was incomplete.");
                return;
            }

            std::vector<EcoMenuOption> reordered;
            reordered.reserve(categoryIt->options.size());
            for (const auto optionIndex : order) {
                const auto found = std::find_if(categoryIt->options.begin(), categoryIt->options.end(),
                    [optionIndex](const auto& option) { return option.optionIndex == optionIndex; });
                if (found == categoryIt->options.end()) {
                    log::Warn("Menu-builder entry order rejected because it referenced an unknown entry.");
                    return;
                }
                reordered.push_back(*found);
            }
            categoryIt->options = std::move(reordered);
            changed = SaveMenuOrder(currentMenu_);
        } else if (command == "reset") {
            changed = ResetWeaponVisibility(currentMenu_.weapon);
        } else if (command == "reset-order") {
            changed = ResetWeaponOrder(currentMenu_.weapon);
            if (changed) {
                std::stable_sort(currentMenu_.categories.begin(), currentMenu_.categories.end(), [](const auto& left, const auto& right) {
                    return left.categoryIndex < right.categoryIndex;
                });
                for (auto& category : currentMenu_.categories) {
                    std::stable_sort(category.options.begin(), category.options.end(), [](const auto& left, const auto& right) {
                        return left.optionIndex < right.optionIndex;
                    });
                }
            }
        } else if (command == "reset-labels") {
            changed = ResetWeaponLabels(currentMenu_.weapon);
        } else {
            const auto first = command.find(':');
            const auto second = first == std::string_view::npos ? first : command.find(':', first + 1);
            const auto third = second == std::string_view::npos ? second : command.find(':', second + 1);

            if (first == std::string_view::npos) {
                log::Warn("Menu-builder change rejected because its command is malformed.");
                return;
            }

            std::uint32_t categoryIndex = 0;
            const auto categoryEnd = second == std::string_view::npos ? command.size() : second;
            const auto categoryText = command.substr(first + 1, categoryEnd - first - 1);
            const auto categoryResult = std::from_chars(
                categoryText.data(), categoryText.data() + categoryText.size(), categoryIndex);
            if (categoryResult.ec != std::errc{} || categoryResult.ptr != categoryText.data() + categoryText.size()) {
                log::Warn("Menu-builder change rejected because its category index is invalid.");
                return;
            }

            const auto categoryIt = std::find_if(
                currentMenu_.categories.begin(), currentMenu_.categories.end(),
                [categoryIndex](const EcoMenuCategory& category) { return category.categoryIndex == categoryIndex; });
            if (categoryIt == currentMenu_.categories.end()) {
                log::Warn("Menu-builder change rejected because its category was not found.");
                return;
            }

            const auto action = command.substr(0, first);
            if (action == "category-label" && second != std::string_view::npos) {
                std::string label;
                if (!ParseCustomLabel(command.substr(second + 1), label)) {
                    log::Warn("Menu-builder category label rejected because it was invalid or too long.");
                    return;
                }
                changed = SetCategoryLabel(currentMenu_.weapon, *categoryIt, label);
            } else if (action == "option-label" && third != std::string_view::npos) {
                std::uint32_t optionIndex = 0;
                const auto optionText = command.substr(second + 1, third - second - 1);
                const auto optionResult = std::from_chars(
                    optionText.data(), optionText.data() + optionText.size(), optionIndex);
                std::string label;
                if (optionResult.ec != std::errc{} || optionResult.ptr != optionText.data() + optionText.size() ||
                    !ParseCustomLabel(command.substr(third + 1), label)) {
                    log::Warn("Menu-builder entry label rejected because it was invalid or too long.");
                    return;
                }
                const auto optionIt = std::find_if(
                    categoryIt->options.begin(), categoryIt->options.end(),
                    [optionIndex](const EcoMenuOption& option) { return option.optionIndex == optionIndex; });
                if (optionIt == categoryIt->options.end()) {
                    return;
                }
                changed = SetOptionLabel(currentMenu_.weapon, *optionIt, label);
            } else if ((action == "category-up" || action == "category-down") && second == std::string_view::npos) {
                const auto current = static_cast<std::size_t>(std::distance(currentMenu_.categories.begin(), categoryIt));
                const bool movingUp = action == "category-up";
                if ((movingUp && current > 0) || (!movingUp && current + 1 < currentMenu_.categories.size())) {
                    const auto target = movingUp ? current - 1 : current + 1;
                    std::swap(currentMenu_.categories[current], currentMenu_.categories[target]);
                    changed = SaveMenuOrder(currentMenu_);
                }
            } else if (action == "category" && second != std::string_view::npos && third == std::string_view::npos) {
                const auto hiddenText = command.substr(second + 1);
                if (hiddenText != "0" && hiddenText != "1") {
                    return;
                }
                changed = SetCategoryHidden(currentMenu_.weapon, *categoryIt, hiddenText == "1");
            } else if ((action == "option-up" || action == "option-down") && third == std::string_view::npos && second != std::string_view::npos) {
                std::uint32_t optionIndex = 0;
                const auto optionText = command.substr(second + 1);
                const auto optionResult = std::from_chars(
                    optionText.data(), optionText.data() + optionText.size(), optionIndex);
                if (optionResult.ec != std::errc{} || optionResult.ptr != optionText.data() + optionText.size()) {
                    return;
                }
                const auto optionIt = std::find_if(
                    categoryIt->options.begin(), categoryIt->options.end(),
                    [optionIndex](const EcoMenuOption& option) { return option.optionIndex == optionIndex; });
                if (optionIt == categoryIt->options.end()) {
                    return;
                }
                const auto current = static_cast<std::size_t>(std::distance(categoryIt->options.begin(), optionIt));
                const bool movingUp = action == "option-up";
                if ((movingUp && current > 0) || (!movingUp && current + 1 < categoryIt->options.size())) {
                    const auto target = movingUp ? current - 1 : current + 1;
                    std::swap(categoryIt->options[current], categoryIt->options[target]);
                    changed = SaveMenuOrder(currentMenu_);
                }
            } else if (action == "option" && third != std::string_view::npos) {
                std::uint32_t optionIndex = 0;
                const auto optionText = command.substr(second + 1, third - second - 1);
                const auto optionResult = std::from_chars(
                    optionText.data(), optionText.data() + optionText.size(), optionIndex);
                const auto hiddenText = command.substr(third + 1);
                if (optionResult.ec != std::errc{} || optionResult.ptr != optionText.data() + optionText.size() ||
                    (hiddenText != "0" && hiddenText != "1")) {
                    return;
                }

                const auto optionIt = std::find_if(
                    categoryIt->options.begin(), categoryIt->options.end(),
                    [optionIndex](const EcoMenuOption& option) { return option.optionIndex == optionIndex; });
                if (optionIt == categoryIt->options.end()) {
                    return;
                }
                changed = SetOptionHidden(currentMenu_.weapon, optionIt->omod, hiddenText == "1");
            } else {
                log::Warn("Menu-builder change rejected because its action is unsupported.");
                return;
            }
        }

        const auto& diagnostics = GetUserSettingsDiagnostics();
        const bool saved = !diagnostics.dirty || SaveUserPreferences();
        if (changed && !saved) {
            log::Warn("Menu-builder change could not be saved.");
        }

        RefreshBuilderPayload();
        log::Info(changed ? "Menu-builder change saved." : "Menu-builder state already matched the request.");
    }

    void PrismaBridge::OnSettingsChangeRequested(const char* argument)
    {
        if (!menuOpen_ || viewMode_ != ViewMode::Settings || !argument) {
            log::Warn("General-settings change rejected because the settings page is not ready.");
            return;
        }

        const std::string_view command(argument);
        if (command == "back") {
            SetHotkeyCaptureActive(false);
            RequestViewSwitch(ViewMode::MenuBuilder);
            return;
        }

        if (command == "keybind-capture:1" || command == "keybind-capture:0") {
            SetHotkeyCaptureActive(command.back() == '1');
            return;
        }

        const auto parts = SplitCommand(command);
        if (parts.size() == 4 && parts[0] == "keybind") {
            std::uint32_t keycode = 0;
            std::uint32_t modifiers = 0;
            const auto keyResult = std::from_chars(parts[2].data(), parts[2].data() + parts[2].size(), keycode);
            const auto modifierResult = std::from_chars(parts[3].data(), parts[3].data() + parts[3].size(), modifiers);
            const std::string id(parts[1]);
            SetHotkeyCaptureActive(false);
            if (keyResult.ec != std::errc{} || keyResult.ptr != parts[2].data() + parts[2].size() ||
                modifierResult.ec != std::errc{} || modifierResult.ptr != parts[3].data() + parts[3].size() ||
                !SetSharedHotkeyBinding(id, keycode, modifiers)) {
                log::Warn("General-settings keybinding update was rejected.");
            }
            RefreshBuilderPayload();
            return;
        }

        if (parts.size() == 3 && parts[0] == "controller-shortcut") {
            if (!SetControllerShortcut(parts[1], parts[2])) {
                log::Warn("Controller shortcut rejected (invalid, duplicate, or could not be saved).");
            }
            RefreshBuilderPayload();
            return;
        }

        constexpr std::string_view prefix = "bracketed-text:";
        if (command.rfind(prefix, 0) == 0) {
            const auto value = command.substr(prefix.size());
            if (value.size() != 5 || value[1] != ':' || value[3] != ':' ||
                (value[0] != '0' && value[0] != '1') ||
                (value[2] != '0' && value[2] != '1') ||
                (value[4] != '0' && value[4] != '1')) {
                log::Warn("General bracketed-text settings were malformed.");
                return;
            }
            SetBracketedTextPreferences({ value[0] == '1', value[2] == '1', value[4] == '1' });
        } else if (parts.size() >= 2 && parts[0] == "set") {
            auto preferences = GetQuickMenuPreferences();
            const auto boolValue = [&](std::string_view value) -> std::optional<bool> {
                if (value == "1") return true;
                if (value == "0") return false;
                return std::nullopt;
            };
            bool rebuildMenu = false;
            bool updateLogging = false;
            bool updateMenuSlowdown = false;
            if (parts.size() == 3 && parts[1] == "presentation") preferences.presentation = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "opacity") {
                const auto value = ParseFiniteDouble(parts[2]);
                if (!value) return;
                preferences.backgroundOpacity = *value;
            } else if (parts.size() == 3 && parts[1] == "theme") preferences.theme = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "custom-accent") preferences.customAccent = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "custom-text") preferences.customText = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "custom-panel") preferences.customPanel = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "custom-installed") preferences.customInstalled = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "use-authored") {
                const auto value = boolValue(parts[2]); if (!value) return;
                rebuildMenu = preferences.useAuthoredMenus != *value;
                preferences.useAuthoredMenus = *value;
            } else if (parts.size() == 3 && parts[1] == "control-hints") preferences.controlHints = std::string(parts[2]);
            else if (parts.size() == 3 && parts[1] == "close-after-apply") {
                const auto value = boolValue(parts[2]); if (!value) return; preferences.closeAfterApply = *value;
            } else if (parts.size() == 3 && parts[1] == "logging-enabled") {
                const auto value = boolValue(parts[2]); if (!value) return;
                updateLogging = preferences.loggingEnabled != *value;
                preferences.loggingEnabled = *value;
            } else if (parts.size() == 3 && parts[1] == "menu-slowdown") {
                const auto value = ParseFiniteDouble(parts[2]);
                if (!value) return;
                updateMenuSlowdown = preferences.menuSlowdown != *value;
                preferences.menuSlowdown = *value;
            } else if (parts.size() == 4 && parts[1] == "settings-panel-size") {
                const auto width = ParseFiniteDouble(parts[2]);
                const auto height = ParseFiniteDouble(parts[3]);
                if (!width || !height) return;
                preferences.settingsPanelWidth = *width;
                preferences.settingsPanelHeight = *height;
            } else if (parts.size() == 6 && parts[1] == "layout") {
                const auto scale = ParseFiniteDouble(parts[3]);
                const auto positionX = ParseFiniteDouble(parts[4]);
                const auto positionY = ParseFiniteDouble(parts[5]);
                if (!scale || !positionX || !positionY) return;
                QuickMenuLayoutSettings* layout = nullptr;
                if (parts[2] == "cascade") layout = &preferences.cascade;
                else if (parts[2] == "radial") layout = &preferences.radial;
                else if (parts[2] == "hybrid") layout = &preferences.hybrid;
                else if (parts[2] == "horizontal") layout = &preferences.horizontal;
                if (!layout) return;
                *layout = { *scale, *positionX, *positionY };
            } else {
                log::Warn("General-settings change rejected because its setting is unsupported.");
                return;
            }
            const bool changed = SetQuickMenuPreferences(preferences);
            const auto& diagnostics = GetUserSettingsDiagnostics();
            const bool saved = !diagnostics.dirty || SaveUserPreferences();
            if (!saved) {
                log::Warn("General settings could not be saved.");
            } else if (changed) {
                if (updateMenuSlowdown) {
                    UpdateQuickMenuTimeAdjustment(GetQuickMenuPreferences().menuSlowdown);
                }
                if (updateLogging) {
                    log::SetEnabled(GetQuickMenuPreferences().loggingEnabled);
                }
            }
            RefreshBuilderPayload(rebuildMenu);
            return;
        } else if (parts.size() == 2 && parts[0] == "reset") {
            SetHotkeyCaptureActive(false);
            if (parts[1] == "all") {
                ResetAllUserPreferences();
                ResetSharedHotkeyBindings();
                if (!ResetControllerShortcuts()) log::Warn("Controller shortcut reset failed.");
            } else if (parts[1] == "controls") {
                ResetSharedHotkeyBindings();
                if (!ResetControllerShortcuts()) log::Warn("Controller shortcut reset failed.");
                RefreshBuilderPayload();
                return;
            } else {
                auto preferences = GetQuickMenuPreferences();
                const auto& legacy = GetSettings();
                const QuickMenuLayoutSettings defaultLayout{
                    std::clamp(static_cast<double>(legacy.scale), 0.5, 1.5),
                    std::clamp(static_cast<double>(legacy.positionX), 0.05, 0.95),
                    std::clamp(static_cast<double>(legacy.positionY), 0.05, 0.95)
                };
                if (parts[1] == "appearance") {
                    preferences.backgroundOpacity = 0.92;
                    preferences.theme = "default";
                    preferences.customAccent = "#66c4ff";
                    preferences.customText = "#f4faff";
                    preferences.customPanel = "#0b1218";
                    preferences.customInstalled = "#74e398";
                } else if (parts[1] == "presentation") {
                    preferences.presentation = "cascade";
                    preferences.cascade = defaultLayout;
                    preferences.radial = defaultLayout;
                    preferences.hybrid = defaultLayout;
                    preferences.horizontal = defaultLayout;
                } else if (parts[1] == "behavior") {
                    std::string sourceMode = legacy.menuSource;
                    std::transform(sourceMode.begin(), sourceMode.end(), sourceMode.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    preferences.useAuthoredMenus = sourceMode != "generatedonly";
                    preferences.controlHints = legacy.showControlHints ? "always" : "contextual";
                    preferences.closeAfterApply = legacy.closeAfterApply;
                    preferences.loggingEnabled = true;
                    preferences.menuSlowdown = 1.0;
                } else if (parts[1] == "labels") {
                    SetBracketedTextPreferences({ legacy.hideBracketedText, legacy.hideBracketedText, legacy.hideBracketedText });
                } else if (parts[1] == "builder-panel-size") {
                    preferences.builderPanelWidth = 0.0;
                    preferences.builderPanelHeight = 0.0;
                } else if (parts[1] == "settings-panel-size") {
                    preferences.settingsPanelWidth = 0.0;
                    preferences.settingsPanelHeight = 0.0;
                } else {
                    return;
                }
                SetQuickMenuPreferences(preferences);
            }
        } else {
            log::Warn("General-settings change rejected because its action is unsupported.");
            return;
        }

        const auto& diagnostics = GetUserSettingsDiagnostics();
        const bool saved = !diagnostics.dirty || SaveUserPreferences();
        if (!saved) {
            log::Warn("General settings could not be saved.");
        } else if (parts.size() == 2 && parts[0] == "reset" && (parts[1] == "all" || parts[1] == "behavior")) {
            const auto preferences = GetQuickMenuPreferences();
            UpdateQuickMenuTimeAdjustment(preferences.menuSlowdown);
            log::SetEnabled(preferences.loggingEnabled);
        }
        RefreshBuilderPayload(parts.size() == 2 && parts[0] == "reset" && (parts[1] == "all" || parts[1] == "behavior"));
    }

    void PrismaBridge::SendSelectionResult(
        const char* selectionKey,
        bool accepted,
        bool installed,
        const std::string& status,
        const std::string& message)
    {
        if (!api_ || menuView_ == 0 || !api_->IsValid(menuView_) || !viewDomReady_) {
            return;
        }

        std::ostringstream json;
        json
            << "{"
            << "\"selectionKey\":\"" << JsonEscape(selectionKey ? selectionKey : "") << "\","
            << "\"accepted\":" << (accepted ? "true" : "false") << ","
            << "\"installed\":" << (installed ? "true" : "false") << ","
            << "\"status\":\"" << JsonEscape(status) << "\","
            << "\"message\":\"" << JsonEscape(message) << "\""
            << "}";

        const std::string result = json.str();
        api_->InteropCall(menuView_, "k2040SetSelectionResult", result.c_str());
    }

    void PrismaBridge::OnOptionPreviewRequested(const char* argument)
    {
        std::uint32_t categoryIndex = 0;
        std::uint32_t optionIndex = 0;

        if (!menuOpen_ || !currentMenu_.valid) {
            log::Warn("Cascade selection rejected because no valid menu is open.");
            SendSelectionResult(argument, false, false, "menu-not-ready", "The weapon menu is no longer active.");
            return;
        }

        if (!ParseSelectionKey(argument, categoryIndex, optionIndex)) {
            log::Warn("Cascade selection rejected because its key is malformed.");
            SendSelectionResult(argument, false, false, "invalid-selection", "That menu selection is invalid.");
            return;
        }

        const auto categoryIt = std::find_if(
            currentMenu_.categories.begin(), currentMenu_.categories.end(),
            [categoryIndex](const EcoMenuCategory& category) {
                return category.categoryIndex == categoryIndex;
            });

        if (categoryIt == currentMenu_.categories.end() || categoryIt->userHidden || !categoryIt->hasVisibleOptions) {
            log::Warn("Cascade selection rejected because its category is unavailable.");
            SendSelectionResult(argument, false, false, "category-unavailable", "That category is unavailable.");
            return;
        }

        const auto optionIt = std::find_if(
            categoryIt->options.begin(), categoryIt->options.end(),
            [optionIndex](const EcoMenuOption& option) {
                return option.optionIndex == optionIndex;
            });

        if (optionIt == categoryIt->options.end() || optionIt->userHidden || !optionIt->isVisible) {
            log::Warn("Cascade selection rejected because its option is unavailable.");
            SendSelectionResult(argument, false, false, "option-unavailable", "That option is unavailable.");
            return;
        }

        if (optionIt->isDefaultApplied) {
            log::Info(
                "Cascade default selection already effective; no OMOD install or weapon refresh: OMOD=" +
                ToHexFormId(optionIt->omod.formId) + ".");
            SendSelectionResult(argument, false, true, "default-applied",
                "The default material is already applied. No change was made.");
            return;
        }

        if (optionIt->isInstalled) {
            log::Info(
                "Cascade selection confirmed installed option: category=\"" + categoryIt->label +
                "\", option=\"" + optionIt->label + "\", OMOD=" + ToHexFormId(optionIt->omod.formId) + ".");
            SendSelectionResult(argument, false, true, "installed", "This option is already installed. No change was made.");
            return;
        }

        if (!optionIt->isSelectable || !optionIt->isStructurallyValid) {
            log::Info(
                "Cascade selection rejected by runtime validation: category=\"" + categoryIt->label +
                "\", option=\"" + optionIt->label + "\", status=\"" + optionIt->status + "\".");
            SendSelectionResult(argument, false, false, optionIt->status, "This option is currently blocked. No change was made.");
            return;
        }

        bool expected = false;
        if (!attachmentMutationPending_.compare_exchange_strong(expected, true)) {
            SendSelectionResult(argument, false, false, "mutation-pending", "Another attachment change is still being checked.");
            return;
        }

        AttachmentMutationRequest request;
        request.expectedWeaponFormId = currentMenu_.weapon.formId;
        request.targetOmodFormId = optionIt->omod.formId;
        request.consumedAttachPointFormId = optionIt->consumesAttachPoint.formId;

        const std::string selectionKey = argument ? argument : "";
        log::Info(
            "Cascade selection queued for guarded game-thread mutation: category=\"" + categoryIt->label +
            "\", option=\"" + optionIt->label + "\", OMOD=" + ToHexFormId(optionIt->omod.formId) + ".");

        if (const auto* taskInterface = F4SE::GetTaskInterface()) {
            taskInterface->AddTask([this, selectionKey, request]() {
                BeginAttachmentMutation(selectionKey, request);
            });
            return;
        }

        attachmentMutationPending_ = false;
        SendSelectionResult(argument, false, false, "task-interface-unavailable", "The attachment could not be changed safely.");
    }

    void PrismaBridge::BeginAttachmentMutation(
        std::string selectionKey,
        AttachmentMutationRequest request)
    {
        if (!menuOpen_ || !currentMenu_.valid || currentMenu_.weapon.formId != request.expectedWeaponFormId) {
            attachmentMutationPending_ = false;
            log::Warn("Queued attachment mutation cancelled because the menu or equipped-weapon snapshot changed.");
            SendSelectionResult(selectionKey.c_str(), false, false, "menu-state-changed", "The weapon menu changed before the attachment could be applied.");
            return;
        }

        const auto preparation = PrepareAttachmentReturn(request);
        if (!preparation.success) {
            attachmentMutationPending_ = false;
            log::Warn("Attachment return preparation failed: " + preparation.status + " / " + preparation.message);
            SendSelectionResult(selectionKey.c_str(), false, false, preparation.status, preparation.message);
            return;
        }

        request.expectedPreviousOmodFormId = preparation.previousOmodFormId;
        request.unsafeOverrideUsed = preparation.unsafeOverrideUsed;
        request.dependentRemovalOmodFormIds = preparation.dependentRemovalOmodFormIds;

        for (const auto& plannedReturn : preparation.looseReturns) {
            AttachmentLooseReturn preparedReturn = plannedReturn;
            preparedReturn.quantity = 0;
            request.preparedLooseReturns.push_back(preparedReturn);
            auto& progress = request.preparedLooseReturns.back();

            for (std::uint32_t item = 0; item < plannedReturn.quantity; ++item) {
                log::Info(
                    "Returning an installed loose mod through Fallout's pickup path before changing the weapon: form=" +
                    ToHexFormId(plannedReturn.looseFormId) +
                    ", count=" + std::to_string(plannedReturn.countBefore + item) + "->" +
                    std::to_string(plannedReturn.countBefore + item + 1) + ".");

                if (!ReturnLooseModThroughPickup(plannedReturn.looseFormId)) {
                    const bool cancelled = CancelPreparedAttachmentReturn(request);
                    attachmentMutationPending_ = false;
                    log::Warn("Fallout could not return an installed loose mod through its pickup path.");
                    SendSelectionResult(
                        selectionKey.c_str(),
                        false,
                        false,
                        cancelled ? "loose-mod-return-unavailable" : "prepared-return-cleanup-failed",
                        cancelled
                            ? "Fallout's inventory return path was unavailable, so no weapon change was made."
                            : "The inventory return failed and could not be undone. Reload the test save before continuing.");
                    return;
                }
                ++progress.quantity;
            }
        }

        CompleteAttachmentMutation(std::move(selectionKey), request);
    }

    void PrismaBridge::CompleteAttachmentMutation(
        std::string selectionKey,
        AttachmentMutationRequest request)
    {
        if (!menuOpen_ || !currentMenu_.valid || currentMenu_.weapon.formId != request.expectedWeaponFormId) {
            const bool returnCancelled = CancelPreparedAttachmentReturn(request);
            attachmentMutationPending_ = false;
            log::Warn("Queued attachment mutation cancelled because the menu or equipped-weapon snapshot changed.");
            SendSelectionResult(
                selectionKey.c_str(),
                false,
                false,
                returnCancelled ? "menu-state-changed" : "prepared-return-cleanup-failed",
                returnCancelled
                    ? "The weapon menu changed before the attachment could be applied."
                    : "The weapon menu changed and the prepared inventory return could not be undone. Reload the test save before continuing.");
            return;
        }

        auto result = ApplySimpleAttachmentChange(request);
        attachmentMutationPending_ = false;

        if (!result.success) {
            log::Warn("Guarded attachment mutation failed: " + result.status + " / " + result.message);
            SendSelectionResult(selectionKey.c_str(), false, false, result.status, result.message);
            return;
        }

        log::Info("Post-mutation stage: begin read-only UI rebuild.");
        currentWeaponInfo_ = result.weaponInfo;
        currentMenu_ = result.menu;
        ApplyVisibilityPreferences(currentMenu_);
        lastPayload_ = BuildMenuPayload(currentWeaponInfo_, currentMenu_);
        log::Info("Post-mutation stage: menu payload built; beginning Prisma update.");
        PushPayloadToView();
        log::Info("Post-mutation stage: Prisma update returned.");

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* weapon = RE::TESForm::GetFormByID<RE::TESObjectWEAP>(request.expectedWeaponFormId);
        log::Info("Post-mutation stage: engine refresh prerequisites resolved.");
        if (player && weapon) {
            if (REX::FModule::GetExecutingModule().GetFileVersion() ==
                    REL::Version{ 1, 11, 240, 0 } &&
                GetSettings().aeAutoReequipAfterApply) {
                // This remains a strictly opt-in AE-only workaround. Keep only
                // the latest successfully verified stack until final menu
                // close; no repeated equip cycles inside the focused menu.
                pendingAEReequipInfo_ = result.weaponInfo;
                log::Info("Post-mutation stage: coalesced AE weapon refresh for menu close.");
            } else {
                log::Info("Post-mutation stage: checking equipped weapon refresh compatibility.");
                if (RefreshModifiedEquippedItem(player, weapon)) {
                    log::Info("Post-mutation stage: equipped weapon refresh returned.");
                    log::Info("Requested the equipped weapon-slot refresh after the verified attachment transaction.");
                } else {
                    log::Info("Post-mutation stage: incompatible equipped weapon refresh safely skipped.");
                }
            }
        } else {
            log::Warn("Equipped weapon-slot refresh could not be requested because the player or weapon is unavailable.");
        }

        log::Info("Guarded attachment mutation completed and the cascade payload was refreshed.");
        SendSelectionResult(selectionKey.c_str(), true, true, result.status, result.message);
        if (GetQuickMenuPreferences().closeAfterApply) {
            log::Info("Closing the quick menu after the successful attachment change by user preference.");
            CloseMenu();
        }
    }
}
