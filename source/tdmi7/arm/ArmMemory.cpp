#include "tdmi7/CPU.h"
#include "tdmi7/LegacyTestMemory.h"

namespace tdmi7
{
using namespace CPUTypes;

uint32_t SDOffset(bool u, uint32_t newAddr, uint32_t offset)
{
	if (u) newAddr += offset;
	else newAddr -= offset;
	return newAddr;
}

int numOfRegisters(uint16_t registerList)
{
	int numRegs = 0;
	for (int i = 0; i < 16; i++)
	{
		if (registerList & (1 << i)) numRegs++;
	}

	return numRegs;
}
uint32_t CPU::getArmOffset(armInstr instr)
{
	if (!instr.I) // Immediate offset
	{
		return instr.imm;
	}
	else // Register with shift

	{
		uint32_t rmVal = reg[instr.rm];

		if (instr.rm == 15)
		{
			rmVal = pc + 4;
		}

		return SDapplyShift(rmVal, instr.shift_type, instr.shift_amount);
	}
}

int CPU::opA_LDR(armInstr instr)
{
	//rm==15 is the issue
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	uint32_t newAddr = reg[instr.rn];

	// The legacy SST runner exposes PC one instruction ahead of normal execution.
	if (instr.rn == 15)
		newAddr = pc + (legacy::singleStepTestActive ? 4 : 8);

	uint32_t offset = getArmOffset(instr);

	if (instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}



	uint32_t readVal;
	if (instr.B) // Byte
	{
		readVal = read8(newAddr);
	}
	else // Word
	{
		// ARM7TDMI performs the word bus access on an aligned address, then
		// rotates the returned word by the original address low bits.
		uint32_t data = read32(legacy::singleStepTestActive ? newAddr : (newAddr & ~3U));
		uint8_t rotation = (newAddr & 3) * 8;
		readVal = (data >> rotation) | (data << (32 - rotation));
	}

	if (!instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}

	reg[instr.rd] = readVal;
	// Loading r15 replaces the fetch address; unlike a general-purpose
	// destination it must not then fall through to the next ARM word.
	if (instr.rd == 15 && !legacy::singleStepTestActive)
	{
		pc = readVal & ~3U;
		return 3;
	}

	if ((!instr.P || instr.W)  && instr.rn != instr.rd)
	{
		reg[instr.rn] = newAddr;
		if (instr.rn == 15)
		{
			pc += 4;
		}
	}

	pc += 4;

	return 3;
}

int CPU::opA_STR(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	uint32_t newAddr = reg[instr.rn];
	if (instr.rn == 15)
		newAddr = pc + (legacy::singleStepTestActive ? 4 : 8);
	uint32_t offset = getArmOffset(instr);
	if (instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	uint32_t writeVal = reg[instr.rd];
	if (instr.rd == 15) writeVal += 8; // PC stores as PC+12
	if (instr.B) // Byte
	{
		write8(newAddr, writeVal);
	}
	else // Word
	{
		write32(newAddr, writeVal);
	}
	if (!instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	if ((!instr.P || instr.W))
	{
		reg[instr.rn] = newAddr;
		if (instr.rn == 15)
		{
			pc += 4;
		}
	}
	pc += 4;
	return 3;
}

int CPU::opA_LDRH(armInstr instr)
{

	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	uint32_t newAddr = reg[instr.rn];
	if (instr.rn == 15) newAddr = pc + 4;

	uint32_t offset = getHalfWordOffset(instr);

	if (instr.P) 
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}

	uint32_t readAddr = newAddr; // address actually used for read
	uint32_t readVal = read16(newAddr);

	if (!instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	if ((!instr.P || instr.W) && instr.rn != instr.rd)
	{
		reg[instr.rn] = newAddr;
		if (instr.rn == 15)
		{
			pc += 4;
		}
	}

	uint8_t rotation = (readAddr & 1) ? 16 : 0;
	if (rotation)
	{
		uint8_t lo = readVal & 0xFF;
		uint8_t hi = (readVal >> 8) & 0xFF;
		readVal = (lo << 24) | hi;
	}

	reg[instr.rd] = readVal;

	pc += 4;
	return 3;
}

int CPU::opA_STRH(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 2;
	}
	uint32_t newAddr = reg[instr.rn];
	if (instr.rn == 15) newAddr = pc + 4;
	uint32_t offset = getHalfWordOffset(instr);
	if (instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	uint16_t writeVal = reg[instr.rd] & 0xFFFF;
	write16(newAddr, writeVal);
	if (!instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	if ((!instr.P || instr.W))
	{
		reg[instr.rn] = newAddr;
		if (instr.rn == 15)
		{
			pc += 4;
		}
	}
	pc += 4;
	return 2;
}

int CPU::opA_LDRSB(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	uint32_t newAddr = reg[instr.rn];
	if (instr.rn == 15) newAddr = pc + 4;

	uint32_t offset = getHalfWordOffset(instr);

	if (instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}

	uint32_t readVal = read8(newAddr);

	if (!instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	if ((!instr.P || instr.W) && instr.rn != instr.rd)
	{
		reg[instr.rn] = newAddr;
		if (instr.rn == 15)
		{
			pc += 4;
		}
	}

	//MSB in read8 signifies if its all F or not
	if (readVal & 0x80) //if MSB set
	{
		readVal |= 0xFFFFFF00;
	}

	reg[instr.rd] = readVal;

	pc += 4;
	return 3;
}

int CPU::opA_LDRSH(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	uint32_t newAddr = reg[instr.rn];
	if (instr.rn == 15) newAddr = pc + 4;

	uint32_t offset = getHalfWordOffset(instr);

	if (instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}

	uint32_t readAddr = newAddr; // address actually used for read
	uint32_t readVal = read16(newAddr);

	if (!instr.P)
	{
		newAddr = SDOffset(instr.U, newAddr, offset);
	}
	if ((!instr.P || instr.W) && instr.rn != instr.rd)
	{
		reg[instr.rn] = newAddr;
		if (instr.rn == 15)
		{
			pc += 4;
		}
	}
	//before rotation, figure out if the MSB is set
	bool isSetMSB = readVal & 0x8000;
	uint8_t rotation = (readAddr & 1) ? 16 : 0;
	if (rotation)
	{
		uint8_t lo = readVal & 0xFF;
		uint8_t hi = (readVal >> 8) & 0xFF;
		readVal = (lo << 24) | hi;
	}

	if (isSetMSB) // very ugly way of doing things but i wanna speed through coding this now
	{
		if (rotation)
		{
			readVal |= 0xFFFFFF00;
		}
		else
		{
			readVal |= 0xFFFF0000;
		}
	}
	else
	{
		readVal &= 0x0000FFFF;
	}


	reg[instr.rd] = readVal;

	pc += 4;
	return 3;
}

//////////////////////////////////////////////////////////////////////////
//				              ARM OPERATIONS							//
//////////////////////////////////////////////////////////////////////////
//				          LOAD / STORE MULTIPLE      					//
//////////////////////////////////////////////////////////////////////////

int CPU::opA_LDM(armInstr instr)
{

	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;  
	}

	uint16_t registerList = instr.reg_list;

	int numRegs = numOfRegisters(registerList);
	if (numRegs == 0) return 1; // nothing to transfer

	uint32_t startAddr = reg[instr.rn];

	if (!instr.U) startAddr -= (numRegs * 4); // if down bit, subtract now

	bool loadPC = (registerList >> 15) & 0b1; // save if were gonna load into pc
	bool useUserReg = instr.S && !loadPC; // if S is set, we gotta use user reg EXCEPT FOR PC
	bool restoreCPSR = instr.S && loadPC; // we must restore CPSR instead if pc is also target

	uint32_t addr = startAddr; // use this for incrementing through list

	if (instr.rn == 15) addr += 4;

	if (!instr.U)
	{
		if(instr.P) addr -= 4;
		else addr += 4;
	}

	for (uint8_t i = 0; i < 16; i++)
	{
		if (!((registerList >> i) & 0b1)) continue; // skip if not set

		if (instr.P) addr += 4; // pre address increment

		uint32_t val = read32(addr);

		if (!useUserReg)
		{
			reg[i] = val;
		}
		else
		{
			if (i >= 8 && i <= 12 && (curMode == mode::FIQ))
			{
				r8User[i-8] = val; // load the values into user ?
			}
			else if (i == 13 && !(curMode == mode::User || curMode == mode::System))
			{
				r13RegBank[getModeIndex(mode::User)] = val;
			}
			else if (i == 14 && !(curMode == mode::User || curMode == mode::System))
			{
				r14RegBank[getModeIndex(mode::User)] = val;
			}
			else reg[i] = val;
		}

		if (!instr.P)addr += 4;  // post address increment
	}

	if (instr.W) // writeback to reg
	{
		if (!((registerList >> instr.rn) & 0b1))
		{
			uint32_t writebackValue;
			if (instr.U) writebackValue = startAddr + (numRegs * 4);
			else writebackValue = startAddr;

			if (instr.rn >= 8 && instr.rn <= 12 && (curMode == mode::FIQ) && useUserReg)
			{
				r8User[instr.rn - 8] = writebackValue; 
			}

			else if (instr.rn == 13 && !(curMode == mode::User || curMode == mode::System) && useUserReg)
			{
				r13RegBank[getModeIndex(mode::User)] = writebackValue;
			}

			else if (instr.rn == 14 && !(curMode == mode::User || curMode == mode::System) && useUserReg )
			{
				r14RegBank[getModeIndex(mode::User)] = writebackValue;
			}

			else
			{
				if (instr.rn == 15) reg[15] = writebackValue + 4;
				else reg[instr.rn] = writebackValue;
			}
		}
	}


	if (loadPC) // if we loaded to pc
	{
		// Loading r15 does not exchange instruction sets.  The exception-return
		// form restores CPSR (and therefore T) from the active SPSR instead.
		if (restoreCPSR) returnFromException();
		if (!legacy::singleStepTestActive)
		{
			pc &= ~3U;
			return 2 + numRegs;
		}
	}
	pc += 4; // increment pc by 4 if used
	return 2 + numRegs;
}

int CPU::opA_STM(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	uint16_t registerList = instr.reg_list;
	int numRegs = numOfRegisters(registerList);

	if (numRegs == 0)
	{

		registerList = 0x8000; 
		numRegs = 16;
	}

	uint32_t startAddr = reg[instr.rn];
	if (!instr.U) startAddr -= (numRegs * 4); // if down bit, subtract now
	bool useUserReg = instr.S; // if S is set, we gotta use user reg
	uint32_t addr = startAddr; // use this for incrementing through list
	if (instr.rn == 15) addr += 4;
	if (!instr.U)
	{
		if (instr.P) addr -= 4;
		else addr += 4;
	}
	for (uint8_t i = 0; i < 16; i++)
	{
		if (!((registerList >> i) & 0b1)) continue; // skip if not set
		if (instr.P) addr += 4; // pre address increment
		uint32_t val;
		if (!useUserReg)
		{
			val = reg[i];
			if (i == 15) val += 4; // PC stores as PC+12
		}
		else
		{
			if (i >= 8 && i <= 12 && (curMode == mode::FIQ))
			{
				val = r8User[i - 8]; // store the values from user
			}
			else if (i == 13 && !(curMode == mode::User || curMode == mode::System))
			{
				val = r13RegBank[getModeIndex(mode::User)];
			}
			else if (i == 14 && !(curMode == mode::User || curMode == mode::System))
			{
				val = r14RegBank[getModeIndex(mode::User)];
			}
			else
			{
				val = reg[i];
				if (i == 15) val += 4; 
			}
		}
		write32(addr, val);
		if (!instr.P) addr += 4;  // post address increment
	}
	if (instr.W) // writeback to reg
	{

			
			uint32_t writebackValue;
			if (instr.U) writebackValue = startAddr + (numRegs * 4);
			else writebackValue = startAddr;

			if (instr.rn >= 8 && instr.rn <= 12 && (curMode == mode::FIQ) && useUserReg)
			{
				r8User[instr.rn - 8] = writebackValue;
			}
			else if (instr.rn == 13 && !(curMode == mode::User || curMode == mode::System) && useUserReg)
			{
				r13RegBank[getModeIndex(mode::User)] = writebackValue;
			}
			else if (instr.rn == 14 && !(curMode == mode::User || curMode == mode::System) && useUserReg)
			{
				r14RegBank[getModeIndex(mode::User)] = writebackValue;
			}
			else
			{
				if (instr.rn == 15) reg[15] = writebackValue + 4;
				else reg[instr.rn] = writebackValue;
			}
	}
	pc += 4; // increment pc by 4
	return 2 + numRegs;
}

}
