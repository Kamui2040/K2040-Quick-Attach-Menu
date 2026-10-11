import json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
G=ROOT/"dist/Data/MCM/Config/K2040_Quick_Attach_Menu"
S=ROOT/"packaging/mcm-standalone/Data/MCM/Config/K2040_Quick_Attach_Menu"
class MenuVariants(unittest.TestCase):
 def test_variants_and_native_ids_preserved(self):
  grouped=json.loads((G/"config.json").read_text())
  standalone=json.loads((S/"config.json").read_text())
  self.assertEqual(grouped["modName"],"K2040_Quick_Attach_Menu")
  self.assertEqual(grouped["ownerModName"],"K2040_Mods")
  self.assertEqual(len(grouped["pages"]),1)
  self.assertEqual(grouped["pages"][0]["pageDisplayName"],"Quick Attach Menu")
  self.assertEqual(grouped["content"],[])
  self.assertNotIn("ownerModName",standalone)
  self.assertEqual(standalone["content"],grouped["pages"][0]["content"])
  self.assertEqual([a["id"] for a in standalone["content"] if a.get("type")=="hotkey"],["openQuickMenu","openMenuBuilder"])
  self.assertEqual((G/"keybinds.json").read_bytes(),(S/"keybinds.json").read_bytes())
 def test_own_namespace_not_parent(self):
  keys=json.loads((G/"keybinds.json").read_text())
  self.assertEqual(keys["modName"],"K2040_Quick_Attach_Menu")
  self.assertEqual([x["id"] for x in keys["keybinds"]],["openQuickMenu","openMenuBuilder"])
if __name__=="__main__":unittest.main()
