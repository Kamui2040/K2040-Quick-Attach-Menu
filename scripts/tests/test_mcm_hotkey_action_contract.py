"""Quick Attach gameplay keys remain independent from hub MCM."""
from pathlib import Path
import json
import unittest
ROOT=Path(__file__).resolve().parents[2]

class McmActionContract(unittest.TestCase):
    def test_only_hub_owns_settings_mcm_hotkey(self):
        self.assertFalse(any((ROOT/'dist/Data/MCM/Config').rglob('*.json')),
                         'owner must not ship any MCM entry or keybind')

    def test_native_input_stays_active_without_mcm_sync(self):
        text=(ROOT/'src/Hotkey.cpp').read_text()
        init=text.split('void InitializeHotkey()',1)[1].split('void PollHotkey()',1)[0]
        polling=text.split('void PollHotkey()',1)[1].split('const bool openPressedNow',1)[0]
        self.assertIn('ApplyConfiguredHotkeys();',init)
        self.assertNotIn('SeedDefaultMcmBindings();',init)
        self.assertNotIn('RefreshMcmHotkeys',init+polling)
        self.assertNotIn('TryNativeMcmLiveSyncOnGameThread()',polling)
        self.assertNotIn('SaveSharedMcmRegistry',text)
        self.assertNotIn('Keybinds.json',text)
        self.assertNotIn('root.mcm.SetKeybind',text)
        self.assertNotIn('root.mcm.RemapKeybind',text)
        self.assertNotIn('WriteSharedHotkeyBindings(',text)
        self.assertIn('IsHotkeyPressedNow(g_openMenuHotkey)',text)
        self.assertIn('IsHotkeyPressedNow(g_openMenuBuilderHotkey)',text)
        self.assertIn('K2040_IsSettingsEditorOpenV1',text)
        self.assertIn('if (UnifiedEditorIsOpen())',text)
        self.assertIn('g_wasOpenPressedLastPoll.store(openPressedNow)',text)

if __name__=='__main__':unittest.main()
