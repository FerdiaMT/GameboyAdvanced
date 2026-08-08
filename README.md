Fully working ISS for the ARM7TDMI chip

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
