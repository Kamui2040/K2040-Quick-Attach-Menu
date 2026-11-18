# Connecting Quick Attach to the shared settings editor

The K2040 settings hub shows Quick Attach's controls in its own two-pane
window. Quick Attach still owns those values and decides whether an edit is
valid. The hub cannot change weapon attachments.

**Status:** The V2 adapter and Windows x64 build checks passed, but the
integration still needs Fallout 4 OG testing. It is not a release.

## How this module participates

Quick Attach exports `K2040_GetSettingsAdapterV2` using the shared V2 header
and theme client. Its validated V1 JSON manifest lists the editable fields.
The shared two-pane editor renders those controls directly. Quick Attach does
**not** create a second settings renderer.

The V2 owner reads live values through the existing Quick Menu preferences and bracketed-text preferences. When the user edits a field, this mod validates its ID, JSON type, option and numeric bounds **again**, then uses the native owner setters and existing user-preferences save path. The selected layout's size and position affect only its own stored layout. Hiding or omitting a field never clears saved data. Native settings schema 2, INI compatibility, weapon profiles and per-weapon Builder overrides remain unchanged.

Reset All can destroy weapon-specific preferences. The hub currently disables
this action and the V2 owner callback rejects it until a verified, explicit
confirmation flow is provided. The former standalone Settings HTML is retained
only as historical source; it is not installed or accessible in this VNext QA
package. Do not claim destructive reset support in the unified editor.

## Shortcut ownership

The shared hub alone owns the settings-only shortcut and its MCM
configuration. VNext QA reserves unmodified F3 as a provisional hub opener;
QAM neither registers that key nor determines the final hub default.
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
restored Settings button requests K2040_RequestModuleSettingsV1("quick-attach")
from an already-loaded K2040_Unified_Settings.dll on a queued F4SE game-thread
task. This opens the existing central hub with Quick Attach selected, not
a second settings renderer. On accepted request Builder unfocuses and hides
and releases input before the hub's deferred opening task. Missing, outdated,
not-ready or rejecting hub leaves Builder open with an unavailable notice.
No new hotkey is registered. The V1 launcher remains disabled, and the old
settings HTML and hub DLL are excluded from this owner's installer.
The hub supplies the `K2040_RequestModuleSettingsV1` entry point. Quick Attach
asks it to select the Quick Attach page in the existing editor; it does not
open a second editor.

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

The Framework's Presentation selector renders illustrative diagrams for
the six QAM layouts. The QAM manifest selects this illustration using the
existing preview: "attachment-layout" contract, without live game objects or
additional owner-owned settings UI. These descriptions show what the
Framework-owned previews should look like:

- **Cascade:** a vertical category list beside a vertical attachment list,
  not diagonally overlapping cards.
- **Horizontal Bar:** a horizontal category pane and separate attachment
  options rather than one row of unrelated category buttons.
- **Radial:** a circular inner category wheel, plus outer attachment sectors
  centered on the selected category (mouse mode). Controller options can use
  a full outer circle.
- **Compact Hybrid:** the left **semicircle** contains seven visible category
  wedges (scrollable window) and the right is a traditional attachment list,
  not a complete round category ring.
- **Orbit:** eight separate **round buttons** surround a labeled center hub;
  the independent **curved attachment list to the right** centers the
  selected option. It does not use concentric category wedges.
- **Clock Slices:** category wedges form the inner wheel; **all attachments
  that fit the outer 360-degree circumference** appear as narrow, elongated,
  adjacent slices around the selected parent's angular midpoint. Slice count
  is bounded only by measured label widths and a positive seam gap, not a
  fixed five-item page. This is not a static concentric ring of equal sectors.

The actual menu layout comes from `quick-menu-renderer.js`,
`quick-menu-geometry.js` and `quick-menu.css`. The Framework draws the
illustrative previews, and QAM must not create another renderer for the settings hub. Any abstract
diagram that does not match these structures must be revised in the
Framework itself. These illustrations do not change player state.

## What still needs testing

- Canonical V1 manifest, schema and validator remain pinned and independent of the hub's source checkout.
- Native input policy tests reject invalid colors, enums, Boolean types, out-of-range/off-step numbers, unowned fields and direct action invocation.
- Regression tests cover every manifest-editable field's native read/write routes, dangerous reset refusal, unchanged gameplay shortcuts and absence of settings-only keybinds.
- Native C++ tests, Python/JS suites, AJV schema validation and Windows x64 cross-build passed for this development branch.
- The hub's read-only four-owner verifier previously accepted the four V2 exports. This newer optional manifest-field extension and gameplay-key migration require fresh combined-game testing; earlier verification is not an acceptance of these changes.
- Headless Chromium verified shared theme color precedence while preserving a native-configured translucent panel.

**Not yet verified:** the hub's configured settings opener, MCM reassignment, new direct Quick/Builder keyboard and controller assignments, typed edits, actual persistence, static layout previews, theme and palette propagation (especially Hot Omni/Clock), focus and switching between views on Fallout 4 OG 1.10.163. No installed-plugin acceptance was performed for these V2 changes. Until that QA is complete, do not deploy, retire installed settings interfaces, merge the feature into a release branch or publish.

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

## Builder visual ownership (0.5.228 QA)

Framework's four visual themes now style Builder controls, section dividers,
focus and selection within `#builder`. The exact IDs are unchanged: `fallout4`,
`fallout76`, `doom-eternal` and `fallout3-newvegas`. Appearance colors come
from the resolved palette; choosing a theme never selects a different palette.

The six Quick Menu presentations keep their own font measurements, button
shapes and interaction geometry. Theme decoration can add non-interactive
frame strokes, but does not resize, clip or reshape selectable sectors.
Builder's existing Settings button still requests the separate Framework
editor; this change adds no settings shortcut, renderer, DLL or MCM page.

The structural CSS and control checks are automated. Fallout 4 OG in-game
verification is still needed for all four themes, custom/Hot Omni palettes,
long attachment names, controller focus, and switching between Builder and
Framework settings. No HUD layout profile export is added to QAM.
