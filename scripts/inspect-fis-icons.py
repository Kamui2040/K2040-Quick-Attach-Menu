#!/usr/bin/env python3
"""Inspect exported icon symbols in an installed Flash SWF without extracting artwork."""

from __future__ import annotations

import argparse
import json
import struct
import zlib
from pathlib import Path

MAX_FILE_BYTES = 16 * 1024 * 1024
MAX_UNCOMPRESSED_BYTES = 64 * 1024 * 1024
SHAPE_TAGS = {2, 22, 32, 83}
BITMAP_TAGS = {6, 20, 21, 35, 36, 90}


class SWFError(ValueError):
    """Malformed or unsupported SWF input."""


def u16(data: bytes, offset: int = 0) -> int:
    if offset + 2 > len(data):
        raise SWFError("truncated character ID or length")
    return struct.unpack_from("<H", data, offset)[0]


def tags(data: bytes, offset: int):
    while offset < len(data):
        if offset + 2 > len(data):
            raise SWFError("truncated SWF tag header")
        header = u16(data, offset)
        offset += 2
        kind, size = header >> 6, header & 63
        if size == 63:
            if offset + 4 > len(data):
                raise SWFError("truncated long SWF tag length")
            size = struct.unpack_from("<I", data, offset)[0]
            offset += 4
        if size > len(data) - offset:
            raise SWFError("SWF tag exceeds declared file bounds")
        payload = data[offset : offset + size]
        offset += size
        yield kind, payload
        if kind == 0:
            break


def decompress_swf(path: Path) -> bytes:
    if path.stat().st_size > MAX_FILE_BYTES:
        raise SWFError("SWF input exceeds size limit")
    raw = path.read_bytes()
    if len(raw) < 9:
        raise SWFError("SWF header is truncated")
    expected = struct.unpack_from("<I", raw, 4)[0]
    if expected < 9 or expected > MAX_UNCOMPRESSED_BYTES:
        raise SWFError("invalid decompressed SWF size")
    if raw[:3] == b"FWS":
        data = raw
    elif raw[:3] == b"CWS":
        stream = zlib.decompressobj()
        body = stream.decompress(raw[8:], expected - 8 + 1)
        if not stream.eof or stream.unconsumed_tail or len(body) != expected - 8:
            raise SWFError("compressed SWF data length is invalid")
        data = b"FWS" + raw[3:8] + body
    else:
        raise SWFError("unsupported SWF compression or invalid signature")
    if len(data) != expected:
        raise SWFError("SWF file length mismatch")
    return data


def parse_names(data: bytes) -> dict[int, str]:
    if len(data) < 2:
        raise SWFError("truncated symbol name table")
    count = u16(data)
    offset = 2
    result = {}
    if count > 4096:
        raise SWFError("symbol table too large")
    for _ in range(count):
        symbol_id = u16(data, offset)
        offset += 2
        end = data.find(b"\0", offset)
        if end < 0 or end - offset > 512:
            raise SWFError("invalid exported symbol name")
        name = data[offset:end].decode("utf-8", "strict")
        offset = end + 1
        if not name:
            raise SWFError("empty exported symbol name")
        result[symbol_id] = name
    return result


def placed_characters(data: bytes) -> list[int]:
    if len(data) < 4:
        raise SWFError("truncated sprite definition")
    children = []
    for kind, value in tags(data, 4):
        if kind == 4:  # PlaceObject
            children.append(u16(value))
        elif kind == 26:  # PlaceObject2
            if len(value) < 3:
                raise SWFError("truncated PlaceObject2")
            if value[0] & 2:
                children.append(u16(value, 3))
        elif kind == 70:  # PlaceObject3
            if len(value) < 4:
                raise SWFError("truncated PlaceObject3")
            if value[0] & 2:
                children.append(u16(value, 4))
    return children


def inspect_swf(path: Path) -> dict:
    swf = decompress_swf(path)
    nbits = swf[8] >> 3
    rect_end = 8 + (5 + 4 * nbits + 7) // 8
    if rect_end + 4 > len(swf):
        raise SWFError("truncated SWF frame header")
    names: dict[int, str] = {}
    shapes: set[int] = set()
    bitmaps: set[int] = set()
    sprites: dict[int, list[int]] = {}
    for kind, value in tags(swf, rect_end + 4):
        if kind in SHAPE_TAGS:
            shapes.add(u16(value))
        elif kind in BITMAP_TAGS:
            bitmaps.add(u16(value))
        elif kind == 39:
            sprites[u16(value)] = placed_characters(value)
        elif kind in (56, 76):
            names.update(parse_names(value))
    symbols = []
    for char_id, name in sorted(names.items(), key=lambda entry: entry[1]):
        parts = sprites.get(char_id)
        if parts is not None:
            drawable = [part for part in parts if part in shapes or part in bitmaps]
            asset_kind = "sprite"
        elif char_id in shapes:
            drawable = [char_id]
            asset_kind = "shape"
        elif char_id in bitmaps:
            drawable = [char_id]
            asset_kind = "bitmap"
        else:
            drawable = []
            asset_kind = "unsupported"
        symbols.append({
            "name": name,
            "character_id": char_id,
            "type": asset_kind,
            "drawables": drawable,
            "single_shape": len(drawable) == 1 and drawable[0] in shapes and len(parts or drawable) == 1,
        })
    return {
        "format": "SWF",
        "version": swf[3],
        "exported_symbols": len(symbols),
        "vector_shapes": len(shapes),
        "embedded_bitmaps": len(bitmaps),
        "sprite_count": len(sprites),
        "single_shape_symbols": sum(bool(s["single_shape"]) for s in symbols),
        "composite_or_unsupported_symbols": sum(not s["single_shape"] for s in symbols),
        "symbols": symbols,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--swf", type=Path, required=True, help="Path to player-installed icon library")
    parser.add_argument("--list", action="store_true", help="Include individual exported symbol names and shape IDs")
    args = parser.parse_args()
    try:
        result = inspect_swf(args.swf)
    except (OSError, SWFError, UnicodeError, zlib.error) as exc:
        parser.exit(1, f"FAIL: {exc}\n")
    if not args.list:
        result.pop("symbols")
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
