# Current Project State

- Current version: `0.5.180`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Runtime target: Fallout 4 `1.10.163`, matching F4SE, PrismaUI_F4 `2.1.1`
- Release presentation: text-only; icons and weapon preview are not included

The 0.5.180 release candidate builds as a Windows x64 F4SE plugin. Target-runtime
testing passed all four presentations, supported attachment changes including
Mount/Sight and Lower Rail/Laser provider-child paths, close/reopen, Alt-Tab,
save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
