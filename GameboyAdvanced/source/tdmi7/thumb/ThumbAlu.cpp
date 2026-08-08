#include "tdmi7/CPU.h"

namespace tdmi7
{
using namespace CPUTypes;

int CPU::opT_MOV_IMM(thumbInstr instr)
{
	reg[instr.rd] = instr.imm;
	N = instr.imm & 0x80000000;
	Z = instr.imm == 0;
	return 1;
}

int CPU::opT_ADD_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = reg[instr.rn];
	uint32_t result = op1 + op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

int CPU::opT_ADD_IMM(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 + op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

int CPU::opT_ADD_IMM3(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 + op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

int CPU::opT_SUB_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = reg[instr.rn];
	uint32_t result = op1 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

int CPU::opT_SUB_IMM(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

int CPU::opT_SUB_IMM3(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

int CPU::opT_CMP_IMM(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 - op2;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

int CPU::opT_LSL_IMM(thumbInstr instr)
{
	uint32_t value = reg[instr.rs];
	uint32_t shift = instr.imm;
	if (shift == 0)
	{
		reg[instr.rd] = value;
	}
	else
	{
		C = (value >> (32 - shift)) & 1;
		reg[instr.rd] = value << shift;
	}
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_LSR_IMM(thumbInstr instr)
{
	uint32_t value = reg[instr.rs];
	uint32_t shift = instr.imm;
	if (shift == 0) shift = 32;
	C = (value >> (shift - 1)) & 1;
	reg[instr.rd] = (shift == 32) ? 0 : (value >> shift);
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_ASR_IMM(thumbInstr instr)
{
	int32_t value = (int32_t)reg[instr.rs];
	uint32_t shift = instr.imm;
	if (shift == 0) shift = 32;
	C = (value >> (shift - 1)) & 1;
	reg[instr.rd] = (shift == 32) ? (value >> 31) : (value >> shift);
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_AND_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] & reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_EOR_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] ^ reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_LSL_REG(thumbInstr instr)
{
	uint32_t shift = reg[instr.rs] & 0xFF;
	if (shift == 0) {}
	else if (shift < 32)
	{
		C = (reg[instr.rd] >> (32 - shift)) & 1;
		reg[instr.rd] = reg[instr.rd] << shift;
	}
	else if (shift == 32)
	{
		C = reg[instr.rd] & 1;
		reg[instr.rd] = 0;
	}
	else
	{
		C = 0;
		reg[instr.rd] = 0;
	}
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_LSR_REG(thumbInstr instr)
{
	uint32_t shift = reg[instr.rs] & 0xFF;
	if (shift == 0) {}
	else if (shift < 32)
	{
		C = (reg[instr.rd] >> (shift - 1)) & 1;
		reg[instr.rd] = reg[instr.rd] >> shift;
	}
	else if (shift == 32)
	{
		C = (reg[instr.rd] >> 31) & 1;
		reg[instr.rd] = 0;
	}
	else
	{
		C = 0;
		reg[instr.rd] = 0;
	}
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_ASR_REG(thumbInstr instr)
{
	uint32_t shift = reg[instr.rs] & 0xFF;
	int32_t value = (int32_t)reg[instr.rd];
	if (shift == 0) {}
	else if (shift < 32)
	{
		C = (value >> (shift - 1)) & 1;
		reg[instr.rd] = value >> shift;
	}
	else
	{
		C = (value >> 31) & 1;
		reg[instr.rd] = value >> 31;
	}
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_ADC_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t carry = C ? 1 : 0;
	uint32_t result = op1 + op2 + carry;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2 + carry);
	return 1;
}

int CPU::opT_SBC_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t carry = C ? 0 : 1;
	uint32_t result = op1 - op2 - carry;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2 + carry);
	return 1;
}

int CPU::opT_ROR_REG(thumbInstr instr)
{
	uint32_t shift = reg[instr.rs] & 0xFF;
	if (shift == 0) {}
	else
	{
		shift = shift & 0x1F;
		if (shift == 0)
		{
			C = (reg[instr.rd] >> 31) & 1;
		}
		else
		{
			C = (reg[instr.rd] >> (shift - 1)) & 1;
			reg[instr.rd] = (reg[instr.rd] >> shift) | (reg[instr.rd] << (32 - shift));
		}
	}
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_TST_REG(thumbInstr instr)
{
	uint32_t result = reg[instr.rd] & reg[instr.rs];
	N = result & 0x80000000;
	Z = result == 0;
	return 1;
}

int CPU::opT_NEG_REG(thumbInstr instr)
{
	uint32_t op2 = reg[instr.rs];
	uint32_t result = 0 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, 0, op2);
	return 1;
}

int CPU::opT_CMP_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t result = op1 - op2;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

int CPU::opT_CMN_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t result = op1 + op2;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

int CPU::opT_ORR_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] | reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_MUL_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] * reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_BIC_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] & ~reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_MVN_REG(thumbInstr instr)
{
	reg[instr.rd] = ~reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

int CPU::opT_ADD_HI(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] + reg[instr.rs];
	if (instr.rd == 15) reg[15] = (reg[15] & ~1) + 2;

	return 1;
}

int CPU::opT_CMP_HI(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t result = op1 - op2;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

int CPU::opT_MOV_HI(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rs];

	if (instr.rd == 15) reg[15] = (reg[15] & ~1) + 2;

	return 1;
}

}
