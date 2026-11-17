# Current Project State

## 2026-11-17 — CPU Clock efficiency and package inventory hardening (QA)

- Clock now measures only the focused attachment and the neighboring choices
  needed to fill its 360-degree ring. Reuses those fitted results for actual
  sectors and removes the duplicate visible-options enumeration. Large lists
  no longer measure hundreds of off-screen attachment labels on each redraw;
  installed OMOD identity, navigation, line fitting and geometry are unchanged.
- QAM-only packaging uses an explicit 13-file runtime allowlist and rejects
  any unexpected staged file or symlink instead of silently bundling it.
  Retired standalone settings.html stays in source, never in the installer.
- Regression tests cover lazy/eager full-wheel equivalence, large option
  catalogs, rejected staging files, symlinks and required runtime assets.
  Native/CPU in-game acceptance remains separate from source/build validation.

## 2026-11-17 — Fallout 76 Builder focus and CSS cleanup (QA)

- Fixed the flat Fallout 76 Builder's idle left rail obscuring selected,
  hovered and focused accent borders. Both the visual-theme style and the
  old palette fallback were involved; the scoped interaction rule restores
  the semantic accent without changing geometry, installed state or opacity.
- Removed redundant older Builder radius and Quick Menu radius declarations;
  preserved the separately defined final 15px Quick Menu card radius.
- Removed unreachable native MCM registry seed/write/sync branches from
  Hotkey.cpp. Live QAM gameplay shortcut preferences and polling remain intact;
  historical standalone MCM parser/registry tests are retained as test-only.
- Corrected the durable MCM ownership decision for VNext: the Framework owns
  the settings page, QAM retains gameplay binding/input ownership.
- Full source/build checks and in-game CPU visual acceptance remain distinct.

## 2026-11-17 — Clock dynamic circumference capacity (QA)

- Clock no longer inherits Orbit's fixed five-item window. It renders every
  visible attachment that fits within the full outer 360-degree ring, with
  one-line/two-line slices sized by their actual fitted labels, .25-degree
  adjacent gaps and a reserved positive seam.
- The active attachment remains aligned with the selected parent category's
  angular centerline; neighbor slices expand around both sides of that anchor.
  Attachment identity, installation validity, keyboard/wheel navigation,
  colors, opacity and other presentations are unchanged.
- Geometry regression checks cover wrapped indices, mixed widths and full
  circumference including cases where there are more attachments than fit.
  CPU/Proton in-game acceptance is still required.

## 2026-11-17 — Framework Builder outer border correction (QA)

- Replaced the floating dashed header strip and bottom-corner decoration
  with the actual four-sided Builder border. Fallout 4 has a continuous
  industrial border, Fallout 76 a squared solid frame, DOOM Eternal a
  heavier accent edge, and Fallout 3/New Vegas a double terminal border.
- Styling uses existing Framework palette roles and the Builder's existing
  resizable shell. No new pseudo-elements, separate header marks, saved
  preferences, gameplay changes, or Framework DLL.
- Source/CSS checks are separate from pending OG/Proton CPU visual QA.

## 2026-11-17 — Fallout 76 Builder structural alignment

- Aligned Builder-only Fallout 76 panels, choices and actions with the
  K2040 Framework's current flat/squared in-game menu design. The Quick
  Attach Menu's wheel/clock/orbit/cascade/horizontal/hybrid presentation
  dimensions, attachment target geometry and independent button trim were
  deliberately unchanged. Semantic palette and user opacity unaffected.
- In-game CPU/Proton styling acceptance remains pending. No Framework DLL
  or user settings bundled.

## 2026-11-17 — CPU Framework topology across QAM wheels (QA)

- Radial uses two stroke-only rings, category ticks, and a selected parent
  marker; Hybrid uses lower-half guide arcs and a marker that follows its
  seven-category moving window. Orbit uses themed borders and category
  silhouettes with unchanged element size, placement, and click handlers.
- The four Framework theme IDs remain the only visual-theme inputs. Decorations
  are hidden with the Framework absent; no settings, attachment geometry, user
  palette/opacity, data dependencies or Framework binaries were added.
- Source/geometry/interaction checks are separate from pending CPU in-game QA.

## 2026-11-17 — Framework CPU-first theme frame styling (QA)

