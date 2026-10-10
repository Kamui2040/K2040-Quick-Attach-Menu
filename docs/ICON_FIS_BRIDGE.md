# Optional FallUI icon bridge

The experimental icon bridge reads the **player's deployed** `FallUI_IconLib.swf`
from Fallout 4's `Data/Interface` directory. It never bundles or modifies
FallUI/FIS artwork. Missing, unsupported or malformed libraries fail closed
to the existing text-only menu.

The optional native reader (`FisSwfVectors`) decompresses SWF CWS data using
statically linked zlib, parses supported Shape3 paths and PlaceObject2
transforms, and supplies bounded vector geometry to the PrismaUI renderer.
PrismaUI uses inline SVG to draw the installed icons and inherit the active UI
text color. It does not need Java, FFDec, ImageMagick or an external web service.

The renderer independently supports trusted PNG/WebP data URIs for original
mod-owned icons; the FIS bridge does not create these raster assets. No
decompiled FIS files or converted images are saved in the repository, game
installation, or runtime cache.

Only two conservative visual mappings are enabled in this initial integration:
`attachment.generic` -> `m_M8r.Repo.Mod` and
`ammo_caliber.generic` -> `m_M8r.Fo4Wpn.Ammo`.
They apply only to the matching source category labels
`Attachments`, `Ammo`, `Ammunition` and `Caliber`.
They do not claim to represent specific receivers, barrels, sights or stocks.

The mapping is presentation-only: it cannot change equipped OMOD identity,
attachment compatibility, dependency order, inventory, or install/remove
authorization. Cascade and horizontal rows can display icons; the radial and
hybrid presentations retain text-only labels. Normal missing-image fallback
does not change menu operation.

## Validation

Read-only symbol inspection and an isolated native geometry probe are included
under `scripts/`. The Python and Node tests are under `tests/`.
The local FFDec preview helper is **developer-only** and never a player
runtime requirement.

A clean Windows x64 build and static tests are necessary but insufficient.
Before the feature is merged or shipped it must pass Fallout 4/PrismaUI
in-game visual and lifecycle QA, including optional-library absence,
game load/reload, menu switching, theme colors and clean exit. The current
public release remains text-only.
