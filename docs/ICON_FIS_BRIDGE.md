# Optional FallUI icon bridge

Quick Attach Menu can read the **player's deployed** FallUI Icon Library SWF
from Fallout 4's `Data/Interface` directory. The library is optional.
It does not change attachment eligibility, ordering, install/remove
permissions, or equipped-state logic.

## Current research implementation

- The game-side `FisSwfVectors` reader opens the deployed compressed
  `FallUI_IconLib.swf`, decompresses CWS data with statically linked zlib,
  and reconstructs supported Flash Shape3 paths and PlaceObject2 transforms
  **in memory**. It does not copy, modify, or extract the installed SWF to disk.
- The native payload exposes a bounded `iconVectors` object. PrismaUI renders
  this geometry directly as monochrome inline SVG using the active UI text
  color. This preserves the original FIS silhouettes without the rejected
  process of hand-redrawing detailed artwork as 22px symbols.
- The earlier `iconAssets` contract also supports validated transparent
  PNG/WebP data from a trusted native source; it is not used by the FIS reader.
- Cascade/horizontal list rows accept optional symbols; radial and hybrid
  layouts currently retain text-only labels. Missing library, invalid SWF,
  unsupported geometry and unmatched classes fall back to text.
- The first **conservative proof-of-concept mappings** are
  `attachment.generic` -> `m_M8r.Repo.Mod` and
  `ammo_caliber.generic` -> `m_M8r.Fo4Wpn.Ammo`. They are applied only to
  source category labels `Attachments`, `Ammo`, `Ammunition`, or `Caliber`.
  These symbols must **not** be passed off as receiver/barrel/sight/stock icons.
  Broader subtype coverage requires individually reviewed mappings.

The reader is intentionally limited to the installed FallUI Icon Library's
supported Shape3 and PlaceObject2 subset. It fails safely for unsupported
Flash features or file formats; it does not interpret ActionScript. The native
bridge caches the inventory for the current game process, so external mod
changes require a normal game restart.

This implementation is on a **research branch**, not the released plugin.
C++ compile success and browser-side tests do not constitute in-game
compatibility or final visual approval.

## Local inspection and developer-only tests

For portable, read-only icon inventory:
```bash
python3 scripts/inspect-fis-icons.py --swf "/path/to/Data/Interface/FallUI_IconLib.swf"
```

For an independently compiled native geometry probe on Linux:
```bash
c++ -std=c++20 -O2 -Iinclude \
  scripts/fis-swf-vector-probe.cpp src/FisSwfVectors.cpp \
  -lz -o "/path/to/private/probe"
"/path/to/private/probe" "/path/to/Data/Interface/FallUI_IconLib.swf"
```

The older FFDec/ImageMagick preview script remains a developer-only reference
comparison method; neither is a player dependency.

The local tests validated 217 vector shapes and 209 exported symbols from
one installed FallUI Icon Library copy. Three native-silhouette alpha masks
overlapped reference shape renders by approximately 96.5%–100% at 48px,
without claiming gradient/shading fidelity. No other versions have been
confirmed by that test.

## Asset and validation boundaries

Quick Attach Menu does not include or redistribute FIS/FallUI SWFs, symbols,
rasterizations, converted images, or decompiler files. Generated developer
reference images and third-party source SWFs remain outside the repository
and release archives. Runtime reads come only from the game's own deployed
Data directory, not an assumed Amethyst or other mod-manager path.

Before merging or releasing this bridge, validate in a real supported
Fallout 4 + PrismaUI + Proton session: library present/absent, icon visibility
at the intended display size, theme recoloring, fallback, malformed or
unsupported SWF, view reopen, game reload, input handling and clean exit.
The player should not need to install Java, FFDec, ImageMagick, or a zlib DLL.
