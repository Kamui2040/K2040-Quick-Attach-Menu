"""Tests for the fail-closed Fallout 4 AE relocation checker."""
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "check-relocations.py"
SPEC = importlib.util.spec_from_file_location("check_relocations", SCRIPT)
check = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check)


class RelocationTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.exe = self.root / "Fallout4.exe"
        pe = bytearray(0x400)
        pe[:2] = b"MZ"
        struct.pack_into("<I", pe, 0x3C, 0x80)
        pe[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<HH", pe, 0x84, 0x8664, 2)
        struct.pack_into("<H", pe, 0x94, 0xF0)
        section = 0x80 + 24 + 0xF0
        for i, (name, rva, flags) in enumerate([
            (b".text", 0x1000, 0x60000020),
            (b".rdata", 0x2000, 0x40000040),
        ]):
            off = section + i * 40
            pe[off:off + 8] = name.ljust(8, b"\0")
            struct.pack_into("<IIII", pe, off + 8, 0x900, rva, 0x900, 0)
            struct.pack_into("<I", pe, off + 36, flags)
        self.exe.write_bytes(pe)
        self.db = self.root / "version.bin"
        self.db.write_bytes(
            struct.pack("<Q", 2) +
            struct.pack("<QQ", 5, 0x1100) +
            struct.pack("<QQ", 6, 0x2100)
        )

    def test_exact_executable_only(self):
        ids, rvas = check.read_db(self.db)
        sections = check.executable_sections(self.exe)
        self.assertTrue(check.check(ids, rvas, sections, 5)[0])
        self.assertFalse(check.check(ids, rvas, sections, 6)[0])
        self.assertFalse(check.check(ids, rvas, sections, 4)[0])

    def test_malformed_or_unsorted_databases(self):
        original = self.db.read_bytes()
        for invalid in [
            original + b"trailing",
            struct.pack("<Q", 2) + struct.pack("<QQ", 6, 0x1100) +
                struct.pack("<QQ", 5, 0x2100),
            struct.pack("<Q", 2) + struct.pack("<QQ", 5, 0x1100) +
                struct.pack("<QQ", 5, 0x2100),
        ]:
            self.db.write_bytes(invalid)
            with self.assertRaises(ValueError):
                check.read_db(self.db)

    def test_invalid_pe(self):
        self.exe.write_bytes(b"not a PE")
        with self.assertRaises(ValueError):
            check.executable_sections(self.exe)


if __name__ == "__main__":
    unittest.main()
