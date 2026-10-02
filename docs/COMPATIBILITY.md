# Compatibility

## Supported release

| Component | Version |
|---|---|
| Fallout 4 | `1.10.163`, `1.11.240` |
| F4SE | matching runtime release |
| PrismaUI_F4 | compatible with the bundled `2.1.1` API header |
| Plugin | `0.5.192` |
| Architecture | Windows x64 |

Mod Configuration Menu is optional. It supplies registered hotkey controls and
Hotkey Manager visibility; missing MCM bindings fall back independently to the
plugin INI.

ECO is optional. When a compatible authored ECO menu is available, Auto mode
can use it. Otherwise the plugin builds a bounded generated menu from the
equipped weapon, installed OMODs, available loose mods, reachable attachment
points, and the winning OMOD records' MNAM Target OMOD keywords.

## Not validated by this release

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

Version 0.5.192 adds focused regression on both validated runtime
families for MODCOL container handling, legitimate no-MISC/no-loose options,
game-thread hotkey/menu actions, Mouse 4/5 same-key close, Quick Menu ↔ Builder
switching, DialogueMenu blocking, cross-runtime OMOD container reads, and the
previous hotkey-thread crash path. A runtime, F4SE, PrismaUI API/header, or
CommonLibF4 change still requires separate compatibility review.

## PrismaUI SDK provenance

The repository includes only the unchanged PrismaUI API header and its upstream
license. The exact public source revision and SHA-256 are recorded in
`external/prismaui_f4/README.md`. No PrismaUI binaries, Ultralight files, game
files, or dependency archives are distributed in the source repository.
