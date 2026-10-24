"""Generated label precedence contracts; pure C++ behavior is tested separately."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

class GeneratedAttachmentLabelsContract(unittest.TestCase):
    def test_all_json_outputs_share_control_character_escaping(self):
        for source_name in ('AttachmentRuntimeModel.cpp', 'UserSettings.cpp', 'PrismaBridge.cpp'):
            source = (ROOT / 'src' / source_name).read_text()
            self.assertIn('#include "JsonEscape.h"', source)
            self.assertIn('using k2040::JsonEscape;', source)
        self.assertIn('if (c < 0x20)', (ROOT / 'include/JsonEscape.h').read_text())

    def test_generated_omod_name_before_misc_name(self):
        source = (ROOT / 'src/RuntimeState.cpp').read_text()
        generated = source.split('EcoWeaponMenu BuildGenericWeaponMenu_ReadOnly(', 1)[1].split('static EcoWeaponMenu BuildEcoAuthoredWeaponMenu_ReadOnly(', 1)[0]
        self.assertIn("""option.label = ChooseGeneratedAttachmentLabel(
                SafeFullName(candidate.mod),
                SafeFullName(candidate.mod->GetLooseMod()),""", generated)
        self.assertIn('option.omod = candidate.omod;', generated)
        self.assertIn('option.looseMod = candidate.looseMod;', generated)
        self.assertIn('option.isAvailableInInventory = candidate.inventoryAvailable;', generated)

    def test_authored_eco_buttons_remain_authoritative(self):
        source = (ROOT / 'src/RuntimeState.cpp').read_text()
        eco = source.split('static EcoWeaponMenu BuildEcoAuthoredWeaponMenu_ReadOnly(', 1)[1]
        self.assertIn('option.label = optLabelC ? optLabelC : std::string();', eco)

    def test_builder_custom_label_still_overrides_generated_source(self):
        source = (ROOT / 'src/UserSettings.cpp').read_text()
        body = source.split('void ApplyVisibilityPreferences(EcoWeaponMenu& menu)', 1)[1]
        self.assertLess(body.index('option.label = option.sourceLabel;'), body.index('option.label = optionLabel->label;'))
        self.assertIn('option.labelCustomized = true;', body)

if __name__ == '__main__':
    unittest.main()
