# Quick Menu Navigation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make horizontal Quick Menu navigation follow its category/attachment rows, add optional movement passthrough, and keep the focused item fully visible.

**Architecture:** Keep navigation in the existing Quick Menu renderer and inline keyboard dispatcher. Persist `allowMovementWhileOpen` with general preferences and the existing payload; configure the native gameplay-isolation layer on Quick Menu open to allow only the movement event group when enabled. Builder and Settings keep full input isolation.

**Tech Stack:** C++20/CommonLibF4, PrismaUI HTML/CSS/JavaScript, Python `unittest`, Node.js tests, xmake Windows x64 cross-build.

**Spec:** `docs/superpowers/specs/2026-11-04-quick-menu-navigation-design.md`

## Global Constraints

- Horizontal presentation: Left/Right moves within the active row; Up/Down switches category and attachment rows; E/Enter accepts.
- Movement passthrough defaults off; when enabled, WASD is ignored by menu navigation and only `USER_EVENT_FLAG::kMovement` passes to gameplay while Quick Menu is open.
- Builder and Settings continue blocking gameplay input; cascade, hybrid, and radial presentations retain visual-direction navigation.
- Do not weaken attachment validation, input-layer cleanup, or menu-close/opener behavior.
- Use version `0.5.221` for this unreleased candidate; synchronize xmake and F4SE version fields.
- Do not launch Fallout 4 or deploy to a game/mod-manager directory.

## Review Focus

- A legacy preferences file without `allowMovementWhileOpen` must load with movement blocked: Task 2 tests the absent-field default.
- Movement mode must leave every other user event and all non-cursor other events blocked: Task 2 tests the native event mask.
- WASD must reach gameplay only in movement mode while arrows remain menu controls: Task 2 tests actual Quick Menu key dispatch.
- Navigation and held-key repeats must never apply an attachment; only E/Enter acceptance may do so: Task 1 tests activation behavior.
- Focused first/last items must be fully visible at either horizontal edge without moving the outer view: Task 3 tests both row containers and in-range items.

---

### Task 1: Horizontal row navigation and keyboard acceptance

**Files:**
- Modify: `dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js`
- Modify: `dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/menu.html`
- Test: `scripts/tests/unified-renderer-navigation.test.cjs`
- Test: `scripts/tests/controller-navigation.test.cjs`

**Interfaces:**
- Consumes: renderer `presentation`, active pane, visible category/option lists.
- Produces: horizontal-layout `navigateDirection(direction)` behavior shared by keyboard and D-pad; keyboard E and Enter use the existing explicit accept path.

- [ ] **Step 1: Add failing horizontal renderer assertions**

Assert Left/Right changes the selected category in the category pane and selected option in the option pane; Up/Down switches between those panes. Assert cascade, hybrid, and radial geometry remains unchanged and directional movement never applies an option.

- [ ] **Step 2: Run the focused renderer test**

Run: `node scripts/tests/unified-renderer-navigation.test.cjs`
Expected: new horizontal row assertions fail before implementation; existing assertions remain green.

- [ ] **Step 3: Implement horizontal row dispatch**

Update `Renderer.prototype.navigateDirection` in `quick-menu-renderer.js`; retain existing branches for other presentations.

- [ ] **Step 4: Add failing keyboard acceptance assertions**

Extend `controller-navigation.test.cjs` to exercise actual menu key events and assert E/Enter activate once, arrows navigate, and navigation does not install.

- [ ] **Step 5: Implement E/Enter acceptance**

In `menu.html`, route `e` through the existing Enter acceptance branch, preserving repeat suppression and configured opener forwarding.

- [ ] **Step 6: Run browser tests and commit**

Run: `node scripts/tests/unified-renderer-navigation.test.cjs` and `node scripts/tests/controller-navigation.test.cjs`.
Expected: both pass; only explicit E/Enter acceptance activates an option.

### Task 2: Persisted movement preference and input passthrough

**Files:**
- Modify: `include/UserSettings.h`
- Modify: `src/UserSettings.cpp`
- Modify: `src/PrismaBridge.cpp`
- Modify: `dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html`
- Modify: `dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/menu.html`
- Test: `scripts/tests/test_quick_menu_movement_contract.py`
- Test: `scripts/tests/controller-navigation.test.cjs`

**Interfaces:**
- Consumes: existing `QuickMenuPreferences` persistence, payload, and `BSInputEnableLayer` lifecycle.
- Produces: general preference `allowMovementWhileOpen` (false when absent), Settings switch command `set:allow-movement:0|1`, and Quick Menu opening behavior that enables only `USER_EVENT_FLAG::kMovement` when the preference is true.

