# Compatibility

## Supported release

| Component | Version |
|---|---|
| Fallout 4 | `1.10.163`, `1.11.240` |
| F4SE | matching runtime release |
| PrismaUI_F4 | compatible with the bundled `2.1.1` API header |
| Plugin | `0.5.198` |
| Architecture | Windows x64 |

Unreleased 0.5.211 unifies directional/controller navigation across
all four Quick Menu presentations and corrects the previously inverted
Compact Hybrid category wheel. Left/Right consistently change between
category and attachment focus, while Cancel backs out before closing.
LB/RB switch categories or page radial attachments. Directional movement
cannot install attachments. Automated browser/renderer checks pass;
initial user testing on the target setup reports this revision working.
The result is provisional; not every layout, custom input mapping,
repeat pattern or Steam Input/Proton configuration has been verified.

Unreleased 0.5.210 adds native thumbstick navigation in every Quick
Menu presentation and explicit game-mapped confirmation. Steam Input
and XInput must expose a gamepad to Fallout 4. The gamepad mapping is
read from the live ControlMap on the game thread, falling back to
menu Accept and default A where needed. Focused in-game tester feedback confirms the 0.5.210 controller behavior
works in the tested configuration. Remapped buttons, all layouts, repeat
behavior and the broader Steam Input/Proton matrix are not yet fully
validated. Builder and Settings remain keyboard/mouse only.

Unreleased 0.5.209 revises the AE-only experimental re-equip to
coalesce multiple successful OMOD transactions and re-equip the
last verified equipped stack only after the Quick Menu closes.
Focused user testing on AE 1.11.240 confirms the previously reported
repeated-change visual refresh issue is resolved when closing the
Quick Menu. This is not full ammunition, animation or multi-weapon
regression coverage. The flag remains opt-in, while OG 1.10.163
retains its immediate legacy refresh.

Unreleased 0.5.207 offers an experimental automatic re-equip
fallback solely for Fallout 4 AE 1.11.240. Opt in on a disposable
test save with AEAutoReequipAfterApply=true under Behavior in the
plugin INI. OG 1.10.163 keeps the original native refresh.
This workaround has not passed runtime QA and may affect magazines
or weapon animations.

The unreleased 0.5.206 test build skips invalid post-modification
equipped-weapon refresh on AE 1.11.240. Its old Address Library ID
1153963 is absent and maps to non-executable data. Re-equipping
may be necessary for the new visual weapon configuration.
A tested AE-safe immediate refresh is still unavailable.
The change needs in-game validation and is not released.

The read-only scripts/check-relocations.py validation helper accepts an
installed Fallout 4 Windows x64 executable, an AE Address Library using
the verified sorted (ID, RVA) binary format, and one or more exact IDs:

~~~bash
python3 scripts/check-relocations.py /path/to/Fallout4.exe \
  /path/to/version-1-11-240-0.bin 2229234
~~~

It rejects missing IDs, non-executable targets, malformed files, and
unsupported formats instead of following adjacent records. A successful
mapping only proves the address exists in executable memory; it does
not prove the function signature, its update behavior, or whether it
is safe to call after modifying a weapon. No new automatic AE weapon
refresh is enabled by this validation.

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

Version 0.5.198 received focused external confirmation that the reported
Tactical Reload false safety block is resolved. The broader attachment
regression matrix was not independently repeated for that release.

The unreleased 0.5.199 candidate adds guarded provider installation into an
empty, live-reachable attachment point. Target-runtime validation is required
before this path can be considered successful.

The unreleased 0.5.201 follow-up checks for a loaded COBJ recipe creating each
new generated OMOD, in addition to explicit MNAM and reachable attachment
point checks. It retains installed forms for recovery. This safety-first filter
may suppress unusual loose-mod-only entries with no crafting recipe and does
not yet replicate all of the workbench's per-recipe visibility conditions.
It also revises controller focus traversal and its Settings binding editor.
Target-runtime QA remains mandatory before release.

## PrismaUI SDK provenance

The repository includes only the unchanged PrismaUI API header and its upstream
license. The exact public source revision and SHA-256 are recorded in
`external/prismaui_f4/README.md`. No PrismaUI binaries, Ultralight files, game
files, or dependency archives are distributed in the source repository.
