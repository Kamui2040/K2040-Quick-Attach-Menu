#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "data" / "icon_library"

def normalize(value: str) -> str:
    value = value.casefold().replace("_", " ").replace("-", " ")
    value = re.sub(r"[\[\](){}:;,/\\\\]+", " ", value)
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

def classify(value: str):
    aliases = load_aliases()
    normalized = normalize(value)
    if normalized in aliases:
        category, subcategory, matched = aliases[normalized]
        return {"category":category,"subcategory":subcategory,"method":"exact_alias","matched":matched}

    patterns = json.loads((ROOT / "attachment_patterns.json").read_text(encoding="utf-8"))
    padded = f" {normalized} "
    for rule in patterns["ordered_patterns"]:
        for token in rule["tokens"]:
            nt = normalize(token)
            if nt and nt in normalized:
                return {
                    "category":rule["category"],
                    "subcategory":rule["subcategory"],
                    "method":"token_pattern",
                    "matched":token,
                }
    return {"category":"special","subcategory":"other","method":"generic_fallback","matched":None}

parser = argparse.ArgumentParser()
parser.add_argument("name", nargs="+")
args = parser.parse_args()
print(json.dumps(classify(" ".join(args.name)), indent=2))
