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

## Browser-side image contract

The PrismaUI quick-menu renderer has an optional image slot in **cascade and
horizontal list rows**. No icon is shown unless a trusted native payload
supplies both a class ID and matching image bytes. The image property has no
relationship to attachment validity, installation, or menu ordering.

- `parser.categories[n].iconClass`: an optional semantic class for a category.
- `parser.categories[n].options[m].iconClass`: an optional class for an option.
- `iconAssets`: an optional top-level object mapping exact class IDs to
  `data:image/png;base64,...` or `data:image/webp;base64,...` strings.
- Keys must match the existing dot-separated `icon_class` vocabulary.
- Only self-contained PNG/WebP base64 sources are accepted; file paths,
  URLs, SVG and scriptable schemes are rejected by the browser helper.
- Where browser image masks work, the alpha channel takes the theme's text
  color. Otherwise the original PNG/WebP image is displayed.
- Missing symbols/assets leave original text-only rows intact.
- Radial and hybrid labels remain text-only for now.

This is an integration seam only. The current native payload does **not** yet
populate `iconClass` or `iconAssets`. A future native SWF renderer must
read the player's deployed file, resolve reviewed source-neutral icon-class
mappings, render a bounded set of selected icons, and supply the resulting
images without external services or runtime installation tools. Do not
mistake sample browser-data-URI tests for proof that target PrismaUI can
decode both formats.

## Validation boundary

The research tools validate installed SWF structure and local raster output.
The browser helper has deterministic tests for safe URL filtering, masks,
raster fallback, and missing-image behavior. The native bridge and its
real-world PrismaUI rendering, HUD tint, deployment detection, and game
lifecycle have **not** been validated in game. Neither FFDec nor Java nor
ImageMagick is a required dependency of the released mod. No FIS-derived
assets may be committed to Git or packed with the plugin.
