"""Source-level contracts for the standalone Clock Slices presentation."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
UI = ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"


class ClockPresentationContract(unittest.TestCase):
    def test_persistence_and_independent_layout(self):
        header = (ROOT/"include/UserSettings.h").read_text()
        settings = (ROOT/"src/UserSettings.cpp").read_text()
        bridge = (ROOT/"src/PrismaBridge.cpp").read_text()
        for fragment in ("QuickMenuLayoutSettings clock;", "bool shortenClockLabels = true;"):
            self.assertIn(fragment, header)
        for fragment in ('value == "clock"', 'writeLayout("clock"', 'ReadOptionalLayoutMember(*general, "clock"',
                         'storeLayout(general.clock', 'writeBool("shortenClockLabels"',
                         'ReadOptionalBoolMember(*general, "shortenClockLabels"'):
            self.assertIn(fragment, settings)
        for fragment in ('preferences.clock', 'preferences.shortenClockLabels', '"shorten-clock-labels"'):
            self.assertIn(fragment, bridge)

    def test_rendering_and_controls(self):
        renderer = (UI/"quick-menu-renderer.js").read_text()
        geometry = (UI/"quick-menu-geometry.js").read_text()
        css = (UI/"quick-menu.css").read_text()
        settings = (UI/"settings.html").read_text()
        for fragment in ('renderClock(categories, category)', 'clockOptionSlice', 'shortenCategoryWords',
                         'self.activateOption(category, option)', 'self.handleOrbitWheel(event,'):
            self.assertIn(fragment, renderer if fragment != 'clockOptionSlice' else geometry)
        self.assertIn(".presentation-clock .qm-clock-layout", css)
        self.assertIn('data-value="clock"', settings)
        self.assertIn('id="shortenClockLabels"', settings)
        self.assertIn("clockPolygonPath(", renderer)
        self.assertIn("geometry.outerRadius - geometry.innerRadius - 30", renderer)
        self.assertIn("innerRadius: 234, outerRadius: 462", geometry)
        self.assertIn("Reset options", geometry)
        self.assertIn('toggle("shortenClockLabels", "shorten-clock-labels")', settings)


if __name__ == "__main__":
    unittest.main()
