#!/usr/bin/env python3
import argparse
import csv
import json
from pathlib import Path

SECTION_MAP = {
    "Barrels": "barrel",
    "Grips/Stocks": "grip_stock",
    "Grips/Stock": "grip_stock",
    "Stocks": "stock",
    "Receivers": "receiver",
    "Sights": "sights",
    "Magazines": "magazine",
    "Muzzles": "muzzle",
    "Nozzles": "muzzle",
    "Tanks": "magazine",
}

SOURCE_URL = (
    "https://docs.google.com/spreadsheets/d/"
    "1hMMhddeTwhGRD7E5X9aBIH1zF5jFNNOrsxFdBem_IVc/edit?usp=sharing"
)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_path", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    rows = list(csv.reader(args.csv_path.open(encoding="utf-8-sig", newline="")))
    weapons = []
    current_weapon = None
    current_section = None
    weapon_obj = None

    for row in rows:
        cells = [cell.strip() for cell in row]
        first = cells[0] if cells else ""
        rest = cells[1:] if len(cells) > 1 else []

        if not first:
            current_section = None
            continue

        if first in SECTION_MAP:
            if weapon_obj is not None:
                current_section = SECTION_MAP[first]
                weapon_obj["attachments"].setdefault(current_section, [])
            continue

        if all(not cell for cell in rest):
            current_weapon = first
            weapon_obj = {"name": current_weapon, "attachments": {}}
            weapons.append(weapon_obj)
            current_section = None
            continue

        if weapon_obj is not None and current_section is not None:
            bucket = weapon_obj["attachments"][current_section]
            if first not in bucket:
                bucket.append(first)

    weapons = [w for w in weapons if any(w["attachments"].values())]
    total_mods = sum(
        len(names)
        for weapon in weapons
        for names in weapon["attachments"].values()
    )
    unique_mods = {
        name.casefold()
        for weapon in weapons
        for names in weapon["attachments"].values()
        for name in names
    }

    output = {
        "schema": 1,
        "source": {
            "name": "Complete List of All Vanilla Weapon Modifications",
            "url": SOURCE_URL,
            "sheet": "All Ranged Weapons",
        },
        "purpose": (
            "Derived short-name catalog for icon research. "
            "Stats/descriptions from the source sheet are intentionally omitted."
        ),
        "counts": {
            "weapons": len(weapons),
            "attachment_rows": total_mods,
            "unique_attachment_names": len(unique_mods),
        },
        "weapons": weapons,
    }
    args.output.write_text(
        json.dumps(output, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

if __name__ == "__main__":
    main()
