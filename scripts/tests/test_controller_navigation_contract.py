"""Native controller-binding and stick-neutral contracts (static checks)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class ControllerInputContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        cls.poller = (ROOT / "src/Hotkey.cpp").read_text()
        cls.html = (ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/menu.html").read_text()

    def test_mapped_game_control_confirm(self):
        self.assertIn('ReadMappedControllerButton(\n            "Activate", RE::UserEvents::INPUT_CONTEXT_ID::kMainGameplay)', self.bridge)
        self.assertIn('ReadMappedControllerButton(\n                "Accept", RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav)', self.bridge)
        self.assertIn('controls->GetMappedKey(event, RE::INPUT_DEVICE::kGamepad, context)', self.bridge)
        self.assertIn('api_->InteropCall(menuView_, "k2040ControllerConfirmButton", confirm)', self.bridge)
        self.assertIn('CaptureControllerButtonMapping();', self.bridge)
        self.assertIn('void PrismaBridge::CaptureControllerButtonMapping()', self.bridge)
        bind_block = self.bridge.split('void PrismaBridge::BindControllerActions()', 1)[1].split('void PrismaBridge::PushPayloadToView()',1)[0]
        self.assertNotIn('GetMappedKey(', bind_block)
        self.assertIn('if (button === controllerConfirmButton && !repeating)', self.html)
        self.assertNotIn('if (button === "A" && !repeating)', self.html)

    def test_no_collision_between_accept_and_navigation(self):
        self.assertIn('Controller navigation binding skipped because it overlaps confirmation/cancel', self.bridge)
        self.assertIn('if (std::string_view(cancel) == confirm)', self.bridge)

    def test_neutral_and_repeat_stick(self):
        self.assertIn('const bool released = sector < 0 && previousStickSector >= 0', self.poller)
        self.assertIn('now - previousStickDispatch >= std::chrono::milliseconds(190)', self.poller)
        self.assertIn('if (sector < -1 || sector >= 72 ||', self.bridge)
        self.assertIn('if (sector < 0) {', self.html)
        self.assertIn('var direction = Math.floor((sector + 9) / 18) % 4;', self.html)
        self.assertIn('stickRequiresNeutral = true;', self.html)
        self.assertIn('renderer.navigateDirection(["up", "right", "down", "left"][direction]);', self.html)
        self.assertIn('renderer.selectRadialSector(sector)', self.html)

    def test_shared_keyboard_and_controller_navigation(self):
        renderer = (ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js").read_text()
        self.assertIn('Renderer.prototype.navigateDirection = function(direction)', renderer)
        self.assertIn('if (this.settings().presentation === "hybrid") delta = -delta;', renderer)
        self.assertIn('Renderer.prototype.navigateBack = function()', renderer)
        self.assertIn('Renderer.prototype.navigateShoulder = function(delta)', renderer)
        self.assertIn('renderer.navigateShoulder(button === "LB" ? -1 : 1);', self.html)
        self.assertIn('if (!renderer.navigateBack() &&', self.html)
        self.assertIn('renderer.navigateDirection(', self.html)
        self.assertNotIn('if (!radial) renderer.switchPane(1);', self.html)

    def test_versions_in_sync(self):
        self.assertIn('0x000500D3; // 0.5.211', (ROOT / "src/main.cpp").read_text())
        self.assertEqual((ROOT / "xmake.lua").read_text().count('0.5.211'), 3)

if __name__ == "__main__":
    unittest.main()
