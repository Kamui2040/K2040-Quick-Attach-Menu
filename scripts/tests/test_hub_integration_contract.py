"""Quick Attach child-owned MCM integration and standalone contracts."""
from pathlib import Path
import unittest
import json

ROOT=Path(__file__).resolve().parents[2]

class McmChildContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hotkey=(ROOT/"src/Hotkey.cpp").read_text()
        cls.controller=(ROOT/"src/ControllerShortcuts.cpp").read_text()
        cls.main=(ROOT/"src/main.cpp").read_text()
        cls.html=(ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html").read_text()
        cls.user=(ROOT/"src/UserSettings.cpp").read_text()

    def test_no_owner_mcm_pages_or_keybinds(self):
        self.assertFalse(any(x.is_file() for x in (ROOT/"dist/Data/MCM/Config").rglob("*")))
        self.assertIn('GetQuickMenuPreferences()',self.hotkey)
        self.assertIn('SetQuickMenuPreferences(p)',self.hotkey)
        poll=self.hotkey.split('void PollHotkey()',1)[1].split('const bool openPressedNow',1)[0]
        self.assertNotIn('RefreshMcmHotkeys(',poll)

    def test_mature_keyboard_path_stays_active(self):
        self.assertIn('kMcmModName = "K2040_Quick_Attach_Menu"',self.hotkey)
        self.assertIn('IsHotkeyPressedNow(g_openMenuHotkey)',self.hotkey)
        self.assertIn('IsHotkeyPressedNow(g_openMenuBuilderHotkey)',self.hotkey)
        self.assertIn('g_lastQuickActionTick',self.hotkey)
        self.assertIn('g_menuHotkeyReleasePending',self.hotkey)
        self.assertIn('QueueMenuHotkeyActionFromUi',self.hotkey)
        self.assertIn('SaveUserPreferences()',self.hotkey)
        self.assertNotIn('kHubMenuActionId',self.hotkey)
        self.assertNotIn('quickAttachOpen',self.hotkey)

    def test_controller_storage_and_migration(self):
        self.assertIn('"K2040_Quick_Attach_Menu.ini"',self.controller)
        self.assertIn('"K2040_Mods.ini"',self.controller) # read-only migration
        self.assertIn('"K2040_Prisma_Minimap.ini"',self.controller)
        self.assertIn('ReadIniBinding(kOldHubUserFile',self.controller)
        self.assertIn('hub_chord::QuickShouldFire',self.controller)
        self.assertIn('PersistBindings(chord)',self.controller)

    def test_four_visual_themes_keep_palette_separate(self):
        css=(ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/k2040-unified-theme.css").read_text()
        js=(ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/k2040-unified-theme.js").read_text()
        for theme in ["fallout4","fallout76","doom-eternal","fallout3-newvegas"]:
            self.assertIn('data-k2040-visual-theme="'+theme+'"',css)
        self.assertIn('data-k2040-palette',js)
        self.assertIn('data-k2040-visual-theme',js)
        self.assertIn('--k2040-theme-radius',css)
        self.assertIn('--k2040-theme-type',css)

    def test_themes_and_experimental_warning(self):
        self.assertIn('value == "fallout76"',self.user)
        self.assertIn('data-value="fallout76"',self.html)
        self.assertIn('Cheat mode (Experimental)',self.html)
        self.assertIn('Use at your own risk',self.html)

if __name__=="__main__":unittest.main()
