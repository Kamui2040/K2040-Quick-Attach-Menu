"""Quick Attach child-owned MCM integration and standalone contracts."""
from pathlib import Path
import unittest
import json

ROOT=Path(__file__).resolve().parents[2]
CONFIG=ROOT/"dist/Data/MCM/Config/K2040_Quick_Attach_Menu"

class McmChildContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hotkey=(ROOT/"src/Hotkey.cpp").read_text()
        cls.controller=(ROOT/"src/ControllerShortcuts.cpp").read_text()
        cls.main=(ROOT/"src/main.cpp").read_text()
        cls.html=(ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html").read_text()
        cls.user=(ROOT/"src/UserSettings.cpp").read_text()

    def test_child_page_belongs_to_mod(self):
        child=json.loads((CONFIG/"config.json").read_text())
        assert child["modName"]=="K2040_Quick_Attach_Menu"
        assert child["ownerModName"]=="K2040_Mods"
        assert child["minMcmVersion"]>=3
        self.assertEqual(len(child["pages"]),1)
        self.assertEqual(child["pages"][0]["pageDisplayName"],"Quick Attach Menu")
        content=child["pages"][0]["content"]
        self.assertEqual([x["id"] for x in content if x["type"]=="hotkey"],[
            "openQuickMenu","openMenuBuilder"])
        self.assertEqual([x["id"] for x in content if x["type"]=="dropdown"],[
            "iQuickAttachPadPrimary:Controller","iQuickAttachPadModifier:Controller"])
        for x in content:
            if x["type"]=="hotkey":
                self.assertEqual(x["valueOptions"]["allowModifierKeys"],1)
            if x["type"]=="dropdown":
                self.assertEqual(x["valueOptions"]["sourceType"],"ModSettingInt")
                self.assertEqual(len(x["valueOptions"]["options"]),17)

    def test_bundled_parent_and_child_are_independent(self):
        from hashlib import sha256
        parent_file=ROOT/"dist/Data/MCM/Config/K2040_Mods/config.json"
        self.assertEqual(sha256(parent_file.read_bytes()).hexdigest(),
            "8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb")
        parent=json.loads(parent_file.read_text())
        child=json.loads((CONFIG/"config.json").read_text())
        self.assertEqual(parent["modName"],"K2040_Mods")
        self.assertEqual(parent["pages"],[])
        self.assertNotIn("ownerModName",parent)
        self.assertEqual(child["ownerModName"],parent["modName"])
        self.assertEqual(child["modName"],"K2040_Quick_Attach_Menu")
        self.assertEqual(list(parent_file.parent.iterdir()),[parent_file])
        self.assertFalse((ROOT/"packaging/mcm-standalone/Data/MCM/Config/K2040_Quick_Attach_Menu/config.json").exists())

    def test_keybind_handlers_are_real_and_child_owned(self):
        kb=json.loads((CONFIG/"keybinds.json").read_text())
        self.assertEqual(kb["modName"],"K2040_Quick_Attach_Menu")
        self.assertEqual({x["id"]:x["action"] for x in kb["keybinds"]},{
            "openQuickMenu":{"type":"RunConsoleCommand","command":""},
            "openMenuBuilder":{"type":"RunConsoleCommand","command":""}})
        # Native MCM hotkey registry supports RunConsoleCommand, not
        # CallExternalFunction. Execution stays in Quick Attach's native poller.
        self.assertNotIn("CallExternalFunction",(CONFIG/"keybinds.json").read_text())
        self.assertIn('RegisterChildMcmScaleform',self.main)
        self.assertIn('QueueMenuHotkeyActionFromMcm',self.main)
        self.assertIn('HotkeyActionSource::McmChild',self.hotkey)

    def test_mature_keyboard_path_stays_active(self):
        self.assertIn('kMcmModName = "K2040_Quick_Attach_Menu"',self.hotkey)
        self.assertIn('IsHotkeyPressedNow(g_openMenuHotkey)',self.hotkey)
        self.assertIn('IsHotkeyPressedNow(g_openMenuBuilderHotkey)',self.hotkey)
        self.assertIn('g_lastQuickActionTick',self.hotkey)
        self.assertIn('g_menuHotkeyReleasePending',self.hotkey)
        self.assertIn('QueueMenuHotkeyActionFromUi',self.hotkey)
        self.assertIn('WriteSharedHotkeyBindings',self.hotkey)
        self.assertNotIn('kHubMenuActionId',self.hotkey)
        self.assertNotIn('quickAttachOpen',self.hotkey)

    def test_controller_storage_and_migration(self):
        ini=(CONFIG/"settings.ini").read_text()
        self.assertEqual(ini,"[Controller]\niQuickAttachPadPrimary=0\niQuickAttachPadModifier=0\n")
        self.assertIn('"K2040_Quick_Attach_Menu.ini"',self.controller)
        self.assertIn('"K2040_Mods.ini"',self.controller) # read-only migration
        self.assertIn('"K2040_Prisma_Minimap.ini"',self.controller)
        self.assertIn('!std::filesystem::exists(kHubUserFile',self.controller)
        self.assertIn('ReadIniBinding(kOldHubUserFile',self.controller)
        self.assertIn('hub_chord::QuickShouldFire',self.controller)
        self.assertIn('MOVEFILE_REPLACE_EXISTING',self.controller)

    def test_themes_and_experimental_warning(self):
        self.assertIn('value == "fallout76"',self.user)
        self.assertIn('data-value="fallout76"',self.html)
        self.assertIn('Cheat mode (Experimental)',self.html)
        self.assertIn('Use at your own risk',self.html)

if __name__=="__main__":unittest.main()
