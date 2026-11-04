# Quick Menu Navigation and Clipping

## Intent

Make the row-based Quick Menu feel predictable: left/right moves within the
active category or attachment row, up/down switches rows, and E/Enter accepts
the focused attachment. Keep the focused item fully visible when a horizontal
row overflows the screen.

Add an off-by-default **Allow movement while open** preference. When enabled,
the game receives movement input while the Quick Menu stays open, WASD does not
navigate the menu, and arrow keys remain available for menu navigation. Other
gameplay input remains blocked. Existing users keep the current input-blocking
behavior unless they enable the preference.

## Scope and constraints

- Apply row navigation to the horizontal category/attachment presentation;
  preserve geometry-based navigation for cascade and radial presentations.
- Reveal a focused category or attachment in its horizontal scroll row without
  changing the compact layout or hiding entries from keyboard navigation.
- Persist the new preference using the existing general-settings format; a
  missing value means false.
- Keep attachment changes behind the existing explicit confirmation and live
  transaction validation.
- Do not change menu closing, opener handling, controller mappings, or
  attachment safety behavior.

## Approaches

1. **Compact rows with focused-item scrolling (chosen):** retain the current
   horizontal strips and scroll the focused item fully into view. This handles
   long lists without shrinking labels or increasing menu height.
2. **Wrap rows:** show more entries at once but increase menu height and require
   navigation to account for variable wrapped rows.

## Implementation calls

- On row-based horizontal presentation, left/right traverse the active row;
  up/down switch between category and attachment rows. Enter and E accept.
- With movement enabled, WASD is passed through and ignored by menu navigation;
  arrow keys continue to navigate. The native input layer allows only the
  movement event group and continues blocking other gameplay event groups.
- The preference is applied when the Quick Menu opens. Builder and Settings
  views retain their current gameplay isolation, including while changing the
  preference; the next Quick Menu open uses the saved value.

## Validation

Add focused browser tests for row navigation, E/Enter activation, WASD
passthrough, and scroll-into-view behavior. Add a native contract check for the
movement-only input-layer mask and preference default/persistence. Build the
Windows x64 plugin and leave in-game movement and menu interaction for runtime
QA.