- K2040 Framework is the shared settings and appearance owner (same
  existing visual-theme identifiers and optional APIs). QAM retains its own
  renderer, option validity and geometry. Framework DLL is never bundled.
- Four original QAM frame treatments now adapt Builder borders/buttons and
  Quick Menu Clock trim using existing Framework theme IDs: Fallout 4
  industrial rails, Fallout 76 rounded/twin rails, DOOM Eternal angular/
  segmented marks, and Fallout 3/New Vegas double terminal borders.
- New non-interactive SVG rings and category ticks in Clock are purely
  decorative, disabled when the Framework appearance is absent, and use the
  owner's opacity-safe existing palette tokens. The selected parent marker
  tracks the category centerline without altering OMOD or child geometry.
- PrismaUI CPU rendering is the development baseline for visual QA. CPU
  runtime verification is pending; GPU-specific work is deferred.

## 2026-11-17 — Clock child slices follow selected parent category (QA)

- Clock attachment slices are anchored to the center angle of the
  selected category wedge, rather than the global right-hand 0° axis.
  Active category windows, option identity, selection and packed spacing are
  retained. The former five-slice limit has since been removed; radial text
  on the left remains upright.
- Clock uses a centered square 940px drawing area so left, top and bottom
  parent categories cannot place child slices outside the rendered surface.
  Its scale fit was updated accordingly; user-controlled position, scale,
  colors and opacity remain unchanged.
- Source geometry tests cover every category orientation and overflow.
  Fallout 4 OG runtime confirmation remains pending.

## 2026-11-16 — Direct Builder Settings entry in Unified Settings (QA)

- Restored Builder's Settings button to request K2040_RequestModuleSettingsV1
  for quick-attach using the optional dynamically resolved hub export.
- The UI event schedules a game-thread task; on accepted request QAM closes
  Builder/focus/input before the hub's deferred open. Rejection leaves Builder
  visible with a clear notice; no global/weapon preferences are mutated.
- The owner-only installer still excludes the hub DLL, owner MCM pages and
  obsolete settings renderer. Runtime gamepad, focus and Alt-Tab QA pending.

## 2026-11-16 — Orbit category readability and hub-only settings (QA)

- Orbit category circles use full-word labels, a larger bounded diameter and
  ellipsis for genuinely oversized words; no mid-word character stacking.
- Builder no longer routes to the legacy general Settings view. The V1 hub
  launcher fails closed; the V2 adapter and registration descriptor remain.
  This QA installer does not distribute the old standalone settings HTML or
  MCM definitions. Settings remain in the separately installed shared hub.
- Regression/source/native build checks precede required Fallout 4 OG visual
  and keyboard/controller runtime testing; no game runtime acceptance claimed.

## 2026-11-16 — Unified hub visual styles and single MCM owner (QA)

- Quick Menu, Builder and standalone fallback Settings now render four
  independent structural visual themes from the shared hub's resolved
  appearance snapshot: Fallout 4, Fallout 76, DOOM Eternal and Fallout 3 /
  New Vegas. Palette RGB remains independent; existing menu sectors and
  saved user opacity are unchanged.
- This owner build no longer includes K2040_Mods or any mod-specific MCM
  menu, keybind action or defaults. The central hub alone registers the
  F3 provisional settings opener. Existing player MCM registry and old
  controller migration inputs are not altered.
- The native Quick Menu and Builder gameplay hotkeys remain editable in
  the hub's owner settings and stored by this mod. The hub, not QAM's
  legacy settings panel, now captures Proton-intercepted Mouse4/Mouse5.
- Unit/static checks are not OG runtime acceptance; this is QA-only and
  not a game deployment or release.



## 2026-11-15 — Manifest adoption correction (QA-only)

- Removed the duplicate legacy Theme selector from the unified owner manifest. Unified Settings alone owns visual-theme/palette selection; previous native saved theme and standalone fallback remain intact. No player settings are rewritten. Fallout 4 OG runtime QA remains pending.

## 2026-11-15 — Gameplay keybinds, theme inheritance, layout previews (QA-only)

- Quick Menu and Builder gameplay chords are editable directly in the shared V2 editor and saved by the owning mod, never through MCM. Legacy MCM gameplay registrations and the Scaleform callback are disabled; only the unified settings shortcut remains MCM-controlled. Controller gameplay shortcut edits use mod-owned storage.
- Builder/Settings and all six Quick Menu layouts now consume the hub's resolved appearance palette. Shared palette colors supersede legacy Fallout 76 amber while preserving saved opacity.
- The shared editor shows an illustrative, non-live schematic of each of Quick Attach's six presentations. No owner-specific live HUD layout adapter is claimed.
- Source and build QA are separate from in-game Fallout 4 OG/Proton acceptance; no automatic deployment or public release is authorized.

