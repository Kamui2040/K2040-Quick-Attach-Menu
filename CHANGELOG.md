## 0.5.213 (unreleased orientation-aware Builder/Settings navigation tester)

- Follow the visible direction of controls rather than treating every
  collection as a vertical list. In the Builder header toolbar and
  weapon-settings row, Left/Right move between controls and Up/Down
  move between horizontal rows and the category list.
- Preserve Up/Down for vertical category, attachment and dropdown lists,
  and Left/Right for switching between category and attachment panes.
- In Settings, Left/Right traverse each row of the two-column theme and
  presentation grids; Up/Down move between grid rows. The header Back/Close
  actions and Reset confirmation buttons also follow their horizontal order.
- LT has no default menu-navigation action. It remains a selectable
  Quick Menu opener button or modifier (such as LT + another button).
- Keep mapped confirmation, safe dropdown cancel, settings edit mode,
  Quick Menu navigation and AE post-close refresh unchanged.
- Automated interaction and source checks pass; focused in-game controller
  verification of these directional changes remains pending.

## 0.5.212 (unreleased Builder/Settings controller navigation tester)

- Extend the working controller bindings, including the user's mapped
  Activate/Confirm and Cancel buttons, to the Builder and Settings views.
- Use one shared page-focus controller helper for both. Sticks and the
  D-pad move focus; LB/RB change Builder groups or Settings tabs; Confirm
  activates the focused control, and Back returns to the preceding level.
- Dropdown lists have a separate navigation state: movement only highlights
  an option; Confirm selects, and Back closes without changing the value.
  Profile import and Reset Everything dialogs likewise require explicit
  confirmation, with Back cancelling safely.
- Builder navigation selects categories without changing their visibility.
  A separate unassigned X/Y face-button action toggles category/entry
  visibility deliberately. A on a selected entry uses the existing
  explicit show/hide action.
- Settings sliders require an explicit Confirm press to enter editing;
  then Left/Right adjusts them, and Back exits edit mode. Existing
  keyboard and mouse interaction remains available.
- Text editing, colour hex input, and keyboard hotkey capture still need
  a keyboard. Other Builder operations such as drag reordering have not
  been converted to controller-only workflows.
- Automated interaction and source tests pass. In-game Proton/Prisma
  runtime behavior is not yet verified; no release is implied.

## 0.5.211 (unreleased unified Quick Menu navigation tester)

- Use one controller navigation model across Cascade, Radial, Compact
  Hybrid and Horizontal. Up/Down move through visible categories or
  attachment choices; Right enters the attachments pane and Left returns.
- In Compact Hybrid, invert the category-index step only while traversing
  its bottom-to-top semicircle, so D-pad and joystick Up actually move
  upward on screen, and Down actually moves downward.
- Make Cancel back out of attachments in every layout before closing the
  menu. LB/RB change categories without forcing pane focus; Radial
  attachment mode keeps LB/RB paging.
- Share the same directional handling between keyboard, D-pad and sticks.
  D-pad takes precedence until the held stick returns to neutral.
- Movement is selection-only: installing still requires a distinct mapped
  Activate/Confirm press. Preserve the tested AE deferred re-equip behavior.
- Browser and renderer-level navigation tests cover every presentation.
  Initial in-game controller testing reports the 0.5.211 changes working.
  This is a provisional, focused result, not a complete layout/remapping
  or controller compatibility regression.

## 0.5.210 (unreleased controller navigation tester)

- Support either thumbstick for highlight-only navigation in all Quick Menu
  presentations, alongside the existing D-pad. Radial retains angle-based
  highlighting; cascade, hybrid and horizontal use directional steps.
- Keep analog navigation and D-pad actions distinct from attachment
  installation. A separate, non-repeating confirmation press is required.
- Read Fallout 4's current mapped gamepad Activate button for confirmation,
  falling back to menu Accept then A when unavailable. Use the menu Cancel
  mapping when available; avoid duplicate confirm/navigation bindings.
- Send stick-neutral state to clear stale angular selection, suppress
  repeated input drift, and retain game-thread-only Prisma callbacks.
- Preserve the focused AE 0.5.209 post-close auto re-equip workaround.
- Focused in-game tester feedback confirms the 0.5.210 controller update
  works in the tested setup. Remapped-button combinations, all layouts,
  and the full controller/weapon regression matrix remain unverified.

## 0.5.209 (unreleased AE multi-change visual refresh experiment)

- Coalesce multiple successfully verified attachment changes on AE
  1.11.240 into a single exact-stack re-equip when the Quick Menu closes.
  Stop cycling the weapon for each individual attachment while the menu
  and its presentation refresh may still be active.
- Run the final refresh through the existing F4SE game-thread post-close
  path, after Prisma has released focus, then queue first-person
  presentation recovery. Skip stale tasks after a menu reopening or
  game transition; re-check weapon, stack and installed OMOD identities.
