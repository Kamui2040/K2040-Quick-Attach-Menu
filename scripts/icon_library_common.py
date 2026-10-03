#!/usr/bin/env python3
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "data" / "icon_library"

def normalize(value: str) -> str:
    value = value.casefold().replace("_", " ").replace("-", " ")
    value = re.sub(r"[\[\](){}:;,/\\]+", " ", value)
    value = re.sub(r"\s+", " ", value).strip()
    return value

def load_aliases():
    raw = json.loads((ROOT / "attachment_aliases.json").read_text(encoding="utf-8"))
    out = {}
    for category, subs in raw.items():
        for subcategory, names in subs.items():
            for name in names:
                out[normalize(name)] = (category, subcategory, name)
    return out

def classify_slot(slot_value: str):
    normalized = normalize(slot_value or "")
    if not normalized:
        return None
    rules = json.loads((ROOT / "attachment_slot_patterns.json").read_text(encoding="utf-8"))
    for rule in rules["ordered_rules"]:
        for token in rule["tokens"]:
            nt = normalize(token)
            if nt and nt in normalized:
                return {
                    "category": rule["category"],
                    "method": "slot_pattern",
                    "matched": token,
                    "slot": slot_value,
                }
    return None

def classify_name(value: str, category_filter: str | None = None):
    aliases = load_aliases()
    normalized = normalize(value or "")
    exact = aliases.get(normalized)
    if exact and (category_filter is None or exact[0] == category_filter):
        category, subcategory, matched = exact
        return {
            "category": category,
            "subcategory": subcategory,
            "method": "exact_alias",
            "matched": matched,
        }

    patterns = json.loads((ROOT / "attachment_patterns.json").read_text(encoding="utf-8"))
    for rule in patterns["ordered_patterns"]:
        if category_filter is not None and rule["category"] != category_filter:
            continue
        for token in rule["tokens"]:
            nt = normalize(token)
            if nt and nt in normalized:
                return {
                    "category": rule["category"],
                    "subcategory": rule["subcategory"],
                    "method": "token_pattern",
                    "matched": token,
                }
    return None

def classify(value: str, slot_value: str | None = None):
    slot = classify_slot(slot_value or "")
    if slot:
        refined = classify_name(value, slot["category"])
        if refined:
            refined["method"] = f"slot_{refined['method']}"
            refined["slot"] = slot_value
            return refined
        return {
            "category": slot["category"],
            "subcategory": None,
            "method": "slot_category_fallback",
            "matched": slot["matched"],
            "slot": slot_value,
        }

    by_name = classify_name(value)
    if by_name:
        by_name["method"] = f"name_only_{by_name['method']}"
        return by_name

    return {
        "category": "special",
        "subcategory": "other",
        "method": "generic_fallback",
        "matched": None,
    }
