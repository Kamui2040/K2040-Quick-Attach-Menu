from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class JsonPreferencesLimits(unittest.TestCase):
    def test_import_save_and_export_limits_are_consistent(self):
        source = (ROOT / "src/UserSettings.cpp").read_text()
        self.assertIn("constexpr std::uintmax_t kMaximumUserSettingsBytes = 8u * 1024u * 1024u", source)
        self.assertIn("preferencesSize > kMaximumUserSettingsBytes", source)
        self.assertIn("text.size() > kMaximumUserSettingsBytes", source)
        self.assertIn("text.size() > kMaximumWeaponMenuProfileBytes", source)
        self.assertIn("UserSettingsLoadState::ReadError", source)
