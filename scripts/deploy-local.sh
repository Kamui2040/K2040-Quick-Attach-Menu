#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: deploy-local.sh [--dry-run] [--game-data PATH]

Copies the staged dist/Data tree into an explicitly selected Fallout 4 Data directory.

Options:
  --dry-run          Show the source, destination, and files without copying.
  --game-data PATH   Fallout 4 Data directory.
  -h, --help         Show this help.

If --game-data is omitted and XSE_FO4_GAME_PATH is set, its Data subdirectory is used.
EOF
}

dry_run=false
game_data=""

while (($#)); do
    case "$1" in
        --dry-run)
            dry_run=true
            shift
            ;;
        --game-data)
            [[ $# -ge 2 ]] || { echo "FAIL: --game-data requires a path" >&2; exit 2; }
            game_data="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "FAIL: unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$(cd -- "$script_dir/.." && pwd -P)"
dist_data="$project_root/dist/Data"
expected_dll="$dist_data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll"

if [[ -z "$game_data" && -n "${XSE_FO4_GAME_PATH:-}" ]]; then
    game_data="${XSE_FO4_GAME_PATH%/}/Data"
fi

[[ -d "$dist_data" ]] || { echo "FAIL: staged Data directory not found: $dist_data" >&2; exit 1; }
[[ -f "$expected_dll" ]] || { echo "FAIL: staged DLL not found: $expected_dll" >&2; exit 1; }
[[ -n "$game_data" ]] || { echo "FAIL: specify --game-data or set XSE_FO4_GAME_PATH" >&2; exit 1; }
[[ -d "$game_data" ]] || { echo "FAIL: game Data directory not found: $game_data" >&2; exit 1; }

game_data="$(realpath -e -- "$game_data")"

printf 'SOURCE=%s\n' "$dist_data"
printf 'DESTINATION=%s\n' "$game_data"

if $dry_run; then
    echo "DRY_RUN=yes"
    find "$dist_data" -type f -printf '%P\n' | LC_ALL=C sort
    exit 0
fi

cp -a -- "$dist_data/." "$game_data/"

[[ -f "$game_data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll" ]] || {
    echo "FAIL: deployment completed but expected DLL is missing" >&2
    exit 1
}

echo "PASS: local deployment complete"
