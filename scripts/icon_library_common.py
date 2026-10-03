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
                key = normalize(name)
                bucket = out.setdefault(key, [])
                if not any(item[0] == category and item[1] == subcategory for item in bucket):
                    bucket.append((category, subcategory, name))
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
                categories = rule.get("categories")
                if not categories:
                    categories = [rule["category"]]
                return {
                    "categories": categories,
                    "fallback_category": rule.get("fallback_category", categories[0]),
                    "method": "slot_pattern",
                    "matched": token,
                    "slot": slot_value,
                }
    return None

def classify_name(value: str, category_filter=None):
    aliases = load_aliases()
    normalized = normalize(value or "")
    if category_filter is None:
        allowed = None
    elif isinstance(category_filter, str):
        allowed = {category_filter}
    else:
        allowed = set(category_filter)
    exact_matches = aliases.get(normalized, [])
    if exact_matches:
        eligible = exact_matches if allowed is None else [
            match for match in exact_matches if match[0] in allowed
        ]
        if len(eligible) == 1:
            category, subcategory, matched = eligible[0]
            return {
                "category": category,
                "subcategory": subcategory,
                "method": "exact_alias",
                "matched": matched,
            }
        if allowed is None and len(exact_matches) > 1:
            return None
        if len(eligible) > 1:
            return None

    patterns = json.loads((ROOT / "attachment_patterns.json").read_text(encoding="utf-8"))
    for rule in patterns["ordered_patterns"]:
        if allowed is not None and rule["category"] not in allowed:
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
        refined = classify_name(value, slot["categories"])
        if refined:
            refined["method"] = f"slot_{refined['method']}"
            refined["slot"] = slot_value
            refined["slot_categories"] = slot["categories"]
            return refined
        return {
            "category": slot["fallback_category"],
            "subcategory": None,
            "method": "slot_category_fallback",
            "matched": slot["matched"],
            "slot": slot_value,
            "slot_categories": slot["categories"],
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
