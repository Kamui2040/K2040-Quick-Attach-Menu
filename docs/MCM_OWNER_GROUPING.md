# Optional K2040 Mods parent — MCM page variants

Quick Attach Menu's original MCM identity is
`K2040_Quick_Attach_Menu`. Both original hotkey IDs are preserved:
`openQuickMenu` and `openMenuBuilder`. No input-hook, OMOD, native
settings, controller-navigation or UI logic is changed by the
`ownerModName` migration.

The **default bundled** MCM configuration in
`dist/Data/MCM/Config/K2040_Quick_Attach_Menu/config.json` contributes
its existing hotkey controls to the optional `K2040_Mods` parent,
using `ownerModName`. The parent contains no predefined Quick Attach
Menu page.

An **alternative standalone** configuration lives in
`packaging/mcm-standalone/Data/MCM/Config/K2040_Quick_Attach_Menu/config.json`.
It contains exactly the previous Quick Attach MCM content, without
`ownerModName`, for users without K2040 Mods. The accompanying
`keybinds.json` matches the default version byte-for-byte. Install
**one variant only**. The native polling/PrismaUI opener still works
without either MCM menu.

Original MCM's `ownerModName` is source-confirmed. In native MCM, a
child whose parent is missing is queued rather than promoted to a
standalone top-level entry. F4SE Menu Framework's independent legacy
MCM adapter requires in-game grouping tests.

This change does **not** add new controller buttons to Quick Attach's
MCM layout; those are a separate input integration milestone. Any
experimental prior hub controller binding state must be retained until
a dedicated migration is verified.

Run the structure-only checks with
`python3 -m unittest discover -s tests -v`.
No new game runtime compatibility claim is made from these tests.
