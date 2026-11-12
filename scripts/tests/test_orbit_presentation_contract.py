from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
UI = ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"

class OrbitPresentationContract(unittest.TestCase):
    def test_native_persists_fifth_layout(self):
        header = (ROOT / "include/UserSettings.h").read_text()
        source = (ROOT / "src/UserSettings.cpp").read_text()
        bridge = (ROOT / "src/PrismaBridge.cpp").read_text()
        self.assertIn("QuickMenuLayoutSettings orbit;", header)
        self.assertIn("Layout orbit;", source)
        self.assertIn('value == "orbit"', source)
        self.assertIn('writeLayout("orbit", g_document.general.orbit)', source)
        self.assertIn('ReadOptionalLayoutMember(*general, "orbit", g_document.general.orbit)', source)
        self.assertIn("result.orbit = resolveLayout(g_document.general.orbit);", source)
        self.assertIn("sameLayout(current.orbit, value.orbit)", source)
        self.assertIn("storeLayout(general.orbit, value.orbit)", source)
        self.assertIn('quickMenu.presentation == "orbit"', bridge)
        self.assertIn('else if (parts[2] == "orbit") layout = &preferences.orbit;', bridge)
        self.assertIn('preferences.orbit = defaultLayout;', bridge)

    def test_browser_has_orbit_preview_and_shared_geometry(self):
        menu = (UI / "menu.html").read_text()
        settings = (UI / "settings.html").read_text()
        renderer = (UI / "quick-menu-renderer.js").read_text()
        css = (UI / "quick-menu.css").read_text()
        for page in (menu, settings):
            self.assertLess(page.index('src="quick-menu-geometry.js"'), page.index('src="quick-menu-renderer.js"'))
        self.assertIn('data-value="orbit"', settings)
        self.assertIn('presentation === "orbit"', renderer)
        self.assertIn('renderOrbit', renderer)
        self.assertIn('handleOrbitWheel', renderer)
        self.assertIn('.presentation-orbit .qm-orbit-layout', css)
        self.assertIn('.presentation-orbit .qm-orbit-option.orbit-selected', css)
        self.assertIn('wheelArcWidth(', renderer)
        self.assertNotIn('label.slice(0, 17)', renderer)

if __name__ == "__main__":
    unittest.main()
