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
- Controller navigation must move between the settings sidebar and the active
  page controls, activate selections via focus, and preserve row focus when
  Builder re-renders. The controller binding editor uses the existing
  choice-trigger/choice-option style rather than native browser selects.
- Focused Prisma panels use the existing V12 native controller action bridge.
  Menu-opening controller shortcuts are separate from keyboard/MCM bindings.
  XInput is polled on the existing input thread and sends actions through the
  existing game-thread queue. Single buttons and two-button combinations are
  optional and unassigned by default. Never let a controller shortcut bypass
  normal menu-open safety or attachment transaction checks.
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
