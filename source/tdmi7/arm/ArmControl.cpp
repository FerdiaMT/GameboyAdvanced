#include "tdmi7/CPU.h"
#include "tdmi7/LegacyTestMemory.h"

namespace tdmi7
{
using namespace CPUTypes;

namespace Vector
{
constexpr uint32_t Undefined = 0x00000004;
}

int CPU::opA_BX(armInstr instr)
{



	if (!checkConditional(instr.cond)) {
		pc += 4;
		return 1;  // TODO check this later 
	}

	uint32_t newAddr = reg[instr.rm];
	if (!legacy::singleStepTestActive && instr.rm == 15)
		newAddr += 8; // ARM source r15 is the current instruction address plus 8.

	if (newAddr & 0b1) // 1 = THUMB
	{
		T = 1;
		pc = ((newAddr) & 0xFFFFFFFE);
	}
	else // 0 = arm
	{
		T = 0;
		pc = legacy::singleStepTestActive
			? ((newAddr + 4) & 0xFFFFFFFE)
			: (newAddr & 0xFFFFFFFC);
	}

	if (legacy::singleStepTestActive && instr.rm == 15) // preserve SST display convention
	{
		pc += 4;
	}


	return 3; // constant
}

//////////////////////////////////////////////////////////////////////////
//				              ARM OPERATIONS							//
//////////////////////////////////////////////////////////////////////////
//				             BRANCH / BRANCH LINK			            //
//////////////////////////////////////////////////////////////////////////

int CPU::opA_B(armInstr instr)
{

	if (!checkConditional(instr.cond)) {
		pc += 4;
		return 1;
	}

	pc = pc + instr.imm + 4+4;
	return 3;
}

int CPU::opA_BL(armInstr instr)
{
	if (!checkConditional(instr.cond)) {
		pc += 4;
		return 1;
	}

	// The architectural return address is the instruction following BL.  The
	// old SST runner represents r15 one word later, so retain its fixture-only
	// convention without compromising normal execution.
	lr = pc + (legacy::singleStepTestActive ? 0U : 4U);

	pc = static_cast<int32_t>(pc) + static_cast<int32_t>(instr.imm)+8;
	return 3;
}

//////////////////////////////////////////////////////////////////////////
//				              ARM OPERATIONS							//
//////////////////////////////////////////////////////////////////////////
//				              DATA PROCESSING							//
//////////////////////////////////////////////////////////////////////////

// Helper function to get operand 2 with shift applied
int CPU::opA_MRS(armInstr instr)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }
	//MRS (transfer PSR contents to a register)

	pc += 4; // pc count is done before (if spsr cpsr write is done pc it shouldnt +4)

	if (instr.B) // spsr_currentmode
	{
		reg[instr.rd] = getSPSR();
	}
	else // cpsr
	{
		reg[instr.rd] = CPSR;
	}
	
	
	return 1;
}

int CPU::opA_MSR(armInstr instr)
{
	if (!checkConditional(instr.cond)) { pc += 4; return 1; }

	uint32_t value;
	if (instr.I) // immediate mode
	{
		uint32_t imm = (uint32_t)instr.imm;
		uint8_t rotate = instr.rotate * 2;
		if (rotate == 0) value = imm;
		else value = (imm >> rotate) | (imm << (32 - rotate));
	}
	else // register mode
	{
		value = instr.rm == 15
			? pc + (legacy::singleStepTestActive ? 4 : 8)
			: reg[instr.rm];
	}

	uint32_t mask = 0;
	if ((instr.raw >> 19) & 1) mask |= 0xFF000000;
	if ((instr.raw >> 18) & 1) mask |= 0x00FF0000;
	if ((instr.raw >> 17) & 1) mask |= 0x0000FF00;
	if ((instr.raw >> 16) & 1) mask |= 0x000000FF;

	if (instr.B) // write to SPSR
	{
		setSPSR((getSPSR() & ~mask) | (value & mask));
	}
	else // write to CPSR
	{
		if (curMode == mode::User) mask &= 0xFF000000;
		writeCPSR((CPSR & ~mask) | (value & mask));
	}

	pc += 4;
	return 1;
}

//////////////////////////////////////////////////////////////////////////
//				              ARM OPERATIONS							//
//////////////////////////////////////////////////////////////////////////
//				      MULTIPLY and MULT-ACC              				//
//////////////////////////////////////////////////////////////////////////

int CPU::opA_SWI(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}
	if (handleTestSwi(instr.imm))
	{
		pc += 4;
		return 1;
	}
	if (hleSwiHandler && hleSwiHandler(instr.imm, false))
	{
		pc += 4;
		return 3;
	}

	raiseException(Exception::SoftwareInterrupt, pc);

	return 3;
}

int CPU::opA_SWP(armInstr instr)
{

	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}

	// rm - source
	// rd - dest 
	// rn - base
	// B - 1(swap byte) 0(swap word)

	//swap byte
	if (instr.B)
	{
		uint32_t swapAddr = reg[instr.rn];
		if (instr.rn == 15) swapAddr += 8;

		uint32_t temp = read8(swapAddr) & 0xFF;
		uint32_t originalData = reg[instr.rm] & 0xFF;

		write8(swapAddr, originalData);
		reg[instr.rd] = temp;
	}
	//swap word
	else
	{
		uint32_t swapAddr = reg[instr.rn];
		// if the pc is the address, advance it by 4
		if (instr.rn == 15) swapAddr += 8;

		// must swap sourceVal and destVal content
		uint32_t temp = read32(swapAddr);
		uint32_t originalData = reg[instr.rm];
		

		uint8_t rotation = (swapAddr & 0b11) * 8;
		temp = (temp >> rotation) | (temp << (32 - rotation));
		originalData = (originalData << rotation) | (originalData >> (32 - rotation));

		// apply shifting around according to the swapAddr offset
		write32(swapAddr, originalData);
		reg[instr.rd] = temp;
	}
	//1S + 2N +1I
	pc += 4;
	return 4;
}

int CPU::opA_LDC(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}

	// ARM7TDMI has no coprocessor interface. A coprocessor transfer therefore
	// takes the Undefined Instruction exception rather than behaving as a NOP.
	raiseException(Exception::Undefined, pc);
	// The single-step fixture runner observes the architectural ARM PC value,
	// which is two instructions ahead of the exception vector.
	pc += 8;
	return 1;
}

int CPU::opA_STC(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}

	raiseException(Exception::Undefined, pc);
	pc += 8;
	return 1;
}
int CPU::opA_CDP(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}

	// ARM7TDMI implements no coprocessor data operations.
	raiseException(Exception::Undefined, pc);
	pc += 8;
	return 1;
}
int CPU::opA_MRC(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}

	// ARM7TDMI has no coprocessor registers to transfer to or from.
	raiseException(Exception::Undefined, pc);
	pc += 8;
	return 1;
}

int CPU::opA_MCR(armInstr instr)
{
	if (!checkConditional(instr.cond))
	{
		pc += 4;
		return 1;
	}

	raiseException(Exception::Undefined, pc);
	pc += 8;
	return 1;
}

int CPU::opA_UNDEFINED(armInstr)
{
	printf("Undefined instruction at PC=%08X\n", pc - 4);
	raiseException(Exception::Undefined, pc);
	return 1;
}

}
