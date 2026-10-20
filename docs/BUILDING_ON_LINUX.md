# Build and restore Quick Attach Menu on Linux

The [GitHub release](https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.216) contains the tested installation ZIP and SHA-256 checksum. **Restoring the released mod does not require a compiler or Windows SDK.** Verify the archive and install it using a mod manager with matching Fallout 4, F4SE and PrismaUI runtime dependencies documented in [COMPATIBILITY.md](COMPATIBILITY.md).

These instructions recover the **development environment** to compile a Windows x64 F4SE DLL from a fresh public GitHub clone. The maintainer host is Linux x86_64, not Windows.

## Required build inputs

- Git, Bash, Python 3, Node.js, native C++23 compiler, xmake 3.x and LLVM tools `clang-cl`, `lld-link` and `llvm-rc`. Successful tests used xmake 3.1.1+20260827, LLVM 23.1.0, Python 3.15.0 and Node 26.11.1.
- A legally obtained MSVC/Windows SDK tree with `VC/Tools/MSVC/<version>/...` and `Windows Kits/10/...`. Validated x64 inputs: MSVC 14.44.35207 with SDK 10.0.26100.0; MSVC 14.51.36231 with the same SDK also built successfully. Windows SDK/MSVC binary files are not stored in public GitHub or ordinary Google Drive. Preserve lawful acquisition instructions.
- DCCStudios/CommonLibF4 and its pinned commonlib-shared submodule.

## Fresh source and CommonLibF4

```bash
git clone https://github.com/Kamui2040/K2040-Quick-Attach-Menu.git
git clone https://github.com/DCCStudios/commonlibf4.git commonlibf4
git -C commonlibf4 checkout 12beba2a89fe117a14f1707b88c99ecb1b12f8c0
git -C commonlibf4 submodule update --init --recursive
git -C commonlibf4/lib/commonlib-shared rev-parse HEAD
```

The final submodule revision must be `f0b1670ee9caac2e349497f6f3c08a69633a8ea7`; `xmake.lua` enforces both pins.

## Portable Linux build

```bash
cd K2040-Quick-Attach-Menu
export K2040_COMMONLIBF4_ROOT="/absolute/path/to/commonlibf4"
export K2040_WINDOWS_SDK_ROOT="/absolute/path/to/licensed-msvc-and-windows-sdk"
./scripts/build-linux.sh
```

For SDK trees containing multiple versions, set `K2040_MSVC_VERSION` and `K2040_WINSDK_VERSION` to the desired existing folder names. The helper verifies required headers/import libraries and pins, creates disposable Linux aliases for `clang-cl.exe` and `link.exe`, explicitly selects `--mrc=llvm-rc` and runs a clean xmake Windows x64 `releasedbg` build. It validates the Windows PE32+ DLL and does not auto-deploy. Expected DLL: `dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll`.

These two aliases and the explicit resource compiler corrected prior Linux xmake configuration errors. Using the preserved Games SSD MSVC 14.44.35207 / SDK 10.0.26100.0, a fresh checkout then built successfully.

## Test and package after rebuilding

```bash
python3 -m unittest discover -s scripts/tests -p 'test_*.py'
for test_file in scripts/tests/*.test.cjs; do node "$test_file"; done
c++ -std=c++23 -Wall -Wextra -Werror -Iinclude scripts/tests/hub-chord-policy.test.cpp -o /tmp/k2040-quick-attach-chord-test
/tmp/k2040-quick-attach-chord-test
python3 scripts/package-test.py --output /tmp/K2040-Quick-Attach-TEST.zip
git diff --check
```

Confirm PE header/imports, release version, ZIP CRC, MCM shared parent/child, and absence of user-specific settings. A verified fresh-clone build passed 41 Python tests, four JS suites and the native chord test. Its archive contained the same 13 members and matching non-DLL content as the released ZIP; only the newly compiled DLL differed byte-for-byte. The published release ZIP remains the source of truth for exact binary restoration. Deploy and test new builds in-game separately with appropriate permission.

## Recovery boundaries

Public GitHub contains project source, build/test scripts, documentation and the full release ZIP. External pinned CommonLibF4 can be fetched from upstream. Licensed MSVC and Windows SDK input files require independent lawful acquisition. The owner's private Google Drive contains handoff documentation and optional icon concepts, which are not required for the released 0.5.216 build. Fallout saves, mod-manager profiles, local credentials, and desktop settings are not project source artifacts. No separately installed K2040 MCM Hub is required; the shared parent ships with each K2040 mod.
