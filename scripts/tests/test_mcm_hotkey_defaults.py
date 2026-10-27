"""Regression guards for optional MCM keybind initialization."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/Hotkey.cpp").read_text()


class HotkeyRegistryContracts(unittest.TestCase):
    def test_default_binding_id_key_and_modifier(self):
        self.assertIn("{kOpenMenuKeybindId,'K',1u}", SOURCE)
        self.assertIn("{kOpenMenuBuilderKeybindId,'K',3u}", SOURCE)
        self.assertIn("defaultQuick, 1u", SOURCE)
        self.assertIn("defaultQuick, 3u", SOURCE)
        self.assertIn("constexpr auto defaultQuick = static_cast<std::uint32_t>('K');", SOURCE)

    def test_registry_is_initialized_before_refresh(self):
        self.assertIn("void SeedDefaultMcmBindings()", SOURCE)
        initializer = SOURCE.split("void InitializeHotkey()", 1)[1].split("void PollHotkey()", 1)[0]
        self.assertLess(initializer.index("SeedDefaultMcmBindings();"),
                        initializer.index("RefreshMcmHotkeys(true);"))
        writer = SOURCE.split("bool WriteSharedHotkeyBindings(", 1)[1].split("std::ifstream file", 1)[0]
        self.assertIn("SeedDefaultMcmBindings();", writer)

    def test_native_registry_is_never_seeded_before_mcm_dll_load(self):
        seed = SOURCE.split("void SeedDefaultMcmBindings()", 1)[1].split(
            "bool WriteSharedHotkeyBindings", 1)[0]
        guard = SOURCE.split("bool NativeMcmOwnsRegistry()", 1)[1].split(
            "void SeedDefaultMcmBindings()", 1)[0]
        self.assertIn('std::filesystem::is_regular_file("Data/F4SE/Plugins/mcm.dll")', guard)
        self.assertIn('GetModuleHandleW(L"mcm.dll") != nullptr', guard)
        self.assertLess(seed.index("if (NativeMcmOwnsRegistry()) return;"),
                        seed.index("std::filesystem::exists(kMcmKeybindRegistryPath)"))
        self.assertIn('if (std::filesystem::is_regular_file(marker)) return;', seed)
        self.assertNotIn("selected = initialized ? 0 : key", seed)
        self.assertNotIn("selected ? mods : 0", seed)
        self.assertNotIn("selected = 0", seed)
        self.assertIn('std::to_string(key)', seed)
        self.assertIn('std::to_string(mods)', seed)
        self.assertIn('if (!complete) break;', seed)
        self.assertIn("hotkeys-initialized", seed)

    def test_settings_and_reset_do_not_write_the_native_mcm_file_before_dll_load(self):
        setter = SOURCE.split("bool SetSharedHotkeyBinding(", 1)[1].split(
            "bool ResetSharedHotkeyBindings()", 1)[0]
        reset = SOURCE.split("bool ResetSharedHotkeyBindings()", 1)[1].split(
            "void SetHotkeyCaptureActive(", 1)[0]
        for body in (setter, reset):
            self.assertLess(body.index("if (NativeMcmOwnsRegistry())"),
                            body.index("WriteSharedHotkeyBindings("))
        self.assertIn("*keycode ? BindingDisplayName", SOURCE)
        self.assertIn("virtualKey == 0 && modifiers != 0", SOURCE)

    def test_old_zero_chord_collision_explains_only_builder_failing(self):
        # MCM's actual map uses the physical chord as its key.
        # Historical QAM initialized a missing Quick and Builder to (0,0)
        # in that order. The second entry overwrote the first in MCM.
        native_mcm_map = {}
        for action in ("openQuickMenu", "openMenuBuilder"):
            native_mcm_map[(0, 0)] = action
        self.assertEqual(native_mcm_map, {(0, 0): "openMenuBuilder"})
        native_mcm_map[(259, 0)] = "openQuickMenu"
        native_mcm_map[(260, 0)] = "openMenuBuilder"
        self.assertEqual(
            [chord for chord, action in native_mcm_map.items()
             if action == "openMenuBuilder"], [(0, 0), (260, 0)])
        self.assertEqual(
            [chord for chord, action in native_mcm_map.items()
             if action == "openQuickMenu"], [(259, 0)])

    def test_mcm_does_not_override_standalone_default_without_provider(self):
        self.assertIn('GetModuleHandleW(L"mcm.dll")', SOURCE)
        self.assertIn('GetModuleHandleW(L"F4SEMenuFramework.dll")', SOURCE)
        self.assertIn("if (!McmProviderActive())", SOURCE)
        self.assertIn("ApplyConfiguredHotkeys(nullptr);", SOURCE)


if __name__ == "__main__":
    unittest.main()
