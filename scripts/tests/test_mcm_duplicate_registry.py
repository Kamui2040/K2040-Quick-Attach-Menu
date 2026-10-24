"""A duplicated MCM record must not mask the last working user hotkey."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / 'src/Hotkey.cpp').read_text()

class DuplicateMcmRegistryContract(unittest.TestCase):
    def test_prefer_latest_record_for_read_and_settings_write(self):
        body = SOURCE.split('std::optional<JsonObjectRange> FindMcmKeybindObject(', 1)[1].split('std::optional<bool> ReconcileMcmKeybindDuplicates(', 1)[0]
        self.assertIn('FindMcmKeybindObjects(json, id)', body)
        self.assertIn('return matches.back();', body)

    def test_reconcile_before_default_seeding(self):
        body = SOURCE.split('void SeedDefaultMcmBindings()', 1)[1].split('bool WriteSharedHotkeyBindings(', 1)[0]
        self.assertLess(body.index('ReconcileMcmKeybindDuplicates(json)'), body.index('if (FindMcmKeybindObject(json,id)) continue;'))
        self.assertIn('if (changed)', body)
        self.assertIn('MOVEFILE_WRITE_THROUGH', body)
        self.assertIn('std::string cleaned = registry;', SOURCE)
        self.assertIn('kOpenMenuBuilderKeybindId', SOURCE)

    def test_shared_keybind_array_scan_for_all_registry_paths(self):
        lookup = SOURCE.split('std::vector<JsonObjectRange> FindMcmKeybindObjects(',1)[1].split('std::optional<JsonObjectRange> FindMcmKeybindObject(',1)[0]
        seed = SOURCE.split('void SeedDefaultMcmBindings()',1)[1].split('bool WriteSharedHotkeyBindings(',1)[0]
        parser = SOURCE.split('bool ParseMcmKeybindRegistry(',1)[1].split('bool LoadMcmHotkeyOverrides(',1)[0]
        self.assertIn('k2040::FindMcmObjectMemberValue(object, member)', SOURCE)
        for body in (lookup, seed, parser):
            self.assertIn('k2040::ScanMcmKeybindArray(', body)
        self.assertIn('array->objects', lookup)
        self.assertIn('array->closingBracket', seed)
        self.assertIn('for (const auto& [start, end] : array->objects)', parser)
        display = SOURCE.split('std::optional<k2040::SharedHotkeyBinding> ReadSharedHotkeyBinding(',1)[1].split('struct PendingSharedBinding',1)[0]
        self.assertIn('matches.rbegin()',display)
        self.assertIn('if (!keycode || !modifiers || !ParseMcmHotkey(*keycode, *modifiers)) continue;',display)

    def test_native_mouse_capture_is_game_thread_only(self):
        self.assertIn('nativeMouse.Sample(', SOURCE)
        self.assertIn('tasks->AddTask([capturedMouse, modifiers, captureSession]()', SOURCE)
        self.assertIn('OnNativeSettingsMouseCapture(capturedMouse, modifiers)', SOURCE)
        self.assertIn('g_hotkeyCaptureGeneration.load', SOURCE)
        html = (ROOT / 'dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html').read_text()
        self.assertIn('window.k2040CaptureNativeMouse = function(keycode, modifiers)', html)
        bridge = (ROOT / 'src/PrismaBridge.cpp').read_text()
        self.assertIn('PrismaBridge::OnNativeSettingsMouseCapture(', bridge)

    def test_settings_mcm_reset_attempts_public_pause_sync(self):
        self.assertIn('GetMenuOpen(RE::BSFixedString("PauseMenu"))', SOURCE)
        self.assertIn('Invoke("root.mcm.SetKeybind", &result, args, 4)', SOURCE)
        self.assertIn('result.IsBoolean() && result.GetBoolean()', SOURCE)
        self.assertIn('g_mcmSyncTaskQueued.exchange(true', SOURCE)
        self.assertIn('tasks->AddTask([] { TryNativeMcmLiveSyncOnGameThread(); });', SOURCE)
        self.assertIn('registered->keycode == binding.keycode', SOURCE)
        self.assertIn('registered->modifiers == binding.modifiers', SOURCE)
        reset = SOURCE.split('bool ResetSharedHotkeyBindings()', 1)[1].split('void SetHotkeyCaptureActive(',1)[0]
        self.assertIn('StageNativeMcmLiveSync(kOpenMenuKeybindId', reset)
        self.assertIn('StageNativeMcmLiveSync(kOpenMenuBuilderKeybindId', reset)
        self.assertLess(reset.index('if (saved)'), reset.index('StageNativeMcmLiveSync(kOpenMenuKeybindId'))

    def test_settings_capture_is_validated_before_touching_shared_registry(self):
        start = SOURCE.index('bool SetSharedHotkeyBinding(')
        end = SOURCE.index('bool ResetSharedHotkeyBindings(', start)
        body = SOURCE[start:end]
        self.assertLess(body.index('IsSettingsCaptureKeycode(keycode)'), body.index('WriteSharedHotkeyBindings('))
        helper = (ROOT / 'include/SettingsKeyCaptureValidation.h').read_text()
        self.assertIn('keycode >= 144 && keycode <= 145', helper)
        self.assertIn('keycode == 259', helper)

    def test_shared_mcm_registry_write_is_recoverable_under_wine(self):
        persistence = (ROOT / 'include/McmRegistryPersistence.h').read_text()
        self.assertIn('MoveFileExW(', persistence)
        self.assertIn('ERROR_ACCESS_DENIED', persistence)
        self.assertIn('ERROR_SHARING_VIOLATION', persistence)
        self.assertIn('CopyFileW(destination.c_str(), recovery.c_str(), FALSE)', persistence)
        self.assertIn('current != original', persistence)
        self.assertIn('WriteMcmRegistryHandle(target, replacement)', persistence)
        self.assertIn('result.restoreFailed', persistence)
        self.assertIn('k2040::SaveSharedMcmRegistry(', SOURCE)
        self.assertIn('saved.replaceError', SOURCE)

    def test_safe_duplicate_scope(self):
        body = SOURCE.split('std::vector<JsonObjectRange> FindMcmKeybindObjects(',1)[1].split('bool ReplaceJsonUnsignedMember(',1)[0]
        self.assertIn('*modName == kMcmModName && *objectId == id', body)
        self.assertIn('ParseMcmHotkey(*key, *modifiers)', body)
        self.assertIn('k2040::RemoveEarlierMcmKeybindObjects(cleaned, ranges)', body)

if __name__ == '__main__':
    unittest.main()
