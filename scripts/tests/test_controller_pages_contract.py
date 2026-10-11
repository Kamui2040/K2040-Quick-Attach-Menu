"""Builder/Settings controller bindings, dialogs, dropdowns and version tests."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]
ASSET = ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"

class ControllerPagesContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bridge=(ROOT/"src/PrismaBridge.cpp").read_text()
        cls.helper=(ASSET/"controller-pages.js").read_text()
        cls.builder=(ASSET/"builder.html").read_text()
        cls.settings=(ASSET/"settings.html").read_text()
        cls.poller=(ROOT/"src/Hotkey.cpp").read_text()

    def test_shared_controller_scripts_in_both_pages(self):
        for html, mode in ((self.builder,"builder"),(self.settings,"settings")):
            self.assertIn('<script src="controller-pages.js"></script>',html)
            self.assertIn('window.K2040ControllerPages.create("'+mode+'"',html)
            self.assertIn('window.k2040FocusMenuInitial',html)

    def test_native_binding_for_all_views(self):
        block=self.bridge.split("void PrismaBridge::BindControllerActions()",1)[1].split("void PrismaBridge::PushPayloadToView()",1)[0]
        self.assertNotIn('intentionally disabled for Builder/Settings',block)
        self.assertIn('controllerApi_->BindControllerAction(menuView_, button, action)',block)
        self.assertIn('k2040ControllerSecondaryButton',block)
        self.assertIn('k2040ControllerConfirmButton',block)
        self.assertIn('k2040ControllerCancelButton',block)
        self.assertIn('viewMode_ == ViewMode::MenuBuilder',block)
        self.assertIn('std::string_view(button) != confirm',block)
        self.assertIn('std::string_view(button) != cancel',block)
        self.assertIn('controllerInputActive_ = menuOpen_ && controllerApi_;',self.bridge)
        self.assertNotIn('viewMode_ != ViewMode::QuickMenu ||\n            !viewDomReady_',self.bridge)

    def test_mapping_derived_on_game_thread_per_view(self):
        self.assertGreaterEqual(self.bridge.count("CaptureControllerButtonMapping();"),2)
        dom=self.bridge.split("void PrismaBridge::OnDomReady(",1)[1].split("void PrismaBridge::OnCloseRequested",1)[0]
        self.assertNotIn("CaptureControllerButtonMapping()",dom)
        self.assertIn("GetMappedKey(event, RE::INPUT_DEVICE::kGamepad, context)",self.bridge)
        self.assertIn("OnControllerStickSector(sector)",self.poller)

    def test_dropdown_and_modal_are_preemptive(self):
        self.assertIn('function expandedDropdown()',self.helper)
        self.assertIn('function dismissDropdown(panel)',self.helper)
        self.assertIn('function modal()',self.helper)
        self.assertIn('function modalCancel(element)',self.helper)
        self.assertIn('if (open) { dismissDropdown(open); return; }',self.helper)
        self.assertIn('if (dialog) { modalCancel(dialog); return; }',self.helper)
        self.assertIn('if (selected) selected.click();',self.helper)
        self.assertIn('if (name === "up" || name === "down")',self.helper)

    def test_confirmation_is_only_modification_trigger(self):
        self.assertIn('function secondary()',self.helper)
        self.assertIn('if (code === confirmButton) confirm();',self.helper)
        self.assertIn('if (repeat) return;',self.helper)
        self.assertIn('options.isKeyCaptureActive && options.isKeyCaptureActive()',self.helper)
        self.assertIn('cancelKeyCapture: cancelCapture',self.settings)
        self.assertIn('if (value !== Number(e.value))',self.helper)
        self.assertIn('if (!sliderEditing || !e || e.type !== "range") return false;',self.helper)

    def test_horizontal_geometry_contract(self):
        self.assertIn('if (index <= 1) {', self.helper)
        self.assertIn('moveIn(groups[index], direction === "left" ? -1 : 1)', self.helper)
        self.assertIn('focus(groups[index + 1][0]);', self.helper)
        self.assertIn('function settingsGridDirection(node, direction)', self.helper)
        self.assertIn('var columns = 2;', self.helper)
        self.assertIn('function settingsHeader()', self.helper)
        self.assertIn('function settingsRowDirection(node, direction)', self.helper)
        self.assertIn('settingsRowDirection(node, direction)', self.helper)
        self.assertIn('var horizontal = mode === "settings";', self.helper)

    def test_build_version_sync(self):
        self.assertIn("0x000500D6; // 0.5.214",(ROOT/"src/main.cpp").read_text())
        self.assertEqual((ROOT/"xmake.lua").read_text().count("0.5.214"),3)

if __name__=="__main__":unittest.main()
