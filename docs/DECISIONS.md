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
- Builder presentation settings never authorize runtime mutation. Exported
  profiles contain per-weapon presentation preferences only.
- Every live menu rebuild, including the refresh after an attachment change,
  reapplies persisted presentation preferences before reaching the active view.
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
- Diagnostic logging defaults on, can be disabled persistently, and is not
  initialized until the saved logging preference has been loaded.
- The release is text-only. Future icons require a mod-owned semantic mapping
  layer and explicit library adapters. Weapon preview remains removed until a
  renderer lifecycle can pass performance, cleanup, and Alt-Tab testing.
- Build staging and deployment remain separate. A normal build never writes to
  a game or mod-manager directory.
