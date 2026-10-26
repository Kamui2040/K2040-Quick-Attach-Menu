# Known Issues and Limits

## 0.5.221 experimental base-weapon 3D preview (not yet runtime-tested)

PrismaUI_F4 ModelPreview API v4 (introduced in 2.2.0) loads a native NIF
into a view. The first 0.5.221 in-game screenshot showed an empty preview.
Live PrismaUI logs showed `meshes/Meshes/Weapons/...` being read (duplicate
prefix), and WEAP model records supplied non-drawable receiver dummy meshes
(e.g. `10mmRecieverDummy.nif` and `DummyReciever.nif`).

The revised optional Horizontal Bar preview reads only the equipped weapon's
live installed receiver OMOD when its WEAP model is an empty dummy, choosing a
verified, unambiguous drawable receiver NIF where available. It sends the
normal documented path first, retries the relative path once on failure and
exposes native status errors if both fail. This is a BASE/RECEIVER component,
not an engine-assembled weapon with installed attachments, material swaps, or
connect-point transforms. Unavailable/ambiguous models safely stay text-only.

The next screenshot confirmed that the receiver geometry draws, but appears
nearly white. PrismaUI logged six NIF shapes with normal/diffuse texture paths,
and native engine texture SRV resolution. The material/shading discrepancy is
not resolved. A separately opt-in installed-parts multi-NIF comparison and a
35% brightness comparison are now included for controlled in-game diagnosis;
they do not recreate the workbench's attach-node transforms, material swaps,
or assembled model and may produce overlapping parts. These are not release
acceptance evidence.

More recent target-game screenshots show the installed-parts toggle can
render a recognisable weapon and some material details. The user also reported
that the model was stretched horizontally and continuously rotating; the
Horizontal Bar option strip displayed only the initial attachments and could
not reach later offscreen choices. The preview now requests zero native spin
and uses a narrower destination aspect ratio. Option-strip paging buttons,
wheel scrolling, keyboard/controller focus reveal and redraw scroll persistence
are static-tested with 12 options at normal and scaled sizes. The maintainer's
r4 gameplay screenshots subsequently confirmed the model stopped spinning,
that the attachment strip could page, and that some equipped-part textures
appeared. The top category strip still clipped later categories, and W/S did
not match horizontal row geometry. The next candidate adds category paging,
wheel scrolling, scroll-to-category and navigation with Left/Right along the
active row and Up/Down between the two rows; the maintainer confirmed r5
navigation works in-game. E confirmed attachment changes successfully in
r6 user testing; Space did not. r7 removes Space as a confirmation shortcut,
leaving its prior inert/intercepted Quick Menu behavior intact. The r7 change
still needs a focused in-game confirmation and held-key/opener check.

The previous custom renderer failed Alt-Tab lifecycle testing. This revision
still requires fresh game-level model loading, rotation, Alt-Tab, close/reopen,
weapon switching, and compatibility QA before release or default enablement.
No player's MCM settings or installed attachments are changed.

## Fixed in 0.5.219: opener-close input leak

A shortcut overlapping a vanilla game command could also trigger the vanilla
action when it closed Quick Menu or Builder. Quick Attach now retains only its
own gameplay-input layer until the shortcut's base key is physically released,
with a bounded timeout. The maintainer verified the 0.5.219 correction in-game
on OG 1.10.163. Wider runtime/mod-input compatibility is not exhaustively
verified.

## Fixed in 0.5.220: native MCM keybind writer conflict (user-tested)

User runtime QA exposed an independent live MCM manager and disk registry
competing over Quick Attach bindings. Settings Reset wrote defaults directly
into `Data/MCM/Settings/Keybinds.json`, then native MCM saved its earlier
in-memory binding when closing MCM, making previous assignments reappear.
Clearing with Tab could then leave Quick Attach's native poller and MCM out of
sync. The earlier Settings sync also called `root.mcm.SetKeybind`, which
registers, rather than remaps, a binding in upstream MCM.

