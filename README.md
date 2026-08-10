# Playable webAsm version can be found on my site:
https://ferdiamt.github.io/ferdiaPortfolio/#/projects/GBA


### Gameboy Advance emulator


Fully working ISS for the ARM7TDMI chip

[![CTest](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/ctest.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/ctest.yml)
[![Quality analysis](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml)
[![Coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fferdiamt.github.io%2FGameboyAdvanced%2Fcoverage.json)](https://ferdiamt.github.io/GameboyAdvanced/)
[![BIOS bootstrap Ir](https://img.shields.io/endpoint?url=https%3A%2F%2Fferdiamt.github.io%2FGameboyAdvanced%2Fbios-bootstrap-ir.json)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml)

Uses a RTL verilog with a testbench to verify instruction behavior w/ built in random program generator

## Build flow:

```sh
cmake --preset debug
cmake --build --preset debug
```

You can then run the object ./GBA, passing in  your desired ARM7 hex file

### Tests:

The model contains tests for ARM and THUMB mode in the cpu
There is a collection of "common behavior" tests written in C, which are compiled into arm7 machine code

You can run the tests using 

```sh
ctest --preset debug -j 4 
```
The GBA object can be ran with the --trace flag, which will return a Simics like instruction trace

## Browser / WebAssembly build

The browser frontend starts the bundled BIOS and Super Mario Advance 4
automatically. It needs an [Emscripten](https://emscripten.org/) environment;
the native compiler cannot produce the WebAssembly target. The selector in the
top-right switches between the ROMs in `bin/games/`; **Upload ROM** loads a
local `.gba` file into the browser's temporary in-memory filesystem (up to
32 MiB).

```sh
# Activate the local Emscripten SDK for this shell.
source /home/ferdia/emsdk/emsdk_env.sh

emcmake cmake --preset web
cmake --build --preset web
```

Run the `source` command once in each new terminal before configuring a web
build.

Serve `build/web` over HTTP (browsers do not permit the generated page to load
its `.wasm` and `.data` files from `file://`):

```sh
python3 -m http.server --directory build/web 8080
```

Open `http://localhost:8080/index.html`. The page provides Pause and Reset;
keyboard controls are Z/A, X/B, A/L, S/R, arrows/D-pad, Enter/Start, and
Backspace/Select.
