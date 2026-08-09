Fully working ISS for the ARM7TDMI chip

[![CTest](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/ctest.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/ctest.yml)
[![Quality analysis](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/analysis.yml)
[![Callgrind profile](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/profile.yml/badge.svg)](https://github.com/FerdiaMT/GameboyAdvanced/actions/workflows/profile.yml)
[![Coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fferdiamt.github.io%2FGameboyAdvanced%2Fcoverage.json)](https://ferdiamt.github.io/GameboyAdvanced/)

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