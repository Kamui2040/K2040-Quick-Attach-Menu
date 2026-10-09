# Architecture

## Runtime source roles

| Source | Authoritative role | Not authoritative for |
|---|---|---|
| ECO FormLists/Messages | authored player-facing categories, entries, labels, order | installed state, structural validity |
| Equipped `WEAP` | weapon/root identity and base attachment-parent data | complete installed state |
| Default-template OMODs | default/provider graph inputs | current installed state |
| Object-instance OMODs | current installed OMOD identity | authored menu membership |
| OMOD/AP records | structural validity and dependency ordering | authored labels/order |
| DN/state keywords and EditorIDs | diagnostics | installed identity |
| xEdit/exported JSON | development research | runtime behavior |

The runtime model is source-neutral: authored presentation and structural validity are separate concerns.

## Read-only pipeline

```text
equipped weapon
  -> equipped WEAP + instance data
  -> base/default graph inputs
  -> equipped inventory stack
  -> resolved object-instance OMOD forms
  -> authored ECO FormList/Message entries
  -> installed marking by OMOD form identity
  -> AP/provider validation
  -> filtered read-only menu state
```

## Identity rules

- In-session equality uses loaded form identity/runtime FormID.
- Persisted identity uses source plugin plus local FormID/FormKey.
- Display names, EditorIDs, and load-order-dependent full FormIDs are not sufficient persistent keys.

For an authored option with OMOD `O`, installed state is true only when the resolved object-instance OMOD collection contains `O`'s loaded form identity.

Installed state does not by itself imply that an option is visible, selectable, inventory-available, or structurally valid.

## AP/provider rules

- Structural validity requires the consumed attachment point to be reachable.
- Installed providers contribute their provided attachment points to live reachability.
- Provider logic stays generic rather than hard-coding weapon-specific cases.
- Install providers before children.
- Remove children before providers.
- Ordering keywords may help diagnostics or presentation but are not availability truth.

## Player-facing filtering

An option may be exposed only when it belongs to the selected authored structure and passes the applicable internal/hidden, target, structural/provider, and inventory rules.

Generated menus use the OMOD record's authored Target OMOD Keywords (`MNAM`)
for weapon-family compatibility. CommonLibF4's exposed `filterKeywords`
correspond to a different OMOD field (`FNAM`) and are not used as a substitute
for MNAM. Because CommonLibF4 does not expose MNAM structurally, the runtime
reads MNAM from the winning loaded plugin record and resolves those keywords to
the live form table.

An uninstalled generated OMOD must consume an attachment point reachable from
the weapon graph. If its winning OMOD record has MNAM targets, at least one must
be a keyword on the equipped base WEAP. If the record has no MNAM targets, the
OMOD is generic for that reachable attachment point. If MNAM metadata cannot be
resolved safely, the uninstalled candidate fails closed. Installed OMODs remain
visible from live object-instance identity. Source plugin is not a compatibility
gate because patches and add-ons may validly extend a weapon from another
plugin.

Generated menus expose player-facing workbench choices, not every reachable
OMOD record. Linked loose-mod items remain the normal generated path. When
`AllowNoLooseModOptions` is enabled, an uninstalled no-loose OMOD may also be
exposed if it has an explicit resolved MNAM target matching the equipped weapon.
Generic no-MNAM no-loose records stay out of generated discovery because they
cannot be distinguished safely from internal, scripted, legendary-effect, or
helper records. Already-installed no-loose OMODs remain visible as live state.

The Builder enumerates all compatible player-facing generated attachments under
those rules whether or not their loose-mod items are in inventory. The gameplay
Quick Menu uses the same compatibility catalog but suppresses uninstalled
loose-mod entries that are not currently carried. Validated no-loose choices do
not require inventory. Catalog enumeration does not authorize mutation; live
transaction checks remain authoritative.

When cheat mode is enabled, the Quick Menu uses the Builder's compatible
catalog but changes only inventory eligibility. The transaction freezes that
mode in its request, revalidates it before mutation, preserves the selected
loose-mod count exactly, and retains all graph, dependency, stack, identity,
post-change, and rollback checks.

Hidden, internal, and script-only OMODs remain internal even when graph traversal discovers them.
The known Tactical Reload switcher records `TRT_mod_EntryPoint1`,
`TRT_mod_EntryPoint2`, and `TRT_mod_KeywordApply` from
`TacticalReload_IngameSwitch.esp` remain live graph inputs but are not generated
player choices or dependency-removal candidates.

A successful attachment transaction rebuilds the live menu and then reapplies
the same persisted visibility, ordering, and label preferences used on initial
open before sending the refreshed payload to the active view.

## Presentation preferences

Bracketed-text cleanup is presentation-only. The source label remains intact.
The dedicated general settings page independently controls bracketed prefixes,
infixes, and suffixes. The quick menu and builder apply those global controls
unless the identity-keyed per-weapon override selects hide all or show all.

An identity-keyed per-weapon source preference may bypass an authored ECO menu
and request the runtime-generated menu. Enabling that preference clears only
that weapon's visibility, order, and custom-label records before the generated
menu is rebuilt. It does not relax generated-menu compatibility checks or any
attachment transaction safety check.

## Distributable weapon menu profiles