The published 0.5.220 implementation uses native MCM exclusively for live keybind
changes. Settings stages a request until PauseMenu is available, checks the
original disk and live binding, calls `root.mcm.RemapKeybind`, verifies via
`GetKeybind`, then lets MCM serialize its own registry when its menu closes.
Settings shows unapplied changes as pending; neither Set nor Reset directly
rewrites the native MCM file. Previously cleared MCM entries are not re-added
on startup; MCM must restore them before Settings can remap them. A failed
remap does not modify the registry. The maintainer tested the revised
single-writer candidate in-game and reported PASS on 2026-10-25. This confirms
the reported conflict is fixed in the tested setup, not every possible MCM
provider or key combination. Version 0.5.220 is published on GitHub and maintainer-confirmed on Nexus.

## 0.5.220 Settings keybind capture on modifier press (runtime QA pending)

New in-game report: CEF also returned keycode `144` with Shift/Ctrl/Alt,
which the Settings browser recorder accepted as the final key; Windows VK 144
means NumLock, not Shift. The resulting saved chord did not trigger correctly.
The browser recorder now ignores this synthetic code and other lock/modifier
codes. The native Settings save API also rejects these before touching the
shared registry, even if the browser erroneously submits one.

The earlier implementation wrote `Keybinds.json` while native MCM held a
separate live registry, which could cause stale displays or old assignments to
return. The 0.5.220 single-writer candidate addresses that conflict and was
reported working in-game by the maintainer on 2026-10-25. This paragraph
records the historical cause, not a remaining blocker for the tested setup.


A further user test showed that ignoring ordinary modifier keycodes alone is
insufficient: PrismaUI/CEF may also report left/right Shift/Ctrl/Alt as Win32
codes 160–165, and the native MCM registry write can fail when the file is
open. The revised candidate ignores physical modifier-only events and, if
atomic file replacement is blocked, uses a byte-verified recovery-backed write
with error reporting. No result is accepted without game-level retesting.


The Settings page previously filtered Shift/Ctrl/Alt/Meta only by numeric
keyboard codes. PrismaUI/CEF can deliver a modifier-only event with its name
but `keyCode=0`, which the page accidentally committed as a new binding. The
native side then rejects the invalid key and refreshes the saved assignment,
making capture appear to cancel on Shift. Recognize modifier names and physical
codes, ignore unknown keyboard codes, and keep waiting for a real final key or
Mouse 4/5 click. Native MCM capture and the shared registry stay unchanged.
Validate in-game for both keyboard+modifier and mouse+modifier combinations.

## 0.5.220 test candidate — duplicate Menu Builder bindings (runtime QA pending)

A live native MCM `Keybinds.json` contained multiple Quick Attach
`openMenuBuilder` entries: an older Unbound record and newer working mouse
bindings. MCM/Prisma lookup was reading the first record and showing None,
while the native registry parser applied the last one and allowed the action.
The Graphify-guided 0.5.220 follow-up also uses one scanner for the
native MCM keybind registry, rejects malformed arrays and nested-key
impostors, and aligns displayed bindings with the last valid saved key.

The 0.5.220 follow-up reads and edits the last entry consistently, and
reconciles older duplicates on startup by preserving the last valid saved
assignment and retaining other mods' registry records untouched. This is a
settings-file repair and requires in-game retesting; never overwrite or reset
user-assigned keys as a workaround.

## 0.5.218 unreleased MCM keybind registration regression

The 0.5.215 MCM parent/child integration changed both native MCM keybind
`action.type` definitions from `RunConsoleCommand` to `CallExternalFunction`.
Unlike settings-page external callbacks, native MCM's keybind deserializer does
not recognize `CallExternalFunction`, so registration fails and capture can
briefly display a new binding before reverting to None. Revert the registry
definitions to the original supported inert command while retaining native
physical input handling. The 0.5.217 seeding-only candidate failed focused
in-game testing. A stale initialization marker can independently preserve a
previous Unbound choice; do not silently overwrite deliberately unbound keys.
After resetting this mod's saved settings and using the native-supported action
in 0.5.219, the maintainer confirmed that binding and close behavior worked
in-game; the shared registry and other mods' entries remain separately owned.


