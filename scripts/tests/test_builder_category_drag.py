from pathlib import Path
import unittest


class BuilderCategoryDrag(unittest.TestCase):
    def test_starting_drag_keeps_the_dragged_row_in_the_dom(self):
        source = (Path(__file__).resolve().parents[2] /
                  "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/builder.html").read_text()
        start = source.split("function beginDrag(event, info) {", 1)[1].split("\n    }", 1)[0]
        self.assertIn("dragState = info;", start)
        self.assertIn('event.currentTarget.closest(".row").classList.add("dragging");', start)
        self.assertNotIn("render()", start)


if __name__ == "__main__":
    unittest.main()
