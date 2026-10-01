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
- Generated menus do not expose uninstalled OMODs that lack a linked
  loose-mod item. Those records are treated as non-player-facing/internal for
  generated workbench-style discovery; already-installed no-loose-mod OMODs
  remain visible as live state.
- The Builder shows every compatible player-facing generated attachment
  regardless of inventory. The Quick Menu shows installed attachments plus
  compatible alternatives whose loose-mod items are currently in inventory.
- Attachment changes are authorized per operation after live revalidation.
  Unsupported or ambiguous operations fail closed rather than disabling already
  validated paths globally.
- Builder presentation settings never authorize runtime mutation. Exported
  profiles contain per-weapon presentation preferences only.
- MCM is an optional hotkey adapter. The plugin's own polling path remains the
  authority for menu behavior.
- The two opener bindings are also toggles; the other opener switches menus.
  Escape is the universal close action. There is no dedicated close binding.
- The release is text-only. Future icons require a mod-owned semantic mapping
  layer and explicit library adapters. Weapon preview remains removed until a
  renderer lifecycle can pass performance, cleanup, and Alt-Tab testing.
- Build staging and deployment remain separate. A normal build never writes to
  a game or mod-manager directory.