## Unified Settings Editor V2 adapter (source/build validated, runtime pending)

- feature/unified-editor-v2 provides the plain-C V2 owner callbacks for its
  validated JSON manifest, native values, guarded edits and optional theme.
  All editable manifest fields have a native read/write route. The owner now
  independently rejects wrong JSON types, invalid options/colors, and
  out-of-range/off-step numbers before invoking existing persistence.
- Quick Menu and Builder shortcuts are gameplay-only and remain registered;
  this mod has no settings-only shortcut or Ctrl+Shift+B binding. Unified
  Settings owns the central settings hotkey. Existing per-weapon profiles,
  user JSON, INI and standalone Builder/Settings functionality are preserved.
- Shared appearance updates Menu, Builder and Settings via one optional
  theme bridge. Active renderer token overrides now reach all wheel/label
  layers without changing the player's saved palette or opacity.
- The destructive Reset All action is deliberately denied through V2 pending
  a confirmed owner flow. The standalone Settings confirmation is unchanged.
- Source suites, native value-policy tests, canonical schema validation and
  Windows x64 build passed. The hub's read-only integration verifier accepted
  the four owner ABI/export/manifest/theme contracts. **Fallout 4 OG runtime
  QA is not complete; no deployment, merge or release is authorized.**
- More detail: docs/UNIFIED_SETTINGS_EDITOR_V2_INTEGRATION.md.
  V1 schema history is in docs/SETTINGS_SCHEMA_V1_AUDIT.md.

- The current unmerged Clock/Radial/Hybrid UI correction on
  feature/clock-arc-menu addresses the large *unfilled angular gaps* between
  sectors, visible as bright triangles over high-contrast game backgrounds.
  Radial category/attachment and Hybrid category borders now leave only a
  0.24-degree total gap; Clock category gaps are 0.24 degrees and adaptive
  one/two-line attachment sectors are packed with a 0.25-degree gap rather
  than a fixed 15-degree pitch. At tested radii, exposed gaps are around
  0.8–1.5 pixels, compared with as much as 25.5 pixels previously.
  The existing convex SVG fill/outline rendering remains in place.
  Clock's selected option stays centered and thin one-line sector widths
  remain unchanged. Saved opacity, menus' controls and OMOD identity are
  unaffected. Browser comparisons passed; *Fallout 4/Proton in-game
  verification is still required* before calling the artifact fixed.

- The same candidate uses SVG-native horizontal category/option labels on
  Radial and Hybrid, two lines maximum, and a seven-item moving category
  window for dense Hybrid menus. The shared display-only category-word
  shortening option handles singular/plural matches in all six presentations
  without modifying original attachment names or transaction identities.

- Unreleased 0.5.227 Clock Slices candidate (feature/clock-arc-menu)
  builds on the optional 0.5.226 unified-settings adapter and incorporates
  the reviewed Orbit V2 visual refinements. It adds radial category sectors
  with five scrolling annular attachment slices, minimal gaps, aligned text
  and no icons. The per-category word shortening is display-only, on by
  default, can be disabled in Settings, and never changes OMOD identity.
  Clock has independent saved scale and position. Native cross-build, source
  and browser QA are separate from the required target-game acceptance.
  This candidate remains unmerged and unreleased.

- Unreleased `0.5.226` optional unified-settings adapter candidate
  (`feature/unified-settings-adapter`) exports the versioned
  `K2040_GetSettingsAdapterV1` for the new shared K2040 settings hub. It
  opens the existing Prisma Settings panel without moving or copying
  configuration ownership. Existing gameplay shortcuts and standalone menu
  operation are preserved. A Windows x64 build and in-game hub-to-settings
  transition must pass before merge/release. This work is stacked on the
  unmerged 0.5.225 candidate.
- Unreleased `0.5.225` transaction-count safety candidate
  (`fix/transaction-count-bounds`) builds on the unmerged Orbit candidate.
  Inventory return and rollback arithmetic now rejects impossible uint32 counts
  rather than allowing integer wraparound; independent native boundary tests
  cover normal, near-limit, and overflow cases. Source/build validation remains
  separate from game acceptance. Do not merge or publish before the stacked
  runtime and UI QA requirements are satisfied.
