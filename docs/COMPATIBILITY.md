# Compatibility

## Supported release target

| Component | Version |
|---|---|
| Fallout 4 | `1.10.163` |
| F4SE | matching `1.10.163` release |
| PrismaUI_F4 | `2.1.1` |
| Plugin | `0.5.181` hotfix candidate |
| Architecture | Windows x64 |

Mod Configuration Menu is optional. It supplies registered hotkey controls and
Hotkey Manager visibility; missing MCM bindings fall back independently to the
plugin INI.

ECO is optional. When a compatible authored ECO menu is available, Auto mode
can use it. Otherwise the plugin builds a bounded generated menu from the
equipped weapon, installed OMODs, available loose mods, source-family evidence,
and reachable attachment points.

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

This evidence applies to the exact target versions above. A runtime, F4SE,
PrismaUI API/header, or CommonLibF4 change requires source review, a clean build,
and focused target-runtime regression testing.

## PrismaUI SDK provenance

The repository includes only the unchanged PrismaUI API header and its upstream
license. The exact public source revision and SHA-256 are recorded in
`external/prismaui_f4/README.md`. No PrismaUI binaries, Ultralight files, game
files, or dependency archives are distributed in the source repository.


## 0.5.181 hotfix validation

The 0.5.181 source hotfix tightens the runtime-generated menu to require an
equipped-weapon target/filter-keyword match for uninstalled OMOD candidates and
ignores disabled object-instance OMOD entries when determining installed state.

A clean Linux Windows-x64 build has passed for 0.5.181, but focused startup
testing proved the current libxse CommonLibF4 build baseline is not compatible
with the supported Fallout 4 1.10.163 / F4SE 0.6.23 target. Guarding its newer
F4SE API call exposed a second incompatibility: current-generation Address
Library IDs are absent from the 1.10.163 database. The project therefore needs
a CommonLibF4 baseline that explicitly supports old-gen 1.10.163 before the
runtime regression can be repeated.
