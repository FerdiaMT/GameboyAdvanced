#!/usr/bin/env python3
"""Generate short ARM programs and compare the project's ISS with ARM7 RTL.

This deliberately starts with unconditional ARM data-processing instructions.
The vendored RTL does not provide a GBA bus, Thumb implementation, reset port,
or architectural debug API, so widening the generator requires extending its
host wrapper first.
"""

from __future__ import annotations

import argparse
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REGISTER_COUNT = 15
PROGRAM_WORDS = 256
NOP = 0xE1A00000  # MOV r0, r0
STATE_PATTERN = re.compile(r"r(\d+)=([0-9a-fA-F]{8})")
CPSR_PATTERN = re.compile(r"cpsr=([0-9a-fA-F]{8})")
OPCODE_NAMES = {
    0: "and", 1: "eor", 2: "sub", 3: "rsb", 4: "add",
    12: "orr", 13: "mov", 14: "bic", 15: "mvn",
}


def data_processing(opcode: int, rn: int, rd: int, rm: int) -> int:
    return 0xE0000000 | (opcode << 21) | (rn << 16) | (rd << 12) | rm


def immediate(opcode: int, rn: int, rd: int, value: int) -> int:
    return 0xE2000000 | (opcode << 21) | (rn << 16) | (rd << 12) | value


def generate_program(rng: random.Random, instruction_count: int, required_opcode: int) -> list[int]:
    # Keep the first stage deliberately hazard-free. The upstream RTL's
    # forwarding path is incomplete, so every instruction reads only registers
    # that this program never writes. This isolates ALU semantics from pipeline
    # scheduling while still giving each case ten randomized instructions.
    register_ops = (0, 1, 2, 3, 4, 12, 13, 14, 15)
    destinations = rng.sample(range(REGISTER_COUNT), instruction_count)
    program: list[int] = []
    for index, rd in enumerate(destinations):
        # A destination is safe as an input until its own instruction executes.
        # This lets a 15-instruction program remain hazard-free too.
        source_registers = destinations[index:]
        # Each corpus case is assigned one required ALU family below; the
        # remaining operations stay random.  This prevents a small random run
        # from accidentally omitting a supported instruction family.
        opcode = required_opcode if index == 0 else rng.choice(register_ops)
        rn = 0 if opcode in (13, 15) else rng.choice(source_registers)
        program.append(data_processing(opcode, rn, rd, rng.choice(source_registers)))
    return program


def write_hex(path: Path, words: list[int]) -> None:
    path.write_text("".join(f"{word:08x}\n" for word in words), encoding="ascii")


def assembly_listing(program: list[int]) -> str:
    lines = ["; Generated ARM data-processing program", "; r0-r14 start from the matching .state.hex file", ""]
    for index, instruction in enumerate(program):
        opcode = (instruction >> 21) & 0xF
        rd = (instruction >> 12) & 0xF
        rn = (instruction >> 16) & 0xF
        operand2 = f"#0x{instruction & 0xFF:x}" if instruction & (1 << 25) else f"r{instruction & 0xF}"
        mnemonic = OPCODE_NAMES[opcode]
        assembly = f"{mnemonic} r{rd}, {operand2}" if opcode in (13, 15) else f"{mnemonic} r{rd}, r{rn}, {operand2}"
        lines.append(f"{index:02d}: {instruction:08x}    {assembly}")
    return "\n".join(lines) + "\n"


def parse_state(path: Path, marker: str) -> tuple[dict[int, int], int]:
    text = path.read_text(encoding="utf-8")
    line = next((line for line in text.splitlines() if line.startswith(marker)), None)
    if line is None:
        raise RuntimeError(f"{marker} was not produced:\n{text}")
    state = {int(register): int(value, 16) for register, value in STATE_PATTERN.findall(line)}
    if set(state) != set(range(REGISTER_COUNT)):
        raise RuntimeError(f"incomplete {marker} state: {line}")
    cpsr_match = CPSR_PATTERN.search(line)
    if cpsr_match is None:
        raise RuntimeError(f"missing CPSR in {marker} state: {line}")
    return state, int(cpsr_match.group(1), 16)


