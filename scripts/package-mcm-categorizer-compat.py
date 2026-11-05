#!/usr/bin/env python3
"""Package an optional standalone MCM page for legacy MCM Categorizer users.

The main mod always retains its self-contained shared K2040 Mods parent and
ownerModName child. This separate file-override package is opt-in only.
Do not add MCM settings, other mods' configs, or third-party assets.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
CHILD = ROOT / 'dist/Data/MCM/Config/K2040_Quick_Attach_Menu/config.json'
TARGET = 'Data/MCM/Config/K2040_Quick_Attach_Menu/config.json'


def ungrouped_child(source: bytes) -> bytes:
    config = json.loads(source)
    if (config.get('modName') != 'K2040_Quick_Attach_Menu' or
            config.get('ownerModName') != 'K2040_Mods' or
            len(config.get('pages', [])) != 1 or
            config['pages'][0].get('pageDisplayName') != 'Quick Attach Menu'):
        raise ValueError('Expected the canonical grouped Quick Attach MCM child')
    ids = [c['id'] for c in config['pages'][0]['content'] if c.get('type') == 'hotkey']
    if ids != ['openQuickMenu', 'openMenuBuilder']:
        raise ValueError('Unexpected hotkey IDs; refusing to generate compatibility override')
    # No foreign mod config or user settings are ever included.
    config.pop('ownerModName')
    return (json.dumps(config, ensure_ascii=False, indent=2) + '\n').encode('utf-8')


def write_archive(output: Path) -> str:
    result = ungrouped_child(CHILD.read_bytes())
    output = output.resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError('Private build ZIP must not be placed inside Git')
    if output.exists():
        raise FileExistsError(f'Existing package cannot be overwritten: {output}')
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, mode='x', compression=zipfile.ZIP_DEFLATED,
                         compresslevel=9) as zip_file:
        item = zipfile.ZipInfo(TARGET, date_time=(1980, 1, 1, 0, 0, 0))
        item.compress_type = zipfile.ZIP_DEFLATED
        zip_file.writestr(item, result)
    with zipfile.ZipFile(output) as zip_file:
        if zip_file.namelist() != [TARGET] or zip_file.testzip() is not None:
            raise ValueError('Corrupt or unexpected compatibility archive')
        if zip_file.read(TARGET) != result:
            raise ValueError('Compatibility payload verification failure')
    return hashlib.sha256(output.read_bytes()).hexdigest()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='New ZIP path outside the public checkout')
    args = parser.parse_args()
    print('PASS: optional standalone MCM override (1 file, no third-party assets), SHA-256:',
          write_archive(args.output))
