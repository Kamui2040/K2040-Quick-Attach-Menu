#!/usr/bin/env python3
"""Local-only FIS proof of concept: render selected installed SWF symbols as PNG/WebP.

Requires a separately obtained FFDec JAR, Java and ImageMagick. This is not a
runtime bridge or a packaging tool. Output assets must never be added to Git.
Only direct single-shape symbols are supported in this first test.
"""

from __future__ import annotations

import argparse
import importlib.util
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def load_inspector():
    path = Path(__file__).with_name("inspect-fis-icons.py")
    spec = importlib.util.spec_from_file_location("k2040_fis_inspector", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("icon inspector unavailable")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--swf", type=Path, required=True)
    parser.add_argument("--ffdec-jar", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--symbol", action="append", required=True)
    parser.add_argument("--size", type=int, choices=[22, 32, 48, 64], default=48)
    args = parser.parse_args()
    source_root = Path(__file__).resolve().parent.parent
    output = args.output.resolve()
    if output == source_root or source_root in output.parents:
        parser.error("generated artwork must not be written anywhere inside the repository")
    if not args.ffdec_jar.is_file():
        parser.error("FFDec JAR is not installed at the specified path")
    inspector = load_inspector()
    try:
        inventory = inspector.inspect_swf(args.swf)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    by_name = {symbol["name"]: symbol for symbol in inventory["symbols"]}
    selections = []
    if len(args.symbol) > 32:
        parser.error("limit local preview to 32 selected symbols per run")
    for name in args.symbol:
        name = name if name.startswith("m_") else "m_" + name
        if name not in by_name:
            parser.error(f"symbol not exported by installed library: {name}")
        entry = by_name[name]
        if not entry["single_shape"]:
            parser.error(f"symbol needs composite rendering, not available in prototype: {name}")
        if name not in [item["name"] for item in selections]:
            selections.append(entry)
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output, prefix=".swf-private-") as folder:
        shape_dir = Path(folder)
        subprocess.run([
            "java", "-Djava.awt.headless=true", "-jar", str(args.ffdec_jar),
            "-cli", "-format", "shape:svg", "-export", "shape",
            str(shape_dir), str(args.swf),
        ], check=True, timeout=120, stdout=subprocess.DEVNULL)
        for symbol in selections:
            shape_id = symbol["drawables"][0]
            shape_path = shape_dir / f"{shape_id}.svg"
            if not shape_path.is_file():
                shape_path = shape_dir / "shapes" / f"{shape_id}.svg"
            if not shape_path.is_file():
                raise RuntimeError(f"FFDec did not export shape {shape_id}")
            safe_name = re.sub(r"[^A-Za-z0-9_.-]", "_", symbol["name"])
            png = output / f"{safe_name}_{args.size}.png"
            webp = output / f"{safe_name}_{args.size}.webp"
            size = f"{args.size}x{args.size}"
            subprocess.run([
                "magick", "-background", "none", "-density", "192",
                str(shape_path), "-resize", size, "-gravity", "center",
                "-background", "none", "-extent", size, str(png),
            ], check=True, timeout=30, stdout=subprocess.DEVNULL)
            subprocess.run([
                "magick", str(png), "-define", "webp:lossless=true", str(webp),
            ], check=True, timeout=30, stdout=subprocess.DEVNULL)
            if png.stat().st_size == 0 or webp.stat().st_size == 0:
                raise RuntimeError(f"empty image export: {symbol['name']}")
            print(f"PASS: {symbol['name']} rendered to transparent PNG and lossless WebP")
    print(f"PASS: {len(selections)} installed-library symbol(s) rendered; no SWF was modified")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
