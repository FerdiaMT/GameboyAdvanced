# Local analysis tools

These are opt-in developer tools; the normal `debug` and `release` presets are
unchanged.

## Callgrind

```bash
tools/profile_callgrind.sh build/callgrind -- run bin/sma.gba --bios bin/gba_bios.bin --steps 1000000
```

This creates a call-count profile and an annotated text report. Install it on
Fedora with `sudo dnf install valgrind`.

## AddressSanitizer and UBSan

```bash
tools/run_sanitizers.sh
```

This runs the complete CTest suite with memory and undefined-behaviour checks.
On Fedora, install the compiler runtimes first with
`sudo dnf install libasan libubsan`.

## Coverage

```bash
tools/run_coverage.sh
```

This uses GCC `gcov` instrumentation and produces
`build/coverage/report/index.html`. Install the reporting frontend with
`sudo dnf install gcovr`.
