"""Host-side safety and integration tests for the standalone native SWF reader.

Uses only original test bytes, or a separately installed icon library supplied
through K2040_TEST_FIS_SWF. No third-party artwork is part of this test.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which("c++"), "C++ compiler unavailable")
class TestNativeFisVectors(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workspace = tempfile.TemporaryDirectory(prefix="k2040-fis-native-")
        cls.binary = Path(cls.workspace.name) / "fis-swf-vector-probe"
        subprocess.run([
            "c++", "-std=c++20", "-O1", "-Wall", "-Wextra", "-Werror",
            "-I" + str(ROOT / "include"),
            str(ROOT / "src/FisSwfVectors.cpp"),
            str(ROOT / "scripts/fis-swf-vector-probe.cpp"),
            "-lz", "-o", str(cls.binary),
        ], check=True, capture_output=True, text=True, timeout=45)

    @classmethod
    def tearDownClass(cls):
        cls.workspace.cleanup()

    def run_probe(self, data: bytes) -> subprocess.CompletedProcess:
        with tempfile.TemporaryDirectory() as workspace:
            path = Path(workspace) / "unsafe.swf"
            path.write_bytes(data)
            return subprocess.run([str(self.binary), str(path)],
                                  capture_output=True, text=True, timeout=5)

    def test_rejects_truncated_swf(self):
        result = self.run_probe(b"CWS\x0a\x00")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("FAIL:", result.stderr)

    def test_rejects_fake_declared_length(self):
        result = self.run_probe(b"CWS\x0a\xff\xff\xff\x7f\x00")
        self.assertNotEqual(result.returncode, 0)

    def test_rejects_unsupported_compression(self):
        result = self.run_probe(b"FWS\x0a\x09\x00\x00\x00\x00")
        self.assertNotEqual(result.returncode, 0)

    def test_rejects_unbounded_input(self):
        result = self.run_probe(b"CWS\x0a\x09\x00\x00\x00" + b"0" * 1048576)
        self.assertNotEqual(result.returncode, 0)

    def test_missing_file_does_not_crash(self):
        missing = Path(self.workspace.name) / "missing.swf"
        result = subprocess.run([str(self.binary), str(missing)],
                                capture_output=True, text=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)

    def test_installed_library_when_configured(self):
        source = os.environ.get("K2040_TEST_FIS_SWF")
        if not source:
            self.skipTest("No installed library path supplied")
        result = subprocess.run([str(self.binary), source],
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("no artwork copied", result.stdout)


if __name__ == "__main__":
    unittest.main()
