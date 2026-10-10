# Optional installed-library icon bridge (research)

The Quick Attach Menu icon classifier remains source-neutral: live attachment
points and names determine semantic icon classes, not artwork availability.
The library is optional and must not change attachment validity, dependencies,
installation, removal, or category ordering.

## Current evidence

FallUI Icon Library contains named SWF symbols. Its named icons are Flash
vector shapes, not embedded PNG/WebP images. Many classes represent complete
weapons or inventory categories rather than specific weapon components. A
symbol name match alone does not prove a suitable attachment-class match.

An isolated, read-only probe can inspect CWS/FWS SWF symbol tables and simple
sprite-to-shape references. A **research-only** local converter can render
single-shape symbols to transparent PNG and lossless WebP using separately
installed FFDec (Java) and ImageMagick. The converter does **not** support
multi-shape sprites, generalized SWF transformations or timelines. It does not
integrate with the game.

The original installed SWF is never changed. Third-party artwork, symbol
exports, converted images, FFDec binaries and QA captures must not be
committed, shipped, or installed as Quick Attach Menu assets. Any private
preview/cache must remain player-local.

## Read-only inspection

Run from a Linux checkout, pointing at an independently installed icon SWF:

```bash
python3 scripts/inspect-fis-icons.py --swf "/path/to/Data/Interface/FallUI_IconLib.swf"
python3 scripts/inspect-fis-icons.py --swf "/path/to/Data/Interface/FallUI_IconLib.swf" --list
```

Inspection requires only Python's standard library. A file being present in a
mod manager's download/staging area is not proof it is deployed or active.

For an **optional local preview**, obtain the FFDec JAR yourself, provide
Java and ImageMagick on PATH, and choose an output folder outside the repo:

```bash
python3 scripts/preview-fis-icons.py \
  --swf "/path/to/Data/Interface/FallUI_IconLib.swf" \
  --ffdec-jar "/path/to/ffdec.jar" \
  --output "/path/to/private/cache" \
  --symbol M8r.Repo.Mod --size 48
```

This preview is a validation aid, not a dependency, mod installer, or
player-facing setup procedure. It deliberately rejects composite symbols
instead of silently exporting incomplete artwork. Generated PNG/WebP files
are third-party derived assets and remain outside Git.

## Validation boundary

The tools validate installed SWF structure and local raster-image output.
They do not test target PrismaUI image loading, HUD tint, deployment detection,
or in-game rendering. There is no runtime FIS integration in this research
branch, and neither tool is a required dependency of the released mod.
The existing presentation fallback and semantic classifier are unchanged.
