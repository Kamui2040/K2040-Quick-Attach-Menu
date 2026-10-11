# Technical Decisions

Only durable product decisions belong here. Current release state belongs in
`PROJECT_CONTEXT.md`; unresolved limits belong in `docs/KNOWN_ISSUES.md`.

- Runtime behavior uses live game data. xEdit exports are research inputs, not
  runtime dependencies.
- ECO FormLists and Messages define authored player-facing structure. OMOD and
  attachment-point data define structural validity and dependency ordering.
- Installed identity comes from resolved object-instance OMOD form identity.
  Display names, EditorIDs, and load-order-dependent full FormIDs are not stable
  persisted keys.
- Providers are installed before children and removed after children. Hidden,
  internal, and script-only OMODs stay out of player-facing menus.
- Tactical Reload's three known switcher infrastructure OMODs remain in the
  live graph but stay out of generated choices and dependency-removal planning.
- Generated-menu target compatibility uses the winning OMOD record's raw
  `MNAM` Target OMOD / Mod Association keywords, not CommonLibF4
  `filterKeywords`/`FNAM`. A reachable attachment point is also required.
  OMODs without MNAM targets may match by attachment point alone; unresolved
  MNAM metadata fails closed for uninstalled candidates. Source plugin is not
  a compatibility gate.
- Linked loose-mod items remain the normal generated path. With
  `AllowNoLooseModOptions` enabled, an uninstalled no-loose OMOD may also be
  exposed when it has an explicit resolved MNAM target matching the equipped
  weapon. Generic no-MNAM no-loose records remain excluded; already-installed
  no-loose OMODs remain visible as live state.
- The Builder shows every compatible player-facing generated attachment
  regardless of inventory. The Quick Menu shows installed attachments,
  compatible alternatives whose loose-mod items are currently in inventory, and
  validated no-loose choices that pass the explicit-target safety rule.
- Attachment changes are authorized per operation after live revalidation.
  Unsupported or ambiguous operations fail closed rather than disabling already
  validated paths globally.
- A provider may be installed into an empty, live-reachable attachment point
  only when it activates no existing unreachable attachment, changes no
  unrelated installed OMOD identity, and exposes its declared attachment points
  after installation.
- Force Unsafe Swaps is a stable-WEAP, per-weapon escape hatch that defaults
  off and bypasses only a diagnosed dependency-cycle ordering refusal. All
  other transaction checks remain active, and menu profiles cannot carry it.
- Builder presentation settings never authorize runtime mutation. Exported
  profiles contain per-weapon presentation preferences only.
- Every live menu rebuild, including the refresh after an attachment change,
  reapplies persisted presentation preferences before reaching the active view.
- An empty material AP is already the default state: classify one unique
  zero-effect material OMOD as UI-only effective default when the exact live
  stack has no material OMOD. Never misrepresent it as actually installed for
  provider/dependency/rollback checks. Reject unproven material resets and
  ambiguous default records without engine mutation.
- Generated uninstalled OMOD discovery requires a loaded COBJ recipe creating
  the exact OMOD plus matching MNAM and reachable AP. Preserve installed forms
  for recovery. Recipe presence is not the full workbench condition evaluator;
  don't label the resulting list exact workbench parity until validated.
- Builder and Settings directional movement follows the visible geometry:
  Left/Right traverse horizontal toolbars, settings rows, two-column
  choice grids, and side-by-side confirmation controls; Up/Down traverse
  vertical lists or move between visual rows. Dropdowns remain vertical
  and only a separate Confirm applies a selection. LT is a supported
  optional Quick Menu opener binding, not a default navigation command.
- Controller navigation extends to Quick Menu, Builder and Settings with
  separate focus handling appropriate to each presentation. A mapped
  Activate/Confirm press alone triggers an action; navigating a Builder
  category never changes visibility. Custom dropdowns open their own
  focus scope, commit only on Confirm and close without mutation on Back.
  Settings sliders require an explicit edit-mode confirmation; import/reset
  dialogs remain confirmation-gated. Keep text entry and drag reordering
  keyboard/mouse-based until a fully tested controller solution exists.
  The obsolete controller Builder shortcut is ignored without changing MCM keys.
