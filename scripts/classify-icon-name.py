#!/usr/bin/env python3
import argparse
import json
from icon_library_common import classify

parser = argparse.ArgumentParser()
parser.add_argument("name", nargs="+")
parser.add_argument("--slot", default=None)
args = parser.parse_args()
print(json.dumps(classify(" ".join(args.name), args.slot), indent=2))
