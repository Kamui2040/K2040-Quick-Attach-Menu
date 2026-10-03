#!/usr/bin/env python3
import argparse
import json
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

from icon_library_common import classify

API = "https://fallout.fandom.com/api.php"
PAGE = "Fallout_4_OMOD_weapons"

TYPE_TO_SLOT = {
    "Gun receiver": "ap_gun_receiver",
    "Gun Scope": "ap_gun_Scope",
    "Gun Barrel": "ap_gun_Barrel",
    "Gun Grip": "ap_gun_Grip",
    'Gun Muzzle "No Muzzle"': "ap_gun_Muzzle",
    'Gun Mag "Magazine"': "ap_gun_Mag",
}

EXPECTED = {
    "Gun receiver": {"receiver"},
    "Gun Scope": {"sights"},
    "Gun Barrel": {"barrel"},
    "Gun Grip": {"grip_stock", "stock", "pistol_grip"},
    'Gun Muzzle "No Muzzle"': {"muzzle"},
    'Gun Mag "Magazine"': {"magazine"},
}

def fetch_wikitext():
    query = urllib.parse.urlencode(
        {
            "action": "parse",
            "page": PAGE,
            "prop": "wikitext",
            "format": "json",
            "formatversion": "2",
        }
    )
    request = urllib.request.Request(
        f"{API}?{query}",
        headers={"User-Agent": "K2040-Quick-Attach-Menu research"},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)["parse"]["wikitext"]

def parse_rows(text):
    rows, cells = [], []
    for line in text.splitlines():
        if line.startswith("|-"):
            if len(cells) >= 4:
                rows.append(cells[:4])
            cells = []
        elif line.startswith("| ") or line == "|":
            cells.append(line[1:].strip())
    if len(cells) >= 4:
        rows.append(cells[:4])
    return rows

def clean_type(value):
    match = re.search(r"\{\{tooltip\|([^|}]+)", value)
    return match.group(1).strip() if match else value.strip()

def clean_name(value):
    value = re.sub(r"\{\{[^{}]*\}\}", "", value)
    value = re.sub(r"\[\[[^|\]]+\|([^\]]+)\]\]", r"\1", value)
    value = re.sub(r"\[\[([^\]]+)\]\]", r"\1", value)
    return re.sub(r"\s+", " ", value).strip()

def main():
    parser = argparse.ArgumentParser(
        description="Audit the Fallout Wiki weapon OMOD table against the icon classifier."
    )
    parser.add_argument(
        "--wikitext",
        type=Path,
        help="Use a local MediaWiki source file instead of fetching the public API.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print machine-readable summary JSON.",
    )
    args = parser.parse_args()

    text = args.wikitext.read_text(encoding="utf-8") if args.wikitext else fetch_wikitext()
    rows = parse_rows(text)

    audited = 0
    fallback = []
    wrong = []
    for row in rows:
        attachment_type = clean_type(row[0])
        slot = TYPE_TO_SLOT.get(attachment_type)
        if not slot:
            continue
        audited += 1
        name = clean_name(row[1])
        result = classify(name, slot)
        if result.get("subcategory") is None:
            fallback.append({"type": attachment_type, "name": name, "result": result})
        if result["category"] not in EXPECTED[attachment_type]:
            wrong.append({"type": attachment_type, "name": name, "result": result})

    summary = {
        "source": PAGE,
        "license": "CC-BY-SA",
        "audited": audited,
        "refined": audited - len(fallback),
        "fallback": len(fallback),
        "wrong_broad_category": len(wrong),
        "fallback_rows": fallback,
        "wrong_rows": wrong,
    }

    if args.json:
        print(json.dumps(summary, indent=2, ensure_ascii=False))
    else:
        print(
            f"PASS: audited={audited} refined={summary['refined']} "
            f"fallback={len(fallback)} wrong={len(wrong)}"
        )
        for row in fallback:
            print(f"FALLBACK: {row['type']} | {row['name']}")
        for row in wrong:
            print(f"WRONG: {row['type']} | {row['name']} | {row['result']['category']}")

    if wrong:
        raise SystemExit(1)

if __name__ == "__main__":
    main()
