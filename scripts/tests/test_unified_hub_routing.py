"""Only owner V2 settings are installed and invoked; no old Builder editor."""
from pathlib import Path
import importlib.util
import unittest

ROOT=Path(__file__).resolve().parents[2]
UI=ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"

class UnifiedRouting(unittest.TestCase):
    def test_builder_shows_hub_shortcut_without_old_settings_action(self):
        html=(UI/"builder.html").read_text()
        self.assertIn("Settings: K2040 Hub (F3)",html)
        self.assertNotIn('send("open-settings")',html)
        self.assertNotIn('id="settings" class="action"',html)

    def test_packager_does_not_install_legacy_settings_renderer(self):
        pack=ROOT/"scripts/package-test.py"
        spec=importlib.util.spec_from_file_location("qam_retired_settings_pack",pack)
        module=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertIn("retired_settings_view",pack.read_text())
        if not (ROOT/"dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll").exists():
            self.skipTest("compiled owner DLL is required for final package verification")
        files=module.payload()
        self.assertIn("Data/K2040/Settings/Providers/quick-attach.json",files)
        self.assertNotIn("Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html",files)
        self.assertNotIn("Data/F4SE/Plugins/K2040_Unified_Settings.dll",files)

if __name__=="__main__":
    unittest.main()