## Stable GitHub release 0.5.216 dynamic target keyword fix

A Service Rifle user reported that RIS rail equipment remains visible while
installed but disappears from the generated menu after removal. The diagnostic
log identifies a target MNAM keyword present on the weapon's live instance but
not on the base WEAP. Version 0.5.216 requires positive Add Keywords
property evidence from a currently installed, reachable OMOD before it uses
a live-only keyword for generated menu discovery. Removed or unproven keywords
still fail closed. The pinned Linux cross-build, regression suites, and
Windows x64 packaging passed. The original reporting user retested the RIS
attachment rediscovery bug and confirmed that this specific issue is fixed
in-game. This focused user-tested result does not establish exhaustive
OG/AE, Tactical Reload, or unrelated weapon compatibility. The validated
stable ZIP and checksum were promoted on GitHub on 2026-10-20 after the
maintainer reported final gameplay checks passed. The maintainer subsequently
published version 0.5.216 on Nexus Mods as well.

## 0.5.180 generated-menu compatibility

Version 0.5.180 can admit unrelated OMODs when generic attachment points overlap
between weapon families. Earlier 0.5.181 candidates incorrectly tried to infer
the weapon-family gate from CommonLibF4 `filterKeywords`, which exposes OMOD
`FNAM` rather than the authored Target OMOD Keywords in `MNAM`.

The current hotfix reads raw MNAM from the winning OMOD plugin record. A
generated OMOD requires a reachable attachment point and, when MNAM targets are
present, a matching keyword on the equipped base WEAP. OMODs with no MNAM
targets remain generic for their attachment point. Unresolved MNAM metadata
fails closed for uninstalled candidates.

Generated discovery still rejects generic no-MNAM uninstalled OMODs that have no
linked loose-mod item. Runtime QA showed that otherwise internal DLC/helper and
Legendary-effect records can leak into player-facing choices. When
`AllowNoLooseModOptions` is enabled, an uninstalled no-loose OMOD with an
explicit resolved MNAM target matching the equipped weapon is allowed. Already-
installed no-loose OMODs remain visible so live weapon state is not lost.

## 0.5.180 false ambiguous replacement state

Version 0.5.180 can count disabled object-instance OMOD entries as installed,
which can make a valid replacement fail as ambiguous. The 0.5.181 hotfix
candidate ignores disabled entries for installed identity.

## 0.5.215 MCM child and theme work (included in 0.5.216)

- Native MCM supports ownerModName merging. Quick Attach now bundles
  the canonical shared K2040_Mods parent with its child MCM configuration.
  No separately downloaded Hub or alternate MCM layout is needed.
  Amethyst may warn about the same parent file supplied by another
  K2040 mod; this is expected only while the files are byte-identical.
- F4SE Menu Framework accepts standard MCM files but its translation
  source has not demonstrated equivalent ownerModName grouping. Treat
  its grouping and external-function dispatch as unverified until tested.
- Before the Minimap is converted to its own child namespace, its old
  experimental parent settings may exist. Quick Attach reads them only
  to avoid known simultaneous controller chords; this is not a complete
  cross-mod dispatcher for all future mods.
- The maintainer reports the bundled MCM integration working in-game
  for Quick Attach Menu in their tested setup. This confirms focused
  module-level acceptance, not exhaustive keyboard/controller remapping
  or compatibility across all runtime and input configurations.
  A later in-game report confirms Quick Attach, Prisma Minimap and
  Quest Journal all register under the single K2040 Mods parent.
  Page registration passes for this combination. The Minimap and
  Quest Journal still need separate bug fixes; no Quick Attach
  regression was reported. F4SE Menu Framework compatibility and
  other modules' actual hotkey behavior remain unverified.
- Cheat mode remains Experimental. Use at your own risk, including the
  accepted loose-mod return and AE gameplay-effect refresh limitations.

## 0.5.214 Cheat mode work (included in 0.5.216)

The optional global Cheat mode allows otherwise-compatible attachments
to be installed without carrying loose mods or crafting their component
requirements. Normal inventory checks return when the setting is off.
No loose mods are consumed or created in cheat mode, including removals
of previously installed attachments.

