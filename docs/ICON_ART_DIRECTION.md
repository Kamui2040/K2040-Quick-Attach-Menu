# Icon Artwork Direction

The semantic library currently resolves 172 attachment subtypes to 95 stable visual icon classes.
A separate artwork reuse plan groups those classes into 65 proposed base illustrations.
This document defines how the original icons should be authored and reviewed.

## Render contract

- Master format: SVG.
- ViewBox: `0 0 24 24`.
- Expected UI display size: about `22x22` px.
- Keep important geometry inside a roughly `20x20` optical footprint.
- Default stroke: `1.6`.
- Rounded line caps and joins.
- Transparent background.
- Monochrome and recolorable by the UI.
- Silhouette-first. Prefer solid monochrome bodies with transparent negative-space cutouts; use strokes only for secondary detail when needed.
- One dominant silhouette, with at most two secondary details.
- Avoid details that disappear at 22 px.
- No text, logos, trademarks, or exact branded product geometry.
- Artwork must be original; do not trace or redistribute third-party icon assets.

## Visual language

The approved 12-icon pilot SVGs and the approved detailed mechanical-illustration concepts are the primary visual reference. Broad FallUI/FIS conventions may inform readability and monochrome presentation, but do not trace, extract, convert, or redistribute third-party artwork. All production icons must be original authored SVGs.

The set should look like one technical inventory system rather than 95 separate illustrations.

- Prefer the smoother, recognizable silhouettes and tapered/rounded contours seen in Extra Icons/FallUI over blocky rectangular construction.
- Do not derive a production icon from the semantic label alone. Ground each silhouette in real visual references for that attachment type, then author a distinct original shape. The first 18-icon receiver/barrel/muzzle production batch proved that abstracting from labels alone produces unacceptable generic/blocky results.
- Long weapon parts normally use a left-to-right side profile.
- Small mechanical parts use a simplified side profile or schematic symbol.
- Optics use their recognizable outer silhouette, not internal branding.
- Ammunition icons emphasize projectile shape and one effect cue.
- Energy parts use one restrained coil/lightning/finned cue.
- Removal/none classes use the underlying mounting shape plus a clear remove cue.
- Fallback icons stay deliberately generic and should not imply an unsupported subtype.

The icon should remain understandable without color. UI accent colors are presentation state, not part of the artwork.

## Artwork reuse plan

The classifier still produces 95 distinct icon_class values. The manifest
artwork_reuse.class_aliases mapping proposes that 30 of them share one of 65
base SVG illustrations. Every alias points directly to a base class; there
are no chains. The original semantic label remains visible, so reusing a
silhouette never erases a functional distinction in text or changes attachment
compatibility.

Examples:
- receiver.fire_control uses receiver.generic.
- sights.thermal and sights.night_vision use sights.scope.
- magazine.extended and magazine.stick use magazine.generic.
- Most ammunition-effect classes use ammo_caliber.generic.
- special.generic uses the global attachment.generic fallback.

Distinct forms remain separate where shape carries useful information:
drum/cylinder/tube/box-belt magazines, scope vs reflex/magnifier optics,
folding/collapsible stocks, bipod/underbarrel launcher, muzzle suppressor vs
brake, modular handguard, and scope mounts.

The 65-base count is a **proposed production scope**, not 65 completed SVGs.
A reused class resolves to the base class's asset and review status. Historic
rejected SVG drafts stay rejected and must not be shipped. A proposed base can
still be refined or split later after visual review. No runtime UI changes
have been made. Concept-sheet approvals do not automatically approve authored
SVG files.

The existing 50/23/22 batch counts remain the counts of *icon classes*, not
new base illustrations. Use scripts/list-icon-artwork.py to see each class's
resolved base artwork.

## Review order

### Pilot — 12 icons

Individually approved before expanding production:

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

Pilot visual style: **approved**. All 12 pilot SVG assets are also individually **approved** and now define the production baseline for the remaining icons.

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
