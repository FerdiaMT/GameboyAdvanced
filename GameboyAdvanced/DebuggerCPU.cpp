#include "DebuggerCPU.h"
#include "CPU.h"
#include <cstdint>


DebuggerCPU::DebuggerCPU(CPU* cpu)
{
	this->cpu = cpu;
}

void DebuggerCPU::DecodeIns(uint32_t startAddr, uint32_t endAddr)
{
	uint32_t curAddr = startAddr;

	while (curAddr <= endAddr)
	{
		uint32_t curInstruction = cpu->read32(curAddr); // this is the instruction in hex

		//const char* conditional = checkConditional((curInstruction >> 28) & 0xF); // this will return in string from the conditional

		CPU::Operation curOperation = cpu->decode(curInstruction); // this turns the instruction into the decoded operation

		
		printf("PC: 0x%08X, Instruction: 0x%08X, Opcode: %s\n",
			curAddr , curInstruction, cpu->opcodeToString(curOperation));


		curAddr += 4;
	}
}

inline const char* DebuggerCPU::checkConditional(uint8_t cond)
{

	switch (cond)
	{
	case(0x0):return "Z";					break;
	case(0x1):return "!Z";				break;
	case(0x2):return "C";					break;
	case(0x3):return "!C";				break;
	case(0x4):return "N";					break;
	case(0x5):return "!N";				break;
	case(0x6):return "V";					break;
	case(0x7):return "!V";				break;
	case(0x8):return "(C && !Z)";			break;
	case(0x9):return "(!C || Z)";			break;
	case(0xA):return "(N == V)";			break;
	case(0xB):return "(N != V)";			break;
	case(0xC):return "(!Z && (N == V))";	break;
	case(0xD):return "(Z || (N != V))";	break;
	case(0xE):return "ALWAYS TRUE";				break;
	case(0xF):printf("0XF INVALID CND");break;
	}
}
