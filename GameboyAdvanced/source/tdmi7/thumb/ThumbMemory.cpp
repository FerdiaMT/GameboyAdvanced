#include "tdmi7/CPU.h"

namespace tdmi7
{
using namespace CPUTypes;

int countSetBits(uint32_t value)
{
	int count = 0;
	while (value)
	{
		count += value & 1;
		value >>= 1;
	}
	return count;
}


//////////////////////////////////////////////////////////////////////////////////////////
///								    OPS                    							   ///
//////////////////////////////////////////////////////////////////////////////////////////
int CPU::opT_LDR_PC(thumbInstr instr)
{
	// tick() has already advanced pc by two bytes. Thumb's PC-relative base is
	// the current instruction address plus four, rounded down to a word.
	uint32_t address = ((pc + 2) & ~3U) + instr.imm;
	reg[instr.rd] = read32(address);
	return 3;
}

int CPU::opT_LDR_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	reg[instr.rd] = read32(address);
	return 3;
}

int CPU::opT_STR_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	write32(address, reg[instr.rd]);
	return 2;
}

int CPU::opT_LDRB_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	reg[instr.rd] = read8(address);
	return 3;
}

int CPU::opT_STRB_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	write8(address, reg[instr.rd] & 0xFF);
	return 2;
}

int CPU::opT_LDRH_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];

	uint32_t value = read16(address);

	uint32_t misalignment = address & 1; // if missaligned
	if (misalignment != 0)
	{
		value = (value & 0xFF) | (value & 0xFF00) << 16;
	}

	reg[instr.rd] = value;
	return 3;
}

int CPU::opT_STRH_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	write16(address, reg[instr.rd] & 0xFFFF);
	return 2;
}

int CPU::opT_LDRSB_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	int8_t value = (int8_t)read8(address);
	reg[instr.rd] = (int32_t)value;
	return 3;
}

int CPU::opT_LDRSH_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	uint16_t value = read16(address);
	uint32_t signedVal = value;

	if (address & 1) // if unaligned
	{
		value = value & 0xFF;


		if ((value >> 7) & 1)
		{
			signedVal |= 0xFFFFFF00;
		}
		else
		{
			signedVal = value;
		}

	}
	else
	{

		if ((value >> 15) & 1)
		{
			signedVal |= 0xFFFF0000;
		}
	}

	//set 16 to 31 whatever 5 is 



	reg[instr.rd] = signedVal;
	return 3;
}

int CPU::opT_LDR_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	reg[instr.rd] = read32(address);
	return 3;
}

int CPU::opT_STR_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	write32(address, reg[instr.rd]);
	return 2;
}

int CPU::opT_LDRB_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	reg[instr.rd] = read8(address);
	return 3;
}

int CPU::opT_STRB_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	write8(address, reg[instr.rd] & 0xFF);
	return 2;
}

int CPU::opT_LDRH_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	uint32_t value = read16(address); 


	uint32_t misalignment = address & 1; // if missaligned
	if (misalignment != 0)
	{
		value = (value & 0xFF) | (value & 0xFF00) << 16;
	}

	reg[instr.rd] = value;
	return 3;
}
int CPU::opT_STRH_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	write16(address, reg[instr.rd] & 0xFFFF);
	return 2;
}

int CPU::opT_LDR_SP(thumbInstr instr)
{
	uint32_t address = sp + instr.imm;
	reg[instr.rd] = read32(address);
	return 3;
}

int CPU::opT_STR_SP(thumbInstr instr)
{
	uint32_t address = sp + instr.imm;
	write32(address, reg[instr.rd]);
	return 2;
}

int CPU::opT_ADD_PC(thumbInstr instr)
{
	reg[instr.rd] = (pc & ~2) + instr.imm;
	return 1;
}

int CPU::opT_ADD_SP(thumbInstr instr)
{
	//sp += instr.imm;
	reg[instr.rd] = sp+ instr.imm;
	return 1;
}

int CPU::opT_ADD_SP_IMM(thumbInstr instr)
{
	sp = sp + (int32_t)instr.imm;
	//reg[instr.rd] = sp; so i guess this isnt needed ???
	return 1;
}

int CPU::opT_PUSH(thumbInstr instr)
{

	if (instr.imm == 0) // nothing in reg list
	{
		sp -= 4;
		write32(sp, reg[15]);
		sp -= 0x3C;
		return 1;
	}

	for (int i = 15; i >= 0; i--)
	{
		if (instr.imm & (1 << i))
		{
			sp -= 4;
			write32(sp, reg[i]);
		}
	}
	return 1 + countSetBits(instr.imm);
}

int CPU::opT_POP(thumbInstr instr)
{

	if (instr.imm == 0)
	{
		reg[15] = (read32(sp) ) + 2;  
		sp += 0x40; 
		return 1;
	}

	for (int i = 0; i < 16; i++)
	{
		if (instr.imm & (1 << i))
		{
			reg[i] = read32(sp);
			sp += 4;

			if (i == 15) reg[15] = (reg[15]+2)&~1; // Clear bit 0 for THUMB mode
		}
	}
	return 1 + countSetBits(instr.imm);
}

int CPU::opT_STMIA(thumbInstr instr)
{
	uint32_t address = reg[instr.rs];


	if ((instr.imm & 0xFF) == 0) // if loading from an empty list
	{
		write32(address, reg[15]);
		reg[instr.rs] = address + 0x40;
		return 1;
	}

	for (int i = 0; i < 8; i++)
	{
		if (instr.imm & (1 << i))
		{
			write32(address, reg[i]);
			address += 4;
		}
	}

	reg[instr.rs] = address;
	return 1 + countSetBits(instr.imm & 0xFF);
}


int CPU::opT_LDMIA(thumbInstr instr)
{
	uint32_t address = reg[instr.rs];


	if ((instr.imm & 0xFF) == 0) // if loading from an empty list
	{
		reg[15] = read32(address)+2;  
		reg[instr.rs] = address + 0x40;  
		return 1;
	}

	bool baseInList = instr.imm & (1 << instr.rs);

	for (int i = 0; i < 8; i++)
	{
		if (instr.imm & (1 << i))
		{
			reg[i] = read32(address);
			address += 4;
		}
	}

	if (!baseInList) reg[instr.rs] = address;

	return 1 + countSetBits(instr.imm & 0xFF);
}

}
