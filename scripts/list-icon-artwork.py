#!/usr/bin/env python3
import json
from collections import defaultdict
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "data" / "icon_library"
data = json.loads((root / "icon_artwork_manifest.json").read_text(encoding="utf-8"))

entries = data["entries"]
aliases = data.get("artwork_reuse", {}).get("class_aliases", {})
pilot = set(data.get("pilot_set", []))
by_batch = defaultdict(list)
for class_id, entry in entries.items():
    by_batch[str(entry["batch"])].append((class_id, entry))

print(f"TOTAL ICON CLASSES: {len(entries)}")
print(f"PLANNED BASE ARTWORKS: {len(entries) - len(aliases)}")
print(f"REUSED BASE ARTWORKS: {len(aliases)}")
print(f"PILOT SET: {len(pilot)}")
for class_id in data.get("pilot_set", []):
    entry = entries[class_id]
    print(f"  {class_id:<34} {entry['filename']}")

for batch_id in sorted(by_batch):
    batch = data["batches"][batch_id]
    rows = sorted(by_batch[batch_id])
    print(f"\nBATCH {batch_id}: {batch['name']} ({len(rows)})")
    for class_id, entry in rows:
        marker = "*" if class_id in pilot else " "
        base = aliases.get(class_id, {}).get("base_artwork_id", class_id)
        target = entries[base]
        reused = f" -> {base}" if base != class_id else ""
        print(
            f"{marker} {class_id:<34} "
            f"{target['filename']:<40}{reused}"
        )
