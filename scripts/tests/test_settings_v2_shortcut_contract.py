"""Hub owns only the settings shortcut; Quick Attach owns gameplay keys."""
from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[2]
MCM = ROOT / "dist/Data/MCM/Config/K2040_Quick_Attach_Menu"


class SettingsV2ShortcutContract(unittest.TestCase):
    def test_only_gameplay_hotkeys_stay_in_mod_config(self):
        registry = json.loads((MCM / "keybinds.json").read_text())
        config = json.loads((MCM / "config.json").read_text())
        allowed = set()
        self.assertEqual({item["id"] for item in registry["keybinds"]}, allowed)
        pages = config["pages"]
        hotkeys = [item["id"] for page in pages
                   for item in page.get("content", []) if item.get("type") == "hotkey"]
        self.assertEqual(set(hotkeys), allowed)
        self.assertEqual(len(hotkeys), len(allowed))
        self.assertNotIn("openUnifiedSettings", hotkeys)
        self.assertNotIn("openSettings", hotkeys)
        self.assertNotIn("settingsShortcut", hotkeys)

    def test_module_does_not_register_the_central_shortcut(self):
        hotkeys = (ROOT / "src/Hotkey.cpp").read_text()
        self.assertNotIn("openUnifiedSettings", hotkeys)
        self.assertNotIn("Ctrl+Shift+B", hotkeys)
        self.assertIn("ToggleQuickMenu", hotkeys)
        self.assertIn("ToggleBuilderMenu", hotkeys)
        manifest = json.loads((ROOT /
            "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings/"
            "settings-v1-manifest.json").read_text())
        fields = [field for category in manifest["categories"]
                  for group in category["groups"] for field in group["fields"]]
        by_id = {field["id"]: field for field in fields}
        for name in ("quickMenuHotkey", "menuBuilderHotkey"):
            self.assertEqual(by_id[name]["type"], "keybind")
            self.assertNotIn("owner",by_id[name])
        self.assertEqual(by_id["controllerQuickShortcut"]["type"],"select")
        self.assertNotIn("settingsShortcut", by_id)
        self.assertNotIn("openUnifiedSettings", by_id)

    def test_standalone_menu_and_builder_remain_available(self):
        src = (ROOT / "src/PrismaBridge.cpp").read_text()
        html = (ROOT /
            "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/"
            "settings.html").read_text()
        self.assertIn("ViewMode::QuickMenu", src)
        self.assertIn("ViewMode::MenuBuilder", src)
        self.assertIn("ViewMode::Settings", src)
        self.assertIn('id="quickKey"', html)
        self.assertIn('id="builderKey"', html)
        self.assertNotIn("openUnifiedSettings", html)


if __name__ == "__main__":
    unittest.main()
