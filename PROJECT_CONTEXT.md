# Current Project State

- Current stable GitHub release: `0.5.216`
- Previous stable GitHub release: `0.5.199`
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
Both GitHub and Nexus now offer version 0.5.216.
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