def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, check=True, **kwargs)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--iss", type=Path, required=True)
    parser.add_argument("--reference-dir", type=Path, required=True)
    parser.add_argument("--testbench", type=Path, required=True)
    parser.add_argument("--iverilog", default="iverilog")
    parser.add_argument("--vvp", default="vvp")
    parser.add_argument(
        "--seed", type=lambda value: int(value, 0),
        default=int(os.environ.get("ARM7_DIFFERENTIAL_SEED", "0xA7D1FF"), 0),
    )
    parser.add_argument("--cases", type=int, default=int(os.environ.get("ARM7_DIFFERENTIAL_CASES", "50")))
    parser.add_argument(
        "--instructions", type=int,
        default=int(os.environ.get("ARM7_DIFFERENTIAL_INSTRUCTIONS", "10")),
    )
    parser.add_argument(
        "--dump-dir", type=Path,
        default=Path(os.environ["ARM7_DIFFERENTIAL_DUMP_DIR"])
        if os.environ.get("ARM7_DIFFERENTIAL_DUMP_DIR") else None,
        help="keep generated state, program, and assembly files in this empty directory",
    )
    args = parser.parse_args()

    if args.cases <= 0:
        parser.error("--cases must be positive")
    if not 1 <= args.instructions <= REGISTER_COUNT:
        parser.error(f"--instructions must be in [1, {REGISTER_COUNT}] for hazard-free programs")
    if args.dump_dir and args.dump_dir.exists() and any(args.dump_dir.iterdir()):
        parser.error(f"--dump-dir must be empty: {args.dump_dir}")
    iverilog = shutil.which(args.iverilog)
    vvp = shutil.which(args.vvp)
    if not args.iss.is_file():
        parser.error(f"ISS executable is unavailable: {args.iss}")
    if not iverilog:
        parser.error(f"Verilog compiler is unavailable: {args.iverilog}")
    if not vvp:
        parser.error(f"Verilog runtime is unavailable: {args.vvp}")

    sources = [
        args.testbench,
        args.reference_dir / "DeepPipeline.v",
        args.reference_dir / "alu.v",
        args.reference_dir / "cond.v",
        args.reference_dir / "data_cache.v",
        args.reference_dir / "instr_cache.v",
        args.reference_dir / "instr_decode.v",
        args.reference_dir / "mult.v",
        args.reference_dir / "register.v",
        args.reference_dir / "shifter.v",
    ]
    if any(not source.is_file() for source in sources):
        missing = next(str(source) for source in sources if not source.is_file())
        raise RuntimeError(f"reference source is missing: {missing}")

    rng = random.Random(args.seed)
    if args.dump_dir:
        args.dump_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="arm7-differential-") as directory:
        work = Path(directory)
        simulation = work / "arm7_reference.vvp"
        # The upstream RTL uses `type` as an identifier, which is a keyword in
        # SystemVerilog, and relies on ALU opcode macros across source files.
        # Compile it as Verilog-2005 and provide those macros explicitly.
        opcode_defines = [
            "-DAND=0", "-DEOR=1", "-DSUB=2", "-DRSB=3", "-DADD=4", "-DADC=5",
            "-DSBC=6", "-DRSC=7", "-DTST=8", "-DTEQ=9", "-DCMP=10", "-DCMN=11",
            "-DORR=12", "-DMOV=13", "-DBIC=14", "-DMVN=15",
        ]
        run([iverilog, "-g2005", "-o", str(simulation), *opcode_defines,
             *(str(source) for source in sources)])

        for case in range(args.cases):
            initial = [rng.getrandbits(32) for _ in range(REGISTER_COUNT)]
            program = generate_program(rng, args.instructions, tuple(OPCODE_NAMES)[case % len(OPCODE_NAMES)])
            # The reference instruction cache has no reset/read handshake. Its
            # first fetched word is discarded while that cache output settles.
            # A leading architectural NOP aligns the ten generated operations.
            rtl_program = [NOP] + program
            state_path = work / f"case-{case}.state.hex"
            program_path = work / f"case-{case}.program.hex"
            iss_path = work / f"case-{case}.iss.out"
            rtl_path = work / f"case-{case}.rtl.out"
            write_hex(state_path, initial)
            write_hex(program_path, rtl_program + [NOP] * (PROGRAM_WORDS - len(rtl_program)))
            if args.dump_dir:
                prefix = args.dump_dir / f"case-{case:04d}"
                write_hex(prefix.with_suffix(".state.hex"), initial)
                write_hex(prefix.with_suffix(".program.hex"), rtl_program + [NOP] * (PROGRAM_WORDS - len(rtl_program)))
                prefix.with_suffix(".s").write_text(assembly_listing(program), encoding="utf-8")

            run([
                str(args.iss), "--state", str(state_path), "--program", str(program_path),
                "--instructions", str(len(rtl_program)), "--output", str(iss_path),
            ])
            with rtl_path.open("w", encoding="utf-8") as output:
                run([vvp, str(simulation), f"+state={state_path}", f"+program={program_path}"], stdout=output)

            iss_state, iss_cpsr = parse_state(iss_path, "ARM7_ISS_STATE")
            rtl_state, rtl_cpsr = parse_state(rtl_path, "ARM7_REF_STATE")
            # The reference RTL exposes only NZCV; its remaining CPSR bits are
            # not an ARM7 programmer's model. Compare exactly that shared
            # architectural subset.
            if iss_state != rtl_state or (iss_cpsr & 0xF0000000) != (rtl_cpsr & 0xF0000000):
                print(f"differential mismatch: seed=0x{args.seed:x}, case={case}", file=sys.stderr)
                print("initial registers:", " ".join(f"r{i}=0x{value:08x}" for i, value in enumerate(initial)), file=sys.stderr)
                print("program:", " ".join(f"0x{instruction:08x}" for instruction in program), file=sys.stderr)
                for register in range(REGISTER_COUNT):
                    if iss_state[register] != rtl_state[register]:
                        print(
                            f"r{register}: iss=0x{iss_state[register]:08x} rtl=0x{rtl_state[register]:08x}",
                            file=sys.stderr,
                        )
                if (iss_cpsr & 0xF0000000) != (rtl_cpsr & 0xF0000000):
                    print(
                        f"NZCV: iss=0x{iss_cpsr >> 28:x} rtl=0x{rtl_cpsr >> 28:x}",
                        file=sys.stderr,
                    )
                return 1

    print(f"ARM7 differential passed: {args.cases} cases, seed=0x{args.seed:x}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"ARM7 differential setup failed: {error}", file=sys.stderr)
        raise SystemExit(2)