It still blocks incompatible/missing attachment points, unresolved
provider-child relationships, scripted/internal OMODs, dangerous material
resets and unavailable loaded attachment records. The generated menu
still requires the same recipe and weapon-association discovery evidence;
it does not remove recipe/keyword validity checks merely because the
player wants to avoid obtaining the item. The menu does not perform
ordinary workbench crafting, so it does not consume components in
normal mode either.

Automatic inventory consumption during engine mutation is compensated
only for an exact single item, followed by live count verification;
unresolved counts fail with a rollback attempt. Build/static checks
cannot by themselves establish runtime success. Focused user testing
on AE confirms Cheat mode works for the tested weapons and the normal
loose-mod requirement returns when disabled. Compatibility across
additional weapons, provider/dependent relationships, material defaults,
inventory states and game configurations can benefit from public tests.
The maintainer subsequently reported that the broader weapon,
attachment and original-game compatibility checks they performed
also passed. Treat this as maintainer acceptance of the tested scope,
not exhaustive confirmation for all weapons, dependencies, inventory
states or installed mod combinations. Broader public testing remains
the next compatibility confidence step.

### Cheat-installed attachments removed after disabling cheats

An attachment installed free while Cheat mode is enabled becomes a
normal equipped OMOD. If the player then disables Cheat mode and
replaces that attachment, the normal return-item path can add its
associated loose mod to inventory, even though the player never
owned that item. Focused in-game testing confirms this sequence.

This is an accepted, non-blocking Cheat mode side effect. When Cheat
mode is enabled, its own attachment changes continue to avoid loose
mod consumption and returns. To avoid receiving the free returned
item, remove or replace the cheat-installed attachment while Cheat
mode is still enabled, or discard the returned loose mod afterward.
The runtime currently does not persist whether each installed weapon
attachment was acquired using Cheat mode. Preventing this across
mode changes and saves would require per-weapon-instance provenance
and more invasive return handling, with separate migration/runtime
testing. No such behavior change is part of this tester.

### AE visual refresh versus attachment effects

On Fallout 4 AE 1.11.240, the optional post-close automatic re-equip
can update the visible attachment model without immediately activating
the attachment's gameplay behavior. The reported example is a recon
scope on the 10mm pistol: its model appears but aiming still uses
ordinary sights until the player switches to another weapon and back.
The user reports similar behavior with a suppressor.

**Workaround:** switch to another weapon and back after installing
an attachment if its effect has not taken hold. The tester accepts
this limitation for release. The AE refresh path is a likely area
to investigate, but the exact cause has not been established.
Do not claim that the visible model proves scope or suppressor
functionality was refreshed. The existing automatic re-equip only
addresses the presentation of changes; it does not reliably refresh
every dependent gameplay state. Future fixes require separate AE
runtime evidence, and must not reintroduce the known invalid
immediate-refresh call.

## Unreleased 0.5.213 orientation-aware controller focus

A tester reports Builder's horizontally arranged top actions must be
navigated using Up/Down in the initial 0.5.212 tester. The revised
shared focus helper uses Left/Right within the Builder header and
weapon-setting row, with Up/Down moving between rows. Vertical
categories, entries and dropdowns retain Up/Down; Settings two-column
theme/presentation choices and horizontal confirmation controls use
their displayed axes. LT is a configurable Quick Menu opener button
or modifier only; it is not mapped to menu navigation.

Automated input simulation confirms the directional model without
triggering changes merely from focus movement. Focused in-game testing
reports the 0.5.213 orientation fix works on the tested controller.

LT may advance keyboard focus like Tab in Builder and Settings,
while it still aims in normal gameplay. This input behavior is
acceptable and needs no change for release. Quick Attach Menu binds no
LT focused-view navigation action; it only offers LT as an optional
Quick Menu opener/modifier. The source of Tab translation remains
unverified. Ensure LT does not unexpectedly activate a destructive
control if wider controller regression identifies such an effect.
Release publication remains separate.

## Unreleased 0.5.212 Builder and Settings controller focus

