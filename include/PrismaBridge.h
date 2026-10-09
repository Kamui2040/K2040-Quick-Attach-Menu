#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "RuntimeState.h"

#include "PrismaUI_F4_API.h"

namespace k2040
{
    class PrismaBridge
    {
    public:
        bool Initialize();
        void EnsureDockRegistration();
        bool IsAvailable() const;
        bool CanOpenFromHotkey() const;
        bool BeginOpenFromHotkey();
        bool IsMenuFocused() const;
        bool IsMenuBuilderOpen() const;

        void OpenMenu(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu);
        void OpenMenuBuilder(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu);
        void CloseMenu();
        void CloseMenuForSwitch();
        void ResetForGameTransition(const char* reason);

        void OnDomReady(PrismaView view);
        void OnCloseRequested(const char* argument);
        void OnHotkeyActionRequested(const char* argument);
        void OnOptionPreviewRequested(const char* argument);
        void OnBuilderChangeRequested(const char* argument);
        void OnSettingsChangeRequested(const char* argument);
        void OnPauseHoldClosed();

    private:
        enum class ViewMode
        {
            QuickMenu,
            MenuBuilder,
            Settings
        };

        PRISMA_UI_API::IVPrismaUI10* api_ = nullptr;
        PRISMA_UI_API::IVPrismaUI12* controllerApi_ = nullptr;
        PrismaView menuView_ = 0;

        bool pendingPayload_ = false;
        bool pendingFocus_ = false;
        bool viewDomReady_ = false;
        bool menuOpen_ = false;
        bool weaponDrawStateCaptured_ = false;
        bool weaponWasDrawnBeforeOpen_ = false;
        bool menuOpenedInFirstPerson_ = false;
        bool firstPersonGeometryWasHiddenBeforeOpen_ = false;
        bool cursorFallbackRegistered_ = false;
        std::atomic_bool pendingFirstPersonPresentationRefresh_ = false;
        std::atomic_bool attachmentMutationPending_ = false;
        std::atomic_bool viewSwitchPending_ = false;
        ViewMode viewMode_ = ViewMode::QuickMenu;

        std::string lastPayload_;
        std::string builderProfileStatus_;
        std::string builderProfileMessage_;
        std::string builderProfileFileName_;
        EquippedWeaponInfo currentWeaponInfo_;
        EcoWeaponMenu currentMenu_;

        std::string BuildMenuPayload(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu) const;

        void CreateMenuViewIfNeeded();
        void BindControllerActions();
        void OpenView(const EquippedWeaponInfo& weaponInfo, const EcoWeaponMenu& menu, ViewMode mode);
        void RequestViewSwitch(ViewMode mode);
        void SwitchView(ViewMode mode);
        void CloseMenuInternal(bool preserveGameplayIsolation);
        void RefreshBuilderPayload(bool rebuildMenu = false);
        void PushPayloadToView();
        void FocusAndShowMenuView();
        void CaptureWeaponPresentationState();
        void PrepareFirstPersonPresentationRestore();
        void CompleteFirstPersonPresentationRestore(const char* reason);
        void EnsureMenuCursorAfterFocus(std::uint32_t ownerCountBeforeFocus);
        void UnregisterMenuCursorFallback();
        void BeginAttachmentMutation(
            std::string selectionKey,
            AttachmentMutationRequest request);
        void CompleteAttachmentMutation(
            std::string selectionKey,
            AttachmentMutationRequest request);
        void SendSelectionResult(
            const char* selectionKey,
            bool accepted,
            bool installed,
            const std::string& status,
            const std::string& message);
    };

    PrismaBridge& GetPrismaBridge();
}
