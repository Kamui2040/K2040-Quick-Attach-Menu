# Self-contained K2040 Mods MCM integration (0.5.214 development)

Quick Attach Menu includes the **exact canonical K2040 Mods parent**
(`Data/MCM/Config/K2040_Mods/config.json`) inside its own package,
alongside its child MCM page. Users do **not** need to install a
separate MCM Hub.

The parent file is byte-identical to the one bundled with Prisma Minimap:
SHA-256 `8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb`.
If Amethyst reports a file conflict with the minimap for this parent
file, it is intentional; do not edit either parent or let differing
versions silently overwrite each other. The build and tester verify the
hash. Parent pages are discovered by native MCM via `ownerModName`.

## Child-owned controls

`Data/MCM/Config/K2040_Quick_Attach_Menu/config.json` has
`modName=K2040_Quick_Attach_Menu`,
`ownerModName=K2040_Mods`, and a `Quick Attach Menu` page.
The child owns both keyboard IDs `openQuickMenu` and
`openMenuBuilder`, retaining Shift+K and Ctrl+Shift+K standalone
defaults unless the user has assigned overrides.

The Quick Menu controller opener is configured with two
`ModSettingInt` dropdowns: `iQuickAttachPadPrimary` and
`iQuickAttachPadModifier`. Both default to 0 (Unbound). The controller
state is stored in `Data/MCM/Settings/K2040_Quick_Attach_Menu.ini`,
**never** `K2040_Mods.ini`. The existing PrismaUI Quick Menu
controller shortcut selector and MCM dropdowns write to and read the
same effective values. Changes made in MCM refresh in the native
controller poller without a game restart.

Controller shortcut examples: `RB`, `LB+RB`, and
`LT+DUp` (LT+D-pad Up). Only the Quick Menu opener is assigned a
controller shortcut. The Builder's controller menu navigation remains
working, but a Builder *opener* combo was not introduced as part of
this grouping change.

## Migration and safety

The prior standalone
`Data/F4SE/Plugins/K2040_Quick_Attach_Menu/ControllerShortcuts.ini`
is only read for non-destructive migration when the child MCM user
file does not yet exist. Existing child preferences take precedence.
The old file is not deleted; Reset Controller Shortcuts writes an
explicit Unbound assignment into the new child file to prevent an
old shortcut being resurrected.

The plugin keeps its mature controller navigation, focus ownership,
input suppression, game-thread action dispatch and browser-forwarded
keyboard hotkeys. It conservatively disables conflicting controller
buttons/chords observed in installed Minimap or Quest Journal
child-owned settings, and rejects conflicting new PrismaUI assignments.
This is **not** a universal central multi-mod input arbiter.

A grouped child page requires native MCM for display, but Quick
Attach itself and its native hotkeys remain operational without
the MCM plugin installed. F4SE Menu Framework's interpretation of
`ownerModName` has **not** been runtime verified.

## Package/test

```sh
python3 -m unittest discover -s tests -v
python3 scripts/stage-mcm-tester.py --output /unique/output/tester.zip
```

The tester script refuses a missing/non-x64 DLL and a noncanonical
parent, then validates every ZIP member's bytes. The Windows DLL must
first be built with the pinned CommonLibF4 and Linux clang-cl toolchain.
All in-game behavior (native MCM merging with minimap, F4SE Menu
Framework, keyboard/controller binding round trips, OG and AE) still
requires an isolated Amethyst profile. No production deployment or
claim of runtime success is made here.


## Keyboard default-registration fix (October 2026)

Classic MCM may fail to assign a previously unregistered hotkey,
because its AS3 remap handler reads `GetKeybind(...).keycode` without
checking whether the result is null. Quick Attach now initializes
missing records in the global registry at plugin startup, using
`openQuickMenu = Shift+K` and
`openMenuBuilder = Ctrl+Shift+K` **only on first registration**.

Previously saved bindings (including Mouse 4 / Mouse 5) remain
untouched. A persistent child-specific initialization marker
distinguishes first-install defaults from user-cleared bindings;
cleared keys stay Unbound, not silently restored. Registry updates
are scoped to the child's action IDs and written atomically.
The plugin's own F4SE INI shortcuts remain operational when the
MCM runtime provider is absent. Neither the package nor the test
archive contains a replacement user Keybinds.json.

Native MCM hotkey capture and in-memory persistence, especially after
clearing/rebinding a shortcut in the *same session*, must be checked in
the isolated Amethyst profile. A menu close/reopen or game restart
may be required on original MCM builds.
