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

## Unreleased 0.5.205 ordinary radial label overlap

A screenshot from 0.5.203 showed overlapping receiver labels in
keyboard/mouse radial mode. The 0.5.204 pagination only covered
controller radial, not the ordinary mouse interface. The 0.5.205
candidate extends the six-entry page cap to mouse radial with
center paging arrows, wheel/Page Up/Page Down controls, short
wedge labels and a full-name hover caption. All options remain
reachable without changing their install behavior. Target
in-game visual QA is pending; the receiver crash remains unresolved.

## Unreleased 0.5.204 receiver crash and dense radial

Version 0.5.203 logged a successful and verified vanilla 10mm receiver
transition from Advanced Receiver to Rapid Automatic Receiver before the
game crashed after submitting the updated Prisma payload. Without a crash
call stack, the exact failing instruction remains unknown. Version
0.5.204 adds flushed markers before and after the existing payload
submission and equipped-item refresh, without changing either operation.
It is a diagnostic candidate, not a crash fix.

With many receivers, controller radial option labels overlapped. The
option ring now shows at most six choices per page, preserves all choices
across pages via LB/RB, shortens wedge labels, and displays the full
selected label in the wheel center. The mouse layout remains unchanged.
In-game presentation and control tests are still required.

## Unreleased 0.5.203 Quick Menu controller refinement

Previous controller navigation in Builder and Settings was not intuitive
enough for those complex panels. They now remain keyboard/mouse-only until
their layouts are deliberately redesigned. The retired controller Builder
opener is ignored without altering keyboard/MCM shortcuts. Quick Menu uses
the Prisma V12 controller button bridge, plus sampled XInput left/right
stick angle for radial selection. A enters a category and then confirms an
attachment; B backs out and then closes. The controller radial outer
options occupy a full ring, independent of category direction, and D-right
no longer enters the outer ring.

Static direction/activation tests and the cross-build do not establish
behavior on a real Steam Input/Proton controller. Specifically validate
dead-zone stability, A/B stage transitions, stick priority, button mapping,
mouse coexistence, and ordinary Cascade navigation before merging.

## Unreleased 0.5.202 default material safety

A 0.5.201 game session on a 10mm confirmed no previously installed material
OMOD, yet selecting the vanilla No Material OMOD 0024A0D9 attached it to the
object-instance stack and the game crashed after the transaction verified and
the UI payload was sent. The engine refresh remains a probable but unproven
crash site; no crash stack is available. A unique unloaded, zero-effect
material default is now presentation-only "already applied" when the installed
material point is empty, and the selection is rejected before mutation.
Attempts to replace a different installed material with a no-effect reset
OMOD remain disabled pending verification of a safe workbench-equivalent
removal path. This does not prove the overall crash is fixed.

## Unreleased 0.5.201 follow-up

Static and build validation are required for the controller focus/navigation
fixes and recipe-backed generated-OMOD filter. The filter preserves installed
OMODs but requires a COBJ recipe, explicit matching MNAM, and reachable AP for
new generated choices. This is deliberately stricter than previous behavior;
mods that offer loose-only choices without a COBJ may need a separate
workbench-compatibility path. Bench-specific recipe and perk/condition
visibility are not yet modeled precisely. Controller interaction, attachment
catalog accuracy and crash regression all require focused game testing.
The previous equipped-weapon presentation refresh is not modified by this
candidate; no cheat-mode code has been reintroduced.

## Unreleased controller-only 0.5.200 candidate

The controller-only candidate preserves 0.5.199 attachment mutation, inventory,
and equipped-model refresh behavior. Unlike the discarded cheat-mode candidate,
it does not permit inventory-free swaps. Controller navigation and independent
single-/two-button XInput opener shortcuts are not yet in-game validated.
Steam Input mappings and gamepad focus behavior require target-environment QA.

## Unsupported attachment operations fail closed

Ambiguous inventory stacks, unresolved forms, dependency cycles, stale menu
state, and any operation that cannot be completely verified remain unavailable.
Supported leaf, provider-install, and provider-replacement paths continue to use
live operation-level checks, exact inventory verification, post-change
verification, and rollback.

The unreleased 0.5.199 candidate allows a provider attachment to be installed
into an empty, live-reachable attachment point. It remains blocked if the
hypothetical graph would activate an existing unreachable attachment or if the
transaction changes unrelated installed OMOD identities.

Version 0.5.198 recognizes the three known
`TacticalReload_IngameSwitch.esp` infrastructure OMODs without relaxing normal
dependency ordering. It also adds an off-by-default per-weapon override for
dependency-cycle refusals. Focused external testing confirmed that the reported
Tactical Reload false safety block is resolved; the broader attachment
regression matrix was not independently repeated for this release.

## This release is text-only

Inventory and workbench icon tags are not a reliable semantic source for every
weapon attachment category. Version 0.5.180 therefore removes the experimental
icon resolver rather than showing misleading generic icons. Weapon preview was
also removed after its renderer lifecycle failed Alt-Tab safety testing.

Future icon work requires a mod-owned semantic attachment database, explicit
adapters for compatible installed icon libraries, and optional per-weapon
Builder overrides. Neither future feature affects the current menu's core
attachment behavior.

## 0.5.197 original-game runtime is unverified

Version 0.5.197 keeps the gameplay input layer active while the quick
menu, Builder, or Settings page has focus and adds a 0%-100% slowdown range.
Zero leaves the captured game-time multiplier unchanged; 100 fully pauses;
intermediate values scale it proportionally. Focused Fallout 4 `1.11.240`
runtime testing passed for the new features and the unrestricted cursor fix in
all three views. This release was not runtime-tested on Fallout 4
`1.10.163`; its original-game compatibility path is retained but unverified.

## Runtime validation

Version 0.5.192 has focused runtime coverage on Fallout 4
`1.10.163` and `1.11.240` with matching F4SE and compatible PrismaUI_F4. Other
runtime/framework combinations have not passed the same regression and remain
unverified.

Version 0.5.197 has focused runtime coverage on Fallout 4 `1.11.240` for the
new settings, Cascade refresh, cursor ownership, and all three views.
