# Settings manifest: what V1 records

This is the original inventory of settings fields. It explains which values
belong to Quick Attach and which are intentionally kept out of the shared
editor. The manifest lists controls; it never stores player preferences.

**Status of this document:** the V1 inventory is retained for compatibility.
The current VNext branch has a native V2 adapter and a separate integrated
editor, but still needs in-game approval. Statements below about a *future*
adapter describe the earlier V1 design, not the current V2 implementation.

## Files and ownership

- V1 schema: schemas/k2040-settings-manifest.v1.schema.json.
- V1 validator: scripts/validate-settings-manifest.cjs. Both are byte-identical pinned copies.
- Manifest: dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings/settings-v1-manifest.json.
- Module ID: quick-attach, matching the existing hub registry and native adapter.
- Native user-preferences schema version: **2** (src/UserSettings.cpp). Manifest version: **1**.
- Native values, defaults, validation and writes remain in UserSettings.cpp, Settings.cpp, existing MCM, controller settings and weapon-specific Builder. The JSON manifest has no live data or defaults.

## Controls in the V1 inventory

| Area | Exposed by V1 | Retained internally without a duplicate editor control |
| --- | --- | --- |
| Controls | Open Quick Menu and Builder (MCM actions), edit Quick Menu controller shortcut (module action) | Native keycodes/modifiers and controller button records |
| Appearance | Theme, background opacity, four advanced custom colors | HUD-derived colors and legacy UseGameUIColor |
| Presentation | Six layouts; active layout scale and X/Y position | Six separately stored layout records, Builder/Settings panel dimensions |
| Labels | Three bracketed-text flags; shorten category-matching words | Legacy HideBracketedText, per-weapon Builder label overrides |
| Behavior | ECO preference, control hints, close after apply, movement, slowdown | Legacy MenuSource and ShowControlHints aliases |
| Advanced | Cheat mode, diagnostic logging | Internal INI compatibility and attachment-graph safety switches |
| Maintenance | One advanced, confirmation-required Reset All action | Individual section/window reset buttons remain in the standalone legacy UI but are not promoted to separate manifest fields |

All serialized general preferences are either represented by declarative fields or explicitly internal. Existing weapon-specific overrides, profiles, visibility and ordering remain in Builder.

## Rules behind the field mapping

1. **No duplicate layout sliders.** The scale, positionX and positionY fields represent the currently selected presentation. The owning future read/edit adapter must route each edit to that presentation's separately stored cascade/radial/hybrid/horizontal/orbit/clock layout record. Switching presentation refreshes those values without overwriting any other layout.
2. **No automatic legacy migration.** Existing INI, controller INI, MCM binding registry and user JSON retain their authority and values. Hidden/internal fields keep their stored values; hiding or omitting a control never deletes or resets preferences.
3. **Theme-dependent colors remain advanced.** V1 visibility rules accept only toggle prerequisites, whereas theme is a select. Do not invent an invalid conditional. The future shared renderer may implement more advanced dependency semantics only after a new schema contract or agreed common behavior.
4. **Actions require native ownership.** Keyboard hotkey editing belongs to MCM, controller shortcut editing belongs to the mod, and destructive Reset All requires explicit module confirmation. Neither actions nor saved values execute merely by loading the manifest.
5. **One shared renderer.** The manifest is intended for the common K2040 component renderer used by both standalone and unified settings. Do not create a second separately maintained HTML/JS implementation. The current settings.html stays unchanged until the shared renderer and owning native adapter pass the original Fallout 4 runtime checks.
6. **Safety switches stay internal.** Native INI options that affect attachment validity, provider and child ordering, AE refresh or compatibility are not added as ordinary UI toggles.
7. **Avoid redundant reset controls.** Category-level resets and window-resize resets remain available in the current standalone UI. A future common renderer should provide consistent reset affordances, never silently resetting omitted stored values.

## Validation and release boundary

From the repository root:

    npm ci --ignore-scripts --no-audit --no-fund
    npm test
    npm run validate:settings

AJV is a pinned development-only dependency. The schema and validator remain in the source repository, but neither is bundled in the mod's Data/ package. Manifest validation is required before packaging and fails closed if dependencies are absent or the manifest is invalid.

This source-only change does **not** authorize replacement of the existing settings.html, changing user storage, merging unreleased feature branches, installation into Amethyst, or official publication.
