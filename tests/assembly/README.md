# Raw assembly instruction tests

This directory contains small, standalone ARM7TDMI assembly and freestanding C
programs for exercising the emulator. Each `.s` or `.c` source file is compiled
into a flat binary; it is not a complete GBA ROM and does not need a BIOS.

## Build every test

From the repository root, run:

```bash
./tests/assembly/build-all.sh
```

The script writes the generated binaries to `tests/assembly/build/`. That
directory is ignored by Git.

The script requires `clang` and `readelf`. It prefers a cross-target GNU
`objcopy` when available, but automatically falls back to extracting a
relocation-free `.text` section. That fallback is suitable for the
self-contained tests in this directory.

```bash
clang --target=arm-none-eabi
readelf
```

Override either command when needed:

```bash
CLANG=clang OBJCOPY=arm-none-eabi-objcopy ./tests/assembly/build-all.sh
```

## Writing a C test

`.c` files are compiled for ARM7TDMI in ARM mode with no C runtime, standard
library, or startup code. Define `_start` as the entry point and avoid headers,
dynamic allocation, I/O, helper-library operations such as division, and calls
to other C functions. Keep the program self-contained so its `.text` section
has no relocations.

Use inline SWIs to report the result to `GBA --test-swi`:

```c
#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)

__attribute__((noreturn)) void _start(void)
{
    unsigned int value = 0x12 + 0x30;
    if (value == 0x42) TEST_PASS();
    TEST_FAIL();
}
```

`c_add_cmp.c` is a complete working example. Build and run it with:

```bash
./tests/assembly/build-all.sh
./GBA run tests/assembly/build/c_add_cmp.bin --steps 100 --test-swi
```

Every `.c` source is compiled twice: once as ARM (`c_add_cmp.bin`) and once as
Thumb (`thumb_c_add_cmp.bin`). The builder prepends a small ARM entry stub to
each Thumb binary; it uses `bx` to enter the Thumb payload from normal ARM
reset state.

## Run the C suite with CTest

After configuring and building the emulator, run every raw C test with:

```bash
ctest --preset debug -L raw-c --output-on-failure
```

CTest names matching architecture-specific cases as `ARM::c_add_cmp` and
`THUMB::c_add_cmp`. Select either group by name or label:

```bash
ctest --preset debug -R '^ARM::' --output-on-failure
ctest --preset debug -L thumb --output-on-failure
```

CTest first runs `build-all.sh`, then executes all raw C binaries in this
directory. The C baseline currently contains fifteen tests, including arithmetic,
shifts, CRC-style nested loops, halfword/byte/word accesses, signed and
unsigned comparisons, pointer walking, multiplication, and matrix indexing.
Every test must reach `swi #0`;
`swi #1` or exhaustion of the step limit fails the CTest case.

Ten ARM-only assembly regressions complement the generated C programs. They
cover data-processing flags, every standard condition code, shift edge cases,
multiply-long operations, single and block transfers, signed byte/halfword
loads, branch/link exchange, CPSR transfers, and SWP/SWPB. Run them with:

```bash
ctest --preset debug -L raw-assembly --output-on-failure
```

Each CTest run automatically writes one trace per C binary to `traces/`, such
as `traces/c_crc32_zero_block.trace`.

Run the Thumb C subset with:

```bash
ctest --preset debug -L raw-thumb-c --output-on-failure
```

## Run a test

Build the emulator first, then run a generated binary for a bounded number of
instructions:

```bash
cmake --preset debug
cmake --build --preset debug
./tests/assembly/build-all.sh
./GBA run tests/assembly/build/arm_add_cmp.bin --steps 6 --test-swi
```

`arm_add_cmp.s` sets `r0 = 0x12`, `r1 = 0x30`, calculates `r2 = 0x42`, and
finishes with `swi #0`, which makes the runner print a pass result and exit
zero. A failed assertion should branch to `swi #1`; the runner prints a failure
and exits with status 2. `--steps` remains a safety limit; a test that reaches
it without either SWI exits with status 3.

`--test-swi` is deliberately opt-in. Without it, SWIs retain their usual ARM
exception behavior, so this convention does not steal BIOS calls from a normal
GBA program.

## Adding a test

1. Create a `.s` or freestanding `.c` file in this directory.
2. For assembly, target ARM7TDMI: use `.cpu arm7tdmi` and either `.arm` or `.thumb`.
3. Keep the program self-contained: no linker symbols, external data, or
   system calls.
4. End in `swi #0` for pass and branch failed assertions to `swi #1`.
5. Re-run `./tests/assembly/build-all.sh`.

The binary is loaded by `GBA` at address `0x08000000` and starts executing
there.
