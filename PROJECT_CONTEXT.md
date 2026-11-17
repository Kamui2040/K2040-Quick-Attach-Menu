# Current Project State

Last verified: 2026-11-17. This file contains concise contributor-safe current
state. Historical release changes live in `CHANGELOG.md`, technical decisions
in `docs/DECISIONS.md`, and unresolved issues in `docs/KNOWN_ISSUES.md`.

## Published release

- Version **0.5.222** is published on [GitHub](https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.222)
  and [Nexus Mods](https://www.nexusmods.com/fallout4/mods/109575).
- The published release retains the historical per-mod MCM configuration and
  offers an optional Categorizer fallback. Current VNext work is unreleased
  and uses a separately installed shared settings hub instead.

## Active development

- Draft PR #52, `fix/unified-visual-style-mcm-cleanup`, builds on draft
  PR #51 and earlier, still-open feature work. The source review used
  `13ac4f2` as its baseline; later documentation edits do not constitute
  new in-game acceptance. Neither has
  been accepted for public release.
- The six presentations are Cascade, Horizontal Bar, Radial, Compact Hybrid,
  Orbit and Clock Slices. The Clock child fan starts at the selected visible
  parent, fills the measurable 360-degree circumference with whole-word
  one-/two-line labels and a positive seam gap, and fits only visible
  candidates on CPU redraws.
- Radial/Hybrid labels retain readable lines; Hybrid uses a moving
  seven-category window. Display shortening never changes attachment names,
  resolved OMOD identity, inventory checks or operations.
- The four optional Framework theme treatments decorate Builder and Quick
  Menu frames without modifying option targets, opacity or attached state.
  PrismaUI CPU output is the development visual baseline; GPU acceptance is
  not established by source tests.
- Quick Attach's V2 Settings Adapter exposes its controls to the separate
  two-pane hub. The hub owns general settings UI and MCM shortcut; QAM owns
  field validation, preferences, gameplay shortcuts, physical polling and
  attachment transactions. The retired standalone settings view stays in
  source but is excluded from current owner-only installers.
- Builder's Settings button requests the QAM hub section on the game thread;
  it releases view focus on acceptance and stays open on refusal.
- QAM-only installation uses a reviewed 13-file runtime allowlist, rejecting
  symlinks and unrelated staged files. No Framework DLL or QAM MCM page is
  bundled in the VNext candidate.

## Safety and validation

- Runtime changes require exact equipped object-instance OMOD identity,
  provider-before-child installation, child-before-provider removal,
  inventory-count bounds, live graph checks and verifiable rollback.
  Unsupported or ambiguous operations must fail closed.
- The 2026-11-17 cross-project audit reported 23/23 JavaScript tests and
  29/29 Python script runs passing, with matching shared settings contracts.
  That pass did not rebuild the native plugin or run Fallout 4.
- Open target-runtime QA on OG 1.10.163: six presentations, short and long
  text at varied scales, CPU rendering, keyboard/controller navigation,
  Builder-to-hub focus, palettes, persistence, attachment transactions and
  package installation. Do not merge/release based on static evidence alone.

## Documentation ownership

- `AGENTS.md` — durable rules, execution gates and validation.
- `docs/ARCHITECTURE.md`, `docs/DECISIONS.md` — native behavior and design.
- `docs/UNIFIED_SETTINGS_EDITOR_V2_INTEGRATION.md`,
  `docs/SETTINGS_SCHEMA_V1_AUDIT.md` — integrated settings contract.
- `docs/KNOWN_ISSUES.md`, `docs/COMPATIBILITY.md`, `CHANGELOG.md` —
  unresolved limits, published compatibility and release history.