- Apply the radial six-entry page limit to keyboard/mouse too. Provide
  center paging arrows, mouse-wheel and Page Up/Page Down navigation, and
  a full-name hover caption. Never remove entries to fit the wheel.
- Show at most six controller radial attachment entries per page;
  LB/RB changes option pages, and the full selected name appears in the
  center. Never remove available attachments merely to fit the wheel.
- On AE 1.11.240, coalesce attachment changes into one optional automatic
  re-equip when the Quick Menu closes; keep OG 1.10.163 native refreshing
  unchanged. A magazine-count reset during this AE re-equip is an accepted
  usability trade-off. Do not introduce unverified low-level refresh calls
  solely to avoid it. Genuine reserve-ammo loss/duplication, crashes, or
  wrong-weapon equipment remain defects requiring separate validation.
- An Address Library relocation must be verified for the target
  game runtime, not inferred from a nearby ID. The equipped refresh
  ID 1153963 is absent on AE 1.11.240 and caused executable-data
  access violations. Skip AE immediate visual refresh until a
  verified implementation exists, while retaining normal OMOD
  mutations. Keep 1.10.163 behind exact-version and executable
  address checks; do not claim target-runtime PASS from a build.
- Flushed post-mutation stage markers may narrow a crash location,
  but must not be misrepresented as a verified fix or a crash stack.
- Treat Quick Menu controller navigation as presentation-independent:
  Up/Down move focus visually through categories/attachments, Right enters
  options, Left returns to categories, Cancel returns before closing, and
  LB/RB move categories or page radial attachments. Compact Hybrid's
  bottom-to-top semicircle reverses the category-index delta only, so
  physical Up/Down follows on-screen movement. Shared direction dispatch
  is used for keyboard, D-pad and sticks; input never installs without
  separate mapped confirmation.
- In the Quick Menu, both sticks and the D-pad may highlight categories
  or attachments, but no navigation input installs an attachment. Require
  a separate game-mapped Activate/Confirm press; never assume A if Fallout 4
  supplies a supported remapped gamepad button. Avoid assigning that same
  physical button to a menu navigation action.
- XInput stick direction is sampled on the native input thread only while
  a controller-enabled Prisma menu is active, then dispatched through the
  F4SE game-thread task interface. Never call PrismaUI from the poller thread.
  Use left stick preferentially, right as fallback, with a dead zone and
  view-specific navigation. Install requires the currently mapped Confirm,
  and Cancel backs out/closes according to the active page.
- Quick Menu, Builder and Settings bind PrismaUI V12 controller actions
  only while their focused view is active. The optional single/two-button
  Quick opener stays independent of keyboard/MCM bindings and must never
  bypass menu-open or attachment mutation safety checks.
- MCM is an optional hotkey adapter. Native physical polling remains active for
  opener state, focused Prisma pages may forward matching browser events as a
  supplemental path, and duplicate cross-source signals are filtered.
- Menu open/close/switch work is queued to the F4SE game thread. The two opener
  bindings are toggles; the other opener switches menus. Escape is the universal
  close action, and DialogueMenu blocks opener actions. There is no dedicated
  close binding.
- Every focused mod page keeps gameplay input isolated. A shared slowdown
  setting scales the captured pre-menu time multiplier from normal time through
  full pause and restores it only when no other runtime system replaced it.
- Prisma normally owns the cursor for focused views. The plugin verifies the
  engine owner count after focus and uses a validated per-runtime fallback only
  when the provider did not register one; constraints are then cleared. Internal
  page replacement releases fallback state after unfocus and reacquires it for
  the new focused page.
- Diagnostic logging defaults on, can be disabled persistently, and is not
  initialized until the saved logging preference has been loaded.
- The release is text-only. Future icons require a mod-owned semantic mapping
  layer and explicit library adapters. Weapon preview remains removed until a
  renderer lifecycle can pass performance, cleanup, and Alt-Tab testing.
- Build staging and deployment remain separate. A normal build never writes to
  a game or mod-manager directory.