- Unreleased `0.5.224` Orbit presentation candidate
  (`feature/orbit-curved-list`) builds upon the separate `0.5.223`
  crouching and parser-hardening candidate. It adds a circular category selector,
  a curved attachment list with a larger centre selection, measured wedge text
  fitting for Radial and Hybrid, and an independent saved Orbit layout. Source,
  Linux Windows-x64 build, and package tests passed. In-game visual/input QA
  remains required; no release or merge is claimed.
- Unreleased `0.5.223` development candidate (`fix/input-parser-hardening`):
  preserves the sneaking user-event flag when opening a Prisma menu and hardens
  preferences JSON and OMOD file parsing. Source tests, native Windows x64 build,
  package validation and a read-only installed-plugin bounds scan passed.
  On 2026-11-12 the maintainer confirmed the reported crouched Shift+K
  scenario works in-game with this candidate. This confirms the focused
  regression test only; Builder, movement-toggle, controller, profile import,
  and wider attachment compatibility remain unverified for this candidate.
  Shift still overlaps Fallout 4's default Sprint key, so comparative input
  testing may be useful if related reports recur. The candidate remains a
  draft PR and has not been merged or released.

- Stable GitHub release: `0.5.222` (published 2026-11-05 with the installation
  ZIP, SHA-256 and optional MCM Categorizer compatibility file at
  https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.222).
  Nexus notes are prepared; its Files entry still needs upload. Nexus remains
  at version 0.5.220.
- Version `0.5.222` includes native MCM binding recovery and an optional
  one-file standalone config for users whose MCM Categorizer hides the Quick
  Attach page under the shared K2040 Mods parent. Native MCM binding recovery
  was confirmed in game, including after restarting. The Categorizer override
  still needs confirmation with Categorizer enabled; other mod combinations
  were not independently checked.
- Previous GitHub release: `0.5.221` (published 2026-11-05 with the verified
  in-game-accepted ZIP and SHA-256 at
  https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.221).
  It includes the maintainer-tested Hunting Rifle safety fix, intuitive
  horizontal row navigation, optional movement and mouse-look passthrough,
  and compact-row scrolling. Its release ZIP matches the tester-confirmed
  package.
- Hunting Rifle (Fallout4.esm) ECO attachment-swap fix was tested in Fallout 4
  by the maintainer on 2026-11-04 and confirmed working in the reported setup.
  The fix excludes installed default-template OMOD collections from duplicate
  replacement identity and dependent-removal choices while retaining live
  structural graph inputs and genuine ambiguity checks.
- Previous GitHub release: `0.5.220` (published 2026-10-25 with the verified
  in-game-accepted ZIP and SHA-256 at
  https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.220).
  The maintainer confirmed Nexus `0.5.220` on 2026-10-25; the Nexus Files tab
  was not independently checked through an authenticated session.
- `0.5.220` source was validated and merged in PR #36; `main` includes the
  accepted MCM single-writer fix. The final release ZIP has been checked
  against the user-tested archive and published unchanged on GitHub.
  The maintainer confirmed on
  2026-10-25 that the MCM single-writer tester passed in-game, including the
  Settings/MCM binding conflict. This is scoped acceptance of that fix, not
  independent evidence of every other change. Version 0.5.220 also includes
  OMOD FULL labels, shared keybind parsing, guarded non-native registry saves
  and CEF modifier/Mouse 4/5 capture.
- User QA established a native MCM keybind conflict: Settings wrote defaults to
  `Keybinds.json` while MCM retained previous live assignments and later saved
  them back. Upstream MCM's `SetKeybind` registers, rather than remaps, a key.
  The accepted candidate delegates native-MCM changes exclusively to MCM's
  `RemapKeybind`, verifying current/previous values with `GetKeybind` and
  leaving serialization to MCM on close. A cleared native-MCM binding must be
  restored in MCM itself. The maintainer's latest in-game test passed; no
  local game or user bindings were modified by the development run.
