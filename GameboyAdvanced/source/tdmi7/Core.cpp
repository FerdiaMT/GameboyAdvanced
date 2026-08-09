#define _CRT_SECURE_NO_WARNINGS

#include "tdmi7/CPU.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <algorithm>
#include <vector>
#include "tdmi7/Decoder.h"

namespace tdmi7
{
using namespace CPUTypes;

void binprintf(int v)
{
	unsigned int mask = 1 << ((sizeof(int) << 3) - 1);
	while (mask)
	{
		printf("%d", (v & mask ? 1 : 0));
		mask >>= 1;
	}
	printf("\n");
}

namespace Vector // use these for jumping
{
	constexpr uint32_t Reset = 0x00000000;
	constexpr uint32_t Undefined = 0x00000004;
	constexpr uint32_t SWI = 0x00000008;
	constexpr uint32_t PrefetchAbort = 0x0000000C;
	constexpr uint32_t DataAbort = 0x00000010;
	constexpr uint32_t Reserved = 0x00000014;
	constexpr uint32_t IRQ = 0x00000018;
	constexpr uint32_t FIQ = 0x0000001C;
}

CPU::CPU(Bus* bus, Decoder* decoder) : bus(bus), decoder(decoder), sp(reg[13]), lr(reg[14]), pc(reg[15])
{
	reset();

	initializeOpFunctions();
}

CPUTypes::armInstr CPU::decodeArm(uint32_t instr)
{
	return decoder->decodeArm(instr);
}

CPUTypes::thumbInstr CPU::decodeThumb(uint16_t instruction)
{
	return decoder->decodeThumb(instruction);
}

void CPU::reset()
{
	instruction = 0;
	curOP = Operation::UNKNOWN;
	curOpCycles = 0;
	cycleTotal = 0;
	testHalt = TestHalt::None;
	irqPending = false;
	fiqPending = false;
	halted = false;
	std::fill(std::begin(r8FIQ), std::end(r8FIQ), 0);
	std::fill(std::begin(r8User), std::end(r8User), 0);
	std::fill(std::begin(r13RegBank), std::end(r13RegBank), 0);
	std::fill(std::begin(r14RegBank), std::end(r14RegBank), 0);
	std::fill(std::begin(spsrBank), std::end(spsrBank), 0);

	curMode = mode::Supervisor;
	CPSR = static_cast<uint8_t>(mode::Supervisor) | 0xC0;
	for (int i = 0; i < 16; i++) reg[i] = 0;
	r13RegBank[getModeIndex(curMode)] = 0x03007F00;  // Initial supervisor SP
	pc = 0x08000000;
	T = 0;  // DEFAULT TO ARM
	N = Z = C = V = 0;
	unbankRegisters(curMode);
	lr = 0x08000000;
}

void CPU::requestIrq()
{
	irqPending = true;
}

void CPU::requestFiq()
{
	fiqPending = true;
}

void CPU::setIrqLine(bool asserted)
{
	irqPending = asserted;
}

void CPU::enableTestSwiHalt(bool enabled)
{
	testSwiHaltEnabled = enabled;
	testHalt = TestHalt::None;
}

bool CPU::handleTestSwi(uint32_t immediate)
{
	if (!testSwiHaltEnabled)
	{
		return false;
	}

	if (immediate == 0)
	{
		testHalt = TestHalt::Passed;
		return true;
	}

	if (immediate == 1)
	{
		testHalt = TestHalt::Failed;
		return true;
	}

	return false;
}

uint32_t CPU::tick()
{
	if (halted)
	{
		// HALT wakes when an enabled interrupt becomes pending, independently of
		// the CPSR interrupt mask.  Exception entry still honours that mask.
		if (!irqPending && !fiqPending)
		{
			curOpCycles = 1;
			cycleTotal += curOpCycles;
			return cycleTotal;
		}
		halted = false;
	}
	// FIQ has priority over IRQ.  The ARM7 samples these at an instruction
	// boundary, before fetching the next instruction.
	if (fiqPending && !F)
	{
		fiqPending = false;
		raiseException(Exception::Fiq, pc);
		curOpCycles = 3;
		cycleTotal += curOpCycles;
		return cycleTotal;
	}
	if (irqPending && !I)
	{
		irqPending = false;
		raiseException(Exception::Irq, pc);
		curOpCycles = 3;
		cycleTotal += curOpCycles;
		return cycleTotal;
	}

	const bool timingTraceEnabled = bus->isCpuTimingTraceEnabled();
	const size_t timingTraceStart = bus->cpuTimingTrace().size();

	if (!T) // if arm mode
	{
		instruction = fetch32(pc);
		curArmInstr = decodeArm(instruction);

		curOpCycles = armExecute(curArmInstr);

	}
	else // if thumb mode
	{
		uint16_t thumbCode = fetch16(pc);
		curThumbInstr = decodeThumb(thumbCode);
		pc += 2;

		curOpCycles = thumbExecute(curThumbInstr);
	}

	if (timingTraceEnabled)
	{
		// Existing handlers express the ARM7's baseline timing assuming every
		// external access takes one cycle.  Retain the remaining internal part,
		// then replace the external accesses with the bus's configured N/S cost.
		const size_t externalAccesses = bus->cpuTimingExternalAccessCountSince(timingTraceStart);
		const uint32_t internalCycles = curOpCycles > externalAccesses
			? curOpCycles - static_cast<uint32_t>(externalAccesses)
			: 0;
		bus->recordCpuInternal(internalCycles);
		curOpCycles = static_cast<uint16_t>(bus->cpuTimingCyclesSince(timingTraceStart));
	}

	cycleTotal += curOpCycles; // this could be returned and made so the ppu does this many frames too ... 

	return cycleTotal;// doing this for now
}

void CPU::initializeOpFunctions()
{
	for (int i = 0; i < static_cast<int>(armOperation::COUNT); i++)
	{
		opA_functions[i] = nullptr;
	}

	// DATA 
	opA_functions[static_cast<int>(armOperation::ARM_AND)] = &CPU::opA_AND;
	opA_functions[static_cast<int>(armOperation::ARM_EOR)] = &CPU::opA_EOR;
	opA_functions[static_cast<int>(armOperation::ARM_SUB)] = &CPU::opA_SUB;
	opA_functions[static_cast<int>(armOperation::ARM_RSB)] = &CPU::opA_RSB;
	opA_functions[static_cast<int>(armOperation::ARM_ADD)] = &CPU::opA_ADD;
	opA_functions[static_cast<int>(armOperation::ARM_ADC)] = &CPU::opA_ADC;
	opA_functions[static_cast<int>(armOperation::ARM_SBC)] = &CPU::opA_SBC;
	opA_functions[static_cast<int>(armOperation::ARM_RSC)] = &CPU::opA_RSC;
	opA_functions[static_cast<int>(armOperation::ARM_TST)] = &CPU::opA_TST;
	opA_functions[static_cast<int>(armOperation::ARM_TEQ)] = &CPU::opA_TEQ;
	opA_functions[static_cast<int>(armOperation::ARM_CMP)] = &CPU::opA_CMP;
	opA_functions[static_cast<int>(armOperation::ARM_CMN)] = &CPU::opA_CMN;
	opA_functions[static_cast<int>(armOperation::ARM_ORR)] = &CPU::opA_ORR;
	opA_functions[static_cast<int>(armOperation::ARM_MOV)] = &CPU::opA_MOV;
	opA_functions[static_cast<int>(armOperation::ARM_BIC)] = &CPU::opA_BIC;
	opA_functions[static_cast<int>(armOperation::ARM_MVN)] = &CPU::opA_MVN;

	// PSR Transfer
	opA_functions[static_cast<int>(armOperation::ARM_MRS)] = &CPU::opA_MRS;
	opA_functions[static_cast<int>(armOperation::ARM_MSR)] = &CPU::opA_MSR;

	// Load/Store
	opA_functions[static_cast<int>(armOperation::ARM_LDR)] = &CPU::opA_LDR;
	opA_functions[static_cast<int>(armOperation::ARM_STR)] = &CPU::opA_STR;
	opA_functions[static_cast<int>(armOperation::ARM_LDRH)] = &CPU::opA_LDRH;
	opA_functions[static_cast<int>(armOperation::ARM_STRH)] = &CPU::opA_STRH;
	opA_functions[static_cast<int>(armOperation::ARM_LDRSB)] = &CPU::opA_LDRSB;
	opA_functions[static_cast<int>(armOperation::ARM_LDRSH)] = &CPU::opA_LDRSH;
	opA_functions[static_cast<int>(armOperation::ARM_LDM)] = &CPU::opA_LDM;
	opA_functions[static_cast<int>(armOperation::ARM_STM)] = &CPU::opA_STM;

	// Branch
	opA_functions[static_cast<int>(armOperation::ARM_B)] = &CPU::opA_B;
	opA_functions[static_cast<int>(armOperation::ARM_BL)] = &CPU::opA_BL;
	opA_functions[static_cast<int>(armOperation::ARM_BX)] = &CPU::opA_BX;

	// Multiply
	opA_functions[static_cast<int>(armOperation::ARM_MUL)] = &CPU::opA_MUL;
	opA_functions[static_cast<int>(armOperation::ARM_MLA)] = &CPU::opA_MLA;
	opA_functions[static_cast<int>(armOperation::ARM_UMULL)] = &CPU::opA_UMULL;
	opA_functions[static_cast<int>(armOperation::ARM_UMLAL)] = &CPU::opA_UMLAL;
	opA_functions[static_cast<int>(armOperation::ARM_SMULL)] = &CPU::opA_SMULL;
	opA_functions[static_cast<int>(armOperation::ARM_SMLAL)] = &CPU::opA_SMLAL;

	// Special
	opA_functions[static_cast<int>(armOperation::ARM_SWP)] = &CPU::opA_SWP;
	opA_functions[static_cast<int>(armOperation::ARM_SWI)] = &CPU::opA_SWI;

	// Coprocessor
	opA_functions[static_cast<int>(armOperation::ARM_CDP)] = &CPU::opA_CDP;
	opA_functions[static_cast<int>(armOperation::ARM_LDC)] = &CPU::opA_LDC;
	opA_functions[static_cast<int>(armOperation::ARM_STC)] = &CPU::opA_STC;
	opA_functions[static_cast<int>(armOperation::ARM_MRC)] = &CPU::opA_MRC;
	opA_functions[static_cast<int>(armOperation::ARM_MCR)] = &CPU::opA_MCR;

	// Undefined
	opA_functions[static_cast<int>(armOperation::ARM_UNDEFINED)] = &CPU::opA_UNDEFINED;

	///////////////////////////////////////////////////////////////////////
	//								THUMB OPS                            //
	///////////////////////////////////////////////////////////////////////

	opT_functions[static_cast<int>(thumbOperation::THUMB_MOV_IMM)] = &CPU::opT_MOV_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_REG)] = &CPU::opT_ADD_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_IMM)] = &CPU::opT_ADD_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_IMM3)] = &CPU::opT_ADD_IMM3;
	opT_functions[static_cast<int>(thumbOperation::THUMB_SUB_REG)] = &CPU::opT_SUB_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_SUB_IMM)] = &CPU::opT_SUB_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_SUB_IMM3)] = &CPU::opT_SUB_IMM3;
	opT_functions[static_cast<int>(thumbOperation::THUMB_CMP_IMM)] = &CPU::opT_CMP_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LSL_IMM)] = &CPU::opT_LSL_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LSR_IMM)] = &CPU::opT_LSR_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ASR_IMM)] = &CPU::opT_ASR_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_AND_REG)] = &CPU::opT_AND_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_EOR_REG)] = &CPU::opT_EOR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LSL_REG)] = &CPU::opT_LSL_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LSR_REG)] = &CPU::opT_LSR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ASR_REG)] = &CPU::opT_ASR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADC_REG)] = &CPU::opT_ADC_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_SBC_REG)] = &CPU::opT_SBC_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ROR_REG)] = &CPU::opT_ROR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_TST_REG)] = &CPU::opT_TST_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_NEG_REG)] = &CPU::opT_NEG_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_CMP_REG)] = &CPU::opT_CMP_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_CMN_REG)] = &CPU::opT_CMN_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ORR_REG)] = &CPU::opT_ORR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_MUL_REG)] = &CPU::opT_MUL_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_BIC_REG)] = &CPU::opT_BIC_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_MVN_REG)] = &CPU::opT_MVN_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_HI)] = &CPU::opT_ADD_HI;
	opT_functions[static_cast<int>(thumbOperation::THUMB_CMP_HI)] = &CPU::opT_CMP_HI;
	opT_functions[static_cast<int>(thumbOperation::THUMB_MOV_HI)] = &CPU::opT_MOV_HI;
	opT_functions[static_cast<int>(thumbOperation::THUMB_BX)] = &CPU::opT_BX;
	opT_functions[static_cast<int>(thumbOperation::THUMB_BLX_REG)] = &CPU::opT_BLX_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDR_PC)] = &CPU::opT_LDR_PC;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDR_REG)] = &CPU::opT_LDR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STR_REG)] = &CPU::opT_STR_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDRB_REG)] = &CPU::opT_LDRB_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STRB_REG)] = &CPU::opT_STRB_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDRH_REG)] = &CPU::opT_LDRH_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STRH_REG)] = &CPU::opT_STRH_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDRSB_REG)] = &CPU::opT_LDRSB_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDRSH_REG)] = &CPU::opT_LDRSH_REG;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDR_IMM)] = &CPU::opT_LDR_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STR_IMM)] = &CPU::opT_STR_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDRB_IMM)] = &CPU::opT_LDRB_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STRB_IMM)] = &CPU::opT_STRB_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDRH_IMM)] = &CPU::opT_LDRH_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STRH_IMM)] = &CPU::opT_STRH_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDR_SP)] = &CPU::opT_LDR_SP;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STR_SP)] = &CPU::opT_STR_SP;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_PC)] = &CPU::opT_ADD_PC;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_SP)] = &CPU::opT_ADD_SP;
	opT_functions[static_cast<int>(thumbOperation::THUMB_ADD_SP_IMM)] = &CPU::opT_ADD_SP_IMM;
	opT_functions[static_cast<int>(thumbOperation::THUMB_PUSH)] = &CPU::opT_PUSH;
	opT_functions[static_cast<int>(thumbOperation::THUMB_POP)] = &CPU::opT_POP;
	opT_functions[static_cast<int>(thumbOperation::THUMB_STMIA)] = &CPU::opT_STMIA;
	opT_functions[static_cast<int>(thumbOperation::THUMB_LDMIA)] = &CPU::opT_LDMIA;
	opT_functions[static_cast<int>(thumbOperation::THUMB_B_COND)] = &CPU::opT_B_COND;
	opT_functions[static_cast<int>(thumbOperation::THUMB_B)] = &CPU::opT_B;
	opT_functions[static_cast<int>(thumbOperation::THUMB_BL_PREFIX)] = &CPU::opT_BL_PREFIX;
	opT_functions[static_cast<int>(thumbOperation::THUMB_BL_SUFFIX)] = &CPU::opT_BL_SUFFIX;
	opT_functions[static_cast<int>(thumbOperation::THUMB_SWI)] = &CPU::opT_SWI;
	opT_functions[static_cast<int>(thumbOperation::THUMB_UNDEFINED)] = &CPU::opT_UNDEFINED;
}

int CPU::armExecute(armInstr instr)
{
	return (this->*opA_functions[static_cast<int>(instr.type)])(instr);
}

//////////////////////////////////////////////////////////////////////////
//				           MODE HELPER FUNCTIONS						//
//////////////////////////////////////////////////////////////////////////

}
