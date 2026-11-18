"""QAM gameplay bindings cannot mutate the historical shared MCM registry."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / 'src/Hotkey.cpp').read_text()

class McmRegistryIsolationContract(unittest.TestCase):
    def test_no_mcm_file_writer_or_registration_in_runtime(self):
        for forbidden in ('SaveSharedMcmRegistry', 'WriteSharedHotkeyBindings(',
                          'SeedDefaultMcmBindings(', 'Keybinds.json',
                          'root.mcm.SetKeybind', 'root.mcm.RemapKeybind',
                          'root.mcm.ClearKeybind', 'McmRegistryPersistence.h'):
            self.assertNotIn(forbidden, SOURCE)

    def test_no_live_sync_or_registry_reader_in_runtime(self):
        for forbidden in ('TryNativeMcmLiveSyncOnGameThread(',
                          'StageNativeMcmLiveSync(', 'ReadSharedHotkeyBinding(',
                          'RefreshMcmHotkeys(', 'McmKeybindRegistry.h',
                          'NativeMcmBindingRecovery.h'):
            self.assertNotIn(forbidden, SOURCE)

    def test_gameplay_binding_uses_owner_preferences(self):
        self.assertIn('GetQuickMenuPreferences()', SOURCE)
        self.assertIn('SetQuickMenuPreferences(p)', SOURCE)
        self.assertIn('SaveUserPreferences()', SOURCE)
        self.assertIn('gameplay_keys::FromKeycode(', SOURCE)
        self.assertIn('g_openMenuHotkey=k2040::ParseHotkey(', SOURCE)
        self.assertIn('g_openMenuBuilderHotkey=k2040::ParseHotkey(', SOURCE)

    def test_native_mouse_capture_stays_on_game_thread(self):
        self.assertIn('nativeMouse.Sample(', SOURCE)
        self.assertIn('tasks->AddTask([capturedMouse, modifiers, captureSession]()', SOURCE)
        self.assertIn('OnNativeSettingsMouseCapture(capturedMouse, modifiers)', SOURCE)
        self.assertIn('g_hotkeyCaptureGeneration.load', SOURCE)
        html=(ROOT/'dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html').read_text()
        self.assertIn('window.k2040CaptureNativeMouse = function(keycode, modifiers)', html)
        bridge=(ROOT/'src/PrismaBridge.cpp').read_text()
        self.assertIn('PrismaBridge::OnNativeSettingsMouseCapture(', bridge)

    def test_gameplay_startup_and_poll_do_not_touch_mcm(self):
        init=SOURCE.split('void InitializeHotkey()',1)[1].split('void PollHotkey()',1)[0]
        poll=SOURCE.split('void PollHotkey()',1)[1].split('const bool openPressedNow',1)[0]
        self.assertIn('ApplyConfiguredHotkeys();',init)
        for forbidden in ('SeedDefaultMcmBindings(', 'TryNativeMcmLiveSyncOnGameThread(',
                          'RefreshMcmHotkeys(', 'WriteSharedHotkeyBindings('):
            self.assertNotIn(forbidden, init+poll)
        self.assertIn('IsHotkeyPressedNow(g_openMenuHotkey)', SOURCE)
        self.assertIn('IsHotkeyPressedNow(g_openMenuBuilderHotkey)', SOURCE)

    def test_mod_owned_capture_does_not_write_mcm(self):
        start=SOURCE.index('bool SetSharedHotkeyBinding(')
        end=SOURCE.index('bool ResetSharedHotkeyBindings(',start)
        body=SOURCE[start:end]
        self.assertLess(body.index('gameplay_keys::FromKeycode('),
                        body.index('SetQuickMenuPreferences('))
        self.assertIn('SaveUserPreferences()',body)
        helper=(ROOT/'include/SettingsKeyCaptureValidation.h').read_text()
        self.assertIn('keycode >= 144 && keycode <= 145',helper)

    def test_legacy_helpers_are_test_only(self):
        scanner=(ROOT/'include/McmKeybindRegistry.h').read_text()
        guard=(ROOT/'include/NativeMcmBindingRecovery.h').read_text()
        persistence=(ROOT/'include/McmRegistryPersistence.h').read_text()
        self.assertIn('ScanMcmKeybindArray(',scanner)
        self.assertIn('NativeMcmChangeGate',guard)
        self.assertIn('SaveSharedMcmRegistry(',persistence)
        self.assertTrue((ROOT/'scripts/tests/mcm-keybind-deduplication.test.cpp').is_file())
        self.assertTrue((ROOT/'scripts/tests/native-mcm-binding-recovery.test.cpp').is_file())
        for legacy in ('McmKeybindRegistry.h', 'NativeMcmBindingRecovery.h',
                       'McmRegistryPersistence.h'):
            self.assertNotIn('#include "'+legacy+'"',SOURCE)

if __name__=='__main__':
    unittest.main()
