# K2040's Quick Attach Menu

Change weapon attachments in Fallout 4 without going back to a workbench.
Quick Attach can use a compatible ECO-authored menu, but ECO is optional: it
can also build a menu from the equipped weapon and its attachment records.

**Current release: [0.5.222](https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.222).**
It is available on [Nexus Mods](https://www.nexusmods.com/fallout4/mods/109575)
and GitHub. The changes described under *In development* are not in that release.

## What the released mod does

- Shows compatible, inventory-aware attachments in Cascade, Radial Wheel,
  Compact Hybrid or Horizontal Bar menus.
- Lets you open Quick Menu with `Shift+K` and the per-weapon Builder with
  `Ctrl+Shift+K`. Keyboard, mouse and controller navigation are supported.
- Lets you choose which categories and attachments a weapon shows, change
  labels and order, and export or import that weapon's menu profile.
- Saves presentation scale, position, appearance, input settings and menu
  behavior. Controller shortcuts are configurable.
- Checks the equipped weapon, attachment dependencies and inventory before
  making a change. It can refuse a swap that is unsafe or ambiguous rather
  than risk changing the wrong attachment.
- Offers an optional per-weapon **Force unsafe swaps** setting for diagnosed
  dependency cycles. Leave it off unless you understand the affected weapon.

## In development (not part of 0.5.222)

- **Orbit:** a round category selector and a curved attachment list. The
  selected item stays in the center of that list.
- **Clock Slices:** category wedges surrounded by narrow attachment slices.
  The slices fit their labels around the wheel instead of using a fixed
  number of options per page.
- **Clearer labels:** Radial and Compact Hybrid use readable one- or two-line
  labels. Repeated category words can be shortened for display without
  changing the actual attachment name or identity.
- **Unified Settings V2:** a separate K2040 settings hub edits Quick Attach's
  settings directly. Quick Menu and Builder still belong to this mod; the
  hub does not perform attachment changes. The hub is also responsible for
  the optional shared visual themes. It must be installed separately.

These changes have passed source-level checks but still need Fallout 4 OG
1.10.163 testing. In particular, CPU-rendered menus are the current visual
baseline; GPU behavior has not been signed off.

## Requirements

- Fallout 4 runtime `1.10.163` or `1.11.240`
- The matching F4SE release
- PrismaUI_F4 `2.1.1`

Mod Configuration Menu is optional. Without it, the default INI bindings and
the mod's own Settings page remain available.

Other Fallout 4 runtimes, Fallout 4 VR, and mismatched F4SE or PrismaUI builds
are not supported by this release.

## Reporting bugs

Use the [GitHub bug report form](https://github.com/Kamui2040/K2040-Quick-Attach-Menu/issues/new/choose). Include the game, F4SE, PrismaUI_F4, and mod versions, plus steps to reproduce the issue. Remove private information from logs before sharing them.

## Installation

Install the archive with a mod manager. The archive already contains
the shared K2040 Mods parent, so do not download the Hub separately.
Start Fallout 4 through F4SE. Native MCM is optional.

If MCM Categorizer hides the Quick Attach page under **K2040 Mods**, download
the optional **MCM Categorizer compatibility** file from the same GitHub
release and install it after the main archive. Do not install it when the
normal Quick Attach page is visible. It moves only Quick Attach to a standalone
entry; other K2040 pages keep their current grouping.

Default controls:

- `Shift+K`: open or close the quick menu.
- `Ctrl+Shift+K`: open or close the Builder.
- `Escape`: close the active mod menu.

Pressing the other opener while one menu is active switches directly to that
menu. In the VNext development build, all Quick Attach general settings and
gameplay shortcuts are edited in the separately installed K2040 Settings Hub
(provisional shortcut: F3). Builder's Settings button opens Quick Attach's
page there and releases Builder focus when the hub accepts the request. If the hub is missing or not
ready, Builder stays open and reports the problem. Older published releases
retain their MCM and standalone Settings controls.

The Quick Menu controller shortcut is under **Settings > Controls**. Choose a
button and optional modifier, then select **Apply**. It is unassigned by
default. Choose a combination that does not conflict with gameplay. Builder
and Settings also support controller navigation; text entry and drag reordering
require a keyboard or mouse.

Keyboard-and-mouse radial mode also displays at most six attachments
per page. Use the arrows in the wheel center, scroll the mouse wheel,
or press Page Up/Page Down to see every choice. Hovering shows the full
attachment name in the center; only clicking it attempts to apply it.

The controller radial attachment ring displays up to six choices at once
to keep labels readable. While viewing attachments, use LB/RB to switch
pages and point with the stick to highlight an entry; the center caption
shows the full selected name. The ring cannot apply an attachment until A
is pressed.

For the radial Quick Menu, use either thumbstick (left takes precedence).
Point at a category and press A to open it. Point at an attachment and press A
to apply it. Press B to return to categories or close the menu.

Version 0.5.221 adds simpler keyboard navigation:
A/D or Left/Right move across the current row; W/S or Up/Down switch rows; and
E/Enter confirms the selection. When **Allow movement while open** is enabled,
WASD and mouse look reach gameplay, the cursor is hidden, and arrow keys keep
menu navigation available.

The Settings page controls gameplay slowdown while any mod menu is open. The
range runs from normal game time at 0% slowdown to a full pause at 100%.

## Development settings

The older V1 manifest lists the controls Quick Attach exposes. It does not
hold player settings. In the current VNext branch, the V2 adapter connects
those controls to the separate two-pane K2040 editor; Quick Attach still reads,
checks and saves its own values. The old standalone settings page remains in
source for reference but is not included in VNext owner-only packages.

To check the manifest and settings tests before packaging:

```bash
npm ci --ignore-scripts --no-audit --no-fund
npm test
npm run validate:settings
```

See [the settings inventory](docs/SETTINGS_SCHEMA_V1_AUDIT.md) for field
ownership and [the V2 integration notes](docs/UNIFIED_SETTINGS_EDITOR_V2_INTEGRATION.md)
for the current connection to the hub. The hub alone owns the general settings
shortcut; Quick Menu and Builder shortcuts remain part of Quick Attach.

## Builder and settings

The Builder always shows the compatible catalog for the equipped weapon so it
can be configured before every loose mod has been acquired. The gameplay quick
menu remains inventory-limited.

Per-weapon controls can:

- use the global menu-source setting, force an ECO-authored menu, or force a
  generated menu;
- show or hide categories and entries;
- reorder categories and entries by dragging;
- replace labels without changing the source records;
- override global bracketed-text cleanup;
- enable or restore safe swaps for this weapon; the unsafe override affects
  dependency-cycle refusals only;
- export or import that weapon's presentation profile.

General settings include presentation, background opacity, layout scale and
position, color palette, label cleanup, controls, gameplay slowdown, and
diagnostic logging. Published standalone Settings offers section,
position and confirmation-gated full reset. In the VNext shared editor,
destructive Reset All is disabled until a verified confirmation flow exists.

## Weapon menu profiles

Builder exports are written to:

`Data/F4SE/Plugins/K2040_Quick_Attach_Menu/Exports/`

Distributable profiles are installed under:

`Data/F4SE/Plugins/K2040_Quick_Attach_Menu/WeaponMenus/`

Profiles match a weapon by source plugin filename and local FormID. They contain
presentation preferences only and cannot add attachment options or bypass live
inventory and mutation checks. See
[`docs/WEAPON_MENU_PROFILES.md`](docs/WEAPON_MENU_PROFILES.md) for packaging.

## Compatibility and safety

If Quick Attach cannot confirm that a swap is safe, it refuses the change.
That protects installed attachments and inventory from uncertain operations.

In code, this is called *failing closed*: an operation must be fully resolved
and verified before it can run. Supported changes include validated leaf or provider installation
into an empty live-reachable point, one-for-one replacement, and provider
replacement with installed descendants removed in dependency order. An empty
provider install is blocked if it would activate an existing unreachable
attachment or change unrelated installed attachment identities. Ambiguous
inventory stacks, unresolved graphs, dependency cycles, and stale selections
remain blocked.

See [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) and
[`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md) for the release boundary.

## Building

The project builds a Windows x64 F4SE plugin from Linux with xmake and an
xmake-compatible CommonLibF4 checkout. Set the local toolchain paths described
in [`AGENTS.md`](AGENTS.md), then run the documented `releasedbg` configure and
build commands. A verified fresh-clone recovery procedure is in
[`docs/BUILDING_ON_LINUX.md`](docs/BUILDING_ON_LINUX.md).

The committed PrismaUI API header is an unchanged upstream file. Its exact
source revision, checksum, and license are recorded under
[`external/prismaui_f4/`](external/prismaui_f4/).

## License

Project-owned code is licensed under the GNU General Public License v3.0 only.
See [`LICENSE`](LICENSE). The unchanged PrismaUI API header remains covered by
the separate license stored beside it.
