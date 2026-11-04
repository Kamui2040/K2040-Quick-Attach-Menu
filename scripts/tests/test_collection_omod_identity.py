"""Nested OMOD collections are not independent installed attachment choices.

Regression for vanilla weapons with ECO-authored menus whose installed
object-instance data contains both a container and its selected child OMOD.
"""

from dataclasses import dataclass
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[2] / "src/RuntimeState.cpp").read_text(encoding="utf-8")


@dataclass(frozen=True)
class InstalledOmod:
    form_id: int
    consumed_ap: int
    nested_attachment_count: int = 0


def unambiguous_replacement(menu_installed: list[int], live: list[InstalledOmod], ap: int) -> bool:
    """Model the two identities compared by the native fail-closed guard."""
    effective = [record.form_id for record in live
                 if record.consumed_ap == ap and record.nested_attachment_count == 0]
    return len(menu_installed) <= 1 and len(effective) <= 1 and menu_installed == effective


class CollectionOmodIdentityContract(unittest.TestCase):
    def test_runtime_container_recognition_uses_real_nested_records(self):
        body = SOURCE.split("bool IsAttachmentCollectionOmod(const RE::BGSMod::Attachment::Mod* mod)", 1)[1]
        body = body.split("bool IsAttachmentCollectionOmod(std::uint32_t formId)", 1)[0]
        for required in ("TryGetAttachmentContainerData(mod, data)", "data.attachments", "data.attachmentCount > 0"):
            self.assertIn(required, body)

    def test_both_authored_and_generated_menus_ignore_collection_in_identity_and_removals(self):
        identity = SOURCE.split("std::vector<const OmodAttachmentInfo*> liveInstalledAtPoint;", 1)[1].split(
            "const bool menuInstalledCountAmbiguous", 1
        )[0]
        removal = SOURCE.split("std::vector<OmodAttachmentInfo> removals;", 1)[1].split(
            "const bool reachableNow", 1
        )[0]
        for section in (identity, removal):
            self.assertIn("if (IsAttachmentCollectionOmod(installed.omod.formId)) {", section)
            self.assertNotIn("menu.runtimeGenerated && IsAttachmentCollectionOmod", section)
            self.assertIn("continue;", section)
        self.assertIn("liveInstalledAtPoint.push_back(std::addressof(installed));", identity)

    def test_vanilla_hunting_rifle_collection_and_leaf_share_ap(self):
        # Vanilla installed OMODs found in the magazine, muzzle and sight slots.
        # The collection is not an independently installed choice.
        examples = (
            ("magazine", 0x0005D4D7, 0x00178195, 0x0005D4D8, 2),
            ("muzzle", 0x0002249C, 0x00178197, 0x0004F21D, 2),
            ("sight", 0x00022499, 0x0017819D, 0x00148418, 3),
        )
        for category, ap, collection, leaf, nested_count in examples:
            installed = [InstalledOmod(collection, ap, nested_count), InstalledOmod(leaf, ap)]
            for is_generated in (False, True):
                with self.subTest(category=category, runtime_generated=is_generated):
                    self.assertTrue(unambiguous_replacement([leaf], installed, ap))

    def test_two_real_leaf_omods_still_fail_closed(self):
        ap = 0x0005D4D7
        installed = [InstalledOmod(0x00178195, ap, 2), InstalledOmod(0x0005D4D8, ap),
                     InstalledOmod(0x0005D4D9, ap)]
        self.assertFalse(unambiguous_replacement([0x0005D4D8], installed, ap))

    def test_missing_or_mismatched_real_leaf_still_fails_closed(self):
        ap = 0x0005D4D7
        self.assertFalse(unambiguous_replacement([0x0005D4D8], [InstalledOmod(0x00178195, ap, 2)], ap))
        self.assertFalse(unambiguous_replacement([0x0005D4D8], [InstalledOmod(0x0005D4D9, ap)], ap))

    def test_graph_input_and_safety_guards_not_bypassed(self):
        self.assertIn("hypotheticalInstalled.push_back(installed);", SOURCE)
        self.assertIn('return fail("installed-state-ambiguous"', SOURCE)
        self.assertIn('return fail("equipped-stack-ambiguous"', SOURCE)
        self.assertIn('return fail("attachment-point-changed"', SOURCE)
        self.assertIn('return fail("dependency-cycle"', SOURCE)


if __name__ == "__main__":
    unittest.main()
