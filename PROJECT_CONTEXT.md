# Current Project State

- Current hotfix candidate: `0.5.181`
- Last public release: `0.5.180`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Runtime target: Fallout 4 `1.10.163`, matching F4SE, PrismaUI_F4 `2.1.1`
- Release presentation: text-only; icons and weapon preview are not included

Version 0.5.181 is a minimal hotfix based directly on the runtime-verified
0.5.180 source. It changes only generated-menu compatibility filtering,
disabled object-instance OMOD handling, synchronized version metadata, and
hotfix documentation. Build and focused target-runtime validation are pending.

Version 0.5.180 passed target-runtime testing for all four presentations,
supported attachment changes including Mount/Sight and Lower Rail/Laser
provider-child paths, close/reopen, Alt-Tab, save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
