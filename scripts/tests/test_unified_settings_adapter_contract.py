"""The legacy V1 hub callback must never open a separate settings renderer."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]

class LegacyAdapterRetirement(unittest.TestCase):
    def test_v1_entry_point_reports_unavailable(self):
        source=(ROOT/"src/SettingsHubAdapter.cpp").read_text()
        self.assertIn("K2040_GetSettingsAdapterV1",source)
        available=source.split("bool K2040_SETTINGS_CALL Available()",1)[1].split("bool K2040_SETTINGS_CALL Open()",1)[0]
        opener=source.split("bool K2040_SETTINGS_CALL Open()",1)[1].split("extern \"C\"",1)[0]
        self.assertIn("return false;",available)
        self.assertIn("return false;",opener)
        self.assertNotIn("OpenSettingsFromHub()",source)

    def test_v2_remains_sole_native_editor(self):
        v2=(ROOT/"src/SettingsHubAdapterV2.cpp").read_text()
        bridge=(ROOT/"src/PrismaBridge.cpp").read_text()
        self.assertIn("K2040_GetSettingsAdapterV2",v2)
        self.assertNotIn("PrismaBridge::OpenSettingsFromHub()",bridge)
        old=bridge.split('if (command == "open-settings")',1)[1].split("}",1)[0]
        self.assertIn("return;",old)
        self.assertNotIn("RequestViewSwitch",old)

if __name__=="__main__":
    unittest.main()
