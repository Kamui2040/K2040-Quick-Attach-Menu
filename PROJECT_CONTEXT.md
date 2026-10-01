# Current Project State

- Current public release: `0.5.181`
- Previous public release: `0.5.180`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Runtime target: Fallout 4 `1.10.163`, matching F4SE, PrismaUI_F4 `2.1.1`
- Release presentation: text-only; icons and weapon preview are not included

Version 0.5.181 is a minimal hotfix based directly on the runtime-verified
0.5.180 source. It changes generated-menu compatibility filtering, disabled
object-instance OMOD handling, synchronized version metadata, hotfix
documentation, and Windows SDK library-name casing needed by the Linux build
host.

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
Uninstalled generated OMODs without a linked loose-mod item are now excluded,
while already-installed no-loose-mod OMODs remain visible as live state.

Builder shows every compatible player-facing attachment regardless of inventory;
Quick Menu shows installed attachments plus compatible loose mods currently
carried by the player. Focused target-runtime regression passed for generated
compatibility, Prisma Dock discovery, internal Legendary/helper filtering, and
the previously failing sight replacement.

Version 0.5.180 passed target-runtime testing for all four presentations,
supported attachment changes including Mount/Sight and Lower Rail/Laser
provider-child paths, close/reopen, Alt-Tab, save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