Both pages now use a shared controller focus helper with mapped
Confirm/Cancel, sticks and D-pad, pane/tab movement, and dedicated
dropdown and dialog interaction. No dropdown value is committed by
highlight movement; Back closes the open dropdown without saving.
The Builder's visibility commands require an explicit Confirm/secondary
face-button action. Reset Everything stays confirmation-gated.
Settings slider edits are entered using Confirm.

Node event simulations and static contracts are available, but focused
in-game Prisma input and modal/dropdown testing is still required.
Keyboard hotkey capture, rename, hex colour text, and mouse drag-based
Builder reordering are not fully controller-operable in this iteration.
Do not claim complete controller-only editing or release readiness.

## Unreleased 0.5.211 unified navigation and Compact Hybrid

A user reported that controller navigation in Compact Hybrid felt
inconsistent and its D-pad Up/Down actions moved focus in the opposite
visual direction around the category semicircle. The wheel draws
categories from the bottom toward the top, unlike list layouts.
The shared 0.5.211 navigation model reverses only hybrid category
traversal, maps both thumbsticks and D-pad through the same direction
router, and gives Left/Right, Cancel and shoulder buttons consistent
pane/category/page behavior in every presentation. No movement triggers
attachment installation. Browser events and the real renderer have
automated tests; initial user in-game testing reports the navigation
working as far as tested. This is a focused provisional result, not a
complete verification of every presentation, controller mapping or
repeat/transition case.

## Unreleased 0.5.210 controller input refinement

Controller movement now highlights Quick Menu categories/options using
either analog stick or D-pad in every presentation. The browser never
installs an OMOD from movement: the player must separately press the
game-mapped Activate/Confirm button; fallback is menu Accept, then A.
Cancel is read from the game's menu mapping where available. Thumbstick
neutral input clears the remembered angle. D-pad use takes temporary
priority over a held stick to avoid the old radial angle overriding it.

This passed native static checks and deterministic browser action tests.
The user also confirmed the 0.5.210 controller behavior works in their
in-game setup. This is focused confirmation, not exhaustive evidence
for remapped confirmation buttons, navigation across every layout/page,
all repeat behavior, or broader Steam Input/Proton configurations.
This describes the 0.5.210 state; 0.5.212 adds Builder/Settings
navigation while keeping text editing and drag operations mouse/keyboard.

## Unreleased 0.5.209 AE repeated-modification refresh

A user report on 0.5.208 confirmed automatic re-equip works for
individual attachment changes, but repeated modifications inside one
Quick Menu session can leave the visible model reflecting an earlier
change. The live game log recorded eleven successful guarded attachment
transactions and eleven full AE re-equip completions; it does not prove
that each visual model finished loading before the next action.

The 0.5.209 experiment queues only one exact-stack re-equip for the
latest successfully verified state, after Quick Menu close and Prisma
focus release. It preserves ammo safeguards and rejects stale menu,
game-transition or installed OMOD snapshots. Focused user testing of
0.5.209 on AE 1.11.240 reports that repeated attachment changes now
become visually current after closing the Quick Menu. This closes
the reported multi-change visual symptom only.

The tester also reported that loaded ammunition can reset during
automatic re-equip and explicitly accepts that magazine-state trade-off.
The code retains its guarded attempt to restore the prior loaded count;
that attempt is not a guarantee. The ordinary magazine reset is not
itself a release blocker, but unintentional reserve ammunition loss or
duplication, animation problems, crashes, and broader weapon compatibility
remain unverified and are not accepted by that decision.

## Unreleased 0.5.208 AE re-equip and hidden range offset

Focused 0.5.207 in-game testing on AE 1.11.240 reported that the
weapon became unequipped without automatic re-equip. The plugin log
records UnequipObject returning false and immediately aborting the
paired equip. That return value does not establish the final equipment
state. The 0.5.208 experiment now attempts the matching exact-stack
equip even when the initial return was false and rechecks live state.
A fresh game test is required before claiming this actually restores
the weapon or preserves ammo/animations. Do not use 0.5.207's enabled
re-equip tester on a permanent save.

