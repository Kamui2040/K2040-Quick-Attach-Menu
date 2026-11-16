# Quick Attach Menu — Unified Settings Editor V2

**Status:** V2 adapter and Windows x64 source/build validation complete; Fallout 4 OG runtime acceptance pending. This branch is not a release.

## How this module participates

Quick Attach implements the canonical K2040_GetSettingsAdapterV2 plain-C export with the same V2 header and theme client as K2040 Unified Settings. Its validated V1 JSON manifest is the single field catalog. The shared two-pane editor renders those controls directly; this mod does **not** create an additional integrated settings renderer.

The V2 owner reads live values through the existing Quick Menu preferences and bracketed-text preferences. When the user edits a field, this mod validates its ID, JSON type, option and numeric bounds **again**, then uses the native owner setters and existing user-preferences save path. The selected layout's size and position affect only its own stored layout. Hiding or omitting a field never clears saved data. Native settings schema 2, INI compatibility, weapon profiles and per-weapon Builder overrides remain unchanged.

Reset All can destroy weapon-specific preferences. The hub currently disables
this action and the V2 owner callback rejects it until a verified, explicit
confirmation flow is provided. The former standalone Settings HTML is retained
only as historical source; it is not installed or accessible in this VNext QA
package. Do not claim destructive reset support in the unified editor.

## Shortcut ownership

There is one settings-only shortcut across participating mods:
**Ctrl+Shift+B** (assignable in Unified Settings MCM).
Quick Attach has **no native MCM gameplay keybind definitions** or
Scaleform gameplay-hotkey registration. Its Quick Menu and Builder
openers remain gameplay features, and are edited using direct key
recording in the unified Controls page: Shift+K and Ctrl+Shift+K
by default. Saved gameplay chords live in Quick Attach's existing
user-preferences JSON. The legacy plugin INI supplies a fallback
until a user preference exists. The keyboard poller uses only those
owner-owned values; no MCM dependency is required.

The Quick Menu controller shortcut is edited through the same
Controls page and saved in Quick Attach's own ControllerShortcuts.ini.
Old MCM user settings are only read for migration. The installer
does not remove or rewrite a user's existing MCM registry entries.
Old MCM callback registrations are disabled to prevent stale entries
from invoking a second gameplay shortcut.

The standalone Builder remains available for per-weapon menu editing. Its
legacy general Settings button no longer opens a second page; the hub's V2
manifest provides the current general settings. Close Builder and use the hub's
configured shortcut (default F3). The V1 launcher is disabled for this QA
branch, and the standalone settings HTML is excluded from its installer.
It is not an integrated editor panel.

## Shared appearance and static layout previews

The mod's legacy theme preference is deliberately **not exposed**
as a second selector in the unified editor. Its saved value and
standalone fallback remain unchanged; shared theme/palette selection
belongs to Unified Settings.

The hub's optional appearance snapshot is queried by Quick Menu,
Builder and standalone Settings when a Prisma view becomes ready.
Theme callbacks re-query the resolved theme+palette snapshot and apply
transient semantic colors. Palette colors take precedence over the old
Fallout 76 gold/amber CSS for every Quick Menu layout, including Clock
Slices, and over Builder/Settings chrome. Native saved custom colors
and opacity remain intact; standalone appearances still work without
the hub.

The unified editor's Presentation selector now renders a compact
illustrative diagram for each of the six layouts (Cascade, Radial,
Compact Hybrid, Horizontal Bar, Orbit and Clock Slices). Diagrams
are non-live, don't require an equipped weapon and never modify game
state. The canonical manifest schema permits an optional declarative
`preview: "attachment-layout"` selector flag; the shared editor,
not Quick Attach, draws that preview.

## Validation and remaining gate

- Canonical V1 manifest, schema and validator remain pinned and independent of the hub's source checkout.
- Native input policy tests reject invalid colors, enums, Boolean types, out-of-range/off-step numbers, unowned fields and direct action invocation.
- Regression tests cover every manifest-editable field's native read/write routes, dangerous reset refusal, unchanged gameplay shortcuts and absence of settings-only keybinds.
- Native C++ tests, Python/JS suites, AJV schema validation and Windows x64 cross-build passed for this development branch.
- The hub's read-only four-owner verifier previously accepted the four V2 exports. This newer optional manifest-field extension and gameplay-key migration require fresh combined-game testing; earlier verification is not an acceptance of these changes.
- Headless Chromium verified shared theme color precedence while preserving a native-configured translucent panel.

**Not yet verified:** in-game Ctrl+Shift+B opening, MCM reassignment, new direct Quick/Builder keyboard and controller assignments, typed edits, actual persistence, static layout previews, theme and palette propagation (especially Hot Omni/Clock), focus and switching between views on Fallout 4 OG 1.10.163. No installed-plugin acceptance was performed for these V2 changes. Until that QA is complete, do not deploy, retire installed settings interfaces, merge the feature into a release branch or publish.

A QA archive may be built under Downloads for later isolated manual testing, but it must not be installed or published automatically.


## VNext adoption (2026-11-15)

The standalone QA overlay now includes
`Data/K2040/Settings/Providers/quick-attach.json`, matching the hub's
tagged registration schema and the existing loaded V2 adapter identity.
The hub only discovers a successfully loaded, compatible DLL.

The optional `K2040_CopyAppearanceV1` snapshot supplies four theme
identifiers and six built-in or named custom palette choices. Quick Attach
validates the complete schema, token names and all seven HEX colors in native
code before passing a transient snapshot to its existing menu UI. The
three existing views share the JS/CSS bridge; they apply resolved text,
accent, muted and installed colors without rewriting saved Quick Attach
colors or opacity. The existing legacy four-theme query remains a fallback
for older hub versions. New structural visual themes are owned by the hub;
no claim is made that Quick Attach has independently adopted every theme's
frame or typography.

Quick Attach has no persistently visible, draggable HUD widget, so the optional
layout editor V1/V2 contract is not exported. The hub must not advertise a
Quick Attach HUD layout edit button in this candidate.

The bundled latest VNext hub and Quick Attach are *QA-only*: Fallout 4 OG runtime
verification is required for palettes, focus, MCM, attachment actions, input,
and persistence before production promotion.
