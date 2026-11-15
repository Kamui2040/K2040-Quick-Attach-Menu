"""Compile and run a native QAM V2 policy test without the game runtime."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TEST = ROOT / "scripts/tests/settings-v2-value-policy.test.cpp"


class SettingsV2OwnerPolicy(unittest.TestCase):
    def test_native_owner_accepts_only_declared_input_types_and_ranges(self):
        cxx = shutil.which("clang++") or shutil.which("g++")
        self.assertIsNotNone(cxx, "a native C++ compiler is required")
        temp_root = Path.home() / "Downloads/K2040-Quick-Attach-Menu"
        temp_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="settings-v2-native-", dir=temp_root) as directory:
            target = Path(directory) / "test-value-policy"
            compile_result = subprocess.run(
                [cxx, "-std=c++23", "-O0", "-Wall", "-Wextra",
                 "-Werror", "-I", str(ROOT / "include"), str(TEST),
                 "-o", str(target)],
                capture_output=True, text=True, timeout=40, check=False)
            self.assertEqual(compile_result.returncode, 0, compile_result.stderr[-1600:])
            execution = subprocess.run([str(target)], capture_output=True,
                                       text=True, timeout=10, check=False)
            self.assertEqual(execution.returncode, 0, execution.stderr[-1600:])

    def test_adapter_invokes_independent_policy_before_native_mutation(self):
        source = (ROOT / "src/SettingsHubAdapterV2.cpp").read_text()
        self.assertIn('#include "SettingsV2ValuePolicy.h"', source)
        self.assertIn('settings_v2::ValidFieldValue(name,*parsed)', source)
        self.assertLess(source.index("settings_v2::ValidFieldValue(name,*parsed)"),
                        source.index("SetQuickMenuPreferences(p)"))


    def test_every_manifest_editable_field_has_native_read_and_write_coverage(self):
        import json
        import re
        manifest = json.loads((ROOT /
            "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings/"
            "settings-v1-manifest.json").read_text())
        adapter = (ROOT / "src/SettingsHubAdapterV2.cpp").read_text()
        owner_policy = (ROOT / "include/SettingsV2ValuePolicy.h").read_text()
        declared = {f["id"] for category in manifest["categories"]
                    for group in category["groups"] for f in group["fields"]
                    if f["type"] != "action"}
        read_fields = set(re.findall(
            r'(?:str|boolean|number)\("([a-zA-Z0-9]+)"', adapter))
        write_fields = set(re.findall(r'name=="([a-zA-Z0-9]+)"', adapter))
        policy_fields = {field for field in declared
                         if '"'+field+'"' in owner_policy}
        self.assertEqual(read_fields, declared, "every editable field must be readable")
        self.assertEqual(write_fields, declared, "every editable field must be dispatchable")
        self.assertEqual(policy_fields, declared,
                         "every manifest-editable field must have native validation")

    def test_owner_reset_stays_guarded_until_confirmation_is_available(self):
        source = (ROOT / "src/SettingsHubAdapterV2.cpp").read_text()
        section = source[source.index("bool K2040_SETTINGS_V2_CALL Reset("):]
        section = section[:section.index("\n}")]
        self.assertIn("return false;", section)
        self.assertNotIn("ResetAllUserPreferences", section)



if __name__ == "__main__":
    unittest.main()
