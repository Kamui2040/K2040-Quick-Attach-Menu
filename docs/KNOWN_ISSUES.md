# Known Issues and Limits

## 0.5.180 generated-menu compatibility bug

The 0.5.180 runtime-generated fallback can expose OMODs from another weapon when
the records share a source plugin and generic attachment points. The 0.5.181
hotfix candidate adds an equipped-weapon target/filter-keyword requirement.

## 0.5.180 false ambiguous replacement state

The 0.5.180 live object-instance probe can count disabled OMOD entries as
installed. This can make ordinary attachment replacements report that the
current state is not safe to replace automatically. The 0.5.181 hotfix candidate
ignores disabled entries for installed identity.

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
