# K2040's Quick Attach Menu

K2040's Quick Attach Menu lets you change compatible weapon attachments from a
fast in-game menu instead of returning to a workbench.

The menu can use an ECO-authored weapon menu when one is available, or build a
compatible menu from the equipped weapon at runtime. ECO is optional.

The unreleased default-material handling displays an empty material slot as
already having the default material applied. A reset from a non-default
material requires a workbench until a safe direct-removal path is validated.

The unreleased generated-menu filter requires a matching weapon keyword,
reachable attachment point, and a loaded crafting recipe creating each new
attachment. This reduces exposure of internal/scripted options, but exact
workbench conditions and unusual loose-mod-only entries are still being
validated. Already-installed parts remain visible for recovery.

## Features

- Four presentations: Cascade, Radial Wheel, Compact Hybrid, and Horizontal Bar.
- Inventory-aware choices in the gameplay menu.
- Native controller navigation for the Quick Menu only, including radial
  thumbstick direction selection and a configurable Quick Menu opener.
- Per-weapon Builder for visibility, order, labels, menu source, and bracketed
  text handling.
- General settings for keybindings, scale, position, opacity, themes, colors,
  hints, close-after-apply behavior, diagnostic logging, and gameplay slowdown.
- Independent scale and click-drag position for each presentation.
- Resizable Builder and Settings windows with saved dimensions and reset
  controls.
- Optional MCM and Hotkey Manager integration.
- Export and import of distributable per-weapon menu profiles.
- Live attachment validation, provider/child ordering, inventory verification,
  and rollback for supported changes.
- An off-by-default, per-weapon Force unsafe swaps escape hatch for diagnosed
  dependency cycles.

Version 0.5.198 is intentionally text-only. Attachment icons and weapon-preview
rendering are not included in this release.

## Requirements

- Fallout 4 runtime `1.10.163` or `1.11.240`
- The matching F4SE release
- PrismaUI_F4 `2.1.1`

Mod Configuration Menu is optional. Without it, the default INI bindings and
the mod's own Settings page remain available.

Other Fallout 4 runtimes, Fallout 4 VR, and mismatched F4SE or PrismaUI builds
are not supported by this release.

Version 0.5.198 received focused external confirmation that the reported
Tactical Reload false safety block was resolved. The broader attachment
regression matrix was not independently repeated for this release. Version
0.5.197 passed focused runtime testing on Fallout 4 `1.11.240`; its original-game
compatibility path remains included but was not rerun on `1.10.163`.

## Installation

Install the release archive with a mod manager. Its `Data` folder must merge
with the game's `Data` folder. Start Fallout 4 through F4SE.

Default controls:

- `Shift+K`: open or close the quick menu.
- `Ctrl+Shift+K`: open or close the Builder.
- `Escape`: close the active mod menu.

Pressing the other opener while one menu is active switches directly to that
menu. Keybindings can be changed through MCM or the Settings page reached from
the Builder.

Only the **Quick Menu** has a controller shortcut under Settings > Controls.
Select a button and optional modifier, then Apply with keyboard/mouse. It
defaults to unassigned, and leaves MCM/keyboard assignments alone. Choose a
combination unlikely to conflict with gameplay. The Builder and Settings remain
keyboard/mouse pages until controller-specific layouts are designed.

For the radial Quick Menu, use either thumbstick (left takes precedence).
Point at a category, press A to open it, point at an attachment, and press A to
confirm. B returns to the category ring, then closes. The D-pad still works as
a fallback; D-right no longer acts as the radial entry key. Steam Input under
Proton must expose the controller through XInput. This analog path still
requires focused target-runtime testing.

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
build commands.

The committed PrismaUI API header is an unchanged upstream file. Its exact
source revision, checksum, and license are recorded under
[`external/prismaui_f4/`](external/prismaui_f4/).

## License

Project-owned code is licensed under the GNU General Public License v3.0 only.
See [`LICENSE`](LICENSE). The unchanged PrismaUI API header remains covered by
the separate license stored beside it.
