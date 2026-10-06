# Icon Artwork Direction

The semantic library currently resolves 172 attachment subtypes to 95 reusable visual icon classes.
This document defines how those 95 icons should be authored and reviewed.

## Render contract

- Master format: SVG.
- ViewBox: `0 0 24 24`.
- Expected UI display size: about `22x22` px.
- Keep important geometry inside a roughly `20x20` optical footprint.
- Default stroke: `1.6`.
- Rounded line caps and joins.
- Transparent background.
- Monochrome and recolorable by the UI.
- Outline-first. Use solid fill only when it materially improves readability.
- One dominant silhouette, with at most two secondary details.
- Avoid details that disappear at 22 px.
- No text, logos, trademarks, or exact branded product geometry.
- Artwork must be original; do not trace or redistribute third-party icon assets.

## Visual language

The visual reference is the established FallUI/FIS icon-library language: compact monochrome UI symbols with strong silhouettes, restrained internal detail, and clean readability at inventory-icon sizes. The goal is visual compatibility with common FallUI icon mods, not duplication of their individual artwork. All icons in this project remain original authored assets.

The set should look like one technical inventory system rather than 95 separate illustrations.

- Long weapon parts normally use a left-to-right side profile.
- Small mechanical parts use a simplified side profile or schematic symbol.
- Optics use their recognizable outer silhouette, not internal branding.
- Ammunition icons emphasize projectile shape and one effect cue.
- Energy parts use one restrained coil/lightning/finned cue.
- Removal/none classes use the underlying mounting shape plus a clear remove cue.
- Fallback icons stay deliberately generic and should not imply an unsupported subtype.

The icon should remain understandable without color. UI accent colors are presentation state, not part of the artwork.

## Review order

### Pilot — 12 icons

Approve these before producing the complete set:

1. `attachment.generic`
2. `receiver.generic`
3. `barrel.long_precision`
4. `muzzle.suppressor`
5. `muzzle.brake_compensator`
6. `sights.reflex`
7. `sights.scope`
8. `stock.precision`
9. `magazine.drum`
10. `underbarrel.vertical_grip`
11. `rail_accessory.laser_light`
12. `ammo_caliber.generic`

The pilot deliberately mixes simple and complex silhouettes. It is the style gate for stroke weight,
negative space, optical centering, readability, and category consistency.

Pilot visual style: **approved**. This approves the shared visual language only; each individual SVG still requires its own artwork review before its manifest status becomes approved.

### Batch 1 — Core weapon geometry

50 icons:

- Receiver
- Barrel
- Muzzle
- Sights
- Stock
- Magazine

### Batch 2 — Mounting and handling

23 icons:

- Handguard
- Pistol Grip
- Underbarrel
- Mount
- Rail Accessory

### Batch 3 — Systems, effects and fallbacks

22 icons:

- Ammo / Caliber
- Internal
- Cosmetic
- Special
- Grip / Stock fallback
- Global generic fallback

## QA gate

Every icon remains a draft until reviewed.

For each batch verify:

- readable at `22x22`;
- no detached or accidental shapes;
- optical centering is consistent;
- stroke weight matches the rest of the set;
- silhouette differs clearly from nearby classes;
- no brand/logo leakage;
- transparent background;
- SVG viewBox and filename match the artwork manifest.

`data/icon_library/icon_artwork_manifest.json` is the authoritative production list.
