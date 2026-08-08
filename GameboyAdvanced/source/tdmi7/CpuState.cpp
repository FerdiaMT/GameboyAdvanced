#include "tdmi7/CPU.h"

#include <cassert>
#include <cstdio>
#include <string>

namespace tdmi7
{
const char* CPU::CPSRtoString()
{
	static char str[8];

	str[0] = N ? 'N' : '-';
	str[1] = Z ? 'Z' : '-';
	str[2] = C ? 'C' : '-';
	str[3] = V ? 'V' : '-';
	str[4] = I ? 'I' : '-';
	str[5] = F ? 'F' : '-';
	str[6] = T ? 'T' : '-';
	str[7] = '\0';

	return str;
}

std::string CPSRparser(uint32_t num)
{
	std::string str = "";

	for (int i = 0; i <32; i++)
	{
		str += ((num >> i) & 1)==1 ? "X" : "-";
	}
	str += '\0';

	return str;
}




//std::string CPU::CPSRtoStringPASSED(uint32_t base, uint32_t finalS, uint32_t passed)
//{
//	static char str[8];
//	// find the difference between
//	// original and expected
//	// original and passed
//	if (passed != finalS)
//	{
//		return "passed and final differ, BASE " + CPSRparser(passed) + "FINAL "+ CPSRparser(finalS);
//	}
//	//if (base != passed)
//	//{
//	//	return "base and final differ, BASE " + CPSRparser(base) + "FINAL " + CPSRparser(finalS);
//	//}
//
//	
//
//	return str;
//}



CPU::mode CPU::CPSRbitToMode(uint8_t modeBits)
{
	return static_cast<mode>(modeBits & 0x1F);
}

bool CPU::isPrivilegedMode() // readable way to know not in user mode
{
	return (curMode != mode::User);
}
uint8_t CPU::getModeIndex(mode mode) // used for register saving
{
	switch (mode)
	{
	case mode::User:	   return 0;
	case mode::System:     return 0;
	case mode::FIQ:        return 1;
	case mode::IRQ:        return 2;
	case mode::Supervisor: return 3;
	case mode::Abort:      return 4;
	case mode::Undefined:  return 5;
	default:               return 0xFE;
	}
}

//reg banking
void CPU::bankRegisters(mode mode)// save reg val to bank
{
	uint8_t passedModeIndex = getModeIndex(mode);

	r13RegBank[passedModeIndex] = reg[13]; // save 13 and 14 into the reg bank
	r14RegBank[passedModeIndex] = reg[14];

	if (mode == mode::FIQ) // fiq saves its own registers
	{
		r8FIQ[0] = reg[8];
		r8FIQ[1] = reg[9];
		r8FIQ[2] = reg[10];
		r8FIQ[3] = reg[11];
		r8FIQ[4] = reg[12];
	}
	else
	{
		r8User[0] = reg[8];
		r8User[1] = reg[9];
		r8User[2] = reg[10];
		r8User[3] = reg[11];
		r8User[4] = reg[12];
	}

}
void CPU::unbankRegisters(mode mode)  // load reg vals from bank
{
	uint8_t passedModeIndex = getModeIndex(mode);

	reg[13] = r13RegBank[passedModeIndex]; // grab 13 and 14 from the reg bank
	reg[14] = r14RegBank[passedModeIndex];

	if (mode == mode::FIQ) // fiq saves its own registers
	{
		reg[8] = r8FIQ[0];
		reg[9] = r8FIQ[1];
		reg[10] = r8FIQ[2];
		reg[11] = r8FIQ[3];
		reg[12] = r8FIQ[4];
	}
	else
	{
		reg[8] = r8User[0];
		reg[9] = r8User[1];
		reg[10] = r8User[2];
		reg[11] = r8User[3];
		reg[12] = r8User[4];
	}
}

void CPU::switchMode(mode newMode) // main function used for mode switching, calls bank and unbank register etc
{
	mode oldMode = curMode;
	if (oldMode != newMode) // check this first so we dont do a pointless swap
	{
		curMode = newMode;

		bankRegisters(oldMode);
		CPSR = (CPSR & ~0x1F) | static_cast<uint8_t>(newMode); // set the new modes bits (may turn this to a function later)
		unbankRegisters(newMode);
	}


}

void CPU::saveIntoSpsr(uint8_t index)
{
	if (index == 0) assert(0); // if user or system
	spsrBank[index] = CPSR;
}

// excpetion handling
void CPU::enterException(CPU::mode newMode, uint32_t vectorAddr, uint32_t returnAddr)
{
	const uint32_t previousCpsr = CPSR;
	switchMode(newMode); // switch the reg bankings , swaps curMode

	uint8_t newModeIndex = getModeIndex(curMode); // new modes index for switching

	// SPSR captures the caller's CPSR, before switchMode updates the mode bits.
	spsrBank[newModeIndex] = previousCpsr;

	//EXTRA FOR EXCEPTION HANDLING
	CPSR |= 0x80;  // Disable IRQ
	if (newMode == mode::FIQ) CPSR |= 0x40;// turn off FIQ if on FIQ

	lr = returnAddr+2;
	pc = vectorAddr-4;

	lr += 2;
}
void CPU::returnFromException()
{
	mode oldMode = curMode;
	int oldModeIndex = getModeIndex(oldMode);

	if (oldModeIndex > 0) // if not user / system
	{
		uint32_t savedCPSR = spsrBank[oldModeIndex];
		curMode = CPSRbitToMode(savedCPSR & 0x1F);

		bankRegisters(oldMode);
		CPSR = savedCPSR;
		unbankRegisters(curMode);
	}
}

//SPSR helpers
uint32_t CPU::getSPSR()
{
	if (curMode!= mode::User && curMode != mode::System) // if your mode has an SPSR
	{
		uint8_t idx = getModeIndex(curMode);// 1 given to fiq as we find spsrBank from 0
		return spsrBank[idx];
	}
	return CPSR;
}
void  CPU::setSPSR(uint32_t value)
{
	int idx = getModeIndex(curMode);
	if (idx > 0) spsrBank[idx] = value;
}
//CPSR helper
void CPU::writeCPSR(uint32_t value)
{
	// ARM7TDMI implements only modes whose M[4] bit is set.  Writes to CPSR
	// therefore read back with that bit high, including otherwise invalid modes.
	value |= 0x10;
	if (curMode == mode::User && ((value & 0x1F) != static_cast<uint8_t>(mode::User))) // if were in user, and were trying to leave it
	{
		CPSR = (CPSR & 0x000000FF) | (value & 0xFFFFFF00);  // update just flags, ignore rest
		return;
	}
	if (getModeIndex((mode)(value&0x1f)) == 0XFE) // if returns fe (fail) TODO, figure out whats going on here
	{
		//jump up to next valid user mode ?
		//while ((value & 0x1F) != 0x10)
		//{
		//	value += 1;
		//}
	}
	mode newMode = CPSRbitToMode(value & 0x1F);

	if (curMode != newMode) // if we should swap modes
	{
		switchMode(newMode);
	}
	CPSR = value;
}



bool CPU::checkConditional(uint8_t cond) const
{
	if ((cond == 0xE)) [[likely]] return true;

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
	case(0xF):printf("0XF INVALID CONDITIONAL CHECK   ---    ");break;
	}

	return false;
}





/////////////////////////////////////////////
///             OPCODE INSTRS()           ///
/////////////////////////////////////////////

// THIS WILL ADD 2 FOR THUMB, ADD 4 OTHERWISE

}
