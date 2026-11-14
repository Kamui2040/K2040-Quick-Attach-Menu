# K2040's Quick Attach Menu

Change compatible weapon attachments from a quick in-game menu instead of
returning to a workbench. ECO-authored menus are supported when available; ECO
is optional.

**Current GitHub release:** [0.5.222](https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.222), with an installation ZIP, checksum and optional MCM Categorizer compatibility file. Nexus notes are prepared; its Files entry still needs an upload.

## Features

- Four release presentations: Cascade, Radial Wheel, Compact Hybrid, and Horizontal Bar.
  The unreleased 0.5.224 candidate adds Orbit, a circular category selector with
  a curved, centre-focused attachment list, and improves wheel label fitting.
- Inventory-aware choices in the gameplay menu.
- Controller navigation across Quick Menu, Builder and Settings, including
  thumbstick and D-pad focus and a configurable Quick Menu opener.
- Per-weapon Builder for visibility, order, labels, menu source, and bracketed
  text handling.
- General settings for keybindings, scale, position, opacity, themes, colors,
  hints, close-after-apply behavior, diagnostic logging, and gameplay slowdown.
- Unreleased 0.5.227 Clock Slices: one category wheel with individual,
  narrow, elongated curved outer attachment sectors. Labels follow their
  slice angle. Radial and Hybrid wedge labels use readable one- or two-line
  fitting; Hybrid shows a navigable window when categories are numerous.
  Repeated
  category words (including singular/plural variants) are shortened by
  default across all six menu presentations, with a Settings toggle for full
  names. Each presentation retains independent positioning
  and scale. The real attachment identity and application logic are unchanged.
- Independent scale and click-drag position for each presentation.
- Resizable Builder and Settings windows with saved dimensions and reset
  controls.
- Optional MCM and Hotkey Manager integration.
- Export and import of distributable per-weapon menu profiles.
- Live attachment validation, provider/child ordering, inventory verification,
  and rollback for supported changes.
- An off-by-default, per-weapon Force unsafe swaps escape hatch for diagnosed
  dependency cycles.

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
menu. Keybindings can be changed through MCM or the Settings page reached from
the Builder.

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
diagnostic logging. Reset controls
restore either a section, one presentation's position, or the complete
fresh-install state.

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

The plugin fails closed when an attachment operation cannot be fully resolved
or verified. Supported changes include validated leaf or provider installation
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
