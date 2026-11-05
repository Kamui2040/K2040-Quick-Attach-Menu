# Build and restore Quick Attach Menu on Linux

The [current GitHub release v0.5.222](https://github.com/Kamui2040/K2040-Quick-Attach-Menu/releases/tag/v0.5.222) contains the installation ZIP, SHA-256 checksum and optional MCM Categorizer compatibility file. **Installing the release does not require a compiler or Windows SDK.** Use a mod manager and the matching Fallout 4, F4SE, and PrismaUI versions listed in [COMPATIBILITY.md](COMPATIBILITY.md).

These instructions set up a Linux environment to build the Windows x64 F4SE plugin from a fresh GitHub clone.

## Required build inputs

- Git, Bash, Python 3, Node.js, native C++23 compiler, xmake 3.x and LLVM tools `clang-cl`, `lld-link` and `llvm-rc`. The 0.5.222 build and checks passed with xmake 3.0.6+20260117, LLVM 22.1.8, Python 3.14.8, Node 22.23.1 and GCC 16.2.1. LLVM 23.1.5 also built the DLL.
- A legally obtained MSVC/Windows SDK tree with `VC/Tools/MSVC/<version>/...` and `Windows Kits/10/...`. Validated x64 inputs: MSVC 14.44.35207 or 14.51.36231 with SDK 10.0.26100.0. These Microsoft files are not stored in this repository.
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

For SDK trees containing multiple versions, set `K2040_MSVC_VERSION` and `K2040_WINSDK_VERSION` to the desired existing folder names. The helper checks required headers, import libraries, and pinned dependencies. It creates temporary Linux aliases for `clang-cl.exe` and `link.exe`, selects `llvm-rc`, then builds a Windows x64 `releasedbg` DLL. It checks the DLL and does not install it. Expected path: `dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll`.

A fresh checkout was verified with MSVC 14.44.35207 and Windows SDK 10.0.26100.0. The build helper searches Clang's own intrinsic headers before the MSVC and SDK headers so inline Windows runtime code compiles correctly.

## Test and package after rebuilding

```bash
python3 -m unittest discover -s scripts/tests -p 'test_*.py'
for test_file in scripts/tests/*.test.cjs; do node "$test_file"; done
c++ -std=c++23 -Wall -Wextra -Werror -Iinclude scripts/tests/hub-chord-policy.test.cpp -o /tmp/k2040-quick-attach-chord-test
/tmp/k2040-quick-attach-chord-test
python3 scripts/package-test.py --output /tmp/K2040-Quick-Attach-TEST.zip
git diff --check
```

Check the PE header and imports, version, ZIP integrity, shared MCM files, and that no user settings were packaged. The package check does not install the DLL or test it in game. The published release ZIP remains the source for restoring the exact released build.

## Recovery boundaries

The public repository contains the source, build scripts, and documentation. CommonLibF4 is fetched from its upstream repository at the pinned revision. Obtain the Microsoft build files separately and lawfully. The release archive includes the shared K2040 Mods MCM parent, so no separate Hub download is needed. Game saves, mod-manager profiles, credentials, and desktop settings are outside this project.
