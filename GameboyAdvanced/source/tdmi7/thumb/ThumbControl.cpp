#include "tdmi7/CPU.h"

namespace tdmi7
{
using namespace CPUTypes;

namespace Vector
{
constexpr uint32_t SWI = 0x00000010;
}

int CPU::opT_BX(thumbInstr instr)
{
	uint32_t target = reg[instr.rs];
	if (target & 1)
	{
		T = 1;
		pc = (target+2) & ~1; // keep thumb

	}
	else
	{
		T = 0;  //swap arm
		pc = (target + 6)&~1;
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

	lr = pc + (int32_t)instr.imm;
	return 1;
}

int CPU::opT_BL_SUFFIX(thumbInstr instr)
{

	uint32_t target = lr + (int32_t)instr.imm;

	lr = (pc - 2) | 1;
	pc = (target + 2) & ~1U;
	return 3;
}

int CPU::opT_SWI(thumbInstr instr)
{
	if (handleTestSwi(instr.imm))
	{
		return 1;
	}
	enterException(mode::Supervisor, Vector::SWI, pc - 4);
	pc -= 2;
	return 3;
}

int CPU::opT_UNDEFINED(thumbInstr)
{
	//printf("UNDEFINED TRIGGERED, REPLACE LATER WITH PROPER VECTOR HANDLER");
	return 1;
}

}
