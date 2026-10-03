# Icon Semantic Library

This research layer maps attachment display names to a small, reusable icon vocabulary.
It is presentation-only and never participates in attachment compatibility, installed
identity, inventory checks, or mutation authorization.

## Classification model

Classification starts with the physical/functional location on the weapon:

Receiver, Barrel, Handguard, Muzzle, Sights, Stock, Pistol Grip,
Magazine, Underbarrel, Mount, Rail Accessory, Ammo/Caliber, Internal,
Cosmetic, and Special.

Each category has semantic subcategories. Examples:

- Muzzle -> Suppressor / Compensator / Muzzle Brake / Flash Hider / Bayonet
- Sights -> Iron / Reflex-Red Dot / Holographic / Prism / Scope / LPVO / Magnifier
- Underbarrel -> Vertical Grip / Angled Grip / Handstop / Bipod / Grenade Launcher
- Mount -> Optic Rail / Scope Mount / Riser / Offset Mount / Tri-Rail / Adapter
- Rail Accessory -> Laser / Flashlight / Laser+Light / Sensor
- Magazine -> Standard / Extended / Quick-Eject / Stick / Drum / Box / Belt

A named part such as PBS-1 Suppressor, Osprey 45, or KAC Suppressor therefore
shares the Muzzle -> Suppressor icon unless a future visual distinction is justified.

## Matching priority

1. Stable explicit adapter supplied by a supported library/mod.
2. Exact known alias.
3. Normalized known alias.
4. Conservative token/pattern match.
5. Known top-level slot/category fallback.
6. Generic attachment icon.

Never use icon classification as evidence that an OMOD is compatible with a weapon.

## Data files

- data/icon_library/attachment_taxonomy.json: categories/subcategories.
- data/icon_library/attachment_aliases.json: known display-name vocabulary.
- data/icon_library/weapon_names.json: weapon-name vocabulary kept separate from attachments.
- data/icon_library/sources.json: public research provenance.

The corpus is intentionally extensible. Nexus weapon mods are a primary vocabulary source
because modern weapon projects expose far more named parts than the vanilla/DLC game.
Actual runtime structure remains live-game authoritative per docs/ARCHITECTURE.md.
