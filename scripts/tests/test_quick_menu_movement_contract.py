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
        self.assertIn("~static_cast<std::uint32_t>(RE::UserEvents::USER_EVENT_FLAG::kMovement)", isolation)
        self.assertIn("RE::UserEvents::USER_EVENT_FLAG::kMovement,\n            allowMovement,", isolation)
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


if __name__ == "__main__":
    unittest.main()
