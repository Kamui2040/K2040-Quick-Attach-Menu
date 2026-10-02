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
- Prisma browser hotkey forwarding is supplemental, not exclusive. Under Proton/CEF, Mouse 4/5 may not emit DOM mouse events while the view is focused, so the native poller must remain active and use the `GetAsyncKeyState` pressed-since-last-query bit as a mouse-button fallback. Cross-source duplicate opener signals must be suppressed before queuing game-thread work.
- Browser-forwarded actions and browser close requests must queue game work through the F4SE task interface.
- Runtime success requires target-environment evidence.

## Build and validation

Builds target a Windows x64 F4SE DLL from Linux. CommonLibF4 is supplied through
`K2040_COMMONLIBF4_ROOT`; the Windows SDK and LLVM paths are local configuration.

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
