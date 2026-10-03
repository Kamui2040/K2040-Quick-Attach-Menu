#!/usr/bin/env python3
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "data" / "icon_library"
taxonomy = json.loads((root / "attachment_taxonomy.json").read_text(encoding="utf-8"))
aliases = json.loads((root / "attachment_aliases.json").read_text(encoding="utf-8"))
patterns = json.loads((root / "attachment_patterns.json").read_text(encoding="utf-8"))
weapons = json.loads((root / "weapon_names.json").read_text(encoding="utf-8"))
sources = json.loads((root / "sources.json").read_text(encoding="utf-8"))
index = json.loads((root / "nexus_weapon_index.json").read_text(encoding="utf-8"))

valid = {cat: set(subs) for cat, subs in taxonomy["categories"].items()}
errors, seen = [], {}

for cat, subs in aliases.items():
    if cat not in valid:
        errors.append(f"unknown category: {cat}")
        continue
    for sub, names in subs.items():
        if sub not in valid[cat]:
            errors.append(f"unknown subcategory: {cat}/{sub}")
        for name in names:
            key = " ".join(name.casefold().split())
            prior = seen.get(key)
            if prior and prior != (cat, sub):
                errors.append(f"ambiguous alias {name!r}: {prior[0]}/{prior[1]} and {cat}/{sub}")
            seen[key] = (cat, sub)

for rule in patterns.get("ordered_patterns", []):
    cat, sub = rule.get("category"), rule.get("subcategory")
    if cat not in valid or sub not in valid.get(cat, set()):
        errors.append(f"invalid pattern target: {cat}/{sub}")
    if not rule.get("tokens"):
        errors.append(f"pattern has no tokens: {cat}/{sub}")

weapon_seen = set()
for names in weapons.values():
    for name in names:
        key = name.casefold()
        if key in weapon_seen:
            errors.append(f"duplicate weapon name: {name!r}")
        weapon_seen.add(key)

source_ids = [x.get("id") for x in sources]
if len(source_ids) != len(set(source_ids)):
    errors.append("duplicate source id")
if index.get("schema") != 1:
    errors.append("unsupported nexus weapon index schema")

if errors:
    print("FAIL")
    for error in errors:
        print(error)
    raise SystemExit(1)

print(f"PASS: {len(valid)} top-level categories")
print(f"PASS: {len(seen)} attachment aliases")
print(f"PASS: {len(weapon_seen)} weapon names")
print(f"PASS: {len(sources)} provenance sources")
