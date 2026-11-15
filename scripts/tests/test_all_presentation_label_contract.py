"""Common shortening must reach every Quick Menu presentation, not only Clock."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
UI=ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"
class AllPresentationsLabelContract(unittest.TestCase):
    def test_single_label_policy(self):
        code=(UI/"quick-menu-renderer.js").read_text()
        self.assertIn("Renderer.prototype.optionDisplayName = function(category, option)",code)
        self.assertIn("metrics.shortenCategoryWords(fullName,",code)
        self.assertIn("this.optionDisplayName(category, option)",code)
        self.assertIn("self.optionDisplayName(category, option)",code)
    def test_settings_not_clock_exclusive(self):
        html=(UI/"settings.html").read_text()
        self.assertIn("Shorten matching category words",html)
        self.assertIn("all menu layouts",html.lower())
        self.assertNotIn("Clock Slices:",html)
