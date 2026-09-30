# Current Project State

- Current hotfix candidate: `0.5.181`
- Last public release: `0.5.180`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Runtime target: Fallout 4 `1.10.163`, matching F4SE, PrismaUI_F4 `2.1.1`
- Release presentation: text-only; icons and weapon preview are not included

Version 0.5.181 is a minimal hotfix based directly on the runtime-verified
0.5.180 source. It changes generated-menu compatibility filtering, disabled
object-instance OMOD handling, synchronized version metadata, hotfix
documentation, and Windows SDK library-name casing needed by the Linux build
host.

The old-gen-compatible candidate now loads and opens both Prisma menus on
Fallout 4 1.10.163, and Prisma Dock registration is confirmed in runtime QA.
The earlier generated-menu filters were too strict and reduced both menus to
already-installed attachments. The candidate now follows workbench
compatibility instead: reachable attachment point plus Target OMOD /
instantiation-filter keyword matching when target keywords are declared, with
no source-plugin restriction. Builder is intended to show every compatible
attachment regardless of inventory; Quick Menu is intended to show installed
attachments plus compatible loose mods currently carried by the player. This
catalog correction requires a fresh build and focused target-runtime regression
before merge.

Version 0.5.180 passed target-runtime testing for all four presentations,
supported attachment changes including Mount/Sight and Lower Rail/Laser
provider-child paths, close/reopen, Alt-Tab, save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
