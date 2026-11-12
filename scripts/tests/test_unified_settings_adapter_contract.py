from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]
class UnifiedSettingsAdapterContract(unittest.TestCase):
    def test_adapter_is_optional_and_safe(self):
        h=(ROOT/"include/K2040SettingsAdapter.h").read_text()
        src=(ROOT/"src/SettingsHubAdapter.cpp").read_text()
        bridge=(ROOT/"src/PrismaBridge.cpp").read_text()
        self.assertIn("K2040_SETTINGS_ADAPTER_ABI = 1",h)
        self.assertIn('"quick-attach", Available, Open',src)
        self.assertIn('OpenSettingsFromHub()',src)
        self.assertIn('if (menuOpen_ || !CanOpenFromHotkey()) return false;',bridge)
        self.assertIn('viewMode_ = ViewMode::Settings;',bridge)
        self.assertIn('BeginOpenFromHotkey()',bridge)
        self.assertIn('OpenView(weaponInfo, menu, ViewMode::Settings)',bridge)
        self.assertIn('CloseMenuInternal(false)',bridge)
