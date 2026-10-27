# Repository Instructions

## Read order

Before changing the project, read this file and `PROJECT_CONTEXT.md`, then use:

- `docs/ARCHITECTURE.md` and `docs/DECISIONS.md` for runtime data-model work;
- `docs/COMPATIBILITY.md` and `docs/KNOWN_ISSUES.md` for dependency changes;
- `README.md`, `xmake.lua`, and `scripts/` for build work.

## Repository boundary

Keep only public-safe source, deterministic tooling, required runtime assets,
reproducible inputs, licensed third-party files, and contributor-facing
documentation in Git.

Do not commit generated DLLs or packages, logs, screenshots, game files, load
orders, machine-specific paths, credentials, private QA, recovery material, or
unrelated third-party assets. GitHub Actions are not enabled for this project.

## Runtime invariants

- Target Fallout 4 `1.10.163`, matching F4SE, and PrismaUI_F4 `2.1.1` unless a
  compatibility change is explicitly reviewed, rebuilt, and runtime-tested.
- Do not add an xEdit JSON runtime dependency.
- Keep authored presentation separate from live structural validity.
- Use resolved object-instance OMODs for installed identity.
- Do not call `BGSMod::Attachment::Mod::GetData` on NG/AE; CommonLib marks it inlined there. Use the inherited `BGSMod::Container::GetData` when only attachment/property container data is needed.
- Install providers before children and remove children before providers.
- Persist form identity by source plugin and local FormID/FormKey.
- Treat every browser selection and imported profile as untrusted input.
- Revalidate the live equipped stack, inventory, graph, and selected operation
  before mutation. Unsupported or ambiguous operations fail closed.
- Profiles contain presentation preferences only and never weaken live checks.
- A closed Prisma panel is not reused. Game-load and new-game transitions clear
  consumer-owned views so the next open creates a fresh view.
- Closing or switching a Prisma menu must cancel pending DOM-ready focus so a closed menu cannot be focused again later.
- Do not reset hotkey edge state when capture state did not actually change; a held opener must not become a second synthetic press during menu switching.
- Prisma browser hotkey forwarding is supplemental, not exclusive. Under Proton/CEF, Mouse 4/5 may not emit DOM mouse events while the view is focused, so the native poller must remain active. Mouse-button held/release state uses the `GetAsyncKeyState` high-order physical-down bit only; do not fold the low-order pressed-since-last-query bit into held state. Cross-source duplicate opener signals must be suppressed before queuing game-thread work.
- Browser-forwarded actions and browser close requests must queue game work through the F4SE task interface.
- Browser key-capture events are not authoritative under Proton/CEF: a
  modifier-only press may carry synthetic `keyCode=144` (`VK_NUMLOCK`). Reject
  modifier/lock-only capture codes at both browser and native Settings save
  boundaries before mutating MCM bindings. Do not treat MCM's in-memory
  keybind manager as synchronized merely because Keybinds.json was updated.
- Native MCM owns live keybind registrations and saves them to Keybinds.json
  on MCM menu close. When `mcm.dll` is loaded, never write that registry from
  Prisma Settings or use `SetKeybind` (registration) to change an existing
  binding. Queue `RemapKeybind`, verify with `GetKeybind`, and discard newer
  MCM edits rather than overwriting them. MCM-cleared entries stay cleared.
- MCM can change its live chord-keyed registry after PauseMenu opens. A
  single startup duplicate check is insufficient: re-examine changed stable
  live snapshots without modifying another mod's binding, and verify any
  surviving chord after narrowly scoped cleanup. Never erase an ambiguous
  Unbound or conflicting registration merely to make a picker display a key.
- Shared native MCM registry saves must preserve every other mod's binding.
  Some runtimes prevent atomic replacement of an open registry file; an
  alternate write requires a verified recovery copy, a byte-for-byte unchanged
  source, post-write verification, error logging and fail-closed behavior.
- Native MCM keybind definitions must use a type its keybind manager supports. Use inert `RunConsoleCommand` for Quick Attach openers and keep native polling authoritative; `CallExternalFunction` is not a valid MCM keybind registry action.
- Never release a Prisma menu's gameplay-input layer while a keyboard/mouse opener that closed it is still held: the physical key may reach the underlying Pip-Boy. Defer only this mod's release until physical key-up with a bounded failsafe, queue engine cleanup to the game thread, cancel stale work on reopen and always release on game transition.
- Before calling any runtime-specific native relocation, verify its exact
  Address Library ID exists for that game version and that the destination
  is executable. The relocation check alone does not validate a calling
  convention or side effects; both require independent runtime evidence.
- Runtime success requires target-environment evidence.

## Build and validation

Builds target a Windows x64 F4SE DLL from Linux. CommonLibF4 is supplied through
`K2040_COMMONLIBF4_ROOT`; the Windows SDK and LLVM paths are local configuration.
Cross-runtime builds require DCCStudios/CommonLibF4 revision
`12beba2a89fe117a14f1707b88c99ecb1b12f8c0` with commonlib-shared revision
`f0b1670ee9caac2e349497f6f3c08a69633a8ea7`. The configure step rejects other
revisions so a single-runtime or incompatible dependency cannot produce a
candidate DLL.

```bash
export K2040_COMMONLIBF4_ROOT="/path/to/commonlibf4"
export PATH="$K2040_LLVM_TOOL_DIR:$PATH"
xmake f -p windows -a x64 -m releasedbg \
  '--toolchain=clang-cl[llvm]' \
  --sdk="$K2040_WINDOWS_SDK_ROOT" \
  --ar="$K2040_LLVM_TOOL_DIR/link.exe" \
  '--ldflags=/MANIFEST:NO' \
  '--shflags=/MANIFEST:NO' \
  --linkdirs="$K2040_WINDOWS_LINK_DIRS"
xmake -r
```

Expected output:

`dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll`

For every runtime-code change:

1. keep source and build versions synchronized;
2. run formatting/syntax checks and `git diff --check`;
3. perform a clean configure/build;
4. verify the x64 PE DLL and expected imports;
5. review the complete changed-file, privacy, and license scope;
6. perform focused target-runtime testing before claiming runtime success.

Build staging and local deployment are separate. The build must not auto-deploy
to a game or mod-manager directory.
