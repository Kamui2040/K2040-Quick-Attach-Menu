#!/usr/bin/env python3
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "data" / "icon_library"
taxonomy = json.loads((root / "attachment_taxonomy.json").read_text(encoding="utf-8"))
aliases = json.loads((root / "attachment_aliases.json").read_text(encoding="utf-8"))
patterns = json.loads((root / "attachment_patterns.json").read_text(encoding="utf-8"))
slot_patterns = json.loads((root / "attachment_slot_patterns.json").read_text(encoding="utf-8"))
canonical = json.loads((root / "canonical_attach_points.json").read_text(encoding="utf-8"))
weapons = json.loads((root / "weapon_names.json").read_text(encoding="utf-8"))
sources = json.loads((root / "sources.json").read_text(encoding="utf-8"))
index = json.loads((root / "nexus_weapon_index.json").read_text(encoding="utf-8"))
packages = json.loads((root / "weapon_mod_package_index.json").read_text(encoding="utf-8"))
wars = json.loads((root / "wars_catalog_seed.json").read_text(encoding="utf-8"))
vanilla = json.loads((root / "vanilla_ranged_catalog.json").read_text(encoding="utf-8"))

valid = {cat: set(subs) for cat, subs in taxonomy["categories"].items()}

def normalize_alias(value):
    value = value.casefold().replace("_", " ").replace("-", " ")
    value = re.sub(r"[\[\](){}:;,/\\]+", " ", value)
    return re.sub(r"\s+", " ", value).strip()

errors, seen = [], {}

for cat, subs in aliases.items():
    if cat not in valid:
        errors.append(f"unknown category: {cat}")
        continue
    for sub, names in subs.items():
        if sub not in valid[cat]:
            errors.append(f"unknown subcategory: {cat}/{sub}")
        for name in names:
            key = normalize_alias(name)
            prior = seen.setdefault(key, [])
            if any(pcat == cat and psub != sub for pcat, psub in prior):
                errors.append(f"ambiguous alias within category {cat}: {name!r}")
            if (cat, sub) not in prior:
                prior.append((cat, sub))

for rule in patterns.get("ordered_patterns", []):
    cat, sub = rule.get("category"), rule.get("subcategory")
    if cat not in valid or sub not in valid.get(cat, set()):
        errors.append(f"invalid pattern target: {cat}/{sub}")
    if not rule.get("tokens"):
        errors.append(f"pattern has no tokens: {cat}/{sub}")

for rule in slot_patterns.get("ordered_rules", []):
    categories = rule.get("categories")
    if not categories:
        categories = [rule.get("category")]
    for cat in categories:
        if cat not in valid:
            errors.append(f"invalid slot category: {cat}")
    fallback = rule.get("fallback_category", categories[0] if categories else None)
    if fallback not in valid:
        errors.append(f"invalid slot fallback category: {fallback}")
    if not rule.get("tokens"):
        errors.append(f"slot pattern has no tokens: {categories}")

canonical_ids, canonical_edids = set(), set()
for entry in canonical.get("entries", []):
    cats = entry.get("categories") or [entry.get("category")]
    for cat in cats:
        if cat not in valid:
            errors.append(f"invalid canonical attach-point category: {cat}")
    fallback = entry.get("fallback_category")
    if fallback and fallback not in valid:
        errors.append(f"invalid canonical attach-point fallback: {fallback}")
    form_id = entry.get("local_form_id")
    editor_id = entry.get("editor_id")
    if not form_id or form_id in canonical_ids:
        errors.append(f"missing/duplicate canonical attach-point FormID: {form_id}")
    if not editor_id or editor_id.casefold() in canonical_edids:
        errors.append(f"missing/duplicate canonical attach-point EditorID: {editor_id}")
    if form_id:
        canonical_ids.add(form_id)
    if editor_id:
        canonical_edids.add(editor_id.casefold())

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
if packages.get("schema") != 1:
    errors.append("unsupported weapon package index schema")
package_sources = set()
package_names = set()
for package_index in packages.get("indexes", []):
    source = package_index.get("source")
    if not source or source in package_sources:
        errors.append(f"missing/duplicate package-index source: {source}")
    if source:
        package_sources.add(source)
    local_packages = set()
    for package in package_index.get("packages", []):
        if not isinstance(package, str) or not package.strip():
            errors.append(f"invalid package title in {source}")
            continue
        key = package.casefold()
        if key in local_packages:
            errors.append(f"duplicate package title inside {source}: {package!r}")
        local_packages.add(key)
        package_names.add(key)
if wars.get("schema") != 1:
    errors.append("unsupported WARS catalog schema")
if vanilla.get("schema") != 1:
    errors.append("unsupported vanilla ranged catalog schema")

vanilla_weapons = vanilla.get("weapons", [])
computed_rows = 0
computed_names = set()
for weapon in vanilla_weapons:
    if not weapon.get("name"):
        errors.append("vanilla catalog weapon missing name")
    for cat, names in weapon.get("attachments", {}).items():
        if cat not in valid:
            errors.append(f"invalid vanilla catalog category: {cat}")
        if len(names) != len(set(names)):
            errors.append(f"duplicate vanilla catalog entry: {weapon.get('name')}/{cat}")
        computed_rows += len(names)
        computed_names.update(name.casefold() for name in names)

counts = vanilla.get("counts", {})
if counts.get("weapons") != len(vanilla_weapons):
    errors.append("vanilla catalog weapon count mismatch")
if counts.get("attachment_rows") != computed_rows:
    errors.append("vanilla catalog attachment row count mismatch")
if counts.get("unique_attachment_names") != len(computed_names):
    errors.append("vanilla catalog unique-name count mismatch")

if errors:
    print("FAIL")
    for error in errors:
        print(error)
    raise SystemExit(1)

alias_mappings = sum(len(v) for v in seen.values())
cross_slot_ambiguous = sum(len(v) > 1 for v in seen.values())
print(f"PASS: {len(valid)} top-level categories")
print(f"PASS: {len(seen)} unique attachment aliases ({alias_mappings} mappings)")
print(f"PASS: {cross_slot_ambiguous} cross-slot ambiguous aliases handled by slot context")
print(f"PASS: {len(weapon_seen)} weapon names")
print(f"PASS: {len(sources)} provenance sources")
print(f"PASS: {len(package_sources)} discovery indexes / {len(package_names)} unique package titles")
print(f"PASS: {len(canonical.get('entries', []))} canonical attach points")