- Keep the opt-in AE-only setting and safe loaded-ammo restoration.
  OG 1.10.163 uses the original native refresh with no changes.
- Focused user testing on AE 1.11.240 confirmed that several
  modifications made without exiting Quick Menu now become visually
  current on close. Ammunition, animations and other weapon coverage
  still require separate verification.

## 0.5.208 (unreleased AE re-equip follow-up and internal-slot filtering)

- Fix a confirmed 0.5.207 failure mode: ActorEquipManager::UnequipObject
  returned false in an AE test while the weapon became unequipped, and
  the old fallback stopped without requesting the matching re-equip.
- Attempt the exact-stack re-equip even when the unequip return value is
  false, then verify the actual equipped state instead of trusting either
  boolean as an equipment-state guarantee.
- Keep vanilla universal range-offset configuration OMODs in the live
  stack and AP graph, but suppress their internal AP in generated menus.
- OG 1.10.163 remains on the pre-existing refresh path. AE workaround
  remains opt-in and needs focused target-runtime testing.

## 0.5.207 (unreleased AE-only auto re-equip experiment)

- Add an opt-in, exact-stack automatic re-equip alternative only for AE
  1.11.240. OG 1.10.163 retains its original refresh logic.
- The plugin INI switch AEAutoReequipAfterApply defaults to false.
- Snapshot the loaded magazine and restore it only when the same ammunition
  type and sufficient post-modification capacity can be verified.
- No release or automatic deployment before focused game/animation/ammo QA.

## 0.5.206 (unreleased post-attachment crash safety)

- Prevent a confirmed access violation after a successful attachment change
  on Fallout 4 1.11.240: post-modification refresh ID 1153963 is absent from
  that runtime's address library and falls through to non-executable data.
- On runtimes without a verified refresh address, leave the successful OMOD
  transaction in place but skip the unsafe immediate visual refresh.
  Re-equipping the weapon may be needed to update its visible model.
- Restrict the original refresh helper to exactly Fallout 4 1.10.163
  and require its relocation to point inside executable .text.
- Preserve normal attachment/loose-mod transactions, Quick Menu display,
  existing controller and mouse radial paging, and default-material checks.
- In-game QA still required before calling the fix runtime-validated.

## 0.5.205 (unreleased keyboard/mouse radial pagination)

- Apply the six-attachment radial page limit to keyboard-and-mouse mode,
  not just controller mode. Never omit attachments from the selectable list.
- Add center paging arrows, mouse-wheel and Page Up/Page Down navigation,
  while retaining the existing mouse category-centered attachment arc.
- Keep wedge labels short and show the full name of the hovered attachment
  in the wheel center, without installing it until the user clicks.
- Retain the unmodified attachment mutation/refresh code and the 0.5.204
  crash-stage diagnostics. Vanilla receiver crash remains unresolved.

## 0.5.204 (unreleased dense radial and crash diagnostics)

- Limit controller radial attachments to six wedges per page and use LB/RB
  to page through all choices, keeping analog direction selection per page.
- Shorten controller radial wedge labels and show the full selected
  attachment name and page counter in the wheel center.
- Add flushed post-transaction stage markers surrounding the menu
  payload update and equipped-weapon refresh; do not change either call
  or the underlying attachment mutation.
- Receiver crash remains unresolved without a crash call stack and runtime
  confirmation. This is a diagnostic/layout tester, not a crash fix.

## 0.5.203 (unreleased Quick Menu controller refinement)

- Limit controller-specific actions/navigation to the Quick Menu; Builder and
  Settings remain keyboard/mouse only pending their own controller layouts.
- Retire the controller Builder opener. Existing controller shortcut files may
  retain an old Builder entry, but the runtime ignores it; the Quick shortcut
  remains separate from the keyboard/MCM bindings.
- Read left thumbstick (or right when left is neutral) through XInput on the
  existing poller, and queue sampled radial directions on the F4SE game thread.
- In controller radial mode, stick angle selects a category, A enters its
  attachments, stick angle selects an attachment on a full outer wheel, A
  confirms and B returns to categories or closes. D-right no longer enters
  the radial outer ring. D-pad remains a fallback.
- Preserve existing mouse/keyboard radial layout and the 0.5.202
  empty-material default guard. In-game analog QA is still required.

## 0.5.202 (unreleased default-material safety tester)

- Treat a unique, no-effect, unloaded material-reset OMOD as the effective
  default when the equipped weapon has no installed material OMOD.
- Display the default as already applied without adding an OMOD to the
  equipped weapon or calling the weapon refresh.
- Block material-reset actions over existing material mods until a safe
  workbench-equivalent removal operation is verified, and reject ambiguous
  default records rather than choosing one.
- Keep exact installed object-instance OMOD identity, normal attachment
  mutations, and controller behavior unchanged.
- Target-runtime validation still required.

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