- Previous Nexus Mods version: `0.5.220`, confirmed by the maintainer on
  2026-10-25 at https://www.nexusmods.com/fallout4/mods/109575.
  The maintainer tested and accepted the keyboard
  fix in Fallout 4 OG 1.10.163; the deployed DLL matches the published GitHub
  ZIP and the F4SE log records held-opener isolation and key-up release for
  both menus. Build, deterministic packaging and 52 Python tests passed.
  GitHub release: https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.219
  Release tag points at the user-tested commit 8fd5a11517265a14637fec1fa711fc2a93958d43.
  Gaming Mods website PR #63 is merged and live, with the project page and
  release feed updated in five languages. Nexus 0.5.219 is also maintainer-confirmed;
  Gaming Mods website PR #64 updates its Nexus status in all five languages.
- Previous stable GitHub release: `0.5.216` (also available on Nexus Mods)
- Product name: **K2040's Quick Attach Menu**
- Package identifier: `K2040_Quick_Attach_Menu`
- Current-release runtime validation: original reporter confirms the RIS
  attachment rediscovery fix; the maintainer reports final 0.5.216 gameplay
  checks passed. Previous controller, Cheat mode, MCM and OG/AE tests remain
  applicable. Known AE refresh and cross-mode loose-item limitations are accepted.
- Release presentation: text-only; icons and weapon preview are not included

GitHub stable release 0.5.216 was promoted on 2026-10-20, including the
generated-menu compatibility fix on top of
the unified 0.5.215 MCM/theme branch. It addresses a report that a rail weapon
light was visible while installed but disappeared after being removed. The
original menu checked an uninstalled OMOD's MNAM against the base WEAP only;
the reported live weapon also had the needed keyword from installed parts.
The candidate now permits dynamic MNAM matches only with both a matching live
keyword and an active installed OMOD's weapon Keywords/Add property; explicit
keyword removals reject that evidence. Recipe, AP, inventory, Cheat mode,
internal/hidden and transaction checks remain unchanged. The pinned
CommonLibF4 Linux cross-build, Windows x64 DLL/package checks, 41 Python
tests, four JavaScript suites, and native chord test passed. The original
reporting user subsequently tested the RIS attachment rediscovery fix and
confirmed that the reported bug is resolved in their setup. The maintainer
also reports the final combined 0.5.216 in-game tests passed. All five
validated development PRs are merged into main. The stable GitHub ZIP is
byte-identical to the tested preview ZIP and has a published checksum.
The maintainer confirmed that 0.5.216 has also been published on Nexus Mods.
The maintainer reports that v0.5.219 is now published on both GitHub and Nexus Mods. The public Nexus page may lag in unauthenticated caches; check the current Files tab before relying on external metadata.
The known AE visual/effect refresh and Cheat mode item-return behavior remain
accepted and documented.

Version 0.5.215 (included in stable 0.5.216) is an MCM packaging and appearance
candidate. Each Quick Attach installer now bundles the unchanged
K2040_Mods parent (SHA-256
8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb)
alongside its child page, keybinds.json and child-owned controller
settings.ini. The separate MCM Hub download and obsolete alternate
standalone-MCM package are unnecessary. Other K2040 mods may supply
the same byte-identical parent. Existing keyboard action IDs and
Shift+K / Ctrl+Shift+K defaults remain unchanged. Experimental parent
controller preferences are copied once to the child INI if it is not
already present; native shortcuts remain a fallback without MCM.
MCM Scaleform callbacks use the existing game-thread dispatcher.
Classic Pip-Boy has HUD-style chrome and font fallbacks, and Fallout 76
uses the Quest Journal-inspired palette. Cheat mode is labelled
Experimental and use-at-your-own-risk. The maintainer tested the
bundled 0.5.215 MCM integration in-game and reports that it works for
Quick Attach Menu in their setup. This is focused user acceptance,
not independent confirmation of every key/controller combination.
The maintainer later confirmed that Quick Attach, Prisma Minimap and
Quest Journal all register as child entries under the unified K2040
Mods MCM parent in their game. Combined page registration therefore
passes in the tested setup. The other two mods still need separate
bug fixes; those issues were not reported to affect Quick Attach.
This does not establish that their own controls work, or that F4SE
Menu Framework and all input combinations behave identically.

