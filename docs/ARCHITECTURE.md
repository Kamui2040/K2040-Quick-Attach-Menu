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

For generated menus, AP reachability alone does not establish weapon membership because an installed provider can expose generic display points used by unrelated weapons. Installed OMODs are trusted directly. An uninstalled alternative must belong to the source-plugin family proven by the equipped weapon and its default/live OMODs and must share positive instantiation-filter evidence with the equipped weapon, its live instance, or the default/live OMOD family already proven for that weapon; candidates without that target-family evidence fail closed.

The builder may enumerate the complete source-family/AP-compatible generated
catalog so presentation preferences can be configured before loose mods are
acquired. The gameplay quick menu still suppresses uninstalled entries whose
loose mods are absent, and catalog enumeration does not change selection or
mutation eligibility.

Hidden, internal, and script-only OMODs remain internal even when graph traversal discovers them.

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
inventory state, installed attachments, or mutation authorization.

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
registration and conflict reporting; the plugin's physical polling path owns
menu behavior. The two registered opener bindings also act as menu toggles:
the active menu's opener closes it and the other opener switches to the other
menu without releasing gameplay isolation between views. Escape closes either
menu. There is no separate registered close binding.

The mod-owned Prisma builder remains authoritative for per-weapon presentation
settings and links to the mod-owned general settings page. Switching between
those pages creates a fresh Prisma view while preserving gameplay isolation,
the equipped-weapon builder state, and the balanced menu-state guard.

After a game/new-game transition, the plugin prepares one hidden Quick Menu
view so Prisma Dock can discover the mod through the framework's live-view
enumeration. That hidden registration view does not authorize attachment
changes or bypass the normal hotkey/open safety path. The existing Dock
metadata remains informational under PrismaUI `2.1.1`; the Dock contract does
not provide a custom native action for opening the Builder or Quick Menu.

## Attachment transaction safety

Runtime-validated leaf installs, one-for-one replacements, and provider changes
with installed-child removal are available through the normal quick menu. There
is no global mutation switch. Every operation is revalidated against the live
weapon, exact equipped stack, inventory, and hypothetical post-change attachment
graph before it can change anything.

The transaction accepts either a one-for-one replacement or a first leaf install
into an empty, live-reachable attachment point on an exact equipped inventory
stack. The consumed point may come from the base weapon or an unchanged installed
provider. Installing a new provider into an empty point, ambiguous installed or
inventory state, unresolved forms, dependency cycles, stale menu state, and any
operation that cannot be completely verified remain blocked individually.

For a provider replacement, the runtime constructs the hypothetical post-change graph from the base weapon, unchanged installed OMODs, and the selected provider. Installed descendants that were live before but would become unreachable are scheduled for removal deepest-first. Their linked loose mods and the replaced provider's loose mod are returned through temporary world references and Fallout's normal pickup path before any weapon change. Inventory-neutral authored toggles require `AllowNoLooseModOptions`. The weapon transaction removes children before changing the provider, verifies every removed and installed identity plus inventory count, and rebuilds the menu so dependent category visibility follows the new live graph. Rollback restores the previous provider before reinstalling children. Failed pickup references are disabled and marked for deletion. The path does not dispatch Papyrus or edit inventory-stack metadata directly.

Any future attachment path must be separately validated to:

- operate only on validated choices;
- preserve provider/child ordering;
- verify live post-change state;
- handle stale equipped-instance data explicitly;
- never report success from an unchecked native call.

Target-environment in-game evidence is required before a mutation path can be considered successful.
