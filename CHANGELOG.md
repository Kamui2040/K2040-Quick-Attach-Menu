## 0.5.201 (unreleased follow-up)

- Require a loaded crafting recipe creating the exact OMOD plus explicit
  weapon MNAM compatibility before exposing new generated workbench choices.
  Keep installed OMODs visible for recovery.
- Improve Builder controller focus retention, Settings D-pad traversal and
  page-control activation.
- Match controller shortcut selectors to the existing Settings dropdown style.
- No cheat mode and no attachment mutation/visual-refresh changes.
- Build and target-runtime QA required; not exact workbench parity yet.

## 0.5.200 (unreleased controller-only candidate)

- Controller navigation in Quick Menu, Builder and Settings via PrismaUI V12.
- Independent assignable controller shortcuts (single button or two-button
  combination) for opening Quick Menu and Builder; defaults unassigned.
- Preserves all 0.5.199 attachment/inventory checks and normal equipped-weapon
  refresh. No cheat mode.
- Build and in-game compatibility testing pending.

# Changelog

## 0.5.199

- Allow a validated provider attachment to be installed into an empty,
  live-reachable attachment point.
- Keep the operation fail-closed when it would activate an existing unreachable
  attachment, change unrelated installed OMOD identities, or fail to expose the
  provider's attachment points after installation.
- Preserve the existing inventory, live revalidation, post-change verification,
  rollback, and dependency-cycle safety checks. **Force unsafe swaps** remains
  limited to diagnosed dependency cycles.

## 0.5.198

- Keep Tactical Reload's `TRT_mod_EntryPoint1`, `TRT_mod_EntryPoint2`, and
  `TRT_mod_KeywordApply` infrastructure records out of generated player choices
  and dependency-removal planning without weakening normal provider ordering.
- Add a per-weapon **Force unsafe swaps** override, off by default, that logs
  and bypasses only a dependency-cycle ordering refusal. Turning it off restores
  the normal fail-closed behavior for that weapon.
- Log the unresolved OMOD, consumed attachment point, and provided attachment
  points when dependency ordering finds a cycle.

## 0.5.197

- Add a persistent option to disable diagnostic logging.
- Add configurable gameplay slowdown from normal time through full pause while
  the Quick Menu, Builder, or Settings page is open.
- Keep saved hidden categories and attachments filtered when the Cascade menu
  refreshes after a successful attachment change.
- Reject unvalidated CommonLibF4 revisions during configuration to preserve the
  original-game and AE single-DLL compatibility target.
- Verify that PrismaUI focus established engine cursor ownership and use a
  corrected OG/AE registration fallback when it did not. This prevents the
  cursor from being confined to the centered half-resolution region.
- Release plugin fallback cursor state during internal Builder/Settings view
  replacement so the newly focused page always acquires its own cursor owner.

## 0.5.181

Hotfix for generated-menu compatibility and installed-state detection.

- Generated-menu compatibility now reads the winning OMOD record's raw MNAM
  Target OMOD / Mod Association keywords instead of treating CommonLibF4
  FNAM/filter keywords as target compatibility. Candidates require a reachable
  attachment point and a matching MNAM target when one is authored; unresolved
  MNAM metadata fails closed for uninstalled candidates.
- Builder lists every compatible generated attachment regardless of inventory;
  Quick Menu lists installed attachments plus compatible loose mods currently
  carried by the player.
- Uninstalled generated OMODs without a linked loose-mod item are no longer
  exposed as workbench-style choices; this removes internal/helper Legendary
  records while preserving already-installed no-loose-mod state.
- Generated attachments that still need a fallback label now use a humanized
  EDID from the winning OMOD record instead of "Unnamed attachment".
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
