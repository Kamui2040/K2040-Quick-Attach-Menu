from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class InventoryCountSafetyContract(unittest.TestCase):
    def test_return_and_rollback_paths_use_checked_arithmetic(self):
        source = (ROOT / "src/RuntimeState.cpp").read_text()
        self.assertIn('#include "InventoryCountSafety.h"', source)
        self.assertIn('CanAddInventoryCount(existing->countBefore, existing->quantity)', source)
        self.assertIn('ExpectedInventoryCountMatches(', source)
        self.assertIn('PlanInventoryCorrection(current, expected)', source)
        self.assertNotIn('returned.countBefore + returned.quantity', source)
        self.assertNotIn('it->countBefore + it->quantity', source)
        self.assertNotIn('current + 1 == expected', source)
        self.assertNotIn('current == expected + 1', source)