Version 0.5.214 (included in stable 0.5.216) introduced Cheat mode,
based on the 0.5.213 controller and AE work. The off-by-default global
Behavior toggle removes the loose-mod inventory requirement for installing
compatible OMODs. It neither consumes nor generates loose inventory items,
while keeping loaded crafting-recipe existence, explicit weapon-target,
AP reachability, default-material, provider/child order, exact-stack,
rollback and hidden/internal OMOD protections. Profiles cannot enable it;
Force unsafe swaps remains separate. Build/static tests pass.
Focused AE user testing reports successful Cheat mode installs and
confirms that disabling the setting restores normal item requirements.
The tester observed one cross-mode side effect: an attachment installed
free under Cheat mode can yield its associated loose mod when replaced
after Cheat mode is disabled. This is consistent with normal return
handling for installed attachments: the runtime does not remember
whether an individual part was originally acquired for free. No
separate provenance tracking or inventory rewrite is planned for this
accepted limitation. The maintainer now reports that their broader
weapon/attachment and original-game compatibility checks pass as far
as they can tell. This is focused user acceptance, not independently
audited exhaustive test coverage; the remaining confidence step is
broader public testing. On AE 1.11.240 the user also observed a
separate post-attachment refresh limitation: automatic re-equip updates
the visible model, but a 10mm recon scope continues aiming as if it
had ordinary sights until a manual swap away and back. The tester
reports a comparable suppressor effect. The manual weapon switch is
an accepted workaround, not a proven cause or comprehensive fix.

Unreleased 0.5.213 corrects orientation mismatches in the 0.5.212
Builder and Settings controller focus layer. Builder header and weapon
controls now use Left/Right within horizontal rows and Up/Down between
rows, while vertical categories and attachments retain Up/Down. Settings
two-column grids and side-by-side header/confirmation controls follow
their physical arrangement. LT remains an optional Quick Menu opener
button/modifier, not a built-in navigation key. Browser tests pass and
focused in-game testing confirms the orientation fix works. LT was observed behaving like Tab/focus-next in Builder and Settings
while remaining Aim in gameplay. The user accepts that behavior and
does not require a fix. Quick Attach Menu does not explicitly bind
LT to menu navigation; the input translation's source is unverified.
Broader controller regression and release publication remain separate.

Unreleased 0.5.212 introduces shared controller navigation for Builder
and Settings, using the live game-mapped Confirm/Cancel buttons. Dropdown
lists and modal dialogs have explicit navigation and cancel semantics;
Builder category browsing never changes visibility without a separate
button action. Settings sliders require Confirm to enter editing.
This is tested with simulated browser events and native build checks but
needs targeted Proton/Prisma in-game QA. Text entry and drag reordering
still require keyboard/mouse controls.

Unreleased 0.5.211 consolidates input movement/Back/shoulder navigation
across all four Quick Menu presentations. In Compact Hybrid the semicircular
category wheel is drawn bottom-to-top, so Up/Down now reverse only the
underlying category-index step to match screen direction. Sticks, D-pad,
and keyboard share directional behavior; Cancel first returns from options
before closing. Input never installs a mod without explicit mapped Confirm.
Renderer/browser tests pass; initial user in-game testing reports
this revision working as far as tested. The result is provisional;
full layout/remapping and cross-controller QA remain outstanding.

Unreleased 0.5.210 expands Quick Menu controller navigation to both
thumbsticks and the D-pad in radial, cascade, hybrid and horizontal
presentations. Only an explicit mapped gamepad Activate/Confirm press
can install a selected attachment; movement is highlight-only. It reads
the current Fallout 4 control mapping, avoids button conflicts and
retains the tested AE 0.5.209 deferred re-equip. Browser/controller
simulations and Linux cross-build passed, and focused user testing
confirmed the 0.5.210 controller behavior works in the tested setup.
Remapped-button combinations, every layout, and broader Proton/XInput
regression coverage remain unverified.

The unreleased 0.5.209 experiment addresses the 0.5.208 report
that successive OMOD changes sometimes only visually show the first
modification until manually swapping weapons. The AE-only workaround
now coalesces successful changes and schedules a single re-equip
after the Quick Menu closes, with stale-task and exact OMOD/stack
checks. Focused user testing of 0.5.209 on AE 1.11.240 confirmed
that the previously reported multi-attachment visual-refresh problem
is resolved when closing the Quick Menu. The tester accepts the
occasional magazine-count reset during AE re-equip. Total ammunition
integrity, animation behavior, other weapons, and broader regression
remain unverified; release/merge remains blocked.

