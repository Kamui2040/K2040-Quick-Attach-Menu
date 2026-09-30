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
Fallout 4 1.10.163. The first generated-menu filter was too strict in runtime
testing and reduced both menus to already-installed attachments. The candidate
now accepts positive instantiation-filter evidence from the equipped weapon,
live instance, or its proven default/active OMOD family while retaining source
family and attachment-point checks. This adjustment requires a fresh build and
focused target-runtime regression before merge. The same candidate now also
prepares a hidden Quick Menu view after load/new-game transitions so Prisma
Dock can discover the mod through the framework's live-view enumeration;
runtime verification of that Dock entry is pending.

Version 0.5.180 passed target-runtime testing for all four presentations,
supported attachment changes including Mount/Sight and Lower Rail/Laser
provider-child paths, close/reopen, Alt-Tab, save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
