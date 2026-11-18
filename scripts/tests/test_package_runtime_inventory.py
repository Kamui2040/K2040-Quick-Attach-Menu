"""QAM-only packaging refuses unexpected files and symlinked staging."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock

PACKAGE = Path(__file__).resolve().parents[1] / "package-test.py"
SPEC = importlib.util.spec_from_file_location("qam_package_test", PACKAGE)
assert SPEC is not None and SPEC.loader is not None
package = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(package)


class RuntimePackageInventoryTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory(prefix="qam-package-inventory-")
        self.addCleanup(self._temp.cleanup)
        self.dist = Path(self._temp.name) / "Data"
        for name in package.RUNTIME_FILES:
            file = self.dist / name.removeprefix("Data/")
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(b"QAM checked placeholder")

    def test_exact_owner_only_inventory(self):
        data = package.staged_payload_files(self.dist)
        self.assertEqual(set(data), set(package.RUNTIME_FILES))
        self.assertEqual(len(data), 13)
        self.assertFalse(any("K2040_Framework" in key or
                             "K2040_Unified_Settings" in key or
                             key.startswith("Data/MCM/") for key in data))

    def test_retired_standalone_settings_view_is_excluded(self):
        old = self.dist / package.RETIRED_SETTINGS_VIEW.removeprefix("Data/")
        old.write_text("<html>Retired settings view</html>")
        data = package.staged_payload_files(self.dist)
        self.assertNotIn(package.RETIRED_SETTINGS_VIEW, data)
        self.assertEqual(set(data), set(package.RUNTIME_FILES))

    def test_unknown_staged_files_fail_closed(self):
        for name in ("Data/F4SE/Plugins/extra.dll", "Data/MCM/Config/new/config.json",
                     "Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/audit.log"):
            with self.subTest(name=name):
                unexpected = self.dist / name.removeprefix("Data/")
                unexpected.parent.mkdir(parents=True, exist_ok=True)
                unexpected.write_bytes(b"not an authorized asset")
                with self.assertRaisesRegex(ValueError, "Unexpected staged runtime file"):
                    package.staged_payload_files(self.dist)
                unexpected.unlink()

    def test_missing_runtime_asset_fails_closed(self):
        missing = self.dist / "F4SE/Plugins/K2040_Quick_Attach_Menu.ini"
        missing.unlink()
        with self.assertRaisesRegex(ValueError, "Missing required QAM runtime file"):
            package.staged_payload_files(self.dist)

    def test_unexpected_file_is_rejected_before_schema_validation(self):
        unexpected = self.dist / "F4SE/Plugins/unreviewed.dll"
        unexpected.write_bytes(b"unreviewed payload")
        with mock.patch.object(package, "DIST", self.dist):
            with mock.patch.object(package.subprocess, "run",
                                   side_effect=AssertionError("validator must not run")):
                with self.assertRaisesRegex(ValueError, "Unexpected staged runtime file"):
                    package.payload()

    def test_symlink_to_other_file_fails_closed(self):
        private = Path(self._temp.name) / "private-file.txt"
        private.write_text("private test fixture")
        link = self.dist / "F4SE/Plugins/unexpected.txt"
        link.symlink_to(private)
        with self.assertRaisesRegex(ValueError, "Staged Data symlink is not allowed"):
            package.staged_payload_files(self.dist)

    def test_symlink_instead_of_approved_asset_fails_closed(self):
        file = self.dist / "F4SE/Plugins/K2040_Quick_Attach_Menu.ini"
        file.unlink()
        private = Path(self._temp.name) / "private-file.txt"
        private.write_text("private test fixture")
        file.symlink_to(private)
        with self.assertRaisesRegex(ValueError, "Staged Data symlink is not allowed"):
            package.staged_payload_files(self.dist)


if __name__ == "__main__":
    unittest.main()
