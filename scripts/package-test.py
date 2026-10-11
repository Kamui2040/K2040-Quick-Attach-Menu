#!/usr/bin/env python3
"""Build a reproducible isolated Data/ installer from a validated Linux cross-build.

The canonical K2040_Mods MCM parent ships unchanged with each K2040 mod.
Do not insert user MCM preferences or write anything to the active game.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist" / "Data"
PARENT = "Data/MCM/Config/K2040_Mods/config.json"
CHILD = "Data/MCM/Config/K2040_Quick_Attach_Menu/config.json"
KEYBINDS = "Data/MCM/Config/K2040_Quick_Attach_Menu/keybinds.json"
DEFAULTS = "Data/MCM/Config/K2040_Quick_Attach_Menu/settings.ini"
INI = "Data/F4SE/Plugins/K2040_Quick_Attach_Menu.ini"
DLL = "Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll"
CANONICAL_PARENT_SHA256 = "8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb"
AE_DISABLED = b"AEAutoReequipAfterApply=false"
AE_ENABLED = b"AEAutoReequipAfterApply=true"


def payload(*, ae_reequip_tester: bool = False) -> dict[str, bytes]:
    files = sorted(p for p in DIST.rglob("*") if p.is_file())
    if not files:
        raise ValueError("Staged Data files are missing; build first")
    data = {p.relative_to(DIST.parent).as_posix(): p.read_bytes() for p in files}
    for required in (PARENT, CHILD, KEYBINDS, DEFAULTS, INI, DLL):
        if required not in data:
            raise ValueError(f"Missing required build artifact: {required}")

    parent_sha = hashlib.sha256(data[PARENT]).hexdigest()
    if parent_sha != CANONICAL_PARENT_SHA256:
        raise ValueError(f"Shared parent differs from canonical template: {parent_sha}")
    parent = json.loads(data[PARENT])
    if (parent.get("modName") != "K2040_Mods" or parent.get("pages") != [] or
            parent.get("ownerModName") is not None):
        raise ValueError("MCM parent must contain only the unmodified shared root")
    child = json.loads(data[CHILD])
    if (child.get("modName") != "K2040_Quick_Attach_Menu" or
            child.get("ownerModName") != "K2040_Mods" or
            len(child.get("pages", [])) != 1 or
            child["pages"][0].get("pageDisplayName") != "Quick Attach Menu"):
        raise ValueError("Quick Attach must provide one child-owned MCM page")
    controls = child["pages"][0]["content"]
    if [x["id"] for x in controls if x.get("type") == "hotkey"] != [
            "openQuickMenu", "openMenuBuilder"]:
        raise ValueError("Child MCM hotkey controls do not match existing IDs")
    bind = json.loads(data[KEYBINDS])
    if bind.get("modName") != "K2040_Quick_Attach_Menu":
        raise ValueError("Keyboard assignments must remain child-owned")
    actions = {v["id"]: v["action"] for v in bind["keybinds"]}
    if actions != {
        "openQuickMenu": {"type": "CallExternalFunction",
                          "plugin": "K2040_Quick_Attach_Menu", "function": "ToggleQuickMenu"},
        "openMenuBuilder": {"type": "CallExternalFunction",
                             "plugin": "K2040_Quick_Attach_Menu", "function": "ToggleBuilder"},
    }:
        raise ValueError("MCM actions do not match the native child handlers")
    if data[DEFAULTS].decode("utf-8") != (
            "[Controller]\niQuickAttachPadPrimary=0\niQuickAttachPadModifier=0\n"):
        raise ValueError("Controller defaults must be child-owned and unbound")
    if any(x.startswith("Data/MCM/Settings/") for x in data):
        raise ValueError("User MCM settings must not be shipped")
    if any(x.startswith("Data/MCM/Config/K2040_Mods/") and x != PARENT for x in data):
        raise ValueError("Central Hub may not own child bindings or defaults")
    dll = data[DLL]
    if len(dll) < 256 or dll[:2] != b"MZ":
        raise ValueError("DLL does not have a DOS/PE header")
    offset = struct.unpack_from("<I", dll, 0x3C)[0]
    if offset + 6 > len(dll) or dll[offset:offset+4] != b"PE\0\0" or (
            struct.unpack_from("<H", dll, offset + 4)[0] != 0x8664):
        raise ValueError("DLL is not a Windows x64 PE")
    xmake = (ROOT / "xmake.lua").read_text(encoding="utf-8")
    found = re.search(r'set_version\("([0-9]+\.[0-9]+\.[0-9]+)"\)', xmake)
    if not found or found.group(1).encode() not in dll:
        raise ValueError("Compiled DLL does not contain matching xmake version")
    if ae_reequip_tester:
        if data[INI].count(AE_DISABLED) != 1:
            raise ValueError("Expected disabled AE re-equip setting in source INI")
        data[INI] = data[INI].replace(AE_DISABLED, AE_ENABLED)
    return data


def write_archive(output: Path, *, ae_reequip_tester: bool = False) -> str:
    files = payload(ae_reequip_tester=ae_reequip_tester)
    output = output.resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError("Build ZIPs must remain outside future-public Git")
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=9) as z:
        for name, content in sorted(files.items()):
            item = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            item.compress_type = zipfile.ZIP_DEFLATED
            item.external_attr = (0o100644 << 16)
            z.writestr(item, content, compress_type=zipfile.ZIP_DEFLATED,
                       compresslevel=9)
    with zipfile.ZipFile(output) as z:
        if z.testzip() is not None or set(z.namelist()) != set(files):
            raise ValueError("ZIP integrity or inventory validation failed")
        if hashlib.sha256(z.read(PARENT)).hexdigest() != CANONICAL_PARENT_SHA256:
            raise ValueError("ZIP contains an altered Hub parent")
    return hashlib.sha256(output.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="Absolute or relative ZIP output path outside the repository")
    parser.add_argument("--ae-reequip-tester", action="store_true",
                        help="Enable existing AE visual re-equip only in the tester ZIP")
    args = parser.parse_args()
    sha = write_archive(args.output, ae_reequip_tester=args.ae_reequip_tester)
    print(f"PASS: archive validated SHA256={sha} path={args.output.resolve()}")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        sys.exit(f"FAIL: {exc}")
