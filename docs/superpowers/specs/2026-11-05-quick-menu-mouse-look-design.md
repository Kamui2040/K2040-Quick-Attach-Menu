# Quick Menu mouse look while movement is enabled

## Intent

When **Allow movement while open** is enabled, let mouse movement control the
game camera while the Quick Menu remains focused and arrow keys continue to
navigate its rows. Hide the menu pointer in this mode. Builder and Settings
remain fully isolated.

## Scope and constraints

- Reuse the existing off-by-default `allowMovementWhileOpen` setting and its
  next-open activation behavior.
- Allow only Fallout 4 `kMovement` and `kLooking` user-event groups while the
  setting is active; keep activation, combat and all other gameplay events
  blocked.
- Use Prisma V10 click-through overlay input with zero mouse regions when the
  provider advertises `InputRegions`; keep keyboard focus on Quick Menu.
- Hide Prisma's per-view cursor only for the focused Quick Menu when movement
  is enabled; do not register the native cursor fallback in this mode. Restore
  the default cursor policy on close, page switch, focus failure and transition.
- Continue arrow-key menu navigation. WASD remains gameplay movement and is
  ignored by Quick Menu navigation.
- Keep the provider cursor-policy API optional. If unavailable, keep the menu
  usable and log that the pointer could not be hidden.
- Runtime mouse-look pass-through requires in-game verification; static API
  support does not prove Prisma focus forwards mouse motion to Fallout.

## Approaches

1. **Use the existing toggle for movement and look (chosen):** enable
   `kMovement | kLooking`, focus the menu through click-through overlay input
   with no mouse regions, and hide the cursor only while Quick Menu is focused.
   This preserves keyboard arrow navigation while mouse input reaches gameplay.
2. Add a second mouse-look toggle. This adds another preference and UI control
   for a behavior the maintainer requested as part of the existing toggle.
3. Unfocus Prisma while aiming. This would also lose reliable arrow-key
   delivery to the menu.

## Validation

Add native contract coverage for movement/look allow masks, blocked combat and
activation groups, click-through focus, and cursor-policy set/reset paths. Run
the full automated suite, Windows x64 build and package validation. Gameplay QA
must confirm mouse look, arrow navigation, hidden cursor, blocked attack and
activation, default-off behavior, and unaffected Builder/Settings input.
