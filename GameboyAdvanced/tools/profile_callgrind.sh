#!/usr/bin/env bash
set -euo pipefail

if ! command -v valgrind >/dev/null || ! command -v callgrind_annotate >/dev/null; then
    echo "Callgrind requires valgrind. On Fedora: sudo dnf install valgrind" >&2
    exit 1
fi

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 [output-dir] -- <GBA arguments>" >&2
    echo "Example: $0 build/callgrind -- run bin/sma.gba --bios bin/gba_bios.bin --steps 1000000" >&2
    exit 2
fi

output_dir=build/callgrind
if [[ $1 != -- ]]; then
    output_dir=$1
    shift
fi
if [[ ${1:-} != -- ]]; then
    echo "Expected -- before GBA arguments." >&2
    exit 2
fi
shift
if [[ $# -eq 0 ]]; then
    echo "Missing GBA arguments." >&2
    exit 2
fi

cmake --preset release
cmake --build --preset release --parallel
mkdir -p "$output_dir"
profile="$output_dir/callgrind.out"
report="$output_dir/callgrind.txt"
valgrind --tool=callgrind --callgrind-out-file="$profile" ./GBA "$@"
callgrind_annotate --auto=yes "$profile" > "$report"
echo "Profile: $profile"
echo "Annotated report: $report"
