"""Retired MCM gameplay registration must never be reactivated."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'src/Hotkey.cpp').read_text()
USER=(ROOT/'src/UserSettings.cpp').read_text()

class GameplayBindingContracts(unittest.TestCase):
    def test_defaults_are_independent_of_hub(self):
        policy=(ROOT/'include/GameplayKeybindPolicy.h').read_text()
        self.assertIn('Unbound',policy)
        self.assertIn('return key && !(ctrl&&shift',policy)
        self.assertIn('quickMenuHotkey',USER)
        self.assertIn('menuBuilderHotkey',USER)
        self.assertIn('p.quickMenuHotkey="Shift+K"',SOURCE)
        self.assertIn('p.menuBuilderHotkey="Ctrl+Shift+K"',SOURCE)

    def test_no_native_mcm_seeding_or_polled_registry(self):
        init=SOURCE.split('void InitializeHotkey()',1)[1].split('void PollHotkey()',1)[0]
        polling=SOURCE.split('void PollHotkey()',1)[1].split('const bool openPressedNow',1)[0]
        for forbidden in ('SeedDefaultMcmBindings(', 'RefreshMcmHotkeys(',
                          'TryNativeMcmLiveSyncOnGameThread('):
            self.assertNotIn(forbidden,init+polling)

    def test_direct_saves_do_not_write_mcm_registry(self):
        setter=SOURCE.split('bool SetSharedHotkeyBinding(',1)[1].split('bool ResetSharedHotkeyBindings()',1)[0]
        reset=SOURCE.split('bool ResetSharedHotkeyBindings()',1)[1].split('void SetHotkeyCaptureActive(',1)[0]
        for body in (setter,reset):
            self.assertIn('SaveUserPreferences()',body)
            self.assertNotIn('WriteSharedHotkeyBindings(',body)
            self.assertNotIn('StageNativeMcmLiveSync(',body)
            self.assertNotIn('NativeMcmOwnsRegistry(',body)

if __name__=='__main__':unittest.main()
