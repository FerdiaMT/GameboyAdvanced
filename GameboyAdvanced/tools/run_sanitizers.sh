#!/usr/bin/env bash
set -euo pipefail

asan_library="$(c++ -print-file-name=libasan.so)"
ubsan_library="$(c++ -print-file-name=libubsan.so)"
if [[ "$asan_library" == libasan.so || ! -e "$asan_library" || "$ubsan_library" == libubsan.so || ! -e "$ubsan_library" ]]; then
    echo "Sanitizer runtimes are missing. On Fedora: sudo dnf install libasan libubsan" >&2
    exit 1
fi

cmake --preset sanitize
cmake --build --preset sanitize --parallel
echo "Running CTest under AddressSanitizer + UBSan..."
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 \
    ctest --preset sanitize --output-on-failure "$@"
echo "Sanitizer run passed: no AddressSanitizer or UBSan findings."
