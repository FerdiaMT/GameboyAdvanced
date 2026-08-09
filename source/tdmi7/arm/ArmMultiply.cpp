#include "tdmi7/CPU.h"

namespace tdmi7
{
using namespace CPUTypes;

namespace
{
enum class MultiplyFlavor { Short, LongSigned, LongUnsigned };

bool multiplyRunsFull(uint32_t multiplier, bool isSigned)
{
	if (isSigned)
		return (multiplier & 0xFFFFFF00U) != 0 && (multiplier & 0xFFFFFF00U) != 0xFFFFFF00U
			&& (multiplier & 0xFFFF0000U) != 0 && (multiplier & 0xFFFF0000U) != 0xFFFF0000U
			&& (multiplier & 0xFF000000U) != 0 && (multiplier & 0xFF000000U) != 0xFF000000U;

	return (multiplier & 0xFFFFFF00U) != 0 && (multiplier & 0xFFFF0000U) != 0
		&& (multiplier & 0xFF000000U) != 0;
}

int32_t signExtendLow(uint32_t value, unsigned bits)
{
	const uint32_t sign = 1U << (bits - 1);
	const uint32_t mask = (1U << bits) - 1U;
	return static_cast<int32_t>(((value & mask) ^ sign) - sign);
}

bool multiplyCarryLow(uint32_t multiplicand, uint32_t multiplier, uint32_t accumulator)
{
	// The low bit ensures that negation produces the multiplier's internal
	// carry pattern without affecting the carry bit we ultimately observe.
	multiplicand |= 1U;
	int32_t booth = (multiplier & 1U) ? -1 : 0;
	uint32_t carry = multiplicand * static_cast<uint32_t>(booth);
	uint32_t sum = carry + accumulator;
	uint32_t currentAccumulator = accumulator;

	for (int shift = 29; shift >= 7;)
	{
		for (int i = 0; i < 4; ++i)
		{
			const int32_t nextBooth = signExtendLow(multiplier, 32 - shift);
			const uint32_t factor = static_cast<uint32_t>(nextBooth)
				- static_cast<uint32_t>(booth);
			booth = nextBooth;
			const uint32_t addend = multiplicand * factor;
			currentAccumulator ^= carry ^ addend;
			sum += addend;
			carry = sum - currentAccumulator;
			shift -= 2;
		}
		if (static_cast<uint32_t>(booth) == multiplier)
			break;
	}

	return (carry >> 31) != 0;
}

bool multiplyCarryHigh(uint32_t multiplicand, uint32_t multiplier, uint32_t accumulatorHigh,
	bool isSigned)
{
	const uint32_t md = (isSigned
		? static_cast<uint32_t>(static_cast<int32_t>(multiplicand) >> 6)
		: multiplicand >> 6) | 1U;
	const uint32_t mp = isSigned
		? static_cast<uint32_t>(static_cast<int32_t>(multiplier) >> 26)
		: multiplier >> 26;
	const uint32_t carry = ~accumulatorHigh & 0x20000000U;
	uint32_t accumulator = accumulatorHigh - 0x08000000U;

	const int32_t booth0 = signExtendLow(mp, 5);
	const int32_t booth1 = signExtendLow(mp, 3);
	const int32_t booth2 = signExtendLow(mp, 1);
	const uint32_t factor0 = mp - static_cast<uint32_t>(booth0);
	const uint32_t factor1 = static_cast<uint32_t>(booth0) - static_cast<uint32_t>(booth1);
	const uint32_t factor2 = static_cast<uint32_t>(booth1) - static_cast<uint32_t>(booth2);

	const uint32_t addend2 = md * factor2;
	accumulator -= addend2 & 0x10000000U;
	const uint32_t addend1 = md * factor1;
	accumulator -= addend1 & 0x40000000U;
	uint32_t sum = accumulator + (addend1 & 0x20000000U);
	accumulator -= carry;
	const uint32_t addend0 = md * factor0;
	sum += addend0 & 0x40000000U;
	return ((sum ^ accumulator) >> 31) != 0;
}

bool arm7MultiplyCarry(MultiplyFlavor flavor, uint32_t multiplicand, uint32_t multiplier,
	uint64_t accumulator)
{
	const bool isLong = flavor != MultiplyFlavor::Short;
	const bool isSigned = flavor != MultiplyFlavor::LongUnsigned;
	if (!multiplyRunsFull(multiplier, isSigned))
		return multiplyCarryLow(multiplicand, multiplier, static_cast<uint32_t>(accumulator));
	if (!isLong)
		return (multiplier >> 30) == 2;
	return multiplyCarryHigh(multiplicand, multiplier, static_cast<uint32_t>(accumulator >> 32),
		isSigned);
}
}

int CPU::opA_MUL(armInstr instr)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	pc += 4;
	uint32_t rm = (instr.rm == 15) ? pc + 4 : reg[instr.rm]; //+4 if pc
	uint32_t rs = (instr.rs == 15) ? pc + 4 : reg[instr.rs]; //+4 if pc

	uint32_t res = (static_cast<uint64_t>(rm) * static_cast<uint64_t>(rs)) & 0xFFFFFFFF;
	reg[instr.rd] = res;

	if (instr.S) // set flags
	{
		N = (res >> 31) & 0b1;
		Z = (res == 0);
		C = arm7MultiplyCarry(MultiplyFlavor::Short, rm, rs, 0);
	}

