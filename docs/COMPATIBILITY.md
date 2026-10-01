# Compatibility

## Supported release target

| Component | Version |
|---|---|
| Fallout 4 | `1.10.163` |
| F4SE | matching `1.10.163` release |
| PrismaUI_F4 | `2.1.1` |
| Plugin | `0.5.181` |
| Architecture | Windows x64 |

Mod Configuration Menu is optional. It supplies registered hotkey controls and
Hotkey Manager visibility; missing MCM bindings fall back independently to the
plugin INI.

ECO is optional. When a compatible authored ECO menu is available, Auto mode
can use it. Otherwise the plugin builds a bounded generated menu from the
equipped weapon, installed OMODs, available loose mods, reachable attachment
points, and the winning OMOD records' MNAM Target OMOD keywords.

## Not supported by this release

- Fallout 4 `1.10.980` through `1.10.984`
- Fallout 4 VR
- mismatched F4SE releases
- unreviewed PrismaUI API versions
- xEdit or exported JSON as a runtime dependency

## Runtime validation

Version 0.5.180 passed target-runtime checks for:

- all four menu presentations;
- authored and generated menu paths;
- leaf installation and one-for-one replacement;
- Mount/Sight and Lower Rail/Laser provider-child changes;
- correct loose-mod inventory returns without duplicate `X+` workbench entries;
- Builder and Settings persistence;
- weapon menu profile export/import and exact-weapon filtering;
- menu close/reopen and opener-key switching;
- Alt-Tab, save/load, and normal exit.

This evidence applies to the exact target versions above. Version 0.5.181 keeps
the 0.5.180 runtime/dependency baseline and passed focused regression for
generated compatibility, disabled installed-state entries, Prisma Dock discovery,
internal no-loose-mod filtering, and the previously failing sight replacement.
A runtime, F4SE, PrismaUI API/header, or CommonLibF4 change requires separate
compatibility review.

## PrismaUI SDK provenance

The repository includes only the unchanged PrismaUI API header and its upstream
license. The exact public source revision and SHA-256 are recorded in
`external/prismaui_f4/README.md`. No PrismaUI binaries, Ultralight files, game
files, or dependency archives are distributed in the source repository.