The same session showed an internal universal shotgun range-offset
OMOD exposed as a selectable generated-menu category despite being
absent from the workbench. The generated catalog now hides that
internal AP while leaving its installed identity and graph intact.

## Unreleased 0.5.207 AE re-equip experiment

ECO Redux documents that its native weapon-model refresh can re-equip the
gun and refill the magazine. This optional AE-only tester applies the same
general principle without copying ECO code. Exact-stack re-equip and loaded
ammunition restoration use defensive checks, but animation and ammunition
safety remain unverified in-game. The fallback is off by default.

## Unreleased 0.5.206 verified AE post-modification crash

Addictol Crash Logger captured an access violation executing
Fallout4.exe+24E2BE8 after a verified vanilla Double-Barrel Shotgun
sight replacement. The old refresh call used Address Library ID
1153963, which is absent on game version 1.11.240. Lookup reaches
the adjacent 1153964 at exactly the non-executable crash address.
The return address in the plugin disassembles immediately after
the indirect equipped-refresh call. This explains the previously
unresolved post-transaction AE crashes without assuming a bad save.

The candidate intentionally skips this unsafe visual refresh on AE,
while preserving the verified attachment operation. Its original
1.10.163 path requires an executable target. Players may need to
re-equip the weapon to see the new model. A focused in-game 1.11.240 test subsequently installed
Double-Barrel Shotgun standard sights successfully, verified the new
object-instance OMOD, and logged the incompatible visual refresh
being skipped. The game did not crash. The visible shotgun only
updated when switching weapons. This validates the crash avoidance
for that test, not every weapon or the original-game runtime.

A supported, low-impact immediate visual refresh remains unresolved.
CommonLibF4 exposes AE relocation IDs for actor/equipment management
and queued 3D updates, but these are not proven equivalent to a
weapon-only refresh. Automatic re-equipping may alter ammo, animation,
or equipped state; resetting an entire actor's 3D is too intrusive
to apply speculatively. Do not replace the known-bad pointer with
an untested or cross-runtime relocation.

## Unreleased 0.5.205 ordinary radial label overlap

A screenshot from 0.5.203 showed overlapping receiver labels in
keyboard/mouse radial mode. The 0.5.204 pagination only covered
controller radial, not the ordinary mouse interface. The 0.5.205
candidate extends the six-entry page cap to mouse radial with
center paging arrows, wheel/Page Up/Page Down controls, short
wedge labels and a full-name hover caption. All options remain
reachable without changing their install behavior. Target
in-game visual QA is pending; the receiver crash remains unresolved.

## Unreleased 0.5.204 receiver crash and dense radial

Version 0.5.203 logged a successful and verified vanilla 10mm receiver
transition from Advanced Receiver to Rapid Automatic Receiver before the
game crashed after submitting the updated Prisma payload. Without a crash
call stack, the exact failing instruction remains unknown. Version
0.5.204 adds flushed markers before and after the existing payload
submission and equipped-item refresh, without changing either operation.
It is a diagnostic candidate, not a crash fix.

With many receivers, controller radial option labels overlapped. The
option ring now shows at most six choices per page, preserves all choices
across pages via LB/RB, shortens wedge labels, and displays the full
selected label in the wheel center. The mouse layout remains unchanged.
In-game presentation and control tests are still required.

## Unreleased 0.5.203 Quick Menu controller refinement

Previous controller navigation in Builder and Settings was not intuitive
enough for those complex panels. They now remain keyboard/mouse-only until
their layouts are deliberately redesigned. The retired controller Builder
opener is ignored without altering keyboard/MCM shortcuts. Quick Menu uses
the Prisma V12 controller button bridge, plus sampled XInput left/right
stick angle for radial selection. A enters a category and then confirms an
attachment; B backs out and then closes. The controller radial outer
options occupy a full ring, independent of category direction, and D-right
no longer enters the outer ring.

Static direction/activation tests and the cross-build do not establish
behavior on a real Steam Input/Proton controller. Specifically validate
dead-zone stability, A/B stage transitions, stick priority, button mapping,
mouse coexistence, and ordinary Cascade navigation before merging.

