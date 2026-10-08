# Current Project State

- Current public release: `0.5.198`
- Previous public release: `0.5.197`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Current-release runtime validation: focused external confirmation that the
  reported Tactical Reload false safety block is resolved; the broader
  attachment regression matrix was not independently repeated for 0.5.198
- Release presentation: text-only; icons and weapon preview are not included

Version `0.5.199` is an unreleased compatibility candidate. It allows a
provider attachment to be installed into an empty, live-reachable attachment
point only when the hypothetical graph does not activate an existing unreachable
attachment. The transaction requires the unrelated installed OMOD identities to
remain unchanged, verifies the provider's attachment points after installation,
and retains the existing inventory, rollback, and live revalidation checks.
The clean Linux releasedbg build and static validation passed. Focused
target-runtime validation remains pending.

Version `0.5.198` keeps the three
known `TacticalReload_IngameSwitch.esp` infrastructure OMODs internal and out of
dependency-removal planning, and adds a stable WEAP-keyed **Force unsafe swaps**
override for dependency-cycle refusals only. The override defaults off and is
not included in distributable weapon menu profiles. The clean Linux releasedbg
build and static validation passed. Focused external testing confirmed that the
reported Tactical Reload false **Attachment unable to swap safely** case is
resolved; the broader attachment regression matrix was not independently
repeated for this release.

## Release history

Version `0.5.197`, the previous public release, adds a persistent
diagnostic-logging toggle and a shared
0%-100% gameplay slowdown setting for the Quick Menu, Builder, and Settings
pages. Zero keeps normal time, intermediate values slow proportionally, and 100
fully pauses. The implementation preserves external time changes instead of
blindly restoring a stale multiplier. It also reapplies saved visibility after
a successful Cascade attachment change so hidden categories and attachments do
not reappear in the refreshed menu. The cross-runtime build dependency is now
pinned so an incompatible CommonLibF4 revision fails during configuration
instead of producing an invalid DLL. It also verifies cursor ownership after
PrismaUI focus and uses a validated OG/AE registration fallback when the
provider did not establish an engine cursor owner. Internal Builder/Settings
replacement releases the old fallback state before focusing the new page so
each view reacquires cursor ownership.

Focused runtime testing on Fallout 4 `1.11.240` passed for unrestricted cursor
movement in Quick Menu, Builder, and Settings; the logging toggle; the complete
slowdown range from normal time through full pause; and retained Cascade
visibility after attachment changes. The release retains its original-game
code and build path, but version 0.5.197 was not runtime-tested on Fallout
4 `1.10.163`.

Version 0.5.192 was the previous public release. It keeps the 0.5.181
generated-menu compatibility model and adds validated MODCOL container handling,
generated no-loose attachment support behind the existing safety setting,
game-thread hotkey/menu actions, cross-runtime OMOD container reads, dialogue
open blocking, and corrected Mouse 4/5 toggle and switching state under
Proton/PrismaUI.

Version 0.5.181 loads and opens both Prisma menus on
Fallout 4 1.10.163, and Prisma Dock registration is confirmed in runtime QA.
Runtime testing also proved that both prior generated-menu heuristics were
wrong: source/plugin-family filtering hid valid choices, while using CommonLibF4
`filterKeywords` as the target gate admitted unrelated weapon attachments.
The latter field is OMOD FNAM, not the authored MNAM Target OMOD association.

Version 0.5.181 parses raw MNAM from each winning OMOD plugin record and
uses it with the reachable attachment graph. MNAM-targeted OMODs must match a
keyword on the equipped base WEAP; OMODs with no MNAM are generic for their
reachable attachment point; unresolved MNAM metadata fails closed for
uninstalled candidates. Runtime QA then exposed a second presentation issue:
no-loose-mod Legendary/helper OMODs were being listed as generated choices.
Version 0.5.192 keeps generic no-MNAM no-loose records out of generated
discovery, but allows legitimate uninstalled no-loose choices when
`AllowNoLooseModOptions` is enabled and the OMOD has an explicit MNAM target
matching the equipped weapon. Already-installed no-loose OMODs remain visible as
live state.

Builder shows every compatible player-facing attachment regardless of inventory.
Quick Menu shows installed attachments, compatible loose mods currently carried
by the player, and validated no-loose choices that pass the explicit-target
safety rule. Focused regression passed for generated compatibility, Prisma Dock
discovery, MODCOL weapons, legitimate no-MISC/no-loose attachment changes,
same-hotkey open/close, Quick Menu ↔ Builder switching, dialogue blocking, and
the prior hotkey-thread crash path. Version 0.5.192 has been exercised on
both 1.10.163 and 1.11.240 runtime families.

Version 0.5.180 passed target-runtime testing for all four presentations,
supported attachment changes including Mount/Sight and Lower Rail/Laser
provider-child paths, close/reopen, Alt-Tab, save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
