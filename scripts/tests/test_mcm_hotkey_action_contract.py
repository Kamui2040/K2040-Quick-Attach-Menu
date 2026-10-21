"""Keep native MCM keybind actions compatible with both Fallout runtimes."""
from pathlib import Path
import json
import unittest

ROOT=Path(__file__).resolve().parents[2]

class McmActionContract(unittest.TestCase):
    def test_native_mcm_can_register_both_hotkeys(self):
        config=json.loads((ROOT/'dist/Data/MCM/Config/K2040_Quick_Attach_Menu/keybinds.json').read_text())
        self.assertEqual(config['modName'],'K2040_Quick_Attach_Menu')
        self.assertEqual([v['id'] for v in config['keybinds']],['openQuickMenu','openMenuBuilder'])
        for item in config['keybinds']:
            self.assertEqual(item['action'],{'type':'RunConsoleCommand','command':''})

    def test_native_input_still_performs_shortcut_actions(self):
        hotkey=(ROOT/'src/Hotkey.cpp').read_text()
        self.assertIn('IsHotkeyPressedNow(g_openMenuHotkey)',hotkey)
        self.assertIn('IsHotkeyPressedNow(g_openMenuBuilderHotkey)',hotkey)
        self.assertIn('RefreshMcmHotkeys(false);',hotkey)
        self.assertIn('QueueMenuHotkeyActionFromMcm(',hotkey)

if __name__=='__main__':unittest.main()