- [ ] **Step 1: Add failing preference contract checks**

Assert the optional JSON field is read and written, defaults false, participates in preference equality/update, appears in the payload and Settings switch, and is selected only when opening Quick Menu.

- [ ] **Step 2: Run the focused preference checks**

Run: `python3 -m unittest scripts.tests.test_quick_menu_movement_contract -v`
Expected: missing-field, UI, and payload checks fail before implementation.

- [ ] **Step 3: Add the preference through existing general-settings paths**

Add the bool to `QuickMenuPreferences` and `PreferencesDocument::GeneralPreferences`; wire optional JSON read/write, false default, equality/update, payload field, Settings switch, and `set:allow-movement` parsing.

- [ ] **Step 4: Add failing native input-mask checks**

Assert movement-enabled Quick Menu input permits only `USER_EVENT_FLAG::kMovement`; all other user events and non-cursor other events remain disabled. Assert off restores the all-user-events-disabled mask and Builder/Settings never enable movement.

- [ ] **Step 5: Configure the existing layer on view open**

In `PrismaBridge.cpp`, make `ActivateQuickMenuGameplayIsolation` apply the saved movement mask only for `ViewMode::QuickMenu`. Keep Builder and Settings fully isolated and preserve existing failure cleanup.

- [ ] **Step 6: Make WASD passthrough conditional**

In `menu.html`, when `allowMovementWhileOpen` is true, return before consuming `w`, `a`, `s`, or `d`; arrows continue to navigate in either mode.

- [ ] **Step 7: Verify preference and key dispatch; commit**

Run: `python3 -m unittest scripts.tests.test_quick_menu_movement_contract -v` and `node scripts/tests/controller-navigation.test.cjs`.
Expected: false blocks movement and leaves WASD as navigation; true passes movement, ignores WASD in the UI, and retains arrow navigation.

### Task 3: Reveal focused horizontal items

**Files:**
- Modify: `dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js`
- Test: `scripts/tests/unified-renderer-navigation.test.cjs`

**Interfaces:**
- Consumes: rendered horizontal category/option row containers and their active/focused buttons.
- Produces: `Renderer.prototype.scrollItemIntoView(container, item)`, adjusting only the row’s horizontal scroll offset when the focused item crosses an edge.

- [ ] **Step 1: Add failing edge-visibility assertions**

Test focused items beyond the left and right edges, fully visible items, and both category and option rows; assert only the relevant row’s `scrollLeft` changes.

- [ ] **Step 2: Run the focused renderer test**

Run: `node scripts/tests/unified-renderer-navigation.test.cjs`.
Expected: new visibility assertions fail before implementation.

- [ ] **Step 3: Reveal selection after horizontal render**

Implement `scrollItemIntoView(container, item)` using item/container bounds and invoke it for the active category and focused option after the horizontal layout is attached to the DOM.

- [ ] **Step 4: Run renderer tests and commit**

Run: `node scripts/tests/unified-renderer-navigation.test.cjs`.
Expected: focused edge items become fully visible; in-range items and other presentations do not move.

### Task 4: Version, verified status, and final validation

**Files:**
- Modify: `xmake.lua`
- Modify: `src/main.cpp`
- Modify: `scripts/tests/test_controller_pages_contract.py`
- Modify: `scripts/tests/test_controller_navigation_contract.py`
- Modify: `scripts/tests/test_cheat_mode_contract.py`
- Modify: `scripts/tests/test_ae_reequip_contract.py`
- Modify: `PROJECT_CONTEXT.md`
- Modify: `docs/KNOWN_ISSUES.md`

- [ ] **Step 1: Synchronize candidate version**

Set xmake’s three version strings to `0.5.221`, F4SE’s packed version to `0x000500DD`, and update existing version-contract expectations.

- [ ] **Step 2: Record Hunting Rifle runtime acceptance**

Update project context and the Hunting Rifle issue entry to record the maintainer’s 2026-11-04 Fallout 4 confirmation, scoped to the tested fix.

- [ ] **Step 3: Run full regressions**

Run: `python3 -m unittest discover -s scripts/tests -p 'test_*.py' -v` and `node --test scripts/tests/*.test.cjs`.
Expected: all Python and JavaScript suites pass.

- [ ] **Step 4: Build and package the candidate**

Run the repository’s Windows x64 build, verify the x64 PE DLL and synchronized version, run `git diff --check`, and validate a deterministic test archive outside the repository.

- [ ] **Step 5: Review and commit scoped changes**

Review the complete diff, preserve the earlier uncommitted Builder category drag fix as a separately scoped change, then commit only the planned navigation/movement/documentation files.
