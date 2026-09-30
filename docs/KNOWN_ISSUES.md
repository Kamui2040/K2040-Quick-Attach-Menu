# Known Issues and Limits

## 0.5.180 generated-menu compatibility

Version 0.5.180 can admit unrelated OMODs when source-family and generic
attachment-point checks overlap. The 0.5.181 hotfix candidate additionally
requires positive instantiation-filter evidence from the equipped weapon, its
live instance, or the default/active OMOD family already proven for that
weapon. This keeps compatible alternatives visible without returning to
attachment-point-only matching.

## 0.5.180 false ambiguous replacement state

Version 0.5.180 can count disabled object-instance OMOD entries as installed,
which can make a valid replacement fail as ambiguous. The 0.5.181 hotfix
candidate ignores disabled entries for installed identity.

## Unsupported attachment operations fail closed

Provider installation into an empty point, ambiguous inventory stacks,
unresolved forms, dependency cycles, stale menu state, and any operation that
cannot be completely verified remain unavailable. Supported leaf and provider
replacement paths continue to use live operation-level checks, exact inventory
verification, post-change verification, and rollback.

## This release is text-only

Inventory and workbench icon tags are not a reliable semantic source for every
weapon attachment category. Version 0.5.180 therefore removes the experimental
icon resolver rather than showing misleading generic icons. Weapon preview was
also removed after its renderer lifecycle failed Alt-Tab safety testing.

Future icon work requires a mod-owned semantic attachment database, explicit
adapters for compatible installed icon libraries, and optional per-weapon
Builder overrides. Neither future feature affects the current menu's core
attachment behavior.

## Gameplay is frozen while a mod menu is open

The current input-safety path freezes game time and owns a gameplay input layer
while the quick menu, Builder, or Settings page has focus. Pause, slow-motion,
and normal-speed choices are planned but are not exposed until each mode has
been implemented and runtime-validated.

## Runtime support is intentionally narrow

Version 0.5.180 targets Fallout 4 `1.10.163`, matching F4SE, and PrismaUI_F4
`2.1.1`. Other runtimes and framework versions have not passed the same release
regression and are not supported.
