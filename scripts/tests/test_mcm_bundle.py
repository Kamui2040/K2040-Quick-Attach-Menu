"""Only the unified hub owns an MCM entry; owner packages stay independent."""
from __future__ import annotations
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile
ROOT=Path(__file__).resolve().parents[2]
SPEC=importlib.util.spec_from_file_location("k2040_test_package",ROOT/"scripts/package-test.py")
assert SPEC is not None and SPEC.loader is not None
PACKAGER=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKAGER)
class OwnerMcmTests(unittest.TestCase):
    def test_owner_mcm_config_is_absent(self):
        self.assertFalse(any((ROOT/"dist/Data/MCM/Config").rglob("*config.json")))
        self.assertFalse(any((ROOT/"dist/Data/MCM/Config").rglob("*keybinds.json")))
    def test_owner_package_excludes_mcm(self):
        if not (ROOT/"dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll").is_file():
            self.skipTest("Windows DLL required")
        with tempfile.TemporaryDirectory() as td:
            one=Path(td)/"one.zip"
            two=Path(td)/"two.zip"
            PACKAGER.write_archive(one)
            PACKAGER.write_archive(two)
            self.assertEqual(one.read_bytes(),two.read_bytes())
            with zipfile.ZipFile(one) as z:
                self.assertIsNone(z.testzip())
                self.assertFalse(any(x.startswith("Data/MCM/") for x in z.namelist()))
    def test_existing_mcm_user_settings_not_edited(self):
        self.assertIn('"K2040_Mods.ini"',(ROOT/"src/ControllerShortcuts.cpp").read_text())
        self.assertIn('"K2040_Quick_Attach_Menu.ini"',(ROOT/"src/ControllerShortcuts.cpp").read_text())
if __name__=="__main__":unittest.main()
