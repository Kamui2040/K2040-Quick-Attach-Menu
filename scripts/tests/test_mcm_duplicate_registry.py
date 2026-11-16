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

    def test_retired_native_mcm_sync_is_unreachable_in_polling_path(self):
        init=SOURCE.split('void InitializeHotkey()',1)[1].split('void PollHotkey()',1)[0]
        poll=SOURCE.split('void PollHotkey()',1)[1].split('const bool openPressedNow',1)[0]
        for forbidden in ('RefreshMcmHotkeys(', 'SeedDefaultMcmBindings(',
                          'TryNativeMcmLiveSyncOnGameThread('):
            self.assertNotIn(forbidden,init+poll)

    def test_cleared_native_entries_are_never_fabricated_by_fallback(self):
        seed = SOURCE.split('void SeedDefaultMcmBindings()',1)[1].split(
            'bool WriteSharedHotkeyBindings(',1)[0]
        self.assertIn('if (NativeMcmOwnsRegistry()) return;', seed)
        self.assertIn('if (std::filesystem::is_regular_file(marker)) return;', seed)
        self.assertIn('if (FindMcmKeybindObject(json,id)) continue;', seed)
        self.assertNotIn('selected = initialized ? 0 : key', seed)
        self.assertNotIn('selected = 0', seed)
        self.assertIn('if (!complete) break;', seed)
        self.assertIn('const auto marker = std::filesystem::path(', SOURCE)
        self.assertIn('if (!std::filesystem::is_regular_file(marker))', SOURCE)

    def test_mod_owned_settings_capture_does_not_touch_mcm_registry(self):
        start=SOURCE.index('bool SetSharedHotkeyBinding(')
        end=SOURCE.index('bool ResetSharedHotkeyBindings(',start)
        body=SOURCE[start:end]
        self.assertLess(body.index('gameplay_keys::FromKeycode('),
                        body.index('SetQuickMenuPreferences('))
        self.assertIn('SaveUserPreferences()',body)
        self.assertNotIn('WriteSharedHotkeyBindings(',body)
        helper=(ROOT/'include/SettingsKeyCaptureValidation.h').read_text()
        self.assertIn('keycode >= 144 && keycode <= 145',helper)

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
