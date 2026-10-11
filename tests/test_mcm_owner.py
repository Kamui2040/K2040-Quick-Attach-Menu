"""Source-only assertions for the self-contained native MCM owner contract."""
import hashlib,json,re,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"dist/Data/MCM/Config/K2040_Mods/config.json"
S=ROOT/"Data/MCM/Config/K2040_Mods/config.json"
C=ROOT/"dist/Data/MCM/Config/K2040_Quick_Attach_Menu"
H="8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb"

class McmOwnerContract(unittest.TestCase):
 def test_parent_template_is_exact(self):
  self.assertEqual(P.read_bytes(),S.read_bytes())
  self.assertEqual(hashlib.sha256(P.read_bytes()).hexdigest(),H)
  parent=json.loads(P.read_text())
  self.assertEqual(parent["modName"],"K2040_Mods")
  self.assertEqual(parent["pages"],[])
  self.assertFalse((P.parent/"keybinds.json").exists())
  self.assertFalse((P.parent/"settings.ini").exists())
 def test_child_is_sole_owner_of_bindings_and_controls(self):
  child=json.loads((C/"config.json").read_text())
  self.assertEqual(child["ownerModName"],"K2040_Mods")
  self.assertEqual(child["modName"],"K2040_Quick_Attach_Menu")
  self.assertEqual(child["content"],[])
  self.assertEqual(len(child["pages"]),1)
  self.assertEqual(child["pages"][0]["pageDisplayName"],"Quick Attach Menu")
  content=child["pages"][0]["content"]
  self.assertEqual([c["id"] for c in content if c.get("type")=="hotkey"],
                   ["openQuickMenu","openMenuBuilder"])
  self.assertEqual([c["id"] for c in content if c.get("type")=="dropdown"],
                   ["iQuickAttachPadPrimary:Controller","iQuickAttachPadModifier:Controller"])
  self.assertTrue(all(c["valueOptions"]["sourceType"]=="ModSettingInt" for c in content if c.get("type")=="dropdown"))
  self.assertTrue(all(len(c["valueOptions"]["options"])==17 for c in content if c.get("type")=="dropdown"))
  keys=json.loads((C/"keybinds.json").read_text())
  self.assertEqual(keys["modName"],"K2040_Quick_Attach_Menu")
  self.assertEqual([c["id"] for c in keys["keybinds"]],["openQuickMenu","openMenuBuilder"])
  self.assertEqual((C/"settings.ini").read_text(),
                   "[Controller]\niQuickAttachPadPrimary=0\niQuickAttachPadModifier=0\n")
 def test_native_shortcuts_remain_mature_feature_branch(self):
  native=(ROOT/"src/ControllerShortcuts.cpp").read_text()
  for term in ["ControllerShortcuts.ini","Data/MCM/Settings/K2040_Quick_Attach_Menu.ini",
               "PersistBindings","McmBinding","RefreshFromMcm","MoveFileExW",
               "ConflictsWithOtherMods","g_requireControllerRelease"]:
   self.assertIn(term,native)
  self.assertNotIn("Data/MCM/Settings/K2040_Mods.ini",native)
  self.assertIn('InitializeControllerShortcuts()', (ROOT/"src/Hotkey.cpp").read_text())
  self.assertIn('PollControllerShortcutEdges', (ROOT/"src/Hotkey.cpp").read_text())
  self.assertIn('controller-shortcut:',(ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html").read_text())
  self.assertIn('SetControllerShortcut', (ROOT/"src/PrismaBridge.cpp").read_text())
  self.assertIn('SetSharedHotkeyBinding', (ROOT/"src/PrismaBridge.cpp").read_text())
  self.assertEqual(len(re.findall('set_version\\("0.5.214"\\)',(ROOT/"xmake.lua").read_text())),1)
 def test_build_and_packaging_stage_parent_with_child(self):
  xmake=(ROOT/"xmake.lua").read_text()
  self.assertIn('os.cp(path.join(parent_source,"*"),parent_target)',xmake)
  self.assertIn('Data", "MCM", "Config", "K2040_Mods"',xmake.replace('","', '", "'))
  stage=(ROOT/"scripts/stage-mcm-tester.py").read_text()
  self.assertIn('K2040_Quick_Attach_Menu',stage)
  self.assertIn('EXPECTED=',stage)
  self.assertIn('ZipFile',stage)
if __name__=="__main__":unittest.main()
