# Icon Semantic Library

This research layer maps attachment display names to a small, reusable icon vocabulary.
It is presentation-only and never participates in attachment compatibility, installed
identity, inventory checks, or mutation authorization.

## Classification model

Classification is slot-first. When an attachment-point / slot identity is available, it determines the broad icon category. The display name may only refine the subtype within that category; it cannot move an attachment into a different top-level category.

Classification starts with the physical/functional location on the weapon:

Receiver, Barrel, Handguard, Muzzle, Sights, Stock, Pistol Grip,
Magazine, Underbarrel, Mount, Rail Accessory, Ammo/Caliber, Internal,
Cosmetic, Special, plus a neutral Grip/Stock fallback for ambiguous shared slots.

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
6. If an exact name is known in more than one top-level category, slot context is required; name-only matching deliberately falls back rather than guessing.
7. Otherwise use the generic attachment icon.

Never use icon classification as evidence that an OMOD is compatible with a weapon.

## Visual icon classes

The semantic taxonomy is intentionally more detailed than the artwork set. Every semantic subtype maps to exactly one reusable visual `icon_class`. The current taxonomy has 172 semantic subtypes but only 95 visual classes including the final generic fallback.

Examples:

- `receiver/light`, `receiver/hardened`, `receiver/powerful` -> `receiver.generic`
- `muzzle/compensator`, `muzzle/muzzle_brake`, `muzzle/hybrid_device` -> `muzzle.brake_compensator`
- `sights/reflex_red_dot` -> `sights.reflex`
- `magazine/power_cell`, `magazine/canister`, `magazine/fuel_tank` -> `magazine.energy_feed`

Category fallbacks reuse an existing broad icon class instead of requiring separate fallback-only artwork. Only `attachment.generic` exists solely as the last-resort global fallback.

`scripts/list-icon-classes.py` prints the semantic visual classes grouped by category. `data/icon_library/icon_artwork_manifest.json` is the production manifest, and `docs/ICON_ART_DIRECTION.md` defines the 24x24 SVG style, three production batches, and the 12-icon pilot review gate.

## Shared base artwork

The 95 stable icon_class identifiers are **not** 95 mandatory SVG designs.
The artwork manifest has a flat, single-hop artwork_reuse.class_aliases
mapping: a class either owns its base illustration or points to another
existing class's base asset. The current proposal is **65 distinct base
illustrations** and **30 shared class aliases**. Original approved pilot
artwork is never aliased.

Resolution for future presentation code:

1. Classify the attachment normally, producing the established icon_class.
2. Look up artwork_reuse.class_aliases for that icon_class.
3. If an alias exists, use its base_artwork_id; otherwise use icon_class.
4. Render the base artwork only after that SVG is produced and approved;
   until then retain a safe category or global fallback.
5. Display the original semantic label, not the base artwork name.

This is an authored presentation plan, not a shipped runtime resolver.
Shared artwork never changes compatibility, installation, category, or
subtype data. Older rejected class-specific draft SVGs do not become
acceptable when that class is mapped to a different base icon.

## Data files

- data/icon_library/attachment_taxonomy.json: categories/subcategories.
- data/icon_library/icon_classes.json: semantic subtype -> reusable visual artwork class mapping.
- data/icon_library/icon_artwork_manifest.json: filenames, artwork briefs, batches, pilot set, and approval status.
- data/icon_library/attachment_aliases.json: known display-name vocabulary.
- data/icon_library/attachment_slot_patterns.json: slot/AP to broad-category rules.
- data/icon_library/canonical_attach_points.json: verified vanilla attachment-point identities.
- data/icon_library/community_attach_points.json: mapped standardized community weapon slots used for broad-category classification.
- data/icon_library/weapon_names.json: weapon-name vocabulary kept separate from attachments.
- data/icon_library/vanilla_ranged_catalog.json: derived vanilla weapon/category/mod-name catalog.
- data/icon_library/wars_catalog_seed.json: structured WARS weapon/attachment research seed.
- data/icon_library/weapon_mod_package_index.json: high-yield public weapon-mod coverage indexes.
- data/icon_library/sources.json: public research provenance.

The corpus is intentionally extensible. Nexus weapon mods are a primary vocabulary source
because modern weapon projects expose far more named parts than the vanilla/DLC game.
Large compatibility projects are kept in `weapon_mod_package_index.json` as discovery
indexes; their package titles are not treated as runtime identities or automatically promoted
to canonical weapon names. Actual runtime structure remains live-game authoritative per
docs/ARCHITECTURE.md.

## External OMOD audit

`scripts/audit-fandom-omod-corpus.py` audits the public Fallout Wiki weapon OMOD table
against the classifier. The wiki reports its content license as CC-BY-SA. Raw wiki source
and descriptions are not committed; the audit fetches the public MediaWiki API (or accepts
a local wikitext file) and reports classification coverage only.

```bash
python3 scripts/audit-fandom-omod-corpus.py
```

This audit is research/QA only and is not required by the shipped mod.

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
