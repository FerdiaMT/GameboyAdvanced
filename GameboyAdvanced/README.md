# GameboyAdvanced

[![CTest](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/ctest.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/ctest.yml)
[![Quality analysis](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml)
[![Callgrind profile](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/profile.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/profile.yml)
[![Coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fferdiamt.github.io%2FGameboyAdvanced%2Fcoverage.json)](https://ferdiamt.github.io/GameboyAdvanced/)

An in-progress Game Boy Advance emulator, including ARM7TDMI validation,
graphics, system scheduling, and automated test tooling.

## Continuous validation

- **CTest** runs on each pull request and push to `main`.
- **ASan + UBSan** and **gcov coverage** run in the Quality analysis workflow.
- Coverage HTML and the Shields-compatible `coverage.json` are published to
  GitHub Pages for successful pushes to `main`.
- A Callgrind profile runs manually or weekly and is retained as an artifact.

Enable GitHub Pages once in repository **Settings → Pages**, choosing
**GitHub Actions** as the source. The coverage badge becomes live after the
first successful push to `main`.

For local profiling, sanitizers, and coverage commands, see [tools/README.md](tools/README.md).
