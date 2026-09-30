# Current Project State

- Current hotfix candidate: `0.5.181`
- Last public release: `0.5.180`
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Runtime target: Fallout 4 `1.10.163`, matching F4SE, PrismaUI_F4 `2.1.1`
- Release presentation: text-only; icons and weapon preview are not included

The 0.5.181 hotfix corrects two issues found after the first public release:
runtime-generated menus could admit unrelated OMODs when generic attachment
points and source plugins overlapped, and disabled object-instance OMOD entries
could be counted as installed and cause valid replacements to fail as ambiguous.

The fix requires positive equipped-weapon target/filter-keyword evidence for
uninstalled generated-menu candidates and excludes disabled object-instance
entries from live installed identity.

Source review and a clean Linux Windows-x64 build have passed on the hotfix
branch. The first focused Fallout 4 runtime regression failed because neither
the quick menu nor Builder opened after deployment. Runtime startup/load
diagnosis is now required; this hotfix is not runtime-verified and must not be
merged or released yet.
