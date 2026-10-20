#!/usr/bin/env bash
clear
printf '\033]30;QuickAttach\007'
printf '\033]50;TabColor=#B57CC9;TabActivityColor=#D3A3DF\007'
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd -P)
temp_root="${K2040_BUILD_TEMP_ROOT:-$HOME/Downloads/K2040-Quick-Attach-Menu}"
log="$temp_root/linux-build.log"
fail() {
    clear
    printf '\033]30;QuickAttach\007'
    printf '\033]50;TabColor=#B57CC9;TabActivityColor=#D3A3DF\007'
    printf 'FAIL: %s\n' "$1"
    [[ ! -f "$log" ]] || printf 'LOG=%s\n' "$log"
    exit 1
}
[[ -n "${K2040_COMMONLIBF4_ROOT:-}" ]] || fail "Set K2040_COMMONLIBF4_ROOT to the pinned CommonLibF4 checkout"
[[ -n "${K2040_WINDOWS_SDK_ROOT:-}" ]] || fail "Set K2040_WINDOWS_SDK_ROOT to a licensed MSVC/Windows SDK tree"
for name in git python3 xmake clang-cl lld-link llvm-rc; do
    command -v "$name" >/dev/null || fail "Required tool missing: $name"
done
sdk=$(realpath "$K2040_WINDOWS_SDK_ROOT") || fail "Windows SDK directory is missing"
lib=$(realpath "$K2040_COMMONLIBF4_ROOT") || fail "CommonLibF4 directory is missing"
[[ -d "$sdk/VC/Tools/MSVC" && -d "$sdk/Windows Kits/10/Lib" ]] || fail "SDK missing VC toolset or Windows Kits"
[[ -f "$lib/xmake.lua" ]] || fail "CommonLibF4 xmake.lua is missing"
[[ "$(git -C "$lib" rev-parse HEAD)" == 12beba2a89fe117a14f1707b88c99ecb1b12f8c0 ]] || fail "CommonLibF4 git revision does not match"
[[ "$(git -C "$lib/lib/commonlib-shared" rev-parse HEAD)" == f0b1670ee9caac2e349497f6f3c08a69633a8ea7 ]] || fail "commonlib-shared git revision does not match"
shopt -s nullglob
msvc_dirs=("$sdk"/VC/Tools/MSVC/*)
kit_dirs=("$sdk"/Windows\ Kits/10/Lib/*)
[[ ${#msvc_dirs[@]} -gt 0 && ${#kit_dirs[@]} -gt 0 ]] || fail "MSVC/Windows SDK version folders are missing"
if [[ -n "${K2040_MSVC_VERSION:-}" ]]; then
    msvc_version="$K2040_MSVC_VERSION"
elif (( ${#msvc_dirs[@]} == 1 )); then
    msvc_version=$(basename "${msvc_dirs[0]}")
else
    fail "Multiple MSVC toolsets; specify K2040_MSVC_VERSION"
fi
if [[ -n "${K2040_WINSDK_VERSION:-}" ]]; then
    kit_version="$K2040_WINSDK_VERSION"
elif (( ${#kit_dirs[@]} == 1 )); then
    kit_version=$(basename "${kit_dirs[0]}")
else
    fail "Multiple Windows SDKs; specify K2040_WINSDK_VERSION"
fi
[[ -f "$sdk/VC/Tools/MSVC/$msvc_version/lib/x64/libcmt.lib" ]] || fail "MSVC x64 libcmt.lib missing"
[[ -f "$sdk/VC/Tools/MSVC/$msvc_version/include/vcruntime.h" ]] || fail "MSVC header vcruntime.h missing"
[[ -f "$sdk/Windows Kits/10/Lib/$kit_version/um/x64/kernel32.lib" ]] || fail "Windows SDK x64 kernel32.lib missing"
[[ -f "$sdk/Windows Kits/10/Lib/$kit_version/ucrt/x64/ucrt.lib" ]] || fail "Windows SDK x64 ucrt.lib missing"
[[ -f "$sdk/Windows Kits/10/Include/$kit_version/um/Windows.h" ]] || fail "Windows SDK header Windows.h missing"
mkdir -p "$temp_root"
tools_dir=$(mktemp -d "$temp_root/llvm-tools.XXXXXXXX") || fail "Cannot create a temporary directory"
trap 'rm -rf -- "$tools_dir"' EXIT
ln -s "$(command -v clang-cl)" "$tools_dir/clang-cl.exe"
ln -s "$(command -v lld-link)" "$tools_dir/link.exe"
ln -s "$sdk" "$tools_dir/sdk"
ln -s "$lib" "$tools_dir/commonlib"
export PATH="$tools_dir:$PATH"
export K2040_COMMONLIBF4_ROOT="$tools_dir/commonlib"
windows_sdk="$tools_dir/sdk"
msvc_lib="$windows_sdk/VC/Tools/MSVC/$msvc_version/lib/x64"
um_lib="$windows_sdk/Windows Kits/10/Lib/$kit_version/um/x64"
ucrt_lib="$windows_sdk/Windows Kits/10/Lib/$kit_version/ucrt/x64"
cd "$repo"
if ! xmake f -c -p windows -a x64 -m releasedbg \
    '--toolchain=clang-cl[llvm]' \
    --sdk="$windows_sdk" --ar="$tools_dir/link.exe" \
    --mrc="$(command -v llvm-rc)" \
    '--ldflags=/MANIFEST:NO' '--shflags=/MANIFEST:NO' \
    --linkdirs="$msvc_lib:$um_lib:$ucrt_lib" > "$log" 2>&1; then
    fail "Windows x64 configuration failed"
fi
if ! xmake -r >> "$log" 2>&1; then
    fail "Windows x64 compilation failed"
fi
dll="$repo/dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll"
if ! python3 - "$dll" <<'PY' >> "$log" 2>&1
from pathlib import Path
import struct
import sys
data = Path(sys.argv[1]).read_bytes()
assert data[:2] == b'MZ'
pe = struct.unpack_from('<I', data, 0x3c)[0]
assert data[pe:pe+4] == b'PE\x00\x00'
assert struct.unpack_from('<H', data, pe+4)[0] == 0x8664
assert struct.unpack_from('<H', data, pe+24)[0] == 0x20b
PY
then
    fail "Built Windows x64 DLL header is invalid"
fi
rm -f -- "$log"
clear
printf '\033]30;QuickAttach\007'
printf '\033]50;TabColor=#B57CC9;TabActivityColor=#D3A3DF\007'
printf 'PASS: built Windows x64 DLL and verified PE header\n'
