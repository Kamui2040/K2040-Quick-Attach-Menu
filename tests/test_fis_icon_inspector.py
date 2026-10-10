"""Standalone tests for the read-only FIS SWF icon inventory helper."""

import importlib.util
import struct
import tempfile
import unittest
import zlib
from pathlib import Path


INSPECTOR = Path(__file__).resolve().parents[1] / "scripts" / "inspect-fis-icons.py"
SPEC = importlib.util.spec_from_file_location("fis_icon_inspector", INSPECTOR)
assert SPEC is not None and SPEC.loader is not None
fis = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fis)


def tag(code: int, payload: bytes = b"") -> bytes:
    if len(payload) < 63:
        return struct.pack("<H", code * 64 + len(payload)) + payload
    return struct.pack("<HI", code * 64 + 63, len(payload)) + payload


def swf(symbols=None, placements=None, compressed=False):
    symbols = symbols or {2: "m_M8r.Repo.Mod"}
    placements = placements or [1]
    shapes = b"".join(tag(32, struct.pack("<H", sid) + b"\x00") for sid in placements)
    sprite_parts = b"".join(
        tag(26, b"\x02" + struct.pack("<HH", depth, sid))
        for depth, sid in enumerate(placements, 1)
    )
    sprite = tag(39, struct.pack("<HH", 2, 1) + sprite_parts + tag(0))
    names = struct.pack("<H", len(symbols)) + b"".join(
        struct.pack("<H", sid) + name.encode() + b"\x00"
        for sid, name in symbols.items()
    )
    body = b"\x08\x00" + b"\x00\x18\x01\x00" + shapes + sprite + tag(76, names) + tag(0)
    header = b"FWS" + b"\x0a" + struct.pack("<I", len(body) + 8)
    data = header + body
    return header.replace(b"FWS", b"CWS", 1) + zlib.compress(body) if compressed else data


class TestFISIconInspector(unittest.TestCase):
    def inspect(self, data):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "test.swf"
            path.write_bytes(data)
            return fis.inspect_swf(path)

    def test_uncompressed_single_shape(self):
        result = self.inspect(swf())
        self.assertEqual(result["exported_symbols"], 1)
        self.assertEqual(result["single_shape_symbols"], 1)
        self.assertEqual(result["embedded_bitmaps"], 0)
        self.assertEqual(result["symbols"][0]["name"], "m_M8r.Repo.Mod")
        self.assertEqual(result["symbols"][0]["drawables"], [1])

    def test_compressed_single_shape(self):
        result = self.inspect(swf(compressed=True))
        self.assertEqual(result["single_shape_symbols"], 1)

    def test_composite_not_reported_as_single_shape(self):
        result = self.inspect(swf(placements=[1, 3]))
        self.assertEqual(result["single_shape_symbols"], 0)
        self.assertEqual(result["composite_or_unsupported_symbols"], 1)
        self.assertEqual(result["symbols"][0]["drawables"], [1, 3])

    def test_missing_symbol_target_fails_closed(self):
        result = self.inspect(swf(symbols={9: "m_NoMatch"}))
        self.assertEqual(result["single_shape_symbols"], 0)

    def test_invalid_signature(self):
        with self.assertRaises(fis.SWFError):
            self.inspect(b"BAD\x0a\x00\x00\x00\x00\x00")

    def test_incorrect_claimed_length(self):
        bad = bytearray(swf())
        struct.pack_into("<I", bad, 4, 10)
        with self.assertRaises(fis.SWFError):
            self.inspect(bytes(bad))

    def test_malformed_tag_length(self):
        data = bytearray(swf())
        data[-2:] = struct.pack("<H", 63 | (39 << 6))
        with self.assertRaises(fis.SWFError):
            self.inspect(bytes(data))


if __name__ == "__main__":
    unittest.main()
