# Changelog

## 0.5.181

Hotfix candidate for generated-menu compatibility and installed-state detection.

- Generated-menu compatibility now follows workbench semantics: reachable
  attachment point plus Target OMOD / instantiation-filter keyword matching
  when the OMOD declares target keywords. Source plugin is not used as a
  compatibility boundary.
- Builder lists every compatible generated attachment regardless of inventory;
  Quick Menu lists installed attachments plus compatible loose mods currently
  carried by the player.
- Disabled object-instance OMOD entries no longer count as installed.
- A hidden Quick Menu view is prepared after load/new-game transitions so the
  mod is discoverable in Prisma Dock without bypassing normal menu-open safety.

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
