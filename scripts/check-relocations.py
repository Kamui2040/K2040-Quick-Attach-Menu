#!/usr/bin/env python3
"""Validate Fallout 4 AE Address Library targets without nearest-ID fallback.

Usage:
    python3 scripts/check-relocations.py Fallout4.exe version-1-11-240-0.bin 2229234

The supported database layout is exactly count:u64 then count (id:u64,rva:u64)
pairs sorted by ID. Unsupported layouts fail closed. No files are changed.
"""
from __future__ import annotations

import argparse
import bisect
import struct
from pathlib import Path


def read_db(path: Path) -> tuple[list[int], list[int]]:
    data = path.read_bytes()
    if len(data) < 8:
        raise ValueError("address library header missing")
    count = struct.unpack_from("<Q", data)[0]
    if count == 0 or len(data) != 8 + 16 * count:
        raise ValueError("unsupported address library layout")
    ids: list[int] = []
    rvas: list[int] = []
    for index in range(count):
        record_id, rva = struct.unpack_from("<QQ", data, 8 + 16 * index)
        if ids and record_id <= ids[-1]:
            raise ValueError("address library IDs not strictly increasing")
        ids.append(record_id)
        rvas.append(rva)
    return ids, rvas


def executable_sections(path: Path) -> list[tuple[int, int]]:
    data = path.read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError("not a PE executable")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if pe + 24 > len(data) or data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("invalid PE header")
    machine, count = struct.unpack_from("<HH", data, pe + 4)
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    if machine != 0x8664:
        raise ValueError("expected a Windows x64 PE")
    offset = pe + 24 + optional_size
    if count == 0 or offset + 40 * count > len(data):
        raise ValueError("invalid PE section headers")
    regions = []
    for index in range(count):
        start = offset + 40 * index
        vsize, rva, raw_size = struct.unpack_from("<III", data, start + 8)
        flags = struct.unpack_from("<I", data, start + 36)[0]
        # VirtualSize is the mapped section extent; raw padding is not a
        # valid substitute for executable mapped bytes.
        mapped_size = vsize if vsize else raw_size
        if flags & 0x20000000 and mapped_size:
            regions.append((rva, rva + mapped_size))
    if not regions:
        raise ValueError("PE has no executable sections")
    return regions


def check(ids: list[int], rvas: list[int], regions: list[tuple[int, int]], key: int) -> tuple[bool, str]:
    index = bisect.bisect_left(ids, key)
    if index >= len(ids) or ids[index] != key:
        return False, f"ID {key}: absent (never use the adjacent ID)"
    rva = rvas[index]
    if not any(begin <= rva < end for begin, end in regions):
        return False, f"ID {key}: RVA 0x{rva:X} is not executable"
    return True, f"ID {key}: RVA 0x{rva:X} is in an executable PE section"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_exe", type=Path)
    parser.add_argument("address_library", type=Path)
    parser.add_argument("ids", type=int, nargs="+", help="exact Address Library IDs")
    args = parser.parse_args()
    try:
        ids, rvas = read_db(args.address_library)
        regions = executable_sections(args.game_exe)
    except (OSError, ValueError, struct.error) as exc:
        print(f"FAIL: {exc}")
        return 1
    all_valid = True
    for key in args.ids:
        passed, message = check(ids, rvas, regions, key)
        print(("PASS: " if passed else "FAIL: ") + message)
        all_valid &= passed
    return 0 if all_valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
