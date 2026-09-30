# Changelog

## 0.5.181

Hotfix candidate for generated-menu compatibility and installed-state detection.

- Uninstalled generated-menu OMODs now require an equipped-weapon
  instantiation-filter keyword match in addition to source-family and
  attachment-point checks.
- Disabled object-instance OMOD entries no longer count as installed.

## 0.5.180

First public release of **K2040's Quick Attach Menu**.

- Adds inventory-aware attachment changes from four quick-menu layouts.
- Supports ECO-authored menus and runtime-generated weapon menus.
- Adds per-weapon visibility, ordering, labels, source overrides, bracket
  cleanup overrides, and menu profile import/export.
- Adds a dedicated Settings page, optional MCM/Hotkey Manager integration,
  shared keybindings, themes, opacity, scale, position, and reset controls.
- Adds dependency-aware provider replacement, verified loose-mod returns,
  post-change verification, and rollback for supported transactions.
- Removes the experimental third-party icon resolver and abandoned weapon
  preview. This release is text-only.

Target-runtime testing passed all four presentations, attachment changes,
provider/child changes, menu close/reopen, Alt-Tab, save/load, and normal exit
on Fallout 4 `1.10.163` with matching F4SE and PrismaUI_F4 `2.1.1`.
