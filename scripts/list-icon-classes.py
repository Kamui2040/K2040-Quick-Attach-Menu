#!/usr/bin/env python3
import json
from collections import defaultdict
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "data" / "icon_library"
data = json.loads((root / "icon_classes.json").read_text(encoding="utf-8"))

by_category = defaultdict(list)
for class_id, meta in data["classes"].items():
    category = meta.get("category") or "global"
    by_category[category].append((class_id, meta["label"]))

print(f"TOTAL ARTWORKS: {len(data['classes'])}")
for category in sorted(by_category):
    rows = sorted(by_category[category])
    print(f"\n{category} ({len(rows)})")
    for class_id, label in rows:
        print(f"  {class_id:<34} {label}")
