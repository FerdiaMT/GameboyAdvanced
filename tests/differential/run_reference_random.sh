#!/usr/bin/env bash
set -euo pipefail

# Run a fresh, reproducible corpus through the CTest differential target.
# Usage: tests/differential/run_reference_random.sh [cases] [instructions] [seed] [dump-dir]
# The generator currently caps instructions at 15 so every program can remain
# hazard-free for the upstream reference RTL's incomplete forwarding path.

cases=${1:-50}
instructions=${2:-10}
seed=${3:-$(od -An -N4 -tu4 /dev/urandom | tr -d ' ')}
dump_dir=${4:-}

if ! [[ $cases =~ ^[1-9][0-9]*$ ]]; then
    echo "cases must be a positive integer" >&2
    exit 2
fi
if ! [[ $instructions =~ ^([1-9]|1[0-5])$ ]]; then
    echo "instructions must be an integer from 1 to 15" >&2
    exit 2
fi

# A 50-case run takes roughly 13 seconds on the current reference model. Give
# larger corpora generous headroom, including slower CI hosts.
timeout_seconds=$((cases * 3 / 10 + 30))

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_root"

echo "ARM7 differential: cases=$cases instructions=$instructions seed=$seed timeout=${timeout_seconds}s"
ARM7_DIFFERENTIAL_TIMEOUT=$timeout_seconds cmake --preset debug
cmake --build --preset debug -j2
if [[ -n $dump_dir ]]; then
    echo "Keeping generated cases in: $dump_dir"
    env ARM7_DIFFERENTIAL_CASES=$cases \
        ARM7_DIFFERENTIAL_INSTRUCTIONS=$instructions \
        ARM7_DIFFERENTIAL_SEED=$seed \
        ARM7_DIFFERENTIAL_DUMP_DIR=$dump_dir \
        ctest --preset debug --output-on-failure -R '^ARM::differential_reference$'
else
    env ARM7_DIFFERENTIAL_CASES=$cases \
        ARM7_DIFFERENTIAL_INSTRUCTIONS=$instructions \
        ARM7_DIFFERENTIAL_SEED=$seed \
        ctest --preset debug --output-on-failure -R '^ARM::differential_reference$'
fi
