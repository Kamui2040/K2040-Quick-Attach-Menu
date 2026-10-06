#!/usr/bin/env python3
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "data" / "icon_library"
taxonomy = json.loads((root / "attachment_taxonomy.json").read_text(encoding="utf-8"))
aliases = json.loads((root / "attachment_aliases.json").read_text(encoding="utf-8"))
patterns = json.loads((root / "attachment_patterns.json").read_text(encoding="utf-8"))
slot_patterns = json.loads((root / "attachment_slot_patterns.json").read_text(encoding="utf-8"))
canonical = json.loads((root / "canonical_attach_points.json").read_text(encoding="utf-8"))
community = json.loads((root / "community_attach_points.json").read_text(encoding="utf-8"))
icon_classes = json.loads((root / "icon_classes.json").read_text(encoding="utf-8"))
artwork_manifest = json.loads((root / "icon_artwork_manifest.json").read_text(encoding="utf-8"))
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

if icon_classes.get("schema") != 1:
    errors.append("unsupported icon-class schema")

class_defs = icon_classes.get("classes", {})
subtype_map = icon_classes.get("subtype_map", {})
category_fallbacks = icon_classes.get("category_fallbacks", {})
generic_fallback = icon_classes.get("generic_fallback")

if generic_fallback not in class_defs:
    errors.append(f"missing generic icon class: {generic_fallback}")

mapped_classes = {generic_fallback} if generic_fallback else set()
for cat, subtypes in valid.items():
    mappings = subtype_map.get(cat)
    if mappings is None:
        errors.append(f"missing icon-class category map: {cat}")
        mappings = {}
    missing = subtypes - set(mappings)
    extra = set(mappings) - subtypes
    for sub in sorted(missing):
        errors.append(f"missing icon-class mapping: {cat}/{sub}")
    for sub in sorted(extra):
        errors.append(f"unknown icon-class mapping: {cat}/{sub}")
    for sub, class_id in mappings.items():
        if class_id not in class_defs:
            errors.append(f"unknown icon class for {cat}/{sub}: {class_id}")
        else:
            class_category = class_defs[class_id].get("category")
            if class_category != cat:
                errors.append(
                    f"icon class category mismatch for {cat}/{sub}: "
                    f"{class_id} has {class_category}"
                )
        mapped_classes.add(class_id)

    fallback = category_fallbacks.get(cat)
    if fallback not in class_defs:
        errors.append(f"missing/invalid category icon fallback: {cat} -> {fallback}")
    elif class_defs[fallback].get("category") != cat:
        errors.append(f"category icon fallback mismatch: {cat} -> {fallback}")
    mapped_classes.add(fallback)

extra_category_maps = set(subtype_map) - set(valid)
for cat in sorted(extra_category_maps):
    errors.append(f"unknown icon-class category: {cat}")

unused_classes = set(class_defs) - mapped_classes
for class_id in sorted(unused_classes):
    errors.append(f"unused icon class: {class_id}")

if artwork_manifest.get("schema") != 1:
    errors.append("unsupported artwork-manifest schema")
art_entries = artwork_manifest.get("entries", {})
if set(art_entries) != set(class_defs):
    for class_id in sorted(set(class_defs) - set(art_entries)):
        errors.append(f"missing artwork manifest entry: {class_id}")
    for class_id in sorted(set(art_entries) - set(class_defs)):
        errors.append(f"unknown artwork manifest entry: {class_id}")

filenames = set()
batch_counts = {}
for class_id, entry in art_entries.items():
    class_meta = class_defs.get(class_id, {})
    if entry.get("label") != class_meta.get("label"):
        errors.append(f"artwork label mismatch: {class_id}")
    if entry.get("category") != class_meta.get("category"):
        errors.append(f"artwork category mismatch: {class_id}")
    filename = entry.get("filename")
    if not filename or not filename.endswith(".svg"):
        errors.append(f"invalid artwork filename: {class_id} -> {filename}")
    elif filename in filenames:
        errors.append(f"duplicate artwork filename: {filename}")
    else:
        filenames.add(filename)
    if not entry.get("subject") or not entry.get("distinguishing_feature"):
        errors.append(f"incomplete artwork brief: {class_id}")
    allowed_status = set(artwork_manifest.get("status_values", []))
    status = entry.get("status")
    if status not in allowed_status:
        errors.append(f"invalid artwork status: {class_id} -> {status}")

    if status in {"draft", "approved"}:
        asset_root = artwork_manifest.get("asset_root")
        asset_path = root.parents[1] / asset_root / filename if asset_root and filename else None
        if asset_path is None or not asset_path.is_file():
            errors.append(f"missing artwork asset: {class_id} -> {asset_path}")
        else:
            try:
                svg_root = ET.parse(asset_path).getroot()
                if not svg_root.tag.endswith("svg"):
                    errors.append(f"artwork root is not svg: {class_id}")
                if svg_root.attrib.get("viewBox") != artwork_manifest["render_contract"]["view_box"]:
                    errors.append(f"artwork viewBox mismatch: {class_id}")
                if any(element.tag.endswith("text") for element in svg_root.iter()):
                    errors.append(f"text element forbidden in artwork: {class_id}")
                raw_svg = asset_path.read_text(encoding="utf-8")
                if "currentColor" not in raw_svg:
                    errors.append(f"artwork is not recolor-friendly: {class_id}")
            except Exception as exc:
                errors.append(f"invalid artwork svg {class_id}: {exc}")

    batch = str(entry.get("batch"))
    batch_counts[batch] = batch_counts.get(batch, 0) + 1

for batch_id, batch in artwork_manifest.get("batches", {}).items():
    if batch_counts.get(batch_id, 0) != batch.get("count"):
        errors.append(
            f"artwork batch count mismatch: {batch_id} "
            f"{batch_counts.get(batch_id, 0)} != {batch.get('count')}"
        )

pilot = artwork_manifest.get("pilot_set", [])
if len(pilot) != len(set(pilot)):
    errors.append("duplicate icon class in artwork pilot set")
for class_id in pilot:
    if class_id not in class_defs:
        errors.append(f"unknown artwork pilot icon class: {class_id}")

community_edids = set()
for entry in community.get("entries", []):
    cat = entry.get("category")
    if cat not in valid:
        errors.append(f"invalid community attach-point category: {cat}")
    editor_id = entry.get("editor_id")
    if not editor_id or editor_id.casefold() in community_edids:
        errors.append(f"missing/duplicate community attach-point EditorID: {editor_id}")
    if editor_id:
        community_edids.add(editor_id.casefold())

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
print(f"PASS: {len(community.get('entries', []))} community attach points")
print(f"PASS: {len(class_defs)} visual icon classes")
print(f"PASS: {len(art_entries)} artwork manifest entries / {len(pilot)} pilot icons")
