#!/usr/bin/env bash
set -euo pipefail

if ! command -v gcovr >/dev/null; then
    echo "Coverage reports require gcovr. On Fedora: sudo dnf install gcovr" >&2
    exit 1
fi

cmake --preset coverage
cmake --build --preset coverage --parallel
ctest --preset coverage --output-on-failure "$@"
mkdir -p build/coverage/report
gcovr --root . --object-directory build/coverage \
    --filter 'source/' --filter 'include/' \
    --txt --html-details build/coverage/report/index.html
echo "Coverage report: build/coverage/report/index.html"