## Unreleased 0.5.202 default material safety

A 0.5.201 game session on a 10mm confirmed no previously installed material
OMOD, yet selecting the vanilla No Material OMOD 0024A0D9 attached it to the
object-instance stack and the game crashed after the transaction verified and
the UI payload was sent. The engine refresh remains a probable but unproven
crash site; no crash stack is available. A unique unloaded, zero-effect
material default is now presentation-only "already applied" when the installed
material point is empty, and the selection is rejected before mutation.
Attempts to replace a different installed material with a no-effect reset
OMOD remain disabled pending verification of a safe workbench-equivalent
removal path. This does not prove the overall crash is fixed.

## Unreleased 0.5.201 follow-up

Static and build validation are required for the controller focus/navigation
fixes and recipe-backed generated-OMOD filter. The filter preserves installed
OMODs but requires a COBJ recipe, explicit matching MNAM, and reachable AP for
new generated choices. This is deliberately stricter than previous behavior;
mods that offer loose-only choices without a COBJ may need a separate
workbench-compatibility path. Bench-specific recipe and perk/condition
visibility are not yet modeled precisely. Controller interaction, attachment
catalog accuracy and crash regression all require focused game testing.
The previous equipped-weapon presentation refresh is not modified by this
candidate; no cheat-mode code has been reintroduced.

## Unreleased controller-only 0.5.200 candidate

The controller-only candidate preserves 0.5.199 attachment mutation, inventory,
and equipped-model refresh behavior. Unlike the discarded cheat-mode candidate,
it does not permit inventory-free swaps. Controller navigation and independent
single-/two-button XInput opener shortcuts are not yet in-game validated.
Steam Input mappings and gamepad focus behavior require target-environment QA.

## Unsupported attachment operations fail closed

Ambiguous inventory stacks, unresolved forms, dependency cycles, stale menu
state, and any operation that cannot be completely verified remain unavailable.
Supported leaf, provider-install, and provider-replacement paths continue to use
live operation-level checks, exact inventory verification, post-change
verification, and rollback.

The unreleased 0.5.199 candidate allows a provider attachment to be installed
into an empty, live-reachable attachment point. It remains blocked if the
hypothetical graph would activate an existing unreachable attachment or if the
transaction changes unrelated installed OMOD identities.

Version 0.5.198 recognizes the three known
`TacticalReload_IngameSwitch.esp` infrastructure OMODs without relaxing normal
dependency ordering. It also adds an off-by-default per-weapon override for
dependency-cycle refusals. Focused external testing confirmed that the reported
Tactical Reload false safety block is resolved; the broader attachment
regression matrix was not independently repeated for this release.

## This release is text-only

Inventory and workbench icon tags are not a reliable semantic source for every
weapon attachment category. Version 0.5.180 therefore removes the experimental
icon resolver rather than showing misleading generic icons. Weapon preview was
also removed after its renderer lifecycle failed Alt-Tab safety testing.

Future icon work requires a mod-owned semantic attachment database, explicit
adapters for compatible installed icon libraries, and optional per-weapon
Builder overrides. Neither future feature affects the current menu's core
attachment behavior.

## 0.5.197 original-game runtime is unverified

Version 0.5.197 keeps the gameplay input layer active while the quick
menu, Builder, or Settings page has focus and adds a 0%-100% slowdown range.
Zero leaves the captured game-time multiplier unchanged; 100 fully pauses;
intermediate values scale it proportionally. Focused Fallout 4 `1.11.240`
runtime testing passed for the new features and the unrestricted cursor fix in
all three views. This release was not runtime-tested on Fallout 4
`1.10.163`; its original-game compatibility path is retained but unverified.

## Runtime validation

Version 0.5.192 has focused runtime coverage on Fallout 4
`1.10.163` and `1.11.240` with matching F4SE and compatible PrismaUI_F4. Other
runtime/framework combinations have not passed the same regression and remain
unverified.

Version 0.5.197 has focused runtime coverage on Fallout 4 `1.11.240` for the
new settings, Cascade refresh, cursor ownership, and all three views.
