"""Check optional Categorizer layout without affecting the standard MCM grouping."""
from pathlib import Path
import importlib.util
import json
import tempfile
import unittest
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'dist/Data/MCM/Config/K2040_Quick_Attach_Menu/config.json'
SCRIPT = ROOT / 'scripts/package-mcm-categorizer-compat.py'
spec = importlib.util.spec_from_file_location('qam_mcm_categorizer', SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CategorizerCompat(unittest.TestCase):
    def test_override_only_removes_parent_ownership(self):
        before = json.loads(SOURCE.read_text())
        after = json.loads(module.ungrouped_child(SOURCE.read_bytes()))
        self.assertEqual(before['ownerModName'], 'K2040_Mods')
        self.assertNotIn('ownerModName', after)
        before.pop('ownerModName')
        self.assertEqual(before, after)
        self.assertEqual(after['modName'], 'K2040_Quick_Attach_Menu')
        self.assertEqual(after['pages'][0]['pageDisplayName'], 'Quick Attach Menu')

    def test_archive_is_one_valid_file_and_never_overwrites(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'QAM-MCM-Categorizer-optional.zip'
            sha = module.write_archive(path)
            self.assertEqual(len(sha), 64)
            with ZipFile(path) as archive:
                self.assertIsNone(archive.testzip())
                self.assertEqual(archive.namelist(), [module.TARGET])
                self.assertNotIn('ownerModName', json.loads(archive.read(module.TARGET)))
            with self.assertRaises(FileExistsError):
                module.write_archive(path)

    def test_reject_unknown_child_layout(self):
        obj = json.loads(SOURCE.read_text())
        obj['modName'] = 'AnotherMod'
        with self.assertRaises(ValueError):
            module.ungrouped_child(json.dumps(obj).encode())


if __name__ == '__main__':
    unittest.main()
