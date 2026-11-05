from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class QuickMenuMovementContract(unittest.TestCase):
    def test_preference_round_trips_and_defaults_off(self):
        header = (ROOT / "include/UserSettings.h").read_text()
        settings = (ROOT / "src/UserSettings.cpp").read_text()
        bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        self.assertIn("bool allowMovementWhileOpen = false;", header)
        self.assertIn('writeBool("allowMovementWhileOpen"', settings)
        self.assertIn('ReadOptionalBoolMember(*general, "allowMovementWhileOpen"', settings)
        self.assertIn("allowMovementWhileOpen.value_or(false)", settings)
        self.assertIn("current.allowMovementWhileOpen == value.allowMovementWhileOpen", settings)
        self.assertIn("general.allowMovementWhileOpen = value.allowMovementWhileOpen", settings)
        self.assertIn('allowMovementWhileOpen', bridge)
        self.assertIn('parts[1] == "allow-movement"', bridge)
        self.assertIn('preferences.allowMovementWhileOpen = false;', bridge)

    def test_only_quick_menu_open_can_allow_movement(self):
        bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        menu = (ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/menu.html").read_text()
        settings = (ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html").read_text()
        self.assertIn("g_allowMovementForCurrentQuickMenuOpen = k2040::GetQuickMenuPreferences().allowMovementWhileOpen", bridge)
        self.assertIn("viewMode_ == ViewMode::QuickMenu && g_allowMovementForCurrentQuickMenuOpen", bridge)
        isolation = bridge.split("bool ActivateQuickMenuGameplayIsolation(bool allowMovement)", 1)[1].split("void ReleaseQuickMenuGameplayIsolation()", 1)[0]
        self.assertIn("const auto allowedUserEvents = allowMovement ?", isolation)
        self.assertIn("RE::UserEvents::USER_EVENT_FLAG::kLooking", isolation)
        self.assertIn("~allowedUserEvents", isolation)
        self.assertIn("RE::UserEvents::USER_EVENT_FLAG::kAll", isolation)
        self.assertIn("RE::UserEvents::USER_EVENT_FLAG::kMovement,\n            allowMovement,", isolation)
        self.assertIn("RE::UserEvents::USER_EVENT_FLAG::kLooking,\n            allowMovement,", isolation)
        begin_open = bridge.split("bool PrismaBridge::BeginOpenFromHotkey()", 1)[1].split("std::string PrismaBridge::BuildMenuPayload", 1)[0]
        release = bridge.split("void ReleaseQuickMenuGameplayIsolation()", 1)[1].split("void FinishDeferredQuickMenuGameplayIsolationRelease", 1)[0]
        self.assertIn("g_allowMovementForCurrentQuickMenuOpen = k2040::GetQuickMenuPreferences().allowMovementWhileOpen", begin_open)
        self.assertNotIn("g_allowMovementForCurrentQuickMenuOpen = k2040::GetQuickMenuPreferences().allowMovementWhileOpen", release)
        self.assertIn("ActivateQuickMenuGameplayIsolation(viewMode_ == ViewMode::QuickMenu && g_allowMovementForCurrentQuickMenuOpen)", bridge)
        switch_view = bridge.split("void PrismaBridge::SwitchView(ViewMode mode)", 1)[1].split("void PrismaBridge::", 1)[0]
        self.assertLess(switch_view.index("viewMode_ = mode;"), switch_view.index("FocusAndShowMenuView();"))
        self.assertIn('"allow-movement":"allowMovementWhileOpen"', settings)
        self.assertIn('"set:" + command', settings)
        self.assertIn('renderer.settings().allowMovementWhileOpen && ["w", "a", "s", "d"]', menu)
        self.assertIn('key === "ArrowRight" || key === "d"', menu)

    def test_hidden_cursor_is_scoped_to_quick_menu_and_restored(self):
        bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        focus = bridge.split("void PrismaBridge::FocusAndShowMenuView()", 1)[1].split("void PrismaBridge::", 1)[0]
        switch_view = bridge.split("void PrismaBridge::SwitchView(ViewMode mode)", 1)[1].split("void PrismaBridge::", 1)[0]
        close = bridge.split("void PrismaBridge::CloseMenuInternal(bool preserveGameplayIsolation)", 1)[1].split("void PrismaBridge::ResetForGameTransition", 1)[0]
        transition = bridge.split("void PrismaBridge::ResetForGameTransition", 1)[1].split("void PrismaBridge::", 1)[0]
        self.assertIn("SetViewCursorPolicy", focus)
        self.assertIn("CursorPolicy::Hidden", focus)
        self.assertIn("viewMode_ == ViewMode::QuickMenu && g_allowMovementForCurrentQuickMenuOpen", focus)
        self.assertIn("SetViewCursorPolicy", switch_view)
        self.assertIn("CursorPolicy::Default", switch_view)
        self.assertIn("SetViewCursorPolicy", close)
        self.assertIn("CursorPolicy::Default", close)
        self.assertIn("SetViewCursorPolicy", transition)
        self.assertIn("CursorPolicy::Default", transition)
        self.assertIn("SetViewCursorPolicy", focus)
        self.assertIn("CursorPolicy::Default", focus)
        self.assertIn("does not support a hidden cursor", focus)


if __name__ == "__main__":
    unittest.main()
