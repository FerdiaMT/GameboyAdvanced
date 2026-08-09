# tdmi7 CPU subsystem

This directory contains the complete ARM7TDMI CPU implementation. The rest of
the emulator owns system wiring, the bus, and peripherals; it should not need
to know how an instruction is decoded or executed.

The CPU-facing public headers live in `include/tdmi7/`: `CPU.h`, `Decoder.h`,
`CPUTypes.h`, and `DebuggerCPU.h`. `CPU.h` forward-declares `Decoder`, while
`Decoder.h` depends only on decoded instruction types, keeping the two API
headers independent. `CPU.h` is the façade; its class-member declaration
sections live in `detail/`, `arm/`, and `thumb/` alongside the code they
describe.

- `Core.cpp` owns reset, fetch/decode/dispatch, cycle accounting, and opcode
  handler registration.
- `CpuState.cpp` owns CPSR/SPSR, modes, banked registers, and exceptions.
- `Addressing.cpp` contains instruction operand/address helpers.
- `CpuMemory.cpp` is the CPU-facing Bus access layer.
- `DecoderCore.cpp` owns the decoder façade; `arm/ArmDecoder.cpp` and
  `thumb/ThumbDecoder.cpp` convert raw instructions for their respective modes.
- `arm/ArmDisassembly.cpp` and `thumb/ThumbDisassembly.cpp` format traces for
  their respective modes.
- `arm/` contains the ARM-mode instruction families: data processing, memory,
  multiply, and control flow.
- `thumb/` contains the Thumb-mode instruction families: core, ALU, memory,
  and control flow.
- `LegacyInstructionTests.cpp` isolates fixture loading from normal CPU
  execution. The older Thumb diagnostic harness lives in `tests/legacy/` and
  is built as the separate `gba_legacy_diagnostics` target.

The ARM and Thumb single-step-test (SST) fixtures are prebuilt files in `bin/`
named `arm_*.json.bin` and `thumb_*.json.bin`. Each fixture is executed by
`GBA test arm <fixture>` or `GBA test thumb <fixture>` and is registered
automatically with CTest when present. Run either group with:

```bash
ctest --preset debug -L arm-sst --output-on-failure
ctest --preset debug -L thumb-sst --output-on-failure
```

## Timing trace baseline

`Bus` has opt-in CPU timing instrumentation for deterministic tests.  With it
enabled, each CPU tick records instruction fetches, data reads/writes, and any
remaining internal cycles.  Every external access is classified as sequential
or non-sequential and charged using `Bus::CpuTimingConfig`.

The trace supports both flat configurable N/S costs and a GBA region model.
The latter covers BIOS, EWRAM, IWRAM, I/O, palette/VRAM/OAM, Game Pak wait
state regions, SRAM, WAITCNT, 16-bit Game Pak bus splitting, and the forced
non-sequential access at each Game Pak 128 KiB boundary. `cpu_timing_traces`
exercises this contract for ARM and Thumb fetches, loads, stores, and a branch
pipeline refill. It does not yet model prefetch or DMA/PPU contention. Run it
with:

```bash
ctest --preset debug -L timing --output-on-failure
```

Keep behavior-preserving moves separate from instruction-correctness changes:
the ARM/Thumb CTest baseline is intended to make that distinction visible.

## System clock

`GBA` owns the master-cycle scheduler. Each `GBA::tick()` executes one CPU
instruction, obtains its elapsed cycle delta, then advances every attached
`ClockedDevice` in deterministic registration order. The PPU is the first
device attached by default; its current implementation only tracks cycles,
ready for scanline timing. Timers, DMA, audio, and input will use the same
interface rather than independently counting CPU instructions.