The unreleased 0.5.208 follow-up responds to focused AE 1.11.240
testing: the 0.5.207 exact-stack unequip returned false and the user
observed that the weapon was unequipped without automatic re-equipping.
The fallback now requests the matching equip even on a false unequip
return and checks actual equipped state afterward. Generated menus hide
the vanilla internal universal range offset category, preserving the
underlying OMOD and graph. This revised tester has not passed in-game
QA; leave the fallback opt-in and OG native refresh unchanged.

The unreleased 0.5.207 experiment adds opt-in automatic re-equip
after verified attachment changes only on AE 1.11.240; the OG refresh
remains unchanged. The fallback targets the exact equipped stack and
guards loaded-ammo restoration. It is disabled by default and not
in-game validated. No release or automatic game deployment is authorized.

The unreleased 0.5.206 safety candidate fixes the invalid AE post-mod
refresh call identified in an actual Addictol crash report. On Fallout 4
1.11.240 the used Address Library ID 1153963 is absent and lookup
resolves into non-executable data. The guarded candidate skips that
instant visual refresh on AE while retaining normal OMOD transactions;
the 1.10.163 path is gated by exact version and executable section.
Focused in-game QA on 1.11.240 now confirms the shotgun sight change is
verified without crashing and the invalid refresh is safely skipped.
Switching weapons updates the visible model. This is not proof of
original-runtime compatibility or full attachment regression success.
A native AE-safe automatic weapon model refresh remains unresolved.

The unreleased 0.5.205 radial presentation follow-up extends the
six-option page limit to keyboard/mouse mode, adds center pager buttons,
mouse wheel/Page Up/Page Down navigation, and shows the full hovered
attachment name. The earlier 0.5.204 paging applied only to controller.
In-game UI QA is pending; the receiver crash remains unresolved.

The unreleased 0.5.204 diagnostic/layout tester follows a 10mm vanilla
Advanced-to-Rapid-Automatic receiver swap that verified the actual OMOD
transaction but crashed after the UI update. The exact crash instruction
remains unknown. It adds flushed stage markers around the post-mutation
UI and existing equipped refresh, without altering either operation.
Controller radial option labels were overlapping; this tester uses a
six-entry paged ring with LB/RB paging and a full-name caption.
Build, runtime safety and presentation validation are pending.

The unreleased 0.5.203 controller refinement keeps native controller
navigation and its opener exclusive to the Quick Menu. Builder and Settings
remain keyboard/mouse. Both analog sticks are supported for radial pointing,
preferring left when active; A enters/accepts, B backs out/closes. Analog events
are sampled on the native input thread and delivered to Prisma on the game
thread. The 0.5.202 default material guard and 0.5.201 recipe discovery remain
unchanged. Build and target-runtime analog QA are pending.

The unreleased 0.5.202 tester addresses repeated in-game crashes after
selecting vanilla "No Material" on a 10mm that already had no material OMOD.
It adds UI-only effective-default classification for a unique zero-effect
material default and rejects replacing existing material paint with such a
record until the proper workbench operation can be validated. No synthetic
installed identity, ordinary attachment mutation, or equipped-slot refresh
change is made. The controller/recipe improvements from 0.5.201 remain.
Build and in-game validation are still required.

The unreleased 0.5.201 follow-up adds recipe-backed generated workbench
discovery and fixes controller focus and binding-editor consistency. It does
not change normal attachment mutation or equipped-weapon refresh behavior.
Exact workbench parity and runtime QA remain unverified.

The unreleased 0.5.200 controller-only candidate starts from the proven
0.5.199 attachment/inventory implementation. It adds PrismaUI V12 controller
navigation and independent configurable XInput controller menu-opening
shortcuts, including single buttons or two-button combinations. Controller
shortcuts default to unassigned and do not change keyboard/MCM bindings.
No cheat-mode capability or altered attachment transaction/refresh path is
included. Build, packaging, and target-runtime controller testing are pending.

Version `0.5.199` was the previous stable GitHub release. It allows a
provider attachment to be installed into an empty, live-reachable attachment
point only when the hypothetical graph does not activate an existing unreachable
attachment. The transaction requires the unrelated installed OMOD identities to
remain unchanged, verifies the provider's attachment points after installation,
and retains the existing inventory, rollback, and live revalidation checks.
The clean Linux releasedbg build and static validation passed. Focused external
testing confirmed the reported SREP barrel-to-empty-muzzle path succeeds without
using **Force unsafe swaps**; the broader attachment regression matrix was not
independently repeated for this release.

