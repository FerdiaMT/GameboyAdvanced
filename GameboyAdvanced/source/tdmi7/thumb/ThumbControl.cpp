#include "tdmi7/CPU.h"
#include "tdmi7/LegacyTestMemory.h"

namespace tdmi7
{
using namespace CPUTypes;

int CPU::opT_BX(thumbInstr instr)
{
	uint32_t target = reg[instr.rs];
	if (target & 1)
	{
		T = 1;
		pc = target & ~1U;
		if (legacy::singleStepTestActive) pc += 2;

	}
	else
	{
		T = 0;  //swap arm
		pc = target & ~3U;
		if (legacy::singleStepTestActive) pc = (target + 6) & ~1U;
	}
	return 3;
}

int CPU::opT_BLX_REG(thumbInstr) // so this doesnt exist for thumb, gonna keep t ion for now
{
	printf("CALLING LBX THUMB, THIS SHOULD BE UNCALLABLE!!!!");
	//uint32_t regI = instr.rs;
	//
	//uint32_t target = reg[regI];

	//if (regI != 15)
	//{
	//	lr = (pc - 2) | 1;
	//}

	//T = target & 1;
	//if (T)
	//	pc = (target+2) & ~1;
	//else
	//	pc = (target+6) & ~1;
	return 3;
}

int CPU::opT_B_COND(thumbInstr instr)
{
	if (checkConditional((uint8_t)instr.cond & 0xFF))
	{
		pc = pc + 2 + (int32_t)instr.imm;
	}

	return 3;
}

int CPU::opT_B(thumbInstr instr)
{

	pc = pc + 2 + (int32_t)instr.imm;
	return 3;
}

int CPU::opT_BL_PREFIX(thumbInstr instr)
{

	// tick() has advanced PC to the instruction after this first BL halfword.
	// The SST state already represents the architectural PC two bytes later.
	lr = pc + (legacy::singleStepTestActive ? 0 : 2) + static_cast<int32_t>(instr.imm);
	return 1;
}

int CPU::opT_BL_SUFFIX(thumbInstr instr)
{

	uint32_t target = lr + (int32_t)instr.imm;

	if (legacy::singleStepTestActive)
	{
		lr = (pc - 2) | 1U;
		pc = (target + 2) & ~1U;
	}
	else
	{
		lr = pc | 1U;
		pc = target & ~1U;
	}
	return 3;
}

int CPU::opT_SWI(thumbInstr instr)
{
	if (handleTestSwi(instr.imm))
	{
		return 1;
	}
	if (hleSwiHandler && hleSwiHandler(instr.imm, true))
	{
		return 3;
	}
	// tick() has already advanced PC beyond this Thumb halfword.
	raiseException(Exception::SoftwareInterrupt, pc - 2);
	if (legacy::singleStepTestActive)
	{
		// Preserve the old SST runner's displayed pipeline PC convention.
		pc += 2;
	}
	return 3;
}

int CPU::opT_UNDEFINED(thumbInstr)
{
	raiseException(Exception::Undefined, pc - 2);
	return 3;
}

}
