# Quick Attach Menu — Unified Settings Editor V2

**Status:** V2 adapter and Windows x64 source/build validation complete; Fallout 4 OG runtime acceptance pending. This branch is not a release.

## How this module participates

Quick Attach implements the canonical K2040_GetSettingsAdapterV2 plain-C export with the same V2 header and theme client as K2040 Unified Settings. Its validated V1 JSON manifest is the single field catalog. The shared two-pane editor renders those controls directly; this mod does **not** create an additional integrated settings renderer.

The V2 owner reads live values through the existing Quick Menu preferences and bracketed-text preferences. When the user edits a field, this mod validates its ID, JSON type, option and numeric bounds **again**, then uses the native owner setters and existing user-preferences save path. The selected layout's size and position affect only its own stored layout. Hiding or omitting a field never clears saved data. Native settings schema 2, INI compatibility, weapon profiles and per-weapon Builder overrides remain unchanged.

Reset All can destroy weapon-specific preferences. The V2 callback rejects it until an owner-side confirmation flow is implemented and game-tested. The standalone Settings panel retains its existing confirmation.

## Shortcut ownership

There is **no independent settings-only Quick Attach shortcut to migrate**. The only Quick Attach MCM keyboard actions are openQuickMenu and openMenuBuilder; both are gameplay controls and stay independent. The existing controller Quick Menu shortcut also stays mod-owned. Neither the module manifest nor the native poller assigns Ctrl+Shift+B or creates a replacement settings-only binding.

Unified Settings owns the single settings shortcut, provisionally Ctrl+Shift+B, and its MCM reassignment and registry persistence. Quick Attach never rewrites the shared MCM keybind registry. The standalone Settings panel remains accessible through Builder while the new editor is under development and works when Unified Settings is absent. It is **not** used as an integrated panel in V2.

## Shared appearance

The V2 adapter accepts the hub's effective default, midnight, high-contrast or pipboy theme through its optional theme callback. The hub selects global inheritance or a per-module override; Quick Attach does not store that choice. A new view queries the optional hub theme export when its DOM becomes ready, while an already open view receives live updates.

The same small theme bridge is loaded by the existing Menu, Builder and standalone Settings views. Shared color tokens override renderer inline text/accent colors, including the wheel label and status layers. The owner's background-opacity variables are not overridden, and saved custom colors remain untouched. With no hub installed, the original theme behavior is unchanged.

## Validation and remaining gate

- Canonical V1 manifest, schema and validator remain pinned and independent of the hub's source checkout.
- Native input policy tests reject invalid colors, enums, Boolean types, out-of-range/off-step numbers, unowned fields and direct action invocation.
- Regression tests cover every manifest-editable field's native read/write routes, dangerous reset refusal, unchanged gameplay shortcuts and absence of settings-only keybinds.
- Native C++ tests, Python/JS suites, AJV schema validation and Windows x64 cross-build passed for this development branch.
- The hub's read-only four-owner verifier accepted Quick Attach, Minimap, Quest Journal and HUD V2 exports, matching ABI headers, manifests and theme scripts.
- Headless Chromium verified shared theme color precedence while preserving a native-configured translucent panel.

**Not yet verified:** in-game Ctrl+Shift+B opening, MCM reassignment/clear behavior, typed edits, controller navigation, actual persistence, theme propagation/override and switching between independent panels in Fallout 4 OG 1.10.163. No installed-plugin acceptance was performed for these V2 changes. Until that QA is complete, do not deploy, retire installed settings interfaces, merge the feature into a release branch or publish.

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
