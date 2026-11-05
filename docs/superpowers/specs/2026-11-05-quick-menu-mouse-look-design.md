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
- Apply Prisma's per-view hidden cursor policy only to the focused Quick Menu
  when movement is enabled; reset it on close, page switch, focus failure and
  game transition.
- Continue arrow-key menu navigation. WASD remains gameplay movement and is
  ignored by Quick Menu navigation.
- Keep the provider cursor-policy API optional. If unavailable, keep the menu
  usable and log that the pointer could not be hidden.
- Runtime mouse-look pass-through requires in-game verification; static API
  support does not prove Prisma focus forwards mouse motion to Fallout.

## Approaches

1. **Use the existing toggle for movement and look (chosen):** enable
   `kMovement | kLooking` only for Quick Menu and hide that view's cursor while
   focused. This matches the existing preference and arrow-key navigation.
2. Add a second mouse-look toggle. This adds another preference and UI control
   for a behavior the maintainer requested as part of the existing toggle.
3. Unfocus Prisma while aiming. This would also lose reliable arrow-key
   delivery to the menu.

## Validation

Add native contract coverage for movement/look allow masks, blocked combat and
activation groups, and cursor-policy set/reset paths. Run the full automated
suite, Windows x64 build and package validation. Gameplay QA must confirm mouse
look, arrow navigation, hidden cursor, blocked attack/activation, default-off
behavior, and unaffected Builder/Settings input.
