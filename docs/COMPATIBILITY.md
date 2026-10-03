# Compatibility

## Supported release

| Component | Version |
|---|---|
| Fallout 4 | `1.10.163`, `1.11.240` |
| F4SE | matching runtime release |
| PrismaUI_F4 | compatible with the bundled `2.1.1` API header |
| Plugin | `0.5.197` |
| Architecture | Windows x64 |

The release DLL is built with DCCStudios/CommonLibF4 revision
`12beba2a89fe117a14f1707b88c99ecb1b12f8c0` and commonlib-shared revision
`f0b1670ee9caac2e349497f6f3c08a69633a8ea7`. The build pins these revisions to
preserve one DLL for the supported original and AE runtime families.

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

Version 0.5.197 adds persistent diagnostic-logging control
and configurable menu slowdown from normal game time through full pause. Its
Cascade refresh also reapplies persisted visibility after an attachment change.
Its build now rejects unvalidated CommonLibF4 revisions after an incompatible
checkout produced an Address Library lookup failure on menu open.
The plugin verifies engine cursor ownership after PrismaUI focus. When the
provider does not add an owner, the plugin uses the validated original-game and
AE registration addresses directly, then clears stale constraints. This avoids
the centered half-resolution cursor clamp seen when the cursor count remains
zero. Builder/Settings replacement also releases the prior fallback state before
the new page focuses, allowing every page to reacquire an owner.
Focused Fallout 4 `1.11.240` runtime regression passed for unrestricted cursor
movement in all three views, logging control, the slowdown range through full
pause, and retained Cascade visibility after an attachment change. The exact
0.5.197 was not runtime-tested on Fallout 4 `1.10.163`; the source
and build retain the original-game path and its previously validated address,
but that runtime family is unverified for this release.

## PrismaUI SDK provenance

The repository includes only the unchanged PrismaUI API header and its upstream
license. The exact public source revision and SHA-256 are recorded in
`external/prismaui_f4/README.md`. No PrismaUI binaries, Ultralight files, game
files, or dependency archives are distributed in the source repository.
