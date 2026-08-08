# Execution traces

Run a program with `--trace` to write an instruction trace here:

```bash
./GBA run tests/assembly/build/arm_add_cmp.bin --steps 6 --trace
```

The default output name is based on the input binary, for example
`traces/arm_add_cmp.trace`. A different repository-relative path can be
given after `--trace`.

Each instruction record contains the instruction number, CPU, virtual address,
raw opcode, and disassembly:

```text
inst: [    1] CPU  0 <v:0x08000000> [...] e3a00012 mov ...
```

The opcode field is reconstructed from the bytes fetched by the emulator and
the final field is the emulator's ARM or Thumb disassembly.
