"""Cheat mode must relax inventory rules only, never weapon safety."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
UI = ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"

class CheatModeContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runtime = (ROOT / "src/RuntimeState.cpp").read_text()
        cls.bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        cls.hotkey = (ROOT / "src/Hotkey.cpp").read_text()
        cls.settings = (ROOT / "src/UserSettings.cpp").read_text()
        cls.ui = (UI / "settings.html").read_text()
        cls.renderer = (UI / "quick-menu-renderer.js").read_text()

    def test_opt_in_off_and_persisted(self):
        self.assertIn("bool cheatMode = false;", (ROOT/"include/UserSettings.h").read_text())
        self.assertIn("bool cheatMode = false;", (ROOT/"include/RuntimeState.h").read_text())
        self.assertIn('value_or(false)', self.settings)
        for x in ('writeBool("cheatMode"', '"cheatMode", g_document.general.cheatMode',
                  "current.cheatMode == value.cheatMode",
                  "general.cheatMode = value.cheatMode"):
            self.assertIn(x, self.settings)

    def test_settings_toggle_round_trip(self):
        for x in ('id="cheatMode"', '"cheat-mode":"cheatMode"',
                  'setToggle("cheatMode", !!settings.cheatMode)',
                  'toggle("cheatMode", "cheat-mode")'):
            self.assertIn(x, self.ui)
        for x in ('"cheat-mode"', 'preferences.cheatMode = *value',
                  'preferences.cheatMode = false;',
                  'rebuildMenu = preferences.cheatMode != *value;'):
            self.assertIn(x, self.bridge)
        self.assertIn('quickMenu.cheatMode ? "true" : "false"', self.bridge)

    def test_inventory_only_override(self):
        body=self.runtime.split('const auto applyCheatPolicy =',1)[1].split('const auto weaponSourceOverride',1)[0]
        self.assertIn('option.isStructurallyValid',body)
        self.assertIn('option.isDefaultApplied',body)
        self.assertIn('if (IsEmptyMaterialDefaultOmod(option))',body)
        self.assertIn('option.status = "material-reset-needs-workbench";',body)
        self.assertIn('option.userHidden',body)
        self.assertIn('option.status == "inventory-unavailable"',body)
        self.assertIn('option.status == "no-loose-mod-disallowed"',body)
        self.assertNotIn('option.isStructurallyValid = true',body)
        self.assertIn('MarkImplicitMaterialDefault(generated, weaponInfo);\n            applyCheatPolicy(generated);',self.runtime)
        self.assertIn('MarkImplicitMaterialDefault(authored, weaponInfo);\n            applyCheatPolicy(authored);',self.runtime)
        for text in ('workbenchRecipeOutputs.contains(candidate.omod.formId)',
                     'ResolveOmodTargetMetadata(mod)', 'IsTacticalReloadInfrastructureOmod(',
                     'IsAttachmentCollectionOmod(mod)', 'ap_Gun_UniversalOffset_Range'):
            self.assertIn(text,self.runtime)

    def test_live_validation_and_no_free_inventory(self):
        self.assertIn('request.cheatMode = GetQuickMenuPreferences().cheatMode;',self.bridge)
        self.assertIn('GetQuickMenuPreferences().cheatMode != request.cheatMode',self.bridge)
        self.assertGreaterEqual(self.runtime.count('GetQuickMenuPreferences().cheatMode != request.cheatMode'),2)
        self.assertIn('if (request.cheatMode) return true;',self.runtime)
        self.assertIn('request.cheatMode ? targetLooseBefore : targetLooseBefore - 1',self.runtime)
        self.assertIn('if (request.cheatMode && targetAfterEngine < targetLooseBefore &&',self.runtime)
        self.assertIn('targetAfterEngine != expectedTargetLoose',self.runtime)
        self.assertIn('finalTargetInventory',self.runtime)
        self.assertIn('providerInstallStatePreserved',self.runtime)
        self.assertIn('revalidatedPlan.dependentRemovalOmodFormIds',self.runtime)
        self.assertIn('if (!request.cheatMode && targetLoose && targetLooseBefore == 0)',self.runtime)
        self.assertIn('if (!request.cheatMode && !targetLoose && !GetSettings().allowNoLooseModOptions)',self.runtime)
        self.assertIn('result.status = request.cheatMode ? "attachment-applied-cheat" : "attachment-applied"',self.runtime)

    def test_builder_not_cheat_mode(self):
        self.assertIn('const bool cheatMode = !openBuilder &&',self.hotkey)
        self.assertIn('openBuilder || cheatMode, cheatMode',self.hotkey)
        self.assertIn('Ready (cheat mode; no loose mod needed)',self.renderer)
        self.assertIn('Cheat mode active',self.renderer)

    def test_version(self):
        self.assertIn('0x000500D6; // 0.5.214',(ROOT/'src/main.cpp').read_text())
        self.assertEqual((ROOT/'xmake.lua').read_text().count('0.5.214'),3)

if __name__=="__main__":
    unittest.main()
