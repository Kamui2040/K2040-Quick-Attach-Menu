# Known Issues and Limits

## 0.5.180 generated-menu compatibility

Version 0.5.180 can admit unrelated OMODs when generic attachment points overlap
between weapon families. Earlier 0.5.181 candidates incorrectly tried to infer
the weapon-family gate from CommonLibF4 `filterKeywords`, which exposes OMOD
`FNAM` rather than the authored Target OMOD Keywords in `MNAM`.

The current hotfix reads raw MNAM from the winning OMOD plugin record. A
generated OMOD requires a reachable attachment point and, when MNAM targets are
present, a matching keyword on the equipped base WEAP. OMODs with no MNAM
targets remain generic for their attachment point. Unresolved MNAM metadata
fails closed for uninstalled candidates.

Generated discovery still rejects generic no-MNAM uninstalled OMODs that have no
linked loose-mod item. Runtime QA showed that otherwise internal DLC/helper and
Legendary-effect records can leak into player-facing choices. When
`AllowNoLooseModOptions` is enabled, an uninstalled no-loose OMOD with an
explicit resolved MNAM target matching the equipped weapon is allowed. Already-
installed no-loose OMODs remain visible so live weapon state is not lost.

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

## Configurable gameplay slowdown requires runtime validation

The 0.5.193 candidate keeps the gameplay input layer active while the quick
menu, Builder, or Settings page has focus and adds a 0%-100% slowdown range.
Zero leaves the captured game-time multiplier unchanged; 100 fully pauses;
intermediate values scale it proportionally. Static validation and a clean
build do not replace focused in-game checks for normal time, partial slowdown,
full pause, view switching, close restoration, and external multiplier safety.

## Runtime validation

Version 0.5.192 has focused runtime coverage on Fallout 4
`1.10.163` and `1.11.240` with matching F4SE and compatible PrismaUI_F4. Other
runtime/framework combinations have not passed the same regression and remain
unverified.
