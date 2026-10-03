#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
from icon_library_common import classify, normalize

SIG_KEYS = {"signature", "recordsignature", "record signature"}
EDID_KEYS = {"editorid", "editor id", "edid", "edid editor id"}
NAME_KEYS = {"name", "full", "full name", "full - name"}
SLOT_KEYS = {"attachment point", "attach point", "slot", "ap", "attach parent slot"}

def scalar(value):
    if isinstance(value, (str, int, float)):
        return str(value).strip()
    if isinstance(value, dict):
        for key in ("value", "Value", "text", "Text", "name", "Name"):
            if key in value and isinstance(value[key], (str, int, float)):
                return str(value[key]).strip()
    return ""

def keynorm(key):
    return normalize(str(key)).replace(" ", "")

def pick_field(obj, wanted):
    for key, value in obj.items():
        nk = normalize(str(key))
        compact = nk.replace(" ", "")
        if nk in wanted or compact in {w.replace(" ", "") for w in wanted}:
            got = scalar(value)
            if got:
                return got
    return ""

def detect_signature(obj):
    sig = pick_field(obj, SIG_KEYS).upper()
    if sig in {"WEAP", "OMOD"}:
        return sig
    for key in obj:
        nk = normalize(str(key)).upper()
        if nk in {"WEAP", "OMOD"}:
            return nk
    return ""

def walk(node, source, out):
    if isinstance(node, dict):
        sig = detect_signature(node)
        if sig:
            editor_id = pick_field(node, EDID_KEYS)
            name = pick_field(node, NAME_KEYS)
            slot = pick_field(node, SLOT_KEYS)
            if editor_id or name:
                display = name or editor_id
                item = {
                    "source": source,
                    "signature": sig,
                    "editor_id": editor_id or None,
                    "name": name or None,
                    "slot": slot or None,
                }
                if sig == "OMOD":
                    item["classification"] = classify(display, slot or None)
                out.append(item)
        for value in node.values():
            walk(value, source, out)
    elif isinstance(node, list):
        for value in node:
            walk(value, source, out)

def main():
    parser = argparse.ArgumentParser(
        description="Extract WEAP/OMOD naming candidates from K2040 xEdit JSON exports."
    )
    parser.add_argument("exports", nargs="+", type=Path)
    parser.add_argument("--unknown-only", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    found = []
    for path in args.exports:
        data = json.loads(path.read_text(encoding="utf-8"))
        walk(data, str(path), found)

    unique = {}
    for item in found:
        key = (
            item["signature"],
            normalize(item.get("editor_id") or ""),
            normalize(item.get("name") or ""),
        )
        unique[key] = item

    rows = list(unique.values())
    if args.unknown_only:
        rows = [
            row for row in rows
            if row["signature"] == "WEAP"
            or row.get("classification", {}).get("method") == "generic_fallback"
        ]

    result = {
        "schema": 1,
        "records": rows,
        "counts": {
            "total": len(rows),
            "weapons": sum(r["signature"] == "WEAP" for r in rows),
            "attachments": sum(r["signature"] == "OMOD" for r in rows),
            "unclassified_attachments": sum(
                r["signature"] == "OMOD"
                and r.get("classification", {}).get("method") == "generic_fallback"
                for r in rows
            ),
        },
    }

    text = json.dumps(result, indent=2, ensure_ascii=False) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")

if __name__ == "__main__":
    main()
