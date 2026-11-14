"""Regression contract for CEF annular fills and readable wheel labels."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
UI = ROOT / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"

class AnnularSectorContract(unittest.TestCase):
    def test_convex_fill_path_and_svg_labels(self):
        renderer = (UI/"quick-menu-renderer.js").read_text()
        self.assertIn("function appendSector(group, shapeClass", renderer)
        self.assertIn('tile.setAttribute("class", "qm-sector-tile")', renderer)
        self.assertIn('outline.setAttribute("fill", "none")', renderer)
        self.assertIn('appendSector(group, "qm-wheel-path"', renderer)
        self.assertIn("Renderer.prototype.appendSectorSvgLabel", renderer)
        self.assertIn('textElement.setAttribute("data-wheel-axis", "horizontal")', renderer)
        self.assertIn("this.appendSectorSvgLabel(group,", renderer)
        self.assertIn('svg.setAttribute("viewBox", "0 0 760 760")', renderer)
        self.assertIn("var radialFit = Math.min(", renderer)
        self.assertNotIn('group.appendChild(path);\n    var labelBox = this.appendWheelLabel(', renderer)

    def test_no_translucent_outline_overdraw(self):
        css = (UI/"quick-menu.css").read_text()
        self.assertIn(".qm-sector-tile", css)
        self.assertIn(".qm-wheel-svg-label", css)
        self.assertIn(".qm-wheel-segment > .qm-wheel-path,", css)
        self.assertIn("fill: none !important;", css)
        self.assertIn(".presentation-clock .qm-clock-option.selected > .qm-sector-tile", css)
        self.assertIn("width: 760px;", css)


if __name__ == "__main__":
    unittest.main()