Version `0.5.198` keeps the three
known `TacticalReload_IngameSwitch.esp` infrastructure OMODs internal and out of
dependency-removal planning, and adds a stable WEAP-keyed **Force unsafe swaps**
override for dependency-cycle refusals only. The override defaults off and is
not included in distributable weapon menu profiles. The clean Linux releasedbg
build and static validation passed. Focused external testing confirmed that the
reported Tactical Reload false **Attachment unable to swap safely** case is
resolved; the broader attachment regression matrix was not independently
repeated for this release.

## Release history

Version `0.5.197`, the previous public release, adds a persistent
diagnostic-logging toggle and a shared
0%-100% gameplay slowdown setting for the Quick Menu, Builder, and Settings
pages. Zero keeps normal time, intermediate values slow proportionally, and 100
fully pauses. The implementation preserves external time changes instead of
blindly restoring a stale multiplier. It also reapplies saved visibility after
a successful Cascade attachment change so hidden categories and attachments do
not reappear in the refreshed menu. The cross-runtime build dependency is now
pinned so an incompatible CommonLibF4 revision fails during configuration
instead of producing an invalid DLL. It also verifies cursor ownership after
PrismaUI focus and uses a validated OG/AE registration fallback when the
provider did not establish an engine cursor owner. Internal Builder/Settings
replacement releases the old fallback state before focusing the new page so
each view reacquires cursor ownership.

Focused runtime testing on Fallout 4 `1.11.240` passed for unrestricted cursor
movement in Quick Menu, Builder, and Settings; the logging toggle; the complete
slowdown range from normal time through full pause; and retained Cascade
visibility after attachment changes. The release retains its original-game
code and build path, but version 0.5.197 was not runtime-tested on Fallout
4 `1.10.163`.

Version 0.5.192 was the previous public release. It keeps the 0.5.181
generated-menu compatibility model and adds validated MODCOL container handling,
generated no-loose attachment support behind the existing safety setting,
game-thread hotkey/menu actions, cross-runtime OMOD container reads, dialogue
open blocking, and corrected Mouse 4/5 toggle and switching state under
Proton/PrismaUI.

Version 0.5.181 loads and opens both Prisma menus on
Fallout 4 1.10.163, and Prisma Dock registration is confirmed in runtime QA.
Runtime testing also proved that both prior generated-menu heuristics were
wrong: source/plugin-family filtering hid valid choices, while using CommonLibF4
`filterKeywords` as the target gate admitted unrelated weapon attachments.
The latter field is OMOD FNAM, not the authored MNAM Target OMOD association.

Version 0.5.181 parses raw MNAM from each winning OMOD plugin record and
uses it with the reachable attachment graph. MNAM-targeted OMODs must match a
keyword on the equipped base WEAP; OMODs with no MNAM are generic for their
reachable attachment point; unresolved MNAM metadata fails closed for
uninstalled candidates. Runtime QA then exposed a second presentation issue:
no-loose-mod Legendary/helper OMODs were being listed as generated choices.
Version 0.5.192 keeps generic no-MNAM no-loose records out of generated
discovery, but allows legitimate uninstalled no-loose choices when
`AllowNoLooseModOptions` is enabled and the OMOD has an explicit MNAM target
matching the equipped weapon. Already-installed no-loose OMODs remain visible as
live state.

Builder shows every compatible player-facing attachment regardless of inventory.
Quick Menu shows installed attachments, compatible loose mods currently carried
by the player, and validated no-loose choices that pass the explicit-target
safety rule. Focused regression passed for generated compatibility, Prisma Dock
discovery, MODCOL weapons, legitimate no-MISC/no-loose attachment changes,
same-hotkey open/close, Quick Menu ↔ Builder switching, dialogue blocking, and
the prior hotkey-thread crash path. Version 0.5.192 has been exercised on
both 1.10.163 and 1.11.240 runtime families.

Version 0.5.180 passed target-runtime testing for all four presentations,
supported attachment changes including Mount/Sight and Lower Rail/Laser
provider-child paths, close/reopen, Alt-Tab, save/load, and normal exit.

Unsupported or ambiguous attachment operations remain individually blocked by
live validation. See `docs/KNOWN_ISSUES.md` for the current boundary.
