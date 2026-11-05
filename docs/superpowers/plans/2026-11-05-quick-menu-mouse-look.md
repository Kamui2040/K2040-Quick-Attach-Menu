# Quick Menu mouse look Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the existing movement-while-open preference pass game look input and hide the cursor while arrow keys continue to navigate Quick Menu.

**Architecture:** Extend the existing Quick Menu input mask to allow `kMovement` and `kLooking` only when the saved preference is active for that open. If Prisma advertises V10 `InputRegions`, set an empty mouse-input region list and focus through `FocusOverlay` so mouse input passes to gameplay while keyboard input stays on the page. Hide the cursor and omit native cursor fallback in this mode; restore the default policy on every focus exit path.

**Tech Stack:** C++20/CommonLibF4, PrismaUI_F4 API, Python `unittest` contract checks.

**Spec:** `docs/superpowers/specs/2026-11-05-quick-menu-mouse-look-design.md`

## Global Constraints

- Reuse `allowMovementWhileOpen`; it remains off by default and takes effect on the next Quick Menu open.
- Allow only `kMovement` and `kLooking` while enabled; keep activation, combat and all other gameplay events blocked.
- Hide the Prisma cursor only for the active Quick Menu; Builder and Settings retain their current input and cursor behavior.
- Restore the per-view cursor policy on close, internal page switch, focus failure and game transition.
- Do not launch Fallout 4 or deploy to a mod-manager directory.

## Review Focus

- Movement/look remain blocked when the setting is off; legacy preferences still default off. Existing preference tests cover the absent-field default.
- Combat and activation stay blocked while movement/look are enabled. Task 1 adds native input-mask assertions.
- Quick Menu close/switch/focus failure/game transition cannot leave a hidden cursor on a later view. Task 1 covers all cleanup sites with source contract assertions.
- Builder/Settings never enable look passthrough or hidden cursor. Task 1 tests the Quick Menu-only mode condition.
- Providers without input-region capability retain normal menu focus and log that mouse-look passthrough is unavailable. Providers without the optional cursor-policy export still omit the native cursor fallback in movement mode.

---

### Task 1: Couple cursor look passthrough to Quick Menu movement mode

**Files:**
- Modify: `src/PrismaBridge.cpp`
- Modify: `scripts/tests/test_quick_menu_movement_contract.py`
- Modify: `PROJECT_CONTEXT.md`
- Modify: `CHANGELOG.md`
- Test: `scripts/tests/test_quick_menu_movement_contract.py`

**Interfaces:**
- Consumes: existing `allowMovementWhileOpen` snapshot and `PRISMA_UI_API::SetViewCursorPolicy(PrismaView, CursorPolicy)` optional export.
- Produces: Quick Menu input configuration that allows both `USER_EVENT_FLAG::kMovement` and `USER_EVENT_FLAG::kLooking` when enabled; focused Quick Menu cursor policy `Hidden`; default cursor restored after focus ends.

- [x] **Step 1: Add failing contract assertions**

Assert movement mode enables movement and looking only; the disabled event mask still contains activation and combat; mouse-look mode uses empty input regions with `FocusOverlay`; it skips native cursor fallback; the policy is reset on focus failure, close, page switch and game transition.

- [x] **Step 2: Run the focused contract test**

Run: `python3 -m unittest scripts.tests.test_quick_menu_movement_contract -v`
Expected: new look and cursor lifecycle assertions fail before implementation.

- [x] **Step 3: Implement the minimal native change**

In `PrismaBridge.cpp`, include `kLooking` in the allow mask only when the captured Quick Menu movement preference is true. Use `SetInputRegions(..., nullptr, 0)` and `FocusOverlay` when the provider supports it; set the active view's cursor policy to `Hidden` after focus, omit the native cursor fallback, and restore `Default` for all focus-exit paths.

- [x] **Step 4: Run focused and full validation**

Run: `python3 -m unittest scripts.tests.test_quick_menu_movement_contract -v`, all Python and JavaScript suites, `git diff --check`, Windows x64 build and package validation.
Expected: all automated checks and the build/package checks pass; runtime mouse behavior remains pending manual QA.

**Current implementation validation:** 76 Python tests, all existing
JavaScript suites, the native controller chord test, Windows x64 build/PE
validation, archive check, and `git diff --check` passed. The first in-game
test showed normal focus did not pass mouse input through and the game cursor
fallback remained visible. The click-through overlay fix awaits renewed manual
QA.