	// shortcutting the booths algo here
	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	if (instr.rd == 15)	pc += 4;

	return m + 2;
}

int CPU::opA_MLA(armInstr instr)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	pc += 4;
	uint32_t rm = (instr.rm == 15) ? pc + 4 : reg[instr.rm];
	uint32_t rs = (instr.rs == 15) ? pc + 4 : reg[instr.rs];
	uint32_t rn = (instr.rn == 15) ? pc + 4 : reg[instr.rn];

	uint32_t res = rm * rs + rn;
	reg[instr.rd] = res;

	if (instr.S) // set flags
	{
		N = (res >> 31) & 0b1;
		Z = (res == 0);
		C = arm7MultiplyCarry(MultiplyFlavor::Short, rm, rs, rn);
	}

	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	if (instr.rd == 15) pc += 4;

	return m + 2;
}

//////////////////////////////////////////////////////////////////////////
//				              ARM OPERATIONS							//
//////////////////////////////////////////////////////////////////////////
//				      MULTIPLY LONG and MULT-ACC  LONG s/u        		//
//////////////////////////////////////////////////////////////////////////

int CPU::opA_UMULL(armInstr instr) // unsigned mull (multiply long)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	pc += 4;
	uint32_t rm = (instr.rm == 15) ? pc + 4 : reg[instr.rm];
	uint32_t rs = (instr.rs == 15) ? pc + 4 : reg[instr.rs];

	uint64_t res = (static_cast<uint64_t>(rm) * static_cast<uint64_t>(rs));
	reg[instr.rn] = res & 0xFFFFFFFF;
	reg[instr.rd] = (res >> 32) & 0xFFFFFFFF;
	if (instr.S)
	{
		N = (res >> 63) & 1;
		Z = (res == 0);
		C = arm7MultiplyCarry(MultiplyFlavor::LongUnsigned, rm, rs, 0);
	}

	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	if (instr.rd == 15) pc += 4;
	if (instr.rn == 15 && instr.rd != 15) pc += 4;
	return m + 3;
}

int CPU::opA_UMLAL(armInstr instr)// unsigned mlal
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	pc += 4;
	uint32_t rm = (instr.rm == 15) ? pc + 4 : reg[instr.rm]; //+4 if pc
	uint32_t rs = (instr.rs == 15) ? pc + 4 : reg[instr.rs]; //+4 if pc

	uint64_t res = (static_cast<uint64_t>(rm) * static_cast<uint64_t>(rs));
	uint64_t acc = ((uint64_t)reg[instr.rd] << 32) | reg[instr.rn];
	res += acc;
	reg[instr.rn] = res & 0xFFFFFFFF;
	reg[instr.rd] = (res >> 32) & 0xFFFFFFFF;
	if (instr.S)
	{
		N = (res >> 63) & 1;
		Z = (res == 0);
		C = arm7MultiplyCarry(MultiplyFlavor::LongUnsigned, rm, rs, acc);
	}
	// shortcutting the booths algo here
	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	if (instr.rd == 15)	pc += 8;
	if (instr.rn == 15 && instr.rd !=15) pc += 8;

	return m + 4;
}

int CPU::opA_SMULL(armInstr instr)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	pc += 4;
	uint32_t rm = (instr.rm == 15) ? pc + 4 : reg[instr.rm];
	uint32_t rs = (instr.rs == 15) ? pc + 4 : reg[instr.rs];

	//ugly code alert
	int64_t res = (static_cast<int64_t>(static_cast<int32_t>(rm)) *static_cast<int64_t>(static_cast<int32_t>(rs)));
	reg[instr.rn] = res & 0xFFFFFFFF;
	reg[instr.rd] = (res >> 32) & 0xFFFFFFFF;
	if (instr.S)
	{
		N = (res >> 63) & 1;
		Z = (res == 0);
		C = arm7MultiplyCarry(MultiplyFlavor::LongSigned, rm, rs, 0);
	}

	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	if (instr.rd == 15) pc += 4;
	if (instr.rn == 15 && instr.rd != 15) pc += 4;
	return m + 3;
}

int CPU::opA_SMLAL(armInstr instr)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	pc += 4;
	uint32_t rm = (instr.rm == 15) ? pc + 4 : reg[instr.rm]; //+4 if pc
	uint32_t rs = (instr.rs == 15) ? pc + 4 : reg[instr.rs]; //+4 if pc

	int64_t res = (static_cast<int64_t>(static_cast<int32_t>(rm)) * static_cast<int64_t>(static_cast<int32_t>(rs)));
	uint64_t acc = ((uint64_t)reg[instr.rd] << 32) | reg[instr.rn];
	res += acc;
	reg[instr.rn] = res & 0xFFFFFFFF;
	reg[instr.rd] = (res >> 32) & 0xFFFFFFFF;
	if (instr.S)
	{
		N = (res >> 63) & 1;
		Z = (res == 0);
		C = arm7MultiplyCarry(MultiplyFlavor::LongSigned, rm, rs, acc);
	}
	// shortcutting the booths algo here
	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	if (instr.rd == 15)	pc += 8;
	if (instr.rn == 15 && instr.rd != 15) pc += 8;

	return m + 4;
}

//////////////////////////////////////////////////////////////////////////
//				              ARM OPERATIONS							//
//////////////////////////////////////////////////////////////////////////
//				          SINGLE DATA TRANSFER      					//
//////////////////////////////////////////////////////////////////////////

}
