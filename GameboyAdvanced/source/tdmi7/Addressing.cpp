#include "tdmi7/CPU.h"

namespace tdmi7
{
 uint8_t CPU::pcOffset()
{
	return (T) ? 2 : 4;
}

//HELPER FUNCTIONS FOR DATA PROCESSING

uint32_t CPU::getHalfWordOffset(armInstr instr)
{
	if (instr.I)
	{
		//return instr.imm; ( imm is the default defined, for clarity im doing full here
		uint32_t res = ((instr.raw >> 4) & 0b11110000) | (instr.raw & 0b1111);
		return res;
	}
	else
	{
		// reg offset
		if (instr.rm == 15)
		{
			uint32_t retVal = reg[instr.rm];
			retVal += 4;
			return retVal;
		}
		return reg[instr.rm];
	}
}
uint32_t CPU::SDapplyShift(uint32_t rmVal, uint8_t type, uint8_t amount) // singledata apply shift
{
	switch (type) // use bits 1 and 2 of shift
	{
	case 0b00: return DPshiftLSL(rmVal, amount, nullptr); break; // LSL
	case 0b01: return DPshiftLSR(rmVal, amount, nullptr); break; // LSR
	case 0b10: return DPshiftASR(rmVal, amount, nullptr); break; // ASR
	case 0b11: return DPshiftROR(rmVal, amount, nullptr); break; // ROR
	}

	return 0;
}




 uint8_t CPU::DPgetRn() { return (instruction >> 16) & 0xF; }
 uint8_t CPU::DPgetRd() { return (instruction >> 12) & 0xF; }
 uint8_t CPU::DPgetRs() { return (instruction >> 8) & 0xF; }
 uint8_t CPU::DPgetRm() { return instruction & 0xF; }
 uint8_t CPU::DPgetShift() { return (instruction >> 4) & 0xFF; }
 uint8_t CPU::DPgetImmed() { return instruction & 0xFF; }
 uint8_t CPU::DPgetRotate() { return (2 * ((instruction >> 8) & 0xF)); }
 bool CPU::DPs() { return (instruction >> 20) & 0b1; } // condition code
 bool CPU::DPi() { return (instruction >> 25) & 0b1; } // immediate code
 uint8_t CPU::DPgetShiftAmount(uint8_t shift)
{
	if (shift & 0b1) // shift amount depends on register if this is true
	{
		return reg[DPgetRs()] & 0xFF; // shift amount is bottom 8 bits of Rs
	}
	else
	{
		return (shift >> 3) & 0x1F;  // 5-bit immediate (0-31)
	}
}

CPUTypes::thumbInstr CPU::debugDecodedInstr()
{
	return {};
}

}
