# Current Project State

- Current public release: `0.5.192`
- Previous public release: `0.5.181`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Runtime validation: Fallout 4 `1.10.163` and `1.11.240` with matching F4SE and compatible PrismaUI_F4
- Release presentation: text-only; icons and weapon preview are not included
- Icon research: branch `research/icon-semantic-library` contains a presentation-only semantic vocabulary seeded from vanilla/DLC terminology and major Nexus weapon ecosystems; current seed is 15 top-level categories, 810 attachment aliases, 202 weapon names, and 43 provenance sources; it is not wired into runtime behavior.

Version 0.5.192 is the current public release. It keeps the 0.5.181
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
