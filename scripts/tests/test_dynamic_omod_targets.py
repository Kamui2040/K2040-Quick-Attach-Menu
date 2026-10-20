"""Generated MNAM matching must prove live keywords with installed OMOD data."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "src/RuntimeState.cpp"


class DynamicOmodTargetContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = RUNTIME.read_text(encoding="utf-8")
        cls.catalog = cls.source.split("EcoWeaponMenu BuildGenericWeaponMenu_ReadOnly(", 1)[1].split(
            "EcoWeaponMenu Build", 1
        )[0]

    def test_live_keyword_needs_active_installed_omod_proof(self):
        source = self.catalog
        for required in (
            "constexpr std::uint32_t kWeaponKeywordPropertyTarget = 31u;",
            "weaponInfo.equippedInventoryStackFound",
            "weaponInfo.equippedInventoryStackCount == 1",
            "weaponInfo.installedObjectInstanceRawCount ==",
            "weaponInfo.installedObjectInstanceResolvedCount",
            "weaponInfo.equippedInstanceDataPresent",
            "const auto live = ResolveLiveAttachPoints(weaponInfo);",
            "for (const auto& installed : live.installedOmods)",
            "live.liveReachableAttachPoints",
            "TryGetAttachmentContainerData(installedMod, modData)",
            "property.target != kWeaponKeywordPropertyTarget",
            "property.type != RE::BGSMod::Property::TYPE::kForm",
            "property.op != RE::BGSMod::Property::OP::kAdd",
            "property.op != RE::BGSMod::Property::OP::kRem",
            "for (const auto& liveKeyword : weaponInfo.equippedInstanceKeywords)",
            "installedAddedKeywordIds.insert(keyword->GetFormID());",
            "installedRemovedKeywordIds.insert(keyword->GetFormID());",
            "installedAddedKeywordIds.erase(keywordId);",
        ):
            with self.subTest(requirement=required):
                self.assertIn(required, source)

    def test_uninstalled_candidate_cannot_trust_live_keyword_alone(self):
        matching = self.catalog.split("bool targetMatches = candidate.installed;", 1)[1].split(
            "if (!candidate.installed && !targetMatches)", 1
        )[0]
        self.assertIn("equippedWeapon->HasKeyword(keyword) ||", matching)
        self.assertIn("installedAddedKeywordIds.contains(keyword->GetFormID())", matching)
        self.assertIn("explicitTargetMatched = true;", matching)
        self.assertNotIn("ContainsFormId(weaponInfo.equippedInstanceKeywords", matching)

    def test_existing_discovery_safety_is_preserved(self):
        source = self.catalog
        for required in (
            "workbenchRecipeOutputs.contains(candidate.omod.formId)",
            "IsTacticalReloadInfrastructureOmod(candidate.omod)",
            "IsAttachmentCollectionOmod(mod)",
            "candidate.inventoryAvailable",
            "ContainsFormId(graphAttachPoints, candidate.consumes.formId)",
            "ResolveOmodTargetMetadata(mod)",
        ):
            self.assertIn(required, source)


if __name__ == "__main__":
    unittest.main()