The Builder can export one weapon's presentation preferences as a standalone
JSON profile and import a selected matching profile through the normal atomic
user-settings path. Profiles contain the per-weapon source and bracket overrides,
visibility, ordering, and labels only. They do not contain general settings,
inventory state, installed attachments, mutation authorization, or the
per-weapon Force Unsafe Swaps preference.

Profile discovery scans the mod-owned `WeaponMenus` and `Exports` directories,
accepts only bounded regular `.k2040qam.json` files, and validates the complete
document before presenting it. The target weapon and every referenced form must
round-trip through source plugin plus local FormID and resolve to the expected
form type. Import replaces only the exact target weapon record and restores the
previous in-memory record if the atomic settings save fails. The live menu and
attachment transaction remain authoritative after import.

## Settings adapters

The F4SE plugin INI remains the standalone hotkey source. When MCM has written
an individual registered-hotkey override to its shared registry, that value
takes precedence and is refreshed after leaving MCM. Missing registered values
fall back independently to the plugin INI, so MCM remains optional. MCM owns
registration and conflict reporting. Physical opener state is polled outside
Fallout's gameplay input layer, while focused Prisma pages may forward matching
browser events as a supplemental path. Duplicate cross-source signals are
filtered, and all menu open/close/switch work is queued to the F4SE game thread.
The two registered opener bindings act as toggles: the active menu's opener
closes it and the other opener switches menus without releasing gameplay
isolation between views. Escape closes either menu. DialogueMenu blocks opener
actions. There is no separate registered close binding.

The mod-owned Prisma builder remains authoritative for per-weapon presentation
settings and links to the mod-owned general settings page. Switching between
those pages creates a fresh Prisma view while preserving gameplay isolation,
the equipped-weapon builder state, and the balanced menu-state guard.
Prisma normally owns the cursor while a focused view is active. The plugin
checks the engine cursor-owner count after focus and uses a validated OG/AE
registration fallback when the provider did not add one. It also clears stale
cursor constraints after focus. Internal page replacement releases the prior
fallback state after unfocus so the new page can acquire a fresh owner.

Gameplay isolation always owns the input-disable layer while a mod page has
focus. The general slowdown setting scales the captured pre-menu global time
multiplier from 0% slowdown (unchanged) through 100% slowdown (full pause).
Close and game-transition paths restore the captured multiplier only when the
runtime still matches the value applied by the mod; a newer external time
change is preserved. Switching between mod pages keeps the same isolation span.

Diagnostic logging is enabled by default and can be disabled through general
settings. The persisted choice is loaded before the logger starts, so a
disabled session does not create plugin diagnostic messages. Re-enabling it
starts logging immediately without deleting an existing log file.

After a game/new-game transition, the plugin prepares one hidden Quick Menu
view so Prisma Dock can discover the mod through the framework's live-view
enumeration. That hidden registration view does not authorize attachment
changes or bypass the normal hotkey/open safety path. The existing Dock
metadata remains informational under PrismaUI `2.1.1`; the Dock contract does
not provide a custom native action for opening the Builder or Quick Menu.

## Attachment transaction safety

Runtime-validated leaf and provider installs into empty points, one-for-one
replacements, and provider changes with installed-child removal are available
through the normal quick menu. There is no global mutation switch. Every
operation is revalidated against the live weapon, exact equipped stack,
inventory, and hypothetical post-change attachment graph before it can change
anything.

An identity-keyed per-weapon Force Unsafe Swaps preference defaults off. When
enabled, it bypasses only the final dependency-cycle ordering refusal after the
normal graph analysis and cycle diagnostics have run. It does not bypass menu,
weapon, stack, inventory, form, attachment-point, stale-state, return, mutation,
post-change, or rollback checks. Turning it off restores normal fail-closed
dependency handling for that weapon.

The transaction accepts either a one-for-one replacement or a first install
into an empty, live-reachable attachment point on an exact equipped inventory
stack. The consumed point may come from the base weapon or an unchanged
installed provider. A first provider install is accepted only when its
hypothetical graph does not make an existing unreachable attachment live. The
operation must leave every unrelated installed OMOD identity unchanged and the
refreshed menu must expose each attachment point declared by the provider.
Ambiguous installed or inventory state, unresolved forms, dependency cycles,
stale menu state, and any operation that cannot be completely verified remain
blocked individually.

For a provider replacement, the runtime constructs the hypothetical post-change graph from the base weapon, unchanged installed OMODs, and the selected provider. Installed descendants that were live before but would become unreachable are scheduled for removal deepest-first. Their linked loose mods and the replaced provider's loose mod are returned through temporary world references and Fallout's normal pickup path before any weapon change. Inventory-neutral authored toggles require `AllowNoLooseModOptions`. The weapon transaction removes children before changing the provider, verifies every removed and installed identity plus inventory count, and rebuilds the menu so dependent category visibility follows the new live graph. Rollback restores the previous provider before reinstalling children. Failed pickup references are disabled and marked for deletion. The path does not dispatch Papyrus or edit inventory-stack metadata directly.

Any future attachment path must be separately validated to:

- operate only on validated choices;
- preserve provider/child ordering;
- verify live post-change state;
- handle stale equipped-instance data explicitly;
- never report success from an unchecked native call.

Target-environment in-game evidence is required before a mutation path can be considered successful.
