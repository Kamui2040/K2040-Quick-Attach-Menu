"""Source-level AE-only guard checks, not runtime QA."""
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
class AEReequipContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        cls.settings = (ROOT / "include/Settings.h").read_text()
        cls.ini = (ROOT / "dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.ini").read_text()
    def test_ae_only_opt_in(self):
        body = self.bridge.split("bool TryAutoReequipModifiedWeaponAE(",1)[1].split("bool ActivateQuickMenuGameplayIsolation(",1)[0]
        self.assertIn("REL::Version{ 1, 11, 240, 0 }", body)
        self.assertIn("!k2040::GetSettings().aeAutoReequipAfterApply",body)
        self.assertIn("bool aeAutoReequipAfterApply = false;",self.settings)
        self.assertIn("AEAutoReequipAfterApply=false",self.ini)
        self.assertIn('key == "AEAutoReequipAfterApply"',(ROOT/"src/Settings.cpp").read_text())
    def test_og_guard_remains(self):
        self.assertIn("REL::Version{ 1, 10, 163, 0 }", self.bridge)
        self.assertIn("REL::ID(1153963)",self.bridge)
        self.assertIn("if (RefreshModifiedEquippedItem(player, weapon))",self.bridge)
        self.assertIn("pendingAEReequipInfo_ = result.weaponInfo;",self.bridge)
        self.assertIn("TryAutoReequipModifiedWeaponAE(player, weapon, *deferredAE)",self.bridge)
        self.assertNotIn("TryAutoReequipModifiedWeaponAE(player, weapon, result.weaponInfo)",self.bridge)
    def test_guarded_stack_and_ammo(self):
        body=self.bridge.split("bool TryAutoReequipModifiedWeaponAE(",1)[1].split("bool ActivateQuickMenuGameplayIsolation(",1)[0]
        for needle in ("verified.equippedInventoryStackCount != 1",
            "verified.equippedInventoryStackIndex", "UnequipObject(", "EquipObject(",
            "RE::fallout_cast<RE::EquippedWeaponData*>",
            "loadedAmmo <= after.liveWeaponInstanceData.ammoCapacity",
            "player->GetCurrentAmmo(index) == ammo","player->SetCurrentAmmoCount(index, loadedAmmo)",
            "installedIds(before) != installedIds(verified)"):
            self.assertIn(needle,body)
    def test_false_unequip_result_still_recovers_equipment(self):
        body=self.bridge.split("bool TryAutoReequipModifiedWeaponAE(",1)[1].split("bool ActivateQuickMenuGameplayIsolation(",1)[0]
        self.assertIn("const bool unequipReturned = manager->UnequipObject(",body)
        self.assertIn("const bool equipReturned = manager->EquipObject(",body)
        self.assertLess(body.index("const bool unequipReturned"),body.index("const bool equipReturned"))
        self.assertLess(body.index("const bool equipReturned"),body.index("const auto after = k2040::GetEquippedWeaponInfo();"))
        self.assertNotIn("unequip refused; no further equip call",body)
    def test_internal_range_only_hidden_in_generated_choices(self):
        source=(ROOT/"src/RuntimeState.cpp").read_text()
        body=source.split("EcoWeaponMenu BuildGenericWeaponMenu_ReadOnly(",1)[1].split("static EcoWeaponMenu BuildEcoAuthoredWeaponMenu_ReadOnly(",1)[0]
        self.assertIn('candidate.consumes.editorId == "ap_Gun_UniversalOffset_Range"',body)
        self.assertIn("Generated menu kept universal range offset internal",body)
        self.assertLess(body.index('candidate.consumes.editorId == "ap_Gun_UniversalOffset_Range"'), body.index("candidates.push_back(std::move(candidate))"))
    def test_ae_requip_is_coalesced_and_deferred(self):
        h=(ROOT/"include/PrismaBridge.h").read_text()
        self.assertIn("std::optional<EquippedWeaponInfo> pendingAEReequipInfo_;",h)
        self.assertIn("std::uint64_t menuGeneration_ = 0;",h)
        self.assertIn("pendingAEReequipInfo_ = result.weaponInfo;",self.bridge)
        self.assertIn("deferredAE = std::move(pendingAEReequipInfo_);",self.bridge)
        self.assertIn("TryAutoReequipModifiedWeaponAE(player, weapon, *deferredAE)",self.bridge)
        self.assertIn("taskInterface->AddTask([this, expectedGeneration, restoreFirstPerson,",self.bridge)
        self.assertIn("menuGeneration_ != expectedGeneration || menuOpen_",self.bridge)
        self.assertIn("pendingAEReequipInfo_.reset();",self.bridge)
        self.assertIn("QueueShow1stPerson(true)",self.bridge)
        self.assertLess(self.bridge.index("TryAutoReequipModifiedWeaponAE(player, weapon, *deferredAE)"),
                        self.bridge.index("Queued first-person presentation refresh after post-close weapon work."))
    def test_version_sync(self):

        self.assertIn("0x000500D5; // 0.5.213",(ROOT/"src/main.cpp").read_text())
        self.assertEqual((ROOT/"xmake.lua").read_text().count("0.5.213"),3)
if __name__=="__main__":
    unittest.main()
