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

The ARM single-step-test (SST) fixtures are prebuilt files in `bin/` named
`arm_*.json.bin`. Each fixture is executed by `GBA test arm <fixture>` and is
registered automatically with CTest when present. Run the group with:

```bash
ctest --preset debug -L arm-sst --output-on-failure
```

Keep behavior-preserving moves separate from instruction-correctness changes:
the ARM/Thumb CTest baseline is intended to make that distinction visible.
