# Icon Artwork Direction

The semantic library currently resolves 172 attachment subtypes to 95 stable visual icon classes.
A separate artwork reuse plan groups those classes into 64 proposed base illustrations.
This document defines artwork review and the current research-asset contract.

## Format and display direction

- SVG is **optional**, not the required production format. Support PNG and lossless WebP for detailed artwork; keep SVG for simple silhouettes and existing approved icons.
- Do not reduce approved mechanical illustrations into different, simplified SVG designs merely to meet a 22 px size target. Visual fidelity and recognizability must be reviewed independently of file validity.
- Compare real rendered output at 22, 32, and 48 px before choosing a menu icon size. Concept-sheet inset labels do not count as native-size renders. Screen-space scaling and layout must also be tested in PrismaUI before shipping.
- Prefer transparent assets with monochrome shapes. Raster assets require an explicit HUD-color/tint check; SVG `currentColor` is not a raster recoloring mechanism.
- Keep original semantic icon-class identifiers unchanged. Optional installed FIS/FallUI libraries may supply visual assets through a bridge; do not bundle, alter, or redistribute their artwork.
- Use existing approved pilot icons or text when an optional icon source is unavailable. PrismaUI list rows now accept optional PNG/WebP icon data; reading and rendering installed FIS symbols automatically is **not implemented**. See `docs/ICON_FIS_BRIDGE.md` for the tested browser contract and remaining runtime work.

The current artwork manifest and validator still describe the earlier **SVG-only research assets**. Their format checks must be updated and validated separately before PNG/WebP files can be accepted as production assets. No current production icon status changes with this decision.

## Existing SVG pilot contract

- SVG masters use `viewBox="0 0 24 24"` and were reviewed around `22x22` px.
- Keep important geometry inside a roughly `20x20` optical footprint, with a `1.6` default stroke and rounded caps/joins.
- Use a transparent background, monochrome `currentColor`, and clear cutouts.
- These are the established rules for the 12 approved pilot SVGs, not a restriction on future raster artwork.
- No text, logos, trademarks, or exact branded product geometry in new original icons.

## Visual language

The approved 12-icon pilot SVGs remain valid originals. Approved detailed mechanical-illustration concepts remain visual references, not approved game assets. New standalone artwork may be original PNG, WebP, or SVG. An optional bridge may *reference* separately installed FIS/FallUI artwork at runtime, but may not package, reproduce, or distribute those third-party files.

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
artwork_reuse.class_aliases mapping proposes that 31 of them share one of 64
base SVG illustrations. Every alias points directly to a base class; there
are no chains. The original semantic label remains visible, so reusing a
silhouette never erases a functional distinction in text or changes attachment
compatibility.

Examples:
- receiver.fire_control uses receiver.generic.
- sights.thermal and sights.night_vision use sights.scope.
- magazine.extended and magazine.stick use magazine.generic.
- stock.brace uses stock.collapsible, keeping the brace label separate.
- Most ammunition-effect classes use ammo_caliber.generic.
- special.generic uses the global attachment.generic fallback.

Distinct forms remain separate where shape carries useful information:
drum/cylinder/tube/box-belt magazines, scope vs reflex/magnifier optics,
folding/collapsible stocks, bipod/underbarrel launcher, muzzle suppressor vs
brake, modular handguard, and scope mounts.

The 64-base count is a **proposed artwork scope**, not a count of finished icons.
A reused class resolves to the base class's asset and review status. Historic
rejected SVG drafts and rejected conversion reviews stay rejected and must not
be shipped. A proposed base can still be refined or split after visual review.
No runtime UI changes have been made. Concept-sheet approvals do not
automatically approve raster images or SVG files.

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

The 12 pilot SVGs are individually **approved** and remain valid fallback assets. Their vector-specific constraints do not govern future raster artwork, and their approval does not extend to new or converted icons.

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

For each new artwork candidate verify:

- Readability at the actual proposed UI size (compare 22, 32, and 48 px); do not mistake an enlarged mock-up for pixel-accurate output.
- Recognizability against the individually approved concept, consistent optical centering, and clear distinction from neighboring classes.
- No detached or accidental shapes, unwanted background, text, or brand/logo leakage.
- Actual transparency and HUD recoloring/tint in the target PrismaUI presentation.
- SVG geometry/viewBox or raster image dimensions, media type, and filename pass format-specific validation.
- Final acceptance requires visual review. Local rendering alone does not prove in-game compatibility.

The current `data/icon_library/icon_artwork_manifest.json` remains authoritative for **existing SVG research assets**. PNG/WebP production and installed-library discovery require separately tested resolver and validator changes.
