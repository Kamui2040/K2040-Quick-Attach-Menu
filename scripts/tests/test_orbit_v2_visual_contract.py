"""Keep Orbit V2 uncluttered without changing presentation identity."""
from pathlib import Path
import unittest

UI = Path(__file__).resolve().parents[2] / "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"


class OrbitV2VisualContract(unittest.TestCase):
    def test_presentation_identity_and_preview_settings_unchanged(self):
        settings = (UI / "settings.html").read_text()
        renderer = (UI / "quick-menu-renderer.js").read_text()
        self.assertIn('data-value="orbit"', settings)
        self.assertIn("renderOrbit(categories, category)", renderer)
        self.assertIn("this.options.preview ? clamp(this.options.previewZoom, 1, 2, 1) : 1", renderer)

    def test_uncluttered_orbit_surface(self):
        css = (UI / "quick-menu.css").read_text()
        self.assertIn("Orbit V2", css)
        self.assertNotIn(".qm-orbit-ring::before", css)
        selected = css.split(".presentation-orbit .qm-orbit-option.qm-option-button.orbit-selected {", 1)[1].split("}", 1)[0]
        self.assertIn("background: var(--qm-orbit-focused-bg", selected)
        self.assertIn("border-left: 3px solid var(--qm-accent)", selected)
        self.assertNotIn("background: var(--qm-active)", selected)
        self.assertIn("--qm-orbit-secondary-bg", css)
        self.assertIn(".presentation-orbit .qm-orbit-option.qm-option-button:hover", css)
        self.assertNotIn("opacity: 1 !important", css)
        renderer = (UI / "quick-menu-renderer.js").read_text()
        self.assertNotIn("button.style.opacity = String(position.opacity)", renderer)
        self.assertIn('rgba(panel, Math.max(.15, opacity - .10))', renderer)

    def test_single_attachment_has_no_scroll_prompt(self):
        renderer = (UI / "quick-menu-renderer.js").read_text()
        self.assertIn("if (options.length > 1)", renderer)
        self.assertIn("metrics.orbitOptionWindow(options.length, selectedIndex, 2)", renderer)


if __name__ == "__main__":
    unittest.main()
