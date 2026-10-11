"""Structural and packaged MCM tests. Gameplay MCM grouping needs a real game."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from unittest import mock

ROOT=Path(__file__).resolve().parents[2]
MCM=ROOT/"dist/Data/MCM/Config"
PARENT=MCM/"K2040_Mods/config.json"
CHILD=MCM/"K2040_Quick_Attach_Menu/config.json"
PARENT_HASH="8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb"
SPEC=importlib.util.spec_from_file_location("k2040_test_package",ROOT/"scripts/package-test.py")
assert SPEC is not None and SPEC.loader is not None
PACKAGER=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKAGER)


class BundledMcmTests(unittest.TestCase):
    def test_parent_checksum_is_identical_to_shared_minimap_reference(self):
        self.assertEqual(hashlib.sha256(PARENT.read_bytes()).hexdigest(),PARENT_HASH)
        data=json.loads(PARENT.read_bytes())
        self.assertEqual(data["modName"],"K2040_Mods")
        self.assertEqual(data["pages"],[])
        self.assertEqual([x.name for x in (MCM/"K2040_Mods").iterdir()],["config.json"])

    def test_one_shared_parent_supports_installed_children(self):
        parent=json.loads(PARENT.read_bytes())
        quick=json.loads(CHILD.read_bytes())
        minimap={
            "modName":"K2040_Prisma_Minimap",
            "ownerModName":"K2040_Mods",
            "pages":[{"pageDisplayName":"Prisma Minimap","content":[]}]
        }
        def merged(children):
            return [page["pageDisplayName"] for child in children
                if child["ownerModName"]==parent["modName"]
                for page in child["pages"]]
        self.assertEqual(merged([quick]),["Quick Attach Menu"])
        self.assertEqual(merged([quick,minimap]),["Quick Attach Menu","Prisma Minimap"])
        self.assertEqual(merged([minimap]),["Prisma Minimap"])
        self.assertEqual(parent["pages"],[])  # No central registry mutation.

    def test_parent_mismatch_blocks_packaging(self):
        with mock.patch.object(PACKAGER,"CANONICAL_PARENT_SHA256","0"*64):
            with self.assertRaisesRegex(ValueError,"canonical template"):
                PACKAGER.payload()

    def test_archive_is_reproducible_and_does_not_bundle_user_settings(self):
        if not (ROOT/"dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll").is_file():
            self.skipTest("Windows DLL must be built before packaging")
        source_ini=(ROOT/"dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.ini").read_bytes()
        with tempfile.TemporaryDirectory() as td:
            first=Path(td)/"first.zip"
            second=Path(td)/"second.zip"
            digest=PACKAGER.write_archive(first,ae_reequip_tester=True)
            self.assertEqual(digest,PACKAGER.write_archive(second,ae_reequip_tester=True))
            self.assertEqual(first.read_bytes(),second.read_bytes())
            with zipfile.ZipFile(first) as archive:
                self.assertEqual(archive.testzip(),None)
                self.assertEqual(hashlib.sha256(archive.read(PACKAGER.PARENT)).hexdigest(),
                                 PARENT_HASH)
                self.assertIn(PACKAGER.CHILD,archive.namelist())
                self.assertIn(PACKAGER.KEYBINDS,archive.namelist())
                self.assertIn(PACKAGER.DEFAULTS,archive.namelist())
                self.assertIn(PACKAGER.AE_ENABLED,archive.read(PACKAGER.INI))
                self.assertFalse(any("MCM/Settings/" in p for p in archive.namelist()))
                self.assertEqual(
                    [p for p in archive.namelist() if p.startswith("Data/MCM/Config/K2040_Mods/")],
                    [PACKAGER.PARENT])
        self.assertEqual(source_ini,(ROOT/"dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.ini").read_bytes())


if __name__=="__main__":
    unittest.main()
