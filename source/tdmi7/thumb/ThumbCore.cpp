#include "tdmi7/CPU.h"

namespace tdmi7
{
using namespace CPUTypes;

void CPU::updateFlagsNZCV_Add(uint32_t result, uint32_t op1, uint32_t op2)
{
	N = (result >> 31) & 0x1;
	Z = result == 0;
	C = result < op1;

	V = ((op1 & 0x80000000) == (op2 & 0x80000000)) && ((op1 & 0x80000000) != (result & 0x80000000));
}

void CPU::updateFlagsNZCV_Sub(uint32_t result, uint32_t op1, uint32_t op2)
{
	N = (result >> 31) & 0x1;
	Z = result == 0;
	C = op1 >= op2;
	V = ((op1 & 0x80000000) != (op2 & 0x80000000)) && ((op1 & 0x80000000) != (result & 0x80000000));
}

//////////////////////////////////////////////////////////////////////////////////////////
///								    OPS                    							   ///
//////////////////////////////////////////////////////////////////////////////////////////

int CPU::thumbExecute(thumbInstr instr)
{
	return (this->*opT_functions[static_cast<int>(instr.type)])(instr);
}

}
