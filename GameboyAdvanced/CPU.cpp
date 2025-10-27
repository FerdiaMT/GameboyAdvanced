#include "CPU.h"
#include <cstdint>

CPU::CPU(Bus* bus) : bus(bus) , sp(reg[13]) , lr(reg[14]) , pc(reg[15])
{

}

uint32_t CPU::tick()
{
	if (!T) // if arm mode
	{
		instruction = read32(pc);
		pc += 4;
	}
	else // if thumb mode
	{
		instruction = thumbConversion(read16(pc));
		pc += 2;
	}

	execute();
}

uint32_t CPU::thumbConversion(uint16_t thumbOp)
{

}

inline bool CPU::checkConditional(uint8_t cond) const
{
	if ((cond == 0xE, 1)) [[likely]] return true;


	switch (cond)
	{
	case(0x0):return Z;					break;
	case(0x1):return !Z;				break;
	case(0x2):return C;					break;
	case(0x3):return !C;				break;
	case(0x4):return N;					break;
	case(0x5):return !N;				break;
	case(0x6):return V;					break;
	case(0x7):return !V;				break;
	case(0x8):return (C && !Z);			break;
	case(0x9):return (!C || Z);			break;
	case(0xA):return (N == V);			break;
	case(0xB):return (N != V);			break;
	case(0xC):return (!Z && (N == V));	break;
	case(0xD):return (Z || (N != V));	break;
	case(0xE):return true;				break;
	case(0xF):printf("0XF INVALID CND");break;
	}

}

void CPU::execute()
{
	// first we should check if the conditional is valid
	// 
	// 1010 0101 1010 0101 0101 0101 1010 1010
	//  ^ this is conditional bits
	
	uint8_t conditional = (instruction>>28) & 0xF;
	if (!checkConditional(conditional)) return;

	

}






uint8_t CPU::read8(uint16_t addr, bool bReadOnly = false)
{
	return bus->read8(addr, bReadOnly);
}
uint16_t CPU::read16(uint16_t addr, bool bReadOnly = false)
{
	return bus->read16(addr, bReadOnly);
}
uint32_t CPU::read32(uint16_t addr, bool bReadOnly = false)
{
	return bus->read32(addr, bReadOnly);
}