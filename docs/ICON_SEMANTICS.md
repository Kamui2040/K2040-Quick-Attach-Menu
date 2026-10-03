# Icon Semantic Library

This research layer maps attachment display names to a small, reusable icon vocabulary.
It is presentation-only and never participates in attachment compatibility, installed
identity, inventory checks, or mutation authorization.

## Classification model

Classification is slot-first. When an attachment-point / slot identity is available, it determines the broad icon category. The display name may only refine the subtype within that category; it cannot move an attachment into a different top-level category.

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

1. Attachment-point / slot identity determines the broad category when available.
2. Exact known alias may refine the subtype only inside that slot category.
3. Conservative name token/pattern matching may refine the subtype only inside that slot category.
4. If no subtype is known, use the slot category fallback icon.
5. Only when no slot identity is available, use name-only alias/pattern classification.
6. Otherwise use the generic attachment icon.

Never use icon classification as evidence that an OMOD is compatible with a weapon.

## Data files

- data/icon_library/attachment_taxonomy.json: categories/subcategories.
- data/icon_library/attachment_aliases.json: known display-name vocabulary.
- data/icon_library/weapon_names.json: weapon-name vocabulary kept separate from attachments.
- data/icon_library/sources.json: public research provenance.

The corpus is intentionally extensible. Nexus weapon mods are a primary vocabulary source
because modern weapon projects expose far more named parts than the vanilla/DLC game.
Actual runtime structure remains live-game authoritative per docs/ARCHITECTURE.md.

## xEdit research import

`scripts/extract-icon-candidates.py` accepts JSON created by K2040 xEdit JSON Exporter and
extracts WEAP/OMOD naming candidates. Known OMOD names are classified through the same
semantic alias/pattern library; unknown names can be emitted for manual review. Exported
game/mod data remains local research material and must not be committed.

Example:

```bash
python3 scripts/extract-icon-candidates.py /path/to/export.json --unknown-only --output /tmp/icon-candidates.json
```

This tooling is development-only. The shipped mod does not read xEdit exports at runtime.
