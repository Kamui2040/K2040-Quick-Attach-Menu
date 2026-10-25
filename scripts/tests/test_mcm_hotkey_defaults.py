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

    def test_existing_assignments_and_explicit_unbound_are_preserved(self):
        seed = SOURCE.split("void SeedDefaultMcmBindings()", 1)[1].split(
            "bool WriteSharedHotkeyBindings", 1)[0]
        self.assertIn("if (FindMcmKeybindObject(json,id)) continue;", seed)
        self.assertIn("initialized?0:key", seed)
        self.assertIn("hotkeys-initialized", seed)
        self.assertIn("MOVEFILE_REPLACE_EXISTING", seed)
        self.assertIn("*keycode ? BindingDisplayName", SOURCE)
        self.assertIn("virtualKey == 0 && modifiers != 0", SOURCE)
        self.assertIn("overrides.openMenu=k2040::HotkeyState{}", SOURCE)
        self.assertIn("overrides.openMenuBuilder=k2040::HotkeyState{}", SOURCE)

    def test_mcm_does_not_override_standalone_default_without_provider(self):
        self.assertIn('GetModuleHandleW(L"mcm.dll")', SOURCE)
        self.assertIn('GetModuleHandleW(L"F4SEMenuFramework.dll")', SOURCE)
        self.assertIn("if (!McmProviderActive())", SOURCE)
        self.assertIn("ApplyConfiguredHotkeys(nullptr);", SOURCE)


if __name__ == "__main__":
    unittest.main()
