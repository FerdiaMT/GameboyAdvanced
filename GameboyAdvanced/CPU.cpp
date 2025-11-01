#define _CRT_SECURE_NO_WARNINGS


#include "CPU.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>

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


CPU::CPU(Bus* bus) : bus(bus), sp(reg[13]), lr(reg[14]), pc(reg[15])
{
	reset();

	initializeOpFunctions();
}

void CPU::reset()
{
	instruction = 0;
	curOP = Operation::UNKNOWN;
	curMode = mode::System;
	CPSR = static_cast<uint8_t>(mode::Supervisor) | 0xC0;
	for (int i = 0; i < 16; i++) reg[i] = 0;
	reg[13] = 0x03007F00;  // SP
	pc = 0x08000000;
	T = 0;  // DEFAULT TO ARM
	N = Z = C = V = 0;
	unbankRegisters(curMode);
	lr = 0x08000000;
}



uint32_t CPU::tick()
{
	if (!T) // if arm mode
	{
		instruction = read32(pc);
		pc += 4;

		curOP = decode(instruction);

		printf("MODE:%s ,PC: 0x%08X, Instruction: 0x%08X, Flags: %s , R12: %08X ,Opcode: %s, \n",
			"A",
			pc - pcOffset(), instruction, CPSRtoString(), reg[12], opcodeToString(curOP));

		curOpCycles = execute();

	}
	else // if thumb mode
	{
		uint16_t thumbCode = read16(pc);
		pc += 2;
		curThumbInstr = decodeThumb(thumbCode);

		printf("MODE:%s ,PC: 0x%08X, Instruction: 0x%04X    , Flags: %08X , R12: %s ,Opcode: %s  \n",
			"T",
			pc - pcOffset(), thumbCode, CPSRtoString(), reg[12], thumbToStr(curThumbInstr).c_str());

		curOpCycles = thumbExecute(curThumbInstr);
	}


	cycleTotal += curOpCycles; // this could be returned and made so the ppu does this many frames too ... 

	return cycleTotal;// doing this for now
}



void CPU::initializeOpFunctions()
{
	for (int i = 0; i < static_cast<int>(Operation::COUNT); i++)
	{
		op_functions[i] = nullptr;
	}

	// DATA 
	op_functions[static_cast<int>(Operation::AND)] = &CPU::op_AND;
	op_functions[static_cast<int>(Operation::EOR)] = &CPU::op_EOR;
	op_functions[static_cast<int>(Operation::SUB)] = &CPU::op_SUB;
	op_functions[static_cast<int>(Operation::RSB)] = &CPU::op_RSB;
	op_functions[static_cast<int>(Operation::ADD)] = &CPU::op_ADD;
	op_functions[static_cast<int>(Operation::ADC)] = &CPU::op_ADC;
	op_functions[static_cast<int>(Operation::SBC)] = &CPU::op_SBC;
	op_functions[static_cast<int>(Operation::RSC)] = &CPU::op_RSC;
	op_functions[static_cast<int>(Operation::TST)] = &CPU::op_TST;
	op_functions[static_cast<int>(Operation::TEQ)] = &CPU::op_TEQ;
	op_functions[static_cast<int>(Operation::CMP)] = &CPU::op_CMP;
	op_functions[static_cast<int>(Operation::CMN)] = &CPU::op_CMN;
	op_functions[static_cast<int>(Operation::ORR)] = &CPU::op_ORR;
	op_functions[static_cast<int>(Operation::MOV)] = &CPU::op_MOV;
	op_functions[static_cast<int>(Operation::BIC)] = &CPU::op_BIC;
	op_functions[static_cast<int>(Operation::MVN)] = &CPU::op_MVN;

	//SPR TRANSFER
	op_functions[static_cast<int>(Operation::MRS)] = &CPU::op_MRS;
	op_functions[static_cast<int>(Operation::MSR)] = &CPU::op_MSR;

	// LOAD
	op_functions[static_cast<int>(Operation::LDR)] = &CPU::op_LDR;
	op_functions[static_cast<int>(Operation::STR)] = &CPU::op_STR;
	op_functions[static_cast<int>(Operation::LDRH)] = &CPU::op_LDRH;
	op_functions[static_cast<int>(Operation::STRH)] = &CPU::op_STRH;
	op_functions[static_cast<int>(Operation::LDRSB)] = &CPU::op_LDRSB;
	op_functions[static_cast<int>(Operation::LDRSH)] = &CPU::op_LDRSH;
	op_functions[static_cast<int>(Operation::LDM)] = &CPU::op_LDM;
	op_functions[static_cast<int>(Operation::STM)] = &CPU::op_STM;

	// BRANCH
	op_functions[static_cast<int>(Operation::B)] = &CPU::op_B;
	op_functions[static_cast<int>(Operation::BL)] = &CPU::op_BL;
	op_functions[static_cast<int>(Operation::BX)] = &CPU::op_BX;

	// MULT
	op_functions[static_cast<int>(Operation::MUL)] = &CPU::op_MUL;
	op_functions[static_cast<int>(Operation::MLA)] = &CPU::op_MLA;
	op_functions[static_cast<int>(Operation::UMULL)] = &CPU::op_UMULL;
	op_functions[static_cast<int>(Operation::UMLAL)] = &CPU::op_UMLAL;
	op_functions[static_cast<int>(Operation::SMULL)] = &CPU::op_SMULL;
	op_functions[static_cast<int>(Operation::SMLAL)] = &CPU::op_SMLAL;

	// HALDWORD
	op_functions[static_cast<int>(Operation::LDR)] = &CPU::op_LDR;
	op_functions[static_cast<int>(Operation::STR)] = &CPU::op_STR;
	op_functions[static_cast<int>(Operation::LDRH)] = &CPU::op_LDRH;
	op_functions[static_cast<int>(Operation::STRH)] = &CPU::op_STRH;
	op_functions[static_cast<int>(Operation::LDRSB)] = &CPU::op_LDRSB;
	op_functions[static_cast<int>(Operation::LDRSH)] = &CPU::op_LDRSH;
	op_functions[static_cast<int>(Operation::LDM)] = &CPU::op_LDM;
	op_functions[static_cast<int>(Operation::STM)] = &CPU::op_STM;

	// SPECIAL
	op_functions[static_cast<int>(Operation::SWP)] = &CPU::op_SWP;
	op_functions[static_cast<int>(Operation::SWPB)] = &CPU::op_SWPB;
	op_functions[static_cast<int>(Operation::SWI)] = &CPU::op_SWI;

	// COPROCESSOR
	op_functions[static_cast<int>(Operation::LDC)] = &CPU::op_LDC;
	op_functions[static_cast<int>(Operation::STC)] = &CPU::op_STC;
	op_functions[static_cast<int>(Operation::CDP)] = &CPU::op_CDP;
	op_functions[static_cast<int>(Operation::MRC)] = &CPU::op_MRC;
	op_functions[static_cast<int>(Operation::MCR)] = &CPU::op_MCR;

	// ERRORS
	op_functions[static_cast<int>(Operation::UNKNOWN)] = &CPU::op_UNKNOWN;
	op_functions[static_cast<int>(Operation::UNASSIGNED)] = &CPU::op_UNASSIGNED;
	op_functions[static_cast<int>(Operation::CONDITIONALSKIP)] = &CPU::op_CONDITIONALSKIP;
	op_functions[static_cast<int>(Operation::SINGLEDATATRANSFERUNDEFINED)] = &CPU::op_SINGLEDATATRANSFERUNDEFINED;
	op_functions[static_cast<int>(Operation::DECODEFAIL)] = &CPU::op_DECODEFAIL;


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


CPU::Operation CPU::decode(uint32_t passedIns)
{

	uint8_t conditional = (passedIns >> 28) & 0xF;
	if (!checkConditional(conditional)) return CPU::Operation::CONDITIONALSKIP;

	// so now we get bit 27,26 and 25 to tell us what passedIns to execute
	switch ((passedIns >> 25) & 0x7)
	{
	case(0b000):
	{
		// a few odd cases here
		if ((passedIns & 0x0FFFFFF0) == 0x12FFF10) return CPU::Operation::BX;
		else if ((passedIns & 0x0FB00FF0) == 0x01000090) return CPU::Operation::SWP;
		else if ((passedIns & 0x0F8000F0) == 0x00800090) // multiplyLong, can be long or accumalate
		{
			// can also be signed or unsigned
			switch ((passedIns >> 21) & 0b11)
			{
			case(0b00): return CPU::Operation::UMULL;
			case(0b01): return CPU::Operation::UMLAL;
			case(0b10): return CPU::Operation::SMULL;
			case(0b11): return CPU::Operation::SMLAL;
			}
		}
		else if ((passedIns & 0x0FC000F0) == 0x00000090)
		{
			if ((passedIns >> 21) & 0b1) return CPU::Operation::MLA;
			return CPU::Operation::MUL;
		}
		else if ((passedIns & 0x0E000090) == 0x00000090) // HalfwordTransfer
		{
			uint8_t SH = (passedIns >> 4) & 0b11;

			if (SH == 0) return CPU::Operation::SWP;

			SH = (((passedIns >> 18) & 100) | SH) & 0x7;

			switch (SH)
			{
			case(0b001): return CPU::Operation::STRH;
			case(0b101): return CPU::Operation::LDRH;
			case(0b110): return CPU::Operation::LDRSB;
			case(0b111): return CPU::Operation::LDRSH;
			default:printf("ERROR IN HALFWORD TRANSFER DECODING, GOT SH %d", SH);
			}

		}
		else if ((passedIns & 0x0FBF0FFF) == 0x010F0000) return CPU::Operation::MRS;
		else if ((passedIns & 0x0FB0FFF0) == 0x0120F000 || (passedIns & 0x0FB0F000) == 0x0320F000)return CPU::Operation::MSR;
		else
		{
			// DATA TRANSFER
			uint8_t op = (passedIns >> 21) & 0xF;

			//before we return anything, we should 

			switch (op)
			{
			case(0b0000): return CPU::Operation::AND;break;
			case(0b0001): return CPU::Operation::EOR;break;
			case(0b0010): return CPU::Operation::SUB;break;
			case(0b0011): return CPU::Operation::RSB;break;
			case(0b0100): return CPU::Operation::ADD;break;
			case(0b0101): return CPU::Operation::ADC;break;
			case(0b0110): return CPU::Operation::SBC;break;
			case(0b0111): return CPU::Operation::RSC;break;
			case(0b1000): return CPU::Operation::TST;break;
			case(0b1001): return CPU::Operation::TEQ;break;
			case(0b1010): return CPU::Operation::CMP;break;
			case(0b1011): return CPU::Operation::CMN;break;
			case(0b1100): return CPU::Operation::ORR;break;
			case(0b1101): return CPU::Operation::MOV;break;
			case(0b1110): return CPU::Operation::BIC;break;
			case(0b1111): return CPU::Operation::MVN;break;
			}
		}
	}break;

	case(0b001): // this must be transfer with immediate mode on
	{
		if ((passedIns & 0x0FBF0FFF) == 0x010F0000) return CPU::Operation::MRS;
		else if ((passedIns & 0x0FB0FFF0) == 0x0120F000 || (passedIns & 0x0FB0F000) == 0x0320F000)return CPU::Operation::MSR;
		else
		{
			switch ((passedIns >> 21) & 0xF)
			{
			case(0b0000): return CPU::Operation::AND;break;
			case(0b0001): return CPU::Operation::EOR;break;
			case(0b0010): return CPU::Operation::SUB;break;
			case(0b0011): return CPU::Operation::RSB;break;
			case(0b0100): return CPU::Operation::ADD;break;
			case(0b0101): return CPU::Operation::ADC;break;
			case(0b0110): return CPU::Operation::SBC;break;
			case(0b0111): return CPU::Operation::RSC;break;
			case(0b1000): return CPU::Operation::TST;break;
			case(0b1001): return CPU::Operation::TEQ;break;
			case(0b1010): return CPU::Operation::CMP;break;
			case(0b1011): return CPU::Operation::CMN;break;
			case(0b1100): return CPU::Operation::ORR;break;
			case(0b1101): return CPU::Operation::MOV;break;
			case(0b1110): return CPU::Operation::BIC;break;
			case(0b1111): return CPU::Operation::MVN;break;
			}
		}

	}


	case(0b011): {
		if (!((passedIns >> 4) & 0b1))
		{
			//SINGLE DATA TRANSFER
			if ((passedIns >> 20) & 0b1) return CPU::Operation::LDR;
			return CPU::Operation::STR;
		}
		else { return CPU::Operation::SINGLEDATATRANSFERUNDEFINED; }; break;
	}
	case(0b010): if ((passedIns >> 20) & 0b1) { return CPU::Operation::LDR; }
			   else { return CPU::Operation::STR; }break; // SINGLE DATA TRANSFER (AGAIN)
	case(0b100): if ((passedIns >> 20) & 0b1) { return CPU::Operation::LDM; }
			   else { return CPU::Operation::STM; }break; //BLOCK DATA TRANSFER
	case(0b101): if ((passedIns >> 24) & 0b1) { return CPU::Operation::BL; }
			   else { return CPU::Operation::B; }break;
	case(0b110): if ((passedIns >> 24) & 0b1) { return CPU::Operation::LDC; }
			   else { return CPU::Operation::STC; }break; // COPROCESSOR DATA TRANSFER


	case(0b111):
	{
		if ((passedIns >> 24) & 0b1) return CPU::Operation::SWI;
		else if (!((passedIns >> 4) & 0b1)) return CPU::Operation::CDP;
		else // coprocessor register transfer , MRC , MCR
		{
			if ((passedIns >> 20) & 0b1) { return CPU::Operation::MRC; }
			else { return CPU::Operation::MCR; }break;
		}
	}break;

	}

	return CPU::Operation::DECODEFAIL;

}

int CPU::execute()
{
	int op_index = static_cast<int>(curOP);

	if (op_functions[op_index] == nullptr)
	{
		printf("NO FUNCTION MAPPED TO INDEX %d\n", op_index);
		return 1;
	}

	return(this->*op_functions[op_index])();

}

//////////////////////////////////////////////////////////////////////////
//				           MODE HELPER FUNCTIONS						//
//////////////////////////////////////////////////////////////////////////

const char* CPU::CPSRtoString()
{
	static char str[5];

	str[0] = N ? 'N' : '-';
	str[1] = Z ? 'Z' : '-';
	str[2] = C ? 'C' : '-';
	str[3] = V ? 'V' : '-';
	str[4] = '\0';

	return str;
}

CPU::mode CPU::CPSRbitToMode(uint8_t modeBits)
{
	return static_cast<mode>(modeBits & 0x1F);
}

bool CPU::isPrivilegedMode() // used to quickly tell were not in user mode
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
	default:               return 0;
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
	if (index == 0) return; // if user or system
	spsrBank[index - 1] = CPSR;
}

// excpetion handling
void CPU::enterException(CPU::mode newMode, uint32_t vectorAddr, uint32_t returnAddr)
{
	mode oldMode = curMode; // save our old mode

	switchMode(newMode); // switch the reg bankings , swaps curMode

	uint8_t newModeIndex = getModeIndex(curMode); // new modes index for switching

	saveIntoSpsr(newModeIndex); // saves the current CPSR into the bank

	//EXTRA FOR EXCEPTION HANDLING
	CPSR |= 0x80;  // Disable IRQ
	if (newMode == mode::FIQ) CPSR |= 0x40;// turn off FIQ if on FIQ

	lr = returnAddr;
	pc = vectorAddr;


}
void CPU::returnFromException()
{
	mode oldMode = curMode;
	int oldModeIndex = getModeIndex(oldMode);

	if (oldModeIndex > 0) // if not user / system
	{
		uint32_t savedCPSR = spsrBank[oldModeIndex - 1];
		curMode = CPSRbitToMode(savedCPSR & 0x1F);

		bankRegisters(oldMode);
		CPSR = savedCPSR;
		unbankRegisters(curMode);
	}
}

//SPSR helpers
uint32_t CPU::getSPSR()
{
	uint8_t idx = getModeIndex(curMode);
	if (idx > 0)
	{
		return spsrBank[idx - 1];
	}
	return CPSR;
}
void  CPU::setSPSR(uint32_t value)
{
	int idx = getModeIndex(curMode);
	if (idx > 0) spsrBank[idx - 1] = value;
}
//CPSR helper
void CPU::writeCPSR(uint32_t value)
{
	if (curMode == mode::User && ((value & 0x1F) != static_cast<uint8_t>(mode::User))) // if were in usre, and were trying to leave it
	{
		CPSR = (CPSR & 0x000000FF) | (value & 0xFFFFFF00);  // update just flags, ignore rest
		return;
	}

	mode newMode = CPSRbitToMode(value & 0x1F);

	if (curMode != newMode) // if we should swap modes
	{
		switchMode(newMode);
	}
	else
	{
		CPSR = value;
	}
}



inline bool CPU::checkConditional(uint8_t cond) const
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
}



/////////////////////////////////////////////
///             OPCODE INSTRS()           ///
/////////////////////////////////////////////

// THIS WILL ADD 2 FOR THUMB, ADD 4 OTHERWISE

const inline uint8_t CPU::pcOffset()
{
	return (T) ? 2 : 4;
}

//HELPER FUNCTIONS FOR DATA PROCESSING



const inline uint8_t CPU::DPgetRn() { return (instruction >> 16) & 0xF; }
const inline uint8_t CPU::DPgetRd() { return (instruction >> 12) & 0xF; }
const inline uint8_t CPU::DPgetRs() { return (instruction >> 8) & 0xF; }
const inline uint8_t CPU::DPgetRm() { return instruction & 0xF; }
const inline uint8_t CPU::DPgetShift() { return (instruction >> 4) & 0xFF; }
const inline uint8_t CPU::DPgetImmed() { return instruction & 0xFF; }
const inline uint8_t CPU::DPgetRotate() { return (2 * ((instruction >> 8) & 0xF)); }
const inline bool CPU::DPs() { return (instruction >> 20) & 0b1; } // condition code
const inline bool CPU::DPi() { return (instruction >> 25) & 0b1; } // immediate code
const inline uint8_t CPU::DPgetShiftAmount(uint8_t shift)
{
	if (shift & 0b1) // shift amount depends on register if this is true
	{
		return reg[DPgetRs()] & 0xFF; // shift amount is bottom 8 bits of Rs
	}
	else
	{
		return (shift >> 3) & 0x1F;  // 5-bit immediate (0-31)
	}
}

inline uint32_t CPU::DPshiftLSL(uint32_t value, uint8_t shift_amount, bool* carry_out)
{
	if (shift_amount == 0)
	{
		return value;
	}
	else if (shift_amount < 32)
	{
		if (carry_out) *carry_out = (value >> (32 - shift_amount)) & 1;
		return value << shift_amount;
	}
	else if (shift_amount == 32)
	{
		if (carry_out) *carry_out = value & 1;
		return 0;
	}
	else
	{
		if (carry_out) *carry_out = 0;
		return 0;
	}
}
inline uint32_t CPU::DPshiftLSR(uint32_t value, uint8_t shift_amount, bool* carry_out)
{
	if (shift_amount == 0) shift_amount = 32;
	if (shift_amount < 32)
	{
		if (carry_out) *carry_out = (value >> (shift_amount - 1)) & 1;
		return value >> shift_amount;
	}
	else if (shift_amount == 32)
	{
		if (carry_out) *carry_out = (value >> 31) & 1;
		return 0;
	}
	else
	{
		if (carry_out) *carry_out = 0;
		return 0;
	}
}
inline uint32_t CPU::DPshiftASR(uint32_t value, uint8_t shift_amount, bool* carry_out)
{
	if (shift_amount == 0) shift_amount = 32;
	if (shift_amount < 32)
	{
		if (carry_out) *carry_out = (value >> (shift_amount - 1)) & 1;
		return (int32_t)value >> shift_amount;
	}
	else
	{
		if (value & 0x80000000)
		{
			if (carry_out) *carry_out = 1;
			return 0xFFFFFFFF;
		}
		else
		{
			if (carry_out) *carry_out = 0;
			return 0;
		}
	}
}
inline uint32_t CPU::DPshiftROR(uint32_t value, uint8_t shift_amount, bool* carry_out)
{
	if (shift_amount == 0)
	{
		uint8_t old_carry = C ? 1 : 0;
		if (carry_out) *carry_out = value & 1;
		return (old_carry << 31) | (value >> 1);
	}
	else
	{
		shift_amount &= 0x1F;
		if (shift_amount == 0) return value;
		if (carry_out) *carry_out = (value >> (shift_amount - 1)) & 1;
		return (value >> shift_amount) | (value << (32 - shift_amount));
	}
}
inline uint32_t CPU::DPgetOp2(bool* carryFlag)
{

	if (DPi()) // if immediate mode bit is set
	{
		uint8_t immed = DPgetImmed();
		uint8_t rotateAmt = DPgetRotate();

		uint32_t res = (immed >> rotateAmt) | (immed << (32 - rotateAmt)); // this shifts it to the right and wraps around

		// if were given a valid carryFlag (its being requested) , then check it , otherwise dont bother checking
		if (carryFlag) *carryFlag = (res >> 31) & 1;

		return res;
	}
	else // using the register shift
	{

		uint8_t rmVal = reg[DPgetRm()]; // this is the rms inside val we will shift
		uint8_t shift = DPgetShift(); // this is a very general purpouse value we will have to extract from now

		if (DPgetRm() == 15) rmVal += pcOffset();  // special case if we are shifting the pc

		switch ((shift >> 1) & 0x3)//use bits 1 and 2 of shift
		{
		case 0b00: return DPshiftLSL(rmVal, DPgetShiftAmount(shift), carryFlag);break; // LSL
		case 0b01: return DPshiftLSR(rmVal, DPgetShiftAmount(shift), carryFlag);break; // LSR
		case 0b10: return DPshiftASR(rmVal, DPgetShiftAmount(shift), carryFlag);break;// ASR
		case 0b11: return DPshiftROR(rmVal, DPgetShiftAmount(shift), carryFlag);break; // ROR
		}
	}

	return 0;
}
inline void CPU::setFlagNZC(uint32_t res, bool isCarry) // LOGICAL CHECK
{
	N = (res & 0x80000000) != 0;
	Z = (res == 0);
	C = isCarry;
}
inline void CPU::setFlagsAdd(uint32_t res, uint32_t op1, uint32_t op2)// ADD CHECK
{
	N = (res & 0x80000000) != 0;
	Z = (res == 0);
	C = (res < op1);
	V = (((op1 ^ res) & (op2 ^ res)) & 0x80000000) != 0;
}
inline void CPU::setFlagsSub(uint32_t res, uint32_t op1, uint32_t op2) // SUB CHECK
{
	N = (res & 0x80000000) != 0;
	Z = (res == 0);
	C = (op1 >= op2);
	V = (((op1 ^ op2) & (op1 ^ res)) & 0x80000000) != 0;
}
inline void CPU::setNZ(uint32_t res) // TEST CHECK
{
	N = (res & 0x80000000) != 0;
	Z = (res == 0);
}

inline void CPU::writeALUResult(uint8_t rdI, uint32_t result, bool s)
{
	if (s && rdI == 15)
	{
		returnFromException();
		reg[15] = result & ~3;
	}
	else if (rdI == 15)
	{
		reg[15] = result & ~3;
	}
	else
	{
		reg[rdI] = result;
	}
}

//////////////////////////////////////////////////////////////////////////
//				           CYCLE CALCULATORS							//
//////////////////////////////////////////////////////////////////////////

inline int CPU::dataProcessingCycleCalculator()
{
	int cycles = 1;

	if (!DPi() && (DPgetShift() & 0b1)) cycles += 1;

	if (DPgetRd() == 15) cycles += 3;

	return cycles;
}


//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				             BRANCH EXCHANGE				            //
//////////////////////////////////////////////////////////////////////////

inline int CPU::op_BX()
{
	uint32_t newAddr = reg[instruction & 0xF];

	if (newAddr & 0b1) // 1 = THUMB
	{
		T = 1;
		pc = (newAddr + 4) & 0xFFFFFFFE; //clears bit for valid 2 jumping
	}
	else // 0 = arm
	{
		T = 0;
		pc = (newAddr + 4) & 0xFFFFFFFC; // sets it to a valid num for +4 jumping 
	}

	return 3; // constant
}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				             BRANCH / BRANCH LINK			            //
//////////////////////////////////////////////////////////////////////////

inline int CPU::op_B()
{
	int32_t offset = (instruction & 0xFFFFFF);
	// FFFFFF
	if (offset & 0x800000) offset |= 0xFF000000;

	pc = pc + (offset << 2) + 4;
	return 3;
}

inline int CPU::op_BL() //TODO, make sure this is using correct logic to decide if B or BL
{
	lr = pc + pcOffset(); // this adds 2 in thumb, 4 in ARM

	int32_t offset = (instruction & 0xFFFFFF);
	// FFFFFF
	if (offset & 0x800000) offset |= 0xFF000000;

	pc = static_cast<int32_t>(pc) + static_cast<int32_t>(offset << 2);
	return 3;
}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				              DATA PROCESSING							//
//////////////////////////////////////////////////////////////////////////



// FIRST 8 BINARYS
// AND, ORR, EOR, SUB, RSB, ADD, ADC, SBC, RSC, , BIC
// BIT OPERATIONS // AND, ORR EOR

inline int CPU::op_AND()
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] & DPgetOp2(&isCarry);

	if (DPs()) { setFlagNZC(res, isCarry); }

	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

inline int CPU::op_ORR()
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] & DPgetOp2(&isCarry);


	if (DPs()) { setFlagNZC(res, isCarry); }

	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

inline int CPU::op_EOR()
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] ^ DPgetOp2(&isCarry);


	if (DPs()) { setFlagNZC(res, isCarry); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

// ADD, SUB, ADDC, SUBC

inline int CPU::op_ADD()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 + op2;


	if (DPs() && DPgetRd() != 15) { setFlagsAdd(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
inline int CPU::op_SUB()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 - op2;


	if (DPs() && DPgetRd() != 15) { setFlagsSub(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
inline int CPU::op_ADC()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr) + (uint32_t)C;
	uint32_t res = op1 + op2;


	if (DPs() && DPgetRd() != 15) { setFlagsAdd(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
inline int CPU::op_SBC()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr) - (uint32_t)C;
	uint32_t res = op1 - op2;


	if (DPs() && DPgetRd() != 15) { setFlagsSub(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

// reverse subtract, reverse subtract with carry

inline int CPU::op_RSB()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op2 - op1;


	if (DPs() && DPgetRd() != 15) { setFlagsSub(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
inline int CPU::op_RSC()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr) - (uint32_t)(!C);
	uint32_t res = op2 - op1;


	if (DPs() && DPgetRd() != 15) { setFlagsSub(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

// test ops, for writing to flag

inline int CPU::op_TST() // AND , but we dont write the val in
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] & DPgetOp2(&isCarry);

	if (DPs() && DPgetRd() != 15) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_TEQ() // XOR , but we dont write the val in
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] ^ DPgetOp2(&isCarry);

	if (DPs() && DPgetRd() != 15) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_CMP() // SUB , but we dont write the val in
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 - op2;


	if (DPs() && DPgetRd() != 15) { setFlagsSub(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
inline int CPU::op_CMN() // ADD , but we dont write the val in
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 + op2;


	if (DPs() && DPgetRd() != 15) { setFlagsAdd(res, op1, op2); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
// ops for writing 
inline int CPU::op_MOV()
{
	bool isCarry = C;

	uint32_t res = DPgetOp2(&isCarry);


	if (DPs() && DPgetRd() != 15) { setFlagNZC(res, isCarry); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}
inline int CPU::op_MVN()
{
	bool isCarry = C;

	uint32_t res = ~(DPgetOp2(&isCarry));


	if (DPs() && DPgetRd() != 15) { setFlagNZC(res, isCarry); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

inline int CPU::op_BIC()
{
	bool isCarry = C;
	uint32_t res = reg[DPgetRn()] & ~(DPgetOp2(&isCarry));

	if (DPs() && DPgetRd() != 15) { setFlagNZC(res, isCarry); }
	writeALUResult(DPgetRd(), res, DPs()); // destination, result val , dps
	return dataProcessingCycleCalculator();
}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				      PSR TRANSFER (USED BY DATAOPS) 					//
//////////////////////////////////////////////////////////////////////////


inline int CPU::op_MRS() // move psr to register
{
	if ((instruction >> 22) & 0b1) // if true , read the spsr
	{
		reg[(instruction >> 12) & 0xF] = getSPSR();
	}
	else // if false , read the cpsr
	{
		reg[(instruction >> 12) & 0xF] = CPSR;
	}

	return 1;
}
inline int CPU::op_MSR() // move into psr
{

	uint32_t value = reg[instruction & 0xF];//default on reg mode
	if ((instruction >> 25) & 0b1) // imediate mode
	{
		uint8_t imm = instruction & 0xFF;
		uint8_t rotate = ((instruction >> 8) & 0xF) * 2;
		value = (imm >> rotate) | (imm << (32 - rotate));
	}

	uint32_t mask = 0;
	if ((instruction >> 19) & 0b1) mask |= 0xFF000000;    //  if we should include flag
	if ((instruction >> 16) & 0b1) mask |= 0x000000FF;  // if we should include control

	if ((instruction >> 22) & 0b1) // write to SPSR
	{
		setSPSR((getSPSR() & ~mask) | (value & mask));
	}
	else // write to CPSR
	{
		writeCPSR((CPSR & ~mask) | (value & mask));
	}

	return 1;
}


//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				      MULTIPLY and MULT-ACC              				//
//////////////////////////////////////////////////////////////////////////

inline int CPU::op_MUL()
{
	uint8_t rm = reg[instruction & 0xF];
	uint8_t rs = reg[(instruction >> 8) & 0xF];
	uint8_t rdI = (instruction >> 16) & 0xF;

	uint32_t res = (static_cast<uint64_t>(rm) * static_cast<uint64_t>(rs)) & 0xFFFFFFFF;
	reg[rdI] = res;

	if ((instruction >> 20) & 0b1) // set flags
	{
		N = (res >> 31) & 0b1;
		Z = (res == 0);
	}

	//shortcutting the booths algo here
	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	return m + 2;
}
inline int CPU::op_MLA()
{
	uint8_t rm = reg[instruction & 0xF];
	uint8_t rs = reg[(instruction >> 8) & 0xF];
	uint8_t rn = reg[(instruction >> 12) & 0xF];
	uint8_t rdI = (instruction >> 16 & 0xF);

	uint32_t res = rm * rs + rn;
	reg[rdI] = res;

	if ((instruction >> 20) & 0b1) // set flags
	{
		N = (res >> 31) & 0b1;
		Z = (res == 0);
	}

	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m = 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m = 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m = 3;
	else m = 4;

	return m + 2;
}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				      MULTIPLY LONG and MULT-ACC  LONG s/u        		//
//////////////////////////////////////////////////////////////////////////


inline int CPU::op_UMULL()
{
	return 1;
}
inline int CPU::op_UMLAL()
{
	return 1;
}
inline int CPU::op_SMULL()
{
	return 1;
}
inline int CPU::op_SMLAL()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				          SINGLE DATA TRANSFER      					//
//////////////////////////////////////////////////////////////////////////

inline uint32_t CPU::SDapplyShift(uint32_t rmVal, uint8_t type, uint8_t amount) // singledata apply shift
{
	switch ((type))//use bits 1 and 2 of shift
	{
	case 0b00: return DPshiftLSL(rmVal, amount, nullptr);break; // LSL
	case 0b01: return DPshiftLSR(rmVal, amount, nullptr);break; // LSR
	case 0b10: return DPshiftASR(rmVal, amount, nullptr);break;// ASR
	case 0b11: return DPshiftROR(rmVal, amount, nullptr);break; // ROR
	}

}

inline uint32_t SDOffset(bool u, uint32_t newAddr, uint32_t offset)
{
	if (u) newAddr += offset; else newAddr -= offset;
	return newAddr;
}

inline int CPU::op_LDR()
{

	uint8_t rdI = (instruction >> 12) & 0xF;
	uint8_t rnI = (instruction >> 16) & 0xF;

	bool w = ((instruction >> 21) & 0b1); // if true , write address into base
	bool b = ((instruction >> 22) & 0b1); // if trye transfer byte, otherwise word
	bool u = ((instruction >> 23) & 0b1); //if true, add ofset to base , otheriwse subtract
	bool p = ((instruction >> 24) & 0b1); // if true , add prefix before transfer ,otherwise after
	bool i = ((instruction >> 25) & 0b1); // if true, use reg rm shift, otherwise keep 12 bit unsigned block

	uint32_t newAddr = reg[rnI];

	uint32_t offset = instruction & 0x0FFF;
	if (i) // use reg and rm shift
	{
		offset = SDapplyShift(reg[(instruction & 0xF)], ((instruction >> 5) & 0x3), ((instruction >> 7) & 0x1F));
	}



	if (p) newAddr = SDOffset(u, newAddr, offset);

	uint32_t readVal;
	if (b) readVal = read8(newAddr);
	else
	{
		uint32_t data = read32(newAddr & ~3);
		uint8_t rotation = (newAddr & 3) * 8;
		readVal = (data >> rotation) | (data << (32 - rotation));
	}

	if (!p) newAddr = SDOffset(u, newAddr, offset);
	//TODO , alingment masking for pc perhaps

	if (!p || w) reg[rnI] = newAddr;

	reg[rdI] = readVal;

	return 3;

}

inline int CPU::op_STR()
{
	uint8_t rdI = (instruction >> 12) & 0xF;
	uint8_t rnI = (instruction >> 16) & 0xF;

	bool w = ((instruction >> 21) & 0b1); // if true , write address into base
	bool b = ((instruction >> 22) & 0b1); // if trye transfer byte, otherwise word
	bool u = ((instruction >> 23) & 0b1); //if true, add ofset to base , otheriwse subtract
	bool p = ((instruction >> 24) & 0b1); // if true , add prefix before transfer ,otherwise after
	bool i = ((instruction >> 25) & 0b1); // if true, use reg rm shift, otherwise keep 12 bit unsigned block

	uint32_t newAddr = reg[rnI];
	uint32_t offset = instruction & 0x0FFF;
	if (i) // use reg and rm shift
	{
		offset = SDapplyShift(reg[(instruction & 0xF)], ((instruction >> 5) & 0x3), ((instruction >> 7) & 0x1F));
	}

	if (p) newAddr = SDOffset(u, newAddr, offset);

	uint32_t valToStore = reg[rdI];
	if (rdI == 15) valToStore += pcOffset();

	if (b) write8(newAddr, valToStore & 0xFF);
	else write32(newAddr & ~3, valToStore);

	if (!p) newAddr = SDOffset(u, newAddr, offset);
	//TODO , alingment masking for pc perhaps
	if (!p || w) reg[rnI] = newAddr;
	return 2;
}

inline int CPU::op_LDRH() // unsigned halfword
{
	uint8_t rmI = (instruction) & 0xF;
	uint8_t rdI = (instruction >> 12) & 0xF;
	uint8_t rnI = (instruction >> 16) & 0xF;

	bool w = ((instruction >> 21) & 0b1); // if true , write address into base
	bool u = ((instruction >> 23) & 0b1); //if true, add ofset to base , otheriwse subtract
	bool p = ((instruction >> 24) & 0b1); // if true , add prefix before transfer ,otherwise after

	uint32_t offset = reg[rmI];
	uint32_t newAddr = reg[rnI];

	if (p) newAddr = SDOffset(u, newAddr, offset);

	uint32_t readVal = read16(newAddr);

	if (!p) newAddr = SDOffset(u, newAddr, offset);

	if (!p || w)
	{
		reg[rnI] = newAddr;
	}
	reg[rdI] = readVal;

	return 3;
}
inline int CPU::op_STRH() // store unsigned halfword
{
	uint8_t rmI = (instruction) & 0xF;
	uint8_t rdI = (instruction >> 12) & 0xF;
	uint8_t rnI = (instruction >> 16) & 0xF;

	bool w = ((instruction >> 21) & 0b1); // if true , write address into base
	bool u = ((instruction >> 23) & 0b1); //if true, add ofset to base , otheriwse subtract
	bool p = ((instruction >> 24) & 0b1); // if true , add prefix before transfer ,otherwise after

	uint32_t offset = reg[rmI];
	uint32_t newAddr = reg[rnI];

	if (p) newAddr = SDOffset(u, newAddr, offset);

	uint32_t valToStore = reg[rdI];
	if (rdI == 15) valToStore += pcOffset();
	write16(newAddr, valToStore & 0xFFFF);

	if (!p) newAddr = SDOffset(u, newAddr, offset);

	if (!p || w)
	{
		reg[rnI] = newAddr;
	}

	return 2;
}
inline int CPU::op_LDRSB() //load signed byte
{
	uint8_t rmI = (instruction) & 0xF;
	uint8_t rdI = (instruction >> 12) & 0xF;
	uint8_t rnI = (instruction >> 16) & 0xF;

	bool w = ((instruction >> 21) & 0b1); // if true , write address into base
	bool u = ((instruction >> 23) & 0b1); //if true, add ofset to base , otheriwse subtract
	bool p = ((instruction >> 24) & 0b1); // if true , add prefix before transfer ,otherwise after

	uint32_t offset = reg[rmI];
	uint32_t newAddr = reg[rnI];

	if (p) newAddr = SDOffset(u, newAddr, offset);

	int8_t byteVal = read8(newAddr);
	uint32_t readVal = static_cast<int32_t>(byteVal);

	if (!p) newAddr = SDOffset(u, newAddr, offset);

	if (!p || w)
	{
		reg[rnI] = newAddr;
	}
	reg[rdI] = readVal;

	return 3;
}
inline int CPU::op_LDRSH() //load signed halfword
{
	uint8_t rmI = (instruction) & 0xF;
	uint8_t rdI = (instruction >> 12) & 0xF;
	uint8_t rnI = (instruction >> 16) & 0xF;

	bool w = ((instruction >> 21) & 0b1); // if true , write address into base
	bool u = ((instruction >> 23) & 0b1); //if true, add ofset to base , otheriwse subtract
	bool p = ((instruction >> 24) & 0b1); // if true , add prefix before transfer ,otherwise after

	uint32_t offset = reg[rmI];
	uint32_t newAddr = reg[rnI];

	if (p) newAddr = SDOffset(u, newAddr, offset);

	int8_t HWVal = read16(newAddr);
	uint32_t readVal = static_cast<int32_t>(HWVal);

	if (!p) newAddr = SDOffset(u, newAddr, offset);

	if (!p || w)
	{
		reg[rnI] = newAddr;
	}
	reg[rdI] = readVal;

	return 3;
}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				          LOAD / STORE MULTIPLE      					//
//////////////////////////////////////////////////////////////////////////

inline int numOfRegisters(uint16_t registerList)
{
	int numRegs = 0;
	for (int i = 0; i < 16; i++)
	{
		if (registerList & (1 << i)) numRegs++;
	}

	return numRegs;
}

inline int CPU::op_LDM()
{
	uint16_t registerList = instruction & 0xFFFF;
	uint8_t rnI = (instruction >> 16) & 0xF;
	bool w = (instruction >> 21) & 0b1;
	bool s = (instruction >> 22) & 0b1; // 1 ~ load PSR / force user mode, else dont
	bool u = (instruction >> 23) & 0b1;
	bool p = (instruction >> 24) & 0b1;

	// so if a bit is set in registerList, it is transfered
	int numRegs = numOfRegisters(registerList);
	if (numRegs == 0) return 1; // nothing to transfer

	uint32_t startAddr = reg[rnI];

	if (!u) startAddr -= (numRegs * 4); //if down bit, subtract now

	bool loadPC = (registerList >> 15) & 0b1; // save if were gonna load into pc
	bool useUserReg = s && !loadPC; // if s is set, we gotta use user reg EXCEPT FOR v
	bool restoreCPSR = s && loadPC; // we must restore CPSR instead if pc is also target

	uint32_t addr = startAddr; // use this for incrementing through list
	for (uint8_t i = 0; i < 16; i++)
	{
		if ((registerList >> i) & 0b0) continue; // skip if not set

		if (p) addr += 4; // pre adress increment

		uint32_t val = read32(addr & ~3);

		if (!useUserReg) reg[i] = val;
		else // if useUserReg true (s and not PC) we store into user modes r13 and r14 instead of our own
		{
			if (i == 13) r13RegBank[getModeIndex(mode::User)] = val;
			else if (i == 14) r14RegBank[getModeIndex(mode::User)] = val;
			else reg[i] = val; // if its not 13 or 14
		}

		if (!p) addr += 4; // post adress increment
	}

	if (w) // if writeback is true
	{
		if (!((registerList << rnI) & 0b1) || rnI != 15) // cant write back if both are true
		{
			if (u) reg[rnI] = startAddr + (numRegs * 4);
			else reg[rnI] = startAddr; // decrements already been done at this stage
		}
	}

	if (loadPC) // if we loaded to pc
	{
		if (restoreCPSR) returnFromException();

		if (reg[15] & 0x1) // thumb switching
		{
			T = true;
			reg[15] &= ~0x1;
		}
		else
		{
			T = false;
			reg[15] &= ~0x3;
		}
	}

	return 2 + numRegs;

}
inline int CPU::op_STM()
{
	uint16_t registerList = instruction & 0xFFFF;
	uint8_t rnI = (instruction >> 16) & 0xF;
	bool w = (instruction >> 21) & 0b1;
	bool s = (instruction >> 22) & 0b1;  // 1 ~ load PSR / force user mode, else dont
	bool u = (instruction >> 23) & 0b1;
	bool p = (instruction >> 24) & 0b1;

	int numRegs = numOfRegisters(registerList);
	if (numRegs == 0) return 1;

	uint32_t startAddr = reg[rnI];
	if (!u) startAddr -= (numRegs * 4);  // if down bit, subtract now

	bool storePC = (registerList >> 15) & 0b1;  // check if storing PC

	uint32_t addr = startAddr;
	for (uint8_t i = 0; i < 16; i++)
	{
		if (!((registerList >> i) & 0b1)) continue;

		if (p) addr += 4;  // pre increment

		uint32_t val;
		if (!s || i < 8 || i == 15)
		{
			// standard use
			val = reg[i];
			if (i == 15) val += 4;
		}
		else
		{
			// if s , user mode regs
			if (i >= 13 && i <= 14)
			{
				val = (i == 13) ? r13RegBank[getModeIndex(mode::User)] : r14RegBank[getModeIndex(mode::User)];
			}
			else if (i >= 8 && i <= 12)
			{
				// otherwise only fiq has banking
				if (curMode == mode::FIQ) val = r8FIQ[i - 8]; // this may cause bugs
				else val = reg[i];
			}
			else
			{
				val = reg[i];
			}
		}

		write32(addr & ~3, val);

		if (!p) addr += 4;  // post addr increment
	}

	if (w)  // write back
	{
		if (!((registerList >> rnI) & 0b1))
		{
			if (u) reg[rnI] = startAddr + (numRegs * 4);
			else reg[rnI] = startAddr;
		}
	}

	return 2 + numRegs;
}


inline int CPU::op_SWI()
{
	printf("SWI #%d: r0=%08X r1=%08X r2=%08X\n", (instruction & 0xFFFFFF), reg[0], reg[1], reg[2]); // debugging logger

	enterException(mode::Supervisor, Vector::SWI, pc - 4);

	return 3;
}

inline int CPU::op_SWP() { return 1; }

inline int CPU::op_SWPB() { return 1; }

inline int CPU::op_LDC() { return 1; }
inline int CPU::op_STC() { return 1; }
inline int CPU::op_CDP() { return 1; }
inline int CPU::op_MRC() { return 1; }
inline int CPU::op_MCR() { return 1; }

inline int CPU::op_DECODEFAIL()
{
	printf("Undefined instruction: %08X at PC=%08X\n", instruction, pc - 4);
	enterException(mode::Undefined, Vector::Undefined, pc - 4);
	return 1;
}
inline int CPU::op_UNKNOWN() { return 1; }
inline int CPU::op_UNASSIGNED() { return 1; }
inline int CPU::op_CONDITIONALSKIP() { return 1; }
inline int CPU::op_SINGLEDATATRANSFERUNDEFINED() { return 1; }



/////////////////////////////////////////////
///           THUMB INSTRUCTIONS          ///
/////////////////////////////////////////////
///                decode                 ///
/////////////////////////////////////////////
CPU::thumbInstr CPU::debugDecodedInstr()
{
	thumbInstr debugInstr = {};
	debugInstr.cond = NULL;
	debugInstr.rd = NULL;
	debugInstr.rs = NULL;
	debugInstr.imm = NULL;
	debugInstr.h1 = NULL;
	debugInstr.h2 = NULL;

	return debugInstr;
}

CPU::thumbInstr CPU::decodeThumb(uint16_t instr) // this returns a thumbInstr struct
{
	thumbInstr decodedInstr = {}; // creates empty struct for us to fill
	decodedInstr.type = thumbOperation::THUMB_UNDEFINED;

	//decodedInstr = debugDecodedInstr();


	switch ((instr >> 13) & 0b111)
	{
	case(0b000): // either move shift register , or add/subtract
	{
		if (((instr >> 11) & 0b11) != 0b11) // MOVE SHIFTED REGISTER
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111;
			decodedInstr.imm = (instr >> 6) & 0b11111;

			switch ((instr >> 11) & 0b11)
			{
			case(0):decodedInstr.type = thumbOperation::THUMB_LSL_IMM; break;
			case(1):decodedInstr.type = thumbOperation::THUMB_LSR_IMM; break;
			case(2):decodedInstr.type = thumbOperation::THUMB_ASR_IMM; break;
			}
		}
		else // Add/Subtract
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111;

			switch ((instr >> 9) & 0b11)
			{// 00 reg and, 01, reg sub, 10 immed and, 11 immed sub
			case(0b00): decodedInstr.rn = (instr >> 6) & 0b111; decodedInstr.type = thumbOperation::THUMB_ADD_REG; break;
			case(0b01): decodedInstr.rn = (instr >> 6) & 0b111; decodedInstr.type = thumbOperation::THUMB_SUB_REG; break;
			case(0b10): decodedInstr.imm = (instr >> 6) & 0b111;decodedInstr.type = thumbOperation::THUMB_ADD_IMM; break;
			case(0b11): decodedInstr.imm = (instr >> 6) & 0b111;decodedInstr.type = thumbOperation::THUMB_SUB_IMM; break;
			}
		}
	}break;
	case(0b001): // Move/compare/add/ subtract immediate
	{
		decodedInstr.imm = (instr) & 0xFF;
		decodedInstr.rd = (instr >> 8) & 0b111;

		switch ((instr >> 11) & 0b11)
		{
		case(0):decodedInstr.type = thumbOperation::THUMB_MOV_IMM; break;
		case(1):decodedInstr.type = thumbOperation::THUMB_CMP_IMM; break;
		case(2):decodedInstr.type = thumbOperation::THUMB_ADD_IMM3; break;
		case(3):decodedInstr.type = thumbOperation::THUMB_SUB_IMM3; break;
		}
	}break;
	case(0b010): // (ALU) or (HI register op/bex) or (pc relative) or (load/store w/ reg-offs)  or (load/store se B/HW)
	{
		if (((instr >> 10) & 0b111) == 0b000) //ALU
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111;

			switch ((instr >> 6) & 0b1111)
			{
			case(0b0000):decodedInstr.type = thumbOperation::THUMB_AND_REG; break;
			case(0b0001):decodedInstr.type = thumbOperation::THUMB_EOR_REG; break;
			case(0b0010):decodedInstr.type = thumbOperation::THUMB_LSL_REG; break;
			case(0b0011):decodedInstr.type = thumbOperation::THUMB_LSR_REG; break;
			case(0b0100):decodedInstr.type = thumbOperation::THUMB_ASR_REG; break;
			case(0b0101):decodedInstr.type = thumbOperation::THUMB_ADC_REG; break;
			case(0b0110):decodedInstr.type = thumbOperation::THUMB_SBC_REG; break;
			case(0b0111):decodedInstr.type = thumbOperation::THUMB_ROR_REG; break;
			case(0b1000):decodedInstr.type = thumbOperation::THUMB_TST_REG; break;
			case(0b1001):decodedInstr.type = thumbOperation::THUMB_NEG_REG; break;
			case(0b1010):decodedInstr.type = thumbOperation::THUMB_CMP_REG; break;
			case(0b1011):decodedInstr.type = thumbOperation::THUMB_CMN_REG; break;
			case(0b1100):decodedInstr.type = thumbOperation::THUMB_ORR_REG; break;
			case(0b1101):decodedInstr.type = thumbOperation::THUMB_MUL_REG; break;
			case(0b1110):decodedInstr.type = thumbOperation::THUMB_BIC_REG; break;
			case(0b1111):decodedInstr.type = thumbOperation::THUMB_MVN_REG; break;
			}
		}
		else if (((instr >> 10) & 0b111) == 0b001) //Hi register operations/branch exchange
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111;
			decodedInstr.h1 = (instr >> 7) & 0b1;
			decodedInstr.h2 = (instr >> 6) & 0b1;

			if (decodedInstr.h1) decodedInstr.rd += 8;
			if (decodedInstr.h2) decodedInstr.rs += 8;


			switch ((instr >> 8) & 0b11)
			{
			case 0b00: decodedInstr.type = thumbOperation::THUMB_ADD_HI; break;
			case 0b01: decodedInstr.type = thumbOperation::THUMB_CMP_HI; break;
			case 0b10: decodedInstr.type = thumbOperation::THUMB_MOV_HI; break;
			case 0b11:
			if (decodedInstr.h1) decodedInstr.type = thumbOperation::THUMB_BLX_REG;
			else decodedInstr.type = thumbOperation::THUMB_BX;
			break;
			}
		}
		else if (((instr >> 11) & 0b11) == 0b01) // pc relative load
		{
			decodedInstr.imm = ((instr) & 0xFF) << 2;
			decodedInstr.rd = (instr >> 8) & 0b111;

			decodedInstr.type = thumbOperation::THUMB_LDR_PC;
		}
		else if (((instr >> 12) & 0b1) == 1 && ((instr >> 9) & 0b1) == 0) // load store w reg offset
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111; // where rs is used instead or rb for rbase
			decodedInstr.rn = (instr >> 6) & 0b111; // where rn is used instad or r0

			switch ((instr >> 10) & 0b11)
			{
			case 0b00: decodedInstr.type = thumbOperation::THUMB_STR_REG; break;
			case 0b01: decodedInstr.type = thumbOperation::THUMB_STRB_REG; break;
			case 0b10: decodedInstr.type = thumbOperation::THUMB_LDR_REG; break;
			case 0b11: decodedInstr.type = thumbOperation::THUMB_LDRB_REG; break;
			}
		}
		else if (((instr >> 12) & 0b1) == 1 && ((instr >> 9) & 0b1) == 1) // load store w sign-extended byte / halfwor
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111; // where rs is used instead or rb for rbase
			decodedInstr.rn = (instr >> 6) & 0b111; // where rn is used instad or r0

			switch ((instr >> 10) & 0b11)
			{
			case 0b00: decodedInstr.type = thumbOperation::THUMB_STRH_REG; break;
			case 0b10: decodedInstr.type = thumbOperation::THUMB_LDRH_REG; break;
			case 0b01: decodedInstr.type = thumbOperation::THUMB_LDRSB_REG; break;
			case 0b11: decodedInstr.type = thumbOperation::THUMB_LDRSH_REG; break;
			}
		}
	}break;
	case(0b011): // Load/store with immediate offset
	{
		decodedInstr.rd = (instr) & 0b111;
		decodedInstr.rs = (instr >> 3) & 0b111; // where rs is used instead or rb for rbase
		decodedInstr.imm = (instr >> 6) & 0b11111;

		switch ((instr >> 11) & 0b11)
		{
		case 0b00: decodedInstr.type = thumbOperation::THUMB_STR_IMM; decodedInstr.imm = decodedInstr.imm << 2; break;
		case 0b01: decodedInstr.type = thumbOperation::THUMB_LDR_IMM; decodedInstr.imm = decodedInstr.imm << 2; break;
		case 0b10: decodedInstr.type = thumbOperation::THUMB_STRB_IMM; break;
		case 0b11: decodedInstr.type = thumbOperation::THUMB_LDRB_IMM; break;
		}
	}break;
	case(0b100): //(Load/store halfword) or (SP-relative load/store)
	{
		if (((instr >> 12) & 0b1) == 0b0) //Load / store halfword
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111; // where rs is used instead or rb for rbase
			decodedInstr.imm = ((instr >> 6) & 0b11111) << 1;

			switch ((instr >> 11) & 0b1)
			{
			case 0b0: decodedInstr.type = thumbOperation::THUMB_STRH_IMM; break;
			case 0b1: decodedInstr.type = thumbOperation::THUMB_LDRH_IMM; break;
			}
		}
		else // (SP-relative load/store)
		{
			decodedInstr.imm = (instr & 0xFF) << 2;
			decodedInstr.rd = (instr >> 8) & 0b111;
			decodedInstr.rs = 13;

			switch ((instr >> 11) & 0b1)
			{
			case 0b0: decodedInstr.type = thumbOperation::THUMB_STR_SP; break;
			case 0b1: decodedInstr.type = thumbOperation::THUMB_LDR_SP; break;
			}
		}

	}break;
	case(0b101): // (load addr) or (add ofs to sp) or (push/pop reg)
	{
		if (((instr >> 12) & 0b1) == 0b0) // Load Adress
		{
			decodedInstr.imm = (instr & 0xFF) << 2;
			decodedInstr.rd = (instr >> 8) & 0b111;

			switch ((instr >> 11) & 0b1)
			{
			case 0b0: decodedInstr.type = thumbOperation::THUMB_ADD_PC; break;
			case 0b1: decodedInstr.type = thumbOperation::THUMB_ADD_SP; break;
			}
		}
		else if (((instr >> 8) & 0b11111) == 0b10000) //  (add ofs to sp)
		{
			decodedInstr.imm = (instr & 0b1111111) << 2;

			if ((instr >> 7) & 0b1) decodedInstr.imm = -(int32_t)decodedInstr.imm;
			decodedInstr.type = thumbOperation::THUMB_ADD_SP_IMM;
		}
		else //  (push/pop reg)
		{
			decodedInstr.imm = (instr & 0xFF);
			bool rBit = ((instr >> 8) & 0b1);
			bool lBit = ((instr >> 11) & 0b1);

			if (!lBit) // push
			{
				if (rBit) decodedInstr.imm |= (1 << 14);  // include lr
				decodedInstr.type = thumbOperation::THUMB_PUSH;
			}
			else // pop
			{
				if (rBit) decodedInstr.imm |= (1 << 15);  // include pc
				decodedInstr.type = thumbOperation::THUMB_POP;
			}
		}
	}break;
	case(0b110): // (multi reg load/store) , (cond branch) , (SWI)
	{
		if (((instr >> 12) & 0b1) == 0b0) //(multi reg load/store)
		{
			decodedInstr.imm = (instr & 0xFF);
			decodedInstr.rs = ((instr >> 8) & 0b111);//rs is always a sub for rb
			switch (((instr >> 11) & 0b1))
			{
			case(0b0):decodedInstr.type = thumbOperation::THUMB_STMIA;break;
			case(0b1):decodedInstr.type = thumbOperation::THUMB_LDMIA;break;
			}
		}
		else if (((instr >> 8) & 0b11111) != 0b11111) //  conditional branch (done by making sure it isnt SWI first)
		{
			decodedInstr.cond = (instr >> 8) & 0b1111;
			int8_t offset8 = (instr & 0xFF);
			decodedInstr.imm = (int32_t)offset8 << 1;

			decodedInstr.type = thumbOperation::THUMB_B_COND;
		}
		else // SWI
		{
			decodedInstr.imm = instr & 0xFF;
			decodedInstr.type = thumbOperation::THUMB_SWI;
		}
	}break;
	case(0b111): // (uncond branch) or (long branch w/link)
	{
		if (((instr >> 12) & 0b1) == 0b0) // (uncond branch)
		{
			int16_t offset11 = (instr & 0x7FF);
			if (offset11 & 0x400) offset11 |= 0xF800;
			decodedInstr.imm = (int32_t)offset11 << 1;

			decodedInstr.type = thumbOperation::THUMB_B;
		}
		else // (long branch w/link)
		{
			decodedInstr.imm = (instr & 0x7FF);
			if (!((instr >> 11) & 0b1))
			{
				int16_t offset11 = (instr & 0x7FF);
				if (offset11 & 0x400) offset11 |= 0xF800;
				decodedInstr.imm = (int32_t)offset11 << 12;

				decodedInstr.type = thumbOperation::THUMB_BL_PREFIX;
			}
			else
			{
				decodedInstr.imm = decodedInstr.imm << 1;
				decodedInstr.type = thumbOperation::THUMB_BL_SUFFIX;
			}
		}

	}break;
	}

	return decodedInstr;
}

//////////////////////////////////////////////////////////////////////////////////////////
///								     ALL THUMB FUNCTION								   ///
//////////////////////////////////////////////////////////////////////////////////////////



inline void CPU::updateFlagsNZCV_Add(uint32_t result, uint32_t op1, uint32_t op2)
{
	N = (result >> 31) & 0x1;
	Z = result == 0;
	C = result < op1;

	V = ((op1 & 0x80000000) == (op2 & 0x80000000)) && ((op1 & 0x80000000) != (result & 0x80000000));
}

inline void CPU::updateFlagsNZCV_Sub(uint32_t result, uint32_t op1, uint32_t op2)
{
	N = (result >> 31) & 0x1;
	Z = result == 0;
	C = op1 >= op2;
	V = ((op1 & 0x80000000) != (op2 & 0x80000000)) && ((op1 & 0x80000000) != (result & 0x80000000));
}

inline int countSetBits(uint32_t value)
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


int CPU::thumbExecute(thumbInstr instr)
{
	return (this->*opT_functions[static_cast<int>(instr.type)])(instr);
}

inline int CPU::opT_MOV_IMM(thumbInstr instr)
{
	reg[instr.rd] = instr.imm;
	N = instr.imm & 0x80000000;
	Z = instr.imm == 0;
	return 1;
}

inline int CPU::opT_ADD_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = reg[instr.rn];
	uint32_t result = op1 + op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

inline int CPU::opT_ADD_IMM(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 + op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

inline int CPU::opT_ADD_IMM3(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 + op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

inline int CPU::opT_SUB_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = reg[instr.rn];
	uint32_t result = op1 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

inline int CPU::opT_SUB_IMM(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rs];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

inline int CPU::opT_SUB_IMM3(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

inline int CPU::opT_CMP_IMM(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = instr.imm;
	uint32_t result = op1 - op2;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

inline int CPU::opT_LSL_IMM(thumbInstr instr)
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

inline int CPU::opT_LSR_IMM(thumbInstr instr)
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

inline int CPU::opT_ASR_IMM(thumbInstr instr)
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

inline int CPU::opT_AND_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] & reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

inline int CPU::opT_EOR_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] ^ reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

inline int CPU::opT_LSL_REG(thumbInstr instr)
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

inline int CPU::opT_LSR_REG(thumbInstr instr)
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

inline int CPU::opT_ASR_REG(thumbInstr instr)
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

inline int CPU::opT_ADC_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t carry = C ? 1 : 0;
	uint32_t result = op1 + op2 + carry;
	reg[instr.rd] = result;
	updateFlagsNZCV_Add(result, op1, op2 + carry);
	return 1;
}

inline int CPU::opT_SBC_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t carry = C ? 0 : 1;
	uint32_t result = op1 - op2 - carry;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, op1, op2 + carry);
	return 1;
}

inline int CPU::opT_ROR_REG(thumbInstr instr)
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

inline int CPU::opT_TST_REG(thumbInstr instr)
{
	uint32_t result = reg[instr.rd] & reg[instr.rs];
	N = result & 0x80000000;
	Z = result == 0;
	return 1;
}

inline int CPU::opT_NEG_REG(thumbInstr instr)
{
	uint32_t op2 = reg[instr.rs];
	uint32_t result = 0 - op2;
	reg[instr.rd] = result;
	updateFlagsNZCV_Sub(result, 0, op2);
	return 1;
}

inline int CPU::opT_CMP_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t result = op1 - op2;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

inline int CPU::opT_CMN_REG(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t result = op1 + op2;
	updateFlagsNZCV_Add(result, op1, op2);
	return 1;
}

inline int CPU::opT_ORR_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] | reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

inline int CPU::opT_MUL_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] * reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

inline int CPU::opT_BIC_REG(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] & ~reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

inline int CPU::opT_MVN_REG(thumbInstr instr)
{
	reg[instr.rd] = ~reg[instr.rs];
	N = reg[instr.rd] & 0x80000000;
	Z = reg[instr.rd] == 0;
	return 1;
}

inline int CPU::opT_ADD_HI(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rd] + reg[instr.rs];
	return 1;
}

inline int CPU::opT_CMP_HI(thumbInstr instr)
{
	uint32_t op1 = reg[instr.rd];
	uint32_t op2 = reg[instr.rs];
	uint32_t result = op1 - op2;
	updateFlagsNZCV_Sub(result, op1, op2);
	return 1;
}

inline int CPU::opT_MOV_HI(thumbInstr instr)
{
	reg[instr.rd] = reg[instr.rs];
	return 1;
}

inline int CPU::opT_BX(thumbInstr instr)
{
	uint32_t target = reg[instr.rs];
	if (target & 1)
	{
		T = 1;
		pc = target & ~1; // keep thumb

	}
	else
	{
		T = 0;  //swap arm
		pc = target & ~3;
	}
	return 3;
}

inline int CPU::opT_BLX_REG(thumbInstr instr)
{
	uint32_t target = reg[instr.rs];
	lr = (pc - 2) | 1;
	if (target & 1)
	{
		pc = target & ~1;
	}
	else
	{
		T = 0;
		pc = target & ~3;
	}
	return 3;
}

inline int CPU::opT_LDR_PC(thumbInstr instr)
{
	uint32_t address = ((pc + 4) & ~2) + instr.imm;
	reg[instr.rd] = read32(address);
	return 3;
}

inline int CPU::opT_LDR_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	reg[instr.rd] = read32(address);
	return 3;
}

inline int CPU::opT_STR_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	write32(address, reg[instr.rd]);
	return 2;
}

inline int CPU::opT_LDRB_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	reg[instr.rd] = read8(address);
	return 3;
}

inline int CPU::opT_STRB_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	write8(address, reg[instr.rd] & 0xFF);
	return 2;
}

inline int CPU::opT_LDRH_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	reg[instr.rd] = read16(address);
	return 3;
}

inline int CPU::opT_STRH_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	write16(address, reg[instr.rd] & 0xFFFF);
	return 2;
}

inline int CPU::opT_LDRSB_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	int8_t value = (int8_t)read8(address);
	reg[instr.rd] = (int32_t)value;
	return 3;
}

inline int CPU::opT_LDRSH_REG(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + reg[instr.rn];
	int16_t value = (int16_t)read16(address);
	reg[instr.rd] = (int32_t)value;
	return 3;
}

inline int CPU::opT_LDR_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	reg[instr.rd] = read32(address);
	return 3;
}

inline int CPU::opT_STR_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	write32(address, reg[instr.rd]);
	return 2;
}

inline int CPU::opT_LDRB_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	reg[instr.rd] = read8(address);
	return 3;
}

inline int CPU::opT_STRB_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	write8(address, reg[instr.rd] & 0xFF);
	return 2;
}

inline int CPU::opT_LDRH_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	reg[instr.rd] = read16(address);
	return 3;
}

inline int CPU::opT_STRH_IMM(thumbInstr instr)
{
	uint32_t address = reg[instr.rs] + instr.imm;
	write16(address, reg[instr.rd] & 0xFFFF);
	return 2;
}

inline int CPU::opT_LDR_SP(thumbInstr instr)
{
	uint32_t address = sp + instr.imm;
	reg[instr.rd] = read32(address);
	return 3;
}

inline int CPU::opT_STR_SP(thumbInstr instr)
{
	uint32_t address = sp + instr.imm;
	write32(address, reg[instr.rd]);
	return 2;
}

inline int CPU::opT_ADD_PC(thumbInstr instr)
{
	reg[instr.rd] = (pc + 4 & ~2) + instr.imm;
	return 1;
}

inline int CPU::opT_ADD_SP(thumbInstr instr)
{
	sp += instr.imm;
	reg[instr.rd] = sp;
	return 1;
}

inline int CPU::opT_ADD_SP_IMM(thumbInstr instr)
{
	sp = sp + (int32_t)instr.imm;
	reg[instr.rd] = sp;
	return 1;
}

inline int CPU::opT_PUSH(thumbInstr instr)
{
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

inline int CPU::opT_POP(thumbInstr instr)
{
	for (int i = 0; i < 16; i++)
	{
		if (instr.imm & (1 << i))
		{
			reg[i] = read32(sp);
			sp += 4;
		}
	}
	return 1 + countSetBits(instr.imm);
}

inline int CPU::opT_STMIA(thumbInstr instr)
{
	uint32_t address = reg[instr.rs];
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

inline int CPU::opT_LDMIA(thumbInstr instr)
{
	uint32_t address = reg[instr.rs];
	for (int i = 0; i < 8; i++)
	{
		if (instr.imm & (1 << i))
		{
			reg[i] = read32(address);
			address += 4;
		}
	}
	reg[instr.rs] = address;
	return 1 + countSetBits(instr.imm & 0xFF);
}

inline int CPU::opT_B_COND(thumbInstr instr)
{
	if (checkConditional((uint8_t)instr.cond & 0xFF))
	{

		pc = pc + 2 + (int32_t)instr.imm;
		printf("condition TRUE\n");
	}
	else
	{
		printf("condition false so pc only increased by 2\n");
	}
	return 3;
}

inline int CPU::opT_B(thumbInstr instr)
{

	pc = pc + 2 + (int32_t)instr.imm;
	return 3;
}

inline int CPU::opT_BL_PREFIX(thumbInstr instr)
{

	lr = pc + 4 + (int32_t)instr.imm;
	return 1;
}

inline int CPU::opT_BL_SUFFIX(thumbInstr instr)
{

	uint32_t target = lr + (int32_t)instr.imm;

	lr = (pc + 2) | 1;
	pc = target & ~1;
	return 3;
}

inline int CPU::opT_SWI(thumbInstr instr)
{

	printf("SWI #%d: r0=%08X r1=%08X r2=%08X\n", instr.imm, reg[0], reg[1], reg[2]); // debugging logger
	enterException(mode::Supervisor, Vector::SWI, pc - 4);
	return 3;
}

inline int CPU::opT_UNDEFINED(thumbInstr instr)
{
	printf("UNDEFINED TRIGGERED, REPLACE LATER WITH PROPER VECTOR HANDLER");
	return 1;
}


/////////////////////////////////////////////
///             READ FUNCTIONS            ///
/////////////////////////////////////////////

uint8_t CPU::read8(uint32_t addr, bool bReadOnly)
{
	return bus->read8(addr);
}
uint16_t CPU::read16(uint32_t addr, bool bReadOnly)
{
	return bus->read16(addr);
}
uint32_t CPU::read32(uint32_t addr, bool bReadOnly)
{
	return bus->read32(addr);
}

/////////////////////////////////////////////
///             WRITE FUNCTIONS           ///
/////////////////////////////////////////////

void CPU::write8(uint32_t addr, uint8_t data)
{
	bus->write8(addr, data);
}
void CPU::write16(uint32_t addr, uint16_t data)
{
	bus->write16(addr, data);
}
void CPU::write32(uint32_t addr, uint32_t data)
{
	bus->write32(addr, data);
}

std::string CPU::thumbToStr(CPU::thumbInstr& instr)
{
	std::stringstream ss;

	auto regStr = [&](int regNum) -> std::string
		{
			std::stringstream rs;
			if (regNum == 13)
				rs << "sp[0x" << std::hex << sp << "]" << std::dec;
			else if (regNum == 14)
				rs << "lr[0x" << std::hex << lr << "]" << std::dec;
			else if (regNum == 15)
				rs << "pc[0x" << std::hex << pc << "]" << std::dec;
			else
				rs << "r" << regNum << "[0x" << std::hex << reg[regNum] << "]" << std::dec;
			return rs.str();
		};

	switch (instr.type)
	{
	case thumbOperation::THUMB_LSL_IMM:
	case thumbOperation::THUMB_LSR_IMM:
	case thumbOperation::THUMB_ASR_IMM:
	{
		const char* op = (instr.type == thumbOperation::THUMB_LSL_IMM) ? "lsl" :
			(instr.type == thumbOperation::THUMB_LSR_IMM) ? "lsr" : "asr";
		const char* sym = (instr.type == thumbOperation::THUMB_LSL_IMM) ? "<<" :
			(instr.type == thumbOperation::THUMB_LSR_IMM) ? ">>" : ">>(s)";
		ss << op << "     " << regStr(instr.rd) << ", " << regStr(instr.rs) << ", #" << instr.imm;
		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs) << " " << sym << " " << instr.imm;
		break;
	}

	case thumbOperation::THUMB_ADD_REG:
	case thumbOperation::THUMB_SUB_REG:
	{
		const char* op = (instr.type == thumbOperation::THUMB_ADD_REG) ? "add" : "sub";
		const char* sym = (instr.type == thumbOperation::THUMB_ADD_REG) ? "+" : "-";
		ss << op << "     " << regStr(instr.rd) << ", " << regStr(instr.rs) << ", " << regStr(instr.rn);
		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs) << " " << sym << " " << regStr(instr.rn);
		break;
	}

	case thumbOperation::THUMB_ADD_IMM:
	case thumbOperation::THUMB_SUB_IMM:
	{
		const char* op = (instr.type == thumbOperation::THUMB_ADD_IMM) ? "add" : "sub";
		const char* sym = (instr.type == thumbOperation::THUMB_ADD_IMM) ? "+" : "-";
		ss << op << "     " << regStr(instr.rd) << ", " << regStr(instr.rs) << ", #" << instr.imm;
		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs) << " " << sym << " #" << instr.imm;
		break;
	}

	case thumbOperation::THUMB_MOV_IMM:
	ss << "mov     " << regStr(instr.rd) << ", #0x" << std::hex << instr.imm << std::dec;
	ss << "    | " << regStr(instr.rd) << " = #0x" << std::hex << instr.imm << std::dec;
	break;

	case thumbOperation::THUMB_CMP_IMM:
	ss << "cmp     " << regStr(instr.rd) << ", #0x" << std::hex << instr.imm << std::dec;
	ss << "    | flags = " << regStr(instr.rd) << " - #0x" << std::hex << instr.imm << std::dec;
	break;

	case thumbOperation::THUMB_ADD_IMM3:
	case thumbOperation::THUMB_SUB_IMM3:
	{
		const char* op = (instr.type == thumbOperation::THUMB_ADD_IMM3) ? "add" : "sub";
		const char* sym = (instr.type == thumbOperation::THUMB_ADD_IMM3) ? "+=" : "-=";
		ss << op << "     " << regStr(instr.rd) << ", #0x" << std::hex << instr.imm << std::dec;
		ss << "    | " << regStr(instr.rd) << " " << sym << " #0x" << std::hex << instr.imm << std::dec;
		break;
	}

	case thumbOperation::THUMB_AND_REG:
	ss << "and     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " &= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_EOR_REG:
	ss << "eor     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " ^= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_LSL_REG:
	ss << "lsl     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " <<= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_LSR_REG:
	ss << "lsr     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " >>= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_ASR_REG:
	ss << "asr     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " >>= (signed) " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_ADC_REG:
	ss << "adc     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " += " << regStr(instr.rs) << " + C";
	break;
	case thumbOperation::THUMB_SBC_REG:
	ss << "sbc     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " -= " << regStr(instr.rs) << " - !C";
	break;
	case thumbOperation::THUMB_ROR_REG:
	ss << "ror     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = ror(" << regStr(instr.rd) << ", " << regStr(instr.rs) << ")";
	break;
	case thumbOperation::THUMB_TST_REG:
	ss << "tst     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " & " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_NEG_REG:
	ss << "neg     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = -" << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_CMP_REG:
	ss << "cmp     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " - " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_CMN_REG:
	ss << "cmn     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " + " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_ORR_REG:
	ss << "orr     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " |= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_MUL_REG:
	ss << "mul     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " *= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_BIC_REG:
	ss << "bic     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " &= ~" << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_MVN_REG:
	ss << "mvn     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = ~" << regStr(instr.rs);
	break;

	case thumbOperation::THUMB_ADD_HI:
	ss << "add     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " += " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_CMP_HI:
	ss << "cmp     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " - " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_MOV_HI:
	ss << "mov     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs);
	break;

	case thumbOperation::THUMB_BX:
	ss << "bx      " << regStr(instr.rs);
	ss << "    | pc = " << regStr(instr.rs) << " & ~1, T = bit0";
	break;
	case thumbOperation::THUMB_BLX_REG:
	ss << "blx     " << regStr(instr.rs);
	ss << "    | lr = pc+2, pc = " << regStr(instr.rs) << " & ~1, T = bit0";
	break;

	case thumbOperation::THUMB_LDR_PC:
	{
		uint32_t addr = (pc & ~2) + 4 + instr.imm;
		ss << "ldr     " << regStr(instr.rd) << ", [pc, #0x" << std::hex << instr.imm << "]" << std::dec;
		ss << "    | " << regStr(instr.rd) << " = [0x" << std::hex << addr << "]" << std::dec;
		break;
	}

	case thumbOperation::THUMB_STR_REG:
	case thumbOperation::THUMB_STRB_REG:
	case thumbOperation::THUMB_LDR_REG:
	case thumbOperation::THUMB_LDRB_REG:
	case thumbOperation::THUMB_STRH_REG:
	case thumbOperation::THUMB_LDRSB_REG:
	case thumbOperation::THUMB_LDRH_REG:
	case thumbOperation::THUMB_LDRSH_REG:
	{
		const char* opNames[] = {
			"str", "strb", "ldr", "ldrb", "strh", "ldrsb", "ldrh", "ldrsh"
		};
		int idx = (int)instr.type - (int)thumbOperation::THUMB_STR_REG;
		bool isLoad = (idx >= 2 && idx != 4);

		ss << opNames[idx] << "    " << regStr(instr.rd) << ", [" << regStr(instr.rs) << ", " << regStr(instr.rn) << "]";
		if (isLoad)
			ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rs) << " + " << regStr(instr.rn) << "]";
		else
			ss << "    | [" << regStr(instr.rs) << " + " << regStr(instr.rn) << "] = " << regStr(instr.rd);
		break;
	}

	case thumbOperation::THUMB_STR_IMM:
	case thumbOperation::THUMB_LDR_IMM:
	case thumbOperation::THUMB_STRB_IMM:
	case thumbOperation::THUMB_LDRB_IMM:
	case thumbOperation::THUMB_STRH_IMM:
	case thumbOperation::THUMB_LDRH_IMM:
	{
		const char* opNames[] = {
			"str", "ldr", "strb", "ldrb", "strh", "ldrh"
		};
		int idx = (int)instr.type - (int)thumbOperation::THUMB_STR_IMM;
		bool isLoad = (idx % 2 == 1);

		ss << opNames[idx] << "     " << regStr(instr.rd) << ", [" << regStr(instr.rs) << ", #0x" << std::hex << instr.imm << "]" << std::dec;
		if (isLoad)
			ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rs) << " + #0x" << std::hex << instr.imm << "]" << std::dec;
		else
			ss << "    | [" << regStr(instr.rs) << " + #0x" << std::hex << instr.imm << "] = " << std::dec << regStr(instr.rd);
		break;
	}

	case thumbOperation::THUMB_STR_SP:
	ss << "str     " << regStr(instr.rd) << ", [sp, #0x" << std::hex << instr.imm << "]" << std::dec;
	ss << "    | [sp + #0x" << std::hex << instr.imm << "] = " << std::dec << regStr(instr.rd);
	break;
	case thumbOperation::THUMB_LDR_SP:
	ss << "ldr     " << regStr(instr.rd) << ", [sp, #0x" << std::hex << instr.imm << "]" << std::dec;
	ss << "    | " << regStr(instr.rd) << " = [sp + #0x" << std::hex << instr.imm << "]" << std::dec;
	break;

	case thumbOperation::THUMB_ADD_PC:
	ss << "add     " << regStr(instr.rd) << ", pc, #0x" << std::hex << instr.imm << std::dec;
	ss << "    | " << regStr(instr.rd) << " = pc + #0x" << std::hex << instr.imm << std::dec;
	break;
	case thumbOperation::THUMB_ADD_SP_IMM:
	ss << "add     " << regStr(instr.rd) << ", sp, #0x" << std::hex << instr.imm << std::dec;
	ss << "    | " << regStr(instr.rd) << " = sp + #0x" << std::hex << instr.imm << std::dec;
	break;

	case thumbOperation::THUMB_ADD_SP:
	if ((int32_t)instr.imm < 0)
	{
		ss << "sub     sp, #0x" << std::hex << (-(int32_t)instr.imm) << std::dec;
		ss << "    | sp -= #0x" << std::hex << (-(int32_t)instr.imm) << std::dec;
	}
	else
	{
		ss << "add     sp, #0x" << std::hex << instr.imm << std::dec;
		ss << "    | sp += #0x" << std::hex << instr.imm << std::dec;
	}
	break;

	case thumbOperation::THUMB_PUSH:
	case thumbOperation::THUMB_POP:
	{
		const char* op = (instr.type == thumbOperation::THUMB_PUSH) ? "push" : "pop";
		ss << op << "    {";
		bool first = true;
		for (int i = 0; i < 16; i++)
		{
			if (instr.imm & (1 << i))
			{
				if (!first) ss << ", ";
				ss << regStr(i);
				first = false;
			}
		}
		ss << "}";
		break;
	}

	case thumbOperation::THUMB_STMIA:
	case thumbOperation::THUMB_LDMIA:
	{
		const char* op = (instr.type == thumbOperation::THUMB_STMIA) ? "stmia" : "ldmia";
		ss << op << "   " << regStr(instr.rs) << "!, {";
		bool first = true;
		for (int i = 0; i < 8; i++)
		{
			if (instr.imm & (1 << i))
			{
				if (!first) ss << ", ";
				ss << regStr(i);
				first = false;
			}
		}
		ss << "}";
		break;
	}

	case thumbOperation::THUMB_B_COND:
	{
		const char* condNames[] = {
			"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
			"hi", "ls", "ge", "lt", "gt", "le", "al", "nv"
		};
		int32_t offset = (int32_t)instr.imm;
		uint32_t target = (pc + 4 + offset) & ~1;
		ss << "b" << condNames[instr.cond] << "     0x" << std::hex << target << std::dec;
		ss << "    | if " << condNames[instr.cond] << " then pc = 0x" << std::hex << target << std::dec;
		break;
	}

	case thumbOperation::THUMB_B:
	{
		int32_t offset = (int32_t)instr.imm;
		uint32_t target = (pc + 4 + offset) & ~1;
		ss << "b       0x" << std::hex << target << std::dec;
		ss << "    | pc = 0x" << std::hex << target << std::dec;
		break;
	}

	case thumbOperation::THUMB_BL_PREFIX:
	ss << "bl_hi   0x" << std::hex << instr.imm << std::dec;
	ss << "    | lr = pc + 0x" << std::hex << instr.imm << std::dec;
	break;
	case thumbOperation::THUMB_BL_SUFFIX:
	{
		uint32_t target = (lr + instr.imm) & ~1;
		ss << "bl_lo   0x" << std::hex << target << std::dec;
		ss << "    | pc = 0x" << std::hex << target << ", lr = 0x" << (pc + 2) << std::dec;
		break;
	}

	case thumbOperation::THUMB_SWI:
	ss << "swi     #0x" << std::hex << (instr.imm & 0xFF) << std::dec;
	break;

	case thumbOperation::THUMB_UNDEFINED:
	ss << "undefined";
	break;

	default:
	ss << "unknown";
	break;
	}

	return ss.str();
}

// made specially for this 

//THUMB TESTS
//    thumb_add_cmp_mov_hi.json.bin - 860 passed
//		thumb_add_sp_or_pc.json.bin - 34 passed
//		thumb_add_sub.json.bin -  50000 passed
//		thumb_add_sub_sp.json.bin- 0 passed

//		thumb_bcc.json.bin - 216 passed
//		thumb_bl_blx_prefix.json - 0 passed
//		thumb_bl_suffix.json.bin - o passed
//	     thumb_bx.json - 0 passed
//		thumb_data_proc.json - 46813 passed
//		thumb_ldm_stm.json.bin - 1 passed
//		thumb_ldr_pc_rel.json.bin =- 0
//       thumb_ldr_str_imm_offset.json - 5542


void CPU::runThumbTests()
{
	const char* str = "thumb_bcc.json.bin";

	FILE* f = fopen(str, "rb");
	if (!f)
	{
		printf("ERROR: Could not open test file!\n");
		return;
	}

	int passed = 0;
	int failed = 0;
	int maxFailuresToShow = 10;
	int failuresShown = 0;

	uint32_t magic, numTests, testSize, stateSize, val;
	fread(&magic, 4, 1, f);
	fread(&numTests, 4, 1, f);


	printf("Magic: 0x%08x\n", magic);
	printf("Number of tests: %d\n\n", numTests);


	for (int tNum = 0; tNum < numTests; tNum++)
	{
		fread(&testSize, 4, 1, f);
		fread(&stateSize, 4, 1, f);
		fread(&val, 4, 1, f);
		int amtOfTransactions = (testSize - 368) / 24;
		uint32_t R_init[16];
		fread(R_init, 4, 16, f);
		uint32_t R_fiq_init[7];
		fread(R_fiq_init, 4, 7, f);
		uint32_t R_svc_init[2];
		fread(R_svc_init, 4, 2, f);
		uint32_t R_abt_init[2];
		fread(R_abt_init, 4, 2, f);
		uint32_t R_irq_init[2];
		fread(R_irq_init, 4, 2, f);
		uint32_t R_und_init[2];
		fread(R_und_init, 4, 2, f);
		uint32_t CPSR_init;
		fread(&CPSR_init, 4, 1, f);
		uint32_t SPSR_init[5];
		fread(SPSR_init, 4, 5, f);
		uint32_t pipeline_init[2];
		fread(pipeline_init, 4, 2, f);
		uint32_t access_init;
		fread(&access_init, 4, 1, f);
		uint32_t junkA;
		fread(&junkA, 4, 1, f);
		uint32_t junkB;
		fread(&junkB, 4, 1, f);
		uint32_t R_final[16];
		fread(R_final, 4, 16, f);
		uint32_t R_fiq_final[7];
		fread(R_fiq_final, 4, 7, f);
		uint32_t R_svc_final[2];
		fread(R_svc_final, 4, 2, f);
		uint32_t R_abt_final[2];
		fread(R_abt_final, 4, 2, f);
		uint32_t R_irq_final[2];
		fread(R_irq_final, 4, 2, f);
		uint32_t R_und_final[2];
		fread(R_und_final, 4, 2, f);
		uint32_t CPSR_final;
		fread(&CPSR_final, 4, 1, f);
		uint32_t SPSR_final[5];
		fread(SPSR_final, 4, 5, f);
		uint32_t pipeline_final[2];
		fread(pipeline_final, 4, 2, f);
		uint32_t access_final;
		fread(&access_final, 4, 1, f);;
		junkA;
		fread(&junkA, 4, 1, f);
		junkB;
		fread(&junkB, 4, 1, f);
		uint32_t junkC;
		fread(&junkC, 4, 1, f);
		// TRANSACTIONS
		int transactionCounter = 0;
		while (transactionCounter < amtOfTransactions)
		{
			uint32_t trans_kind, trans_size, trans_addr, trans_data, trans_cycle, trans_access;
			fread(&trans_kind, 4, 1, f);
			fread(&trans_size, 4, 1, f);
			fread(&trans_addr, 4, 1, f);
			fread(&trans_data, 4, 1, f);
			fread(&trans_cycle, 4, 1, f);
			fread(&trans_access, 4, 1, f);
			transactionCounter++;

		}
		uint32_t junkArr2[3];
		fread(&junkArr2, 4, 2, f);
		uint16_t opcode, padding;
		uint32_t base_addr;
		fread(&opcode, 2, 1, f);
		fread(&padding, 2, 1, f);
		fread(&base_addr, 4, 1, f);

		//////////////
		// LOADS
		////////////

		if (tNum ==1)
		{
			reset();

			for (int r = 0; r < 16; r++)
				reg[r] = R_init[r];

			pc = base_addr + 4;

			CPSR = CPSR_init; //load cspr

			for (int r = 0; r < 5; r++) // load spsr
				spsrBank[r] = SPSR_init[r];

			///DECODE / EXECUTE
			thumbInstr decoded = decodeThumb(opcode);
			pc += 2;
			thumbExecute(decoded);

			// Check results - compare ALL registers including PC
			bool testPassed = true;

			for (int r = 0; r < 16; r++)
			{
				if (reg[r] != R_final[r])
				{
					testPassed = false;
					if (failuresShown < maxFailuresToShow)
					{
						printf("Test %d FAILED (opcode 0x%04x @ 0x%08x): r%d = 0x%08x, expected 0x%08x\n",
							tNum, opcode, base_addr, r, reg[r], R_final[r]);
						if (CPSR != CPSR_final)
						{
							printf("Test %d FAILED (opcode 0x%04x @ 0x%08x): CPSR: %08x, expected CPSR: %08x\n",
								tNum, opcode, base_addr, CPSR, CPSR_final);
						}
					}
				}
				else if (CPSR != CPSR_final)
				{
					testPassed = false;
					if (failuresShown < maxFailuresToShow)
					{
						printf("Test %d FAILED (opcode 0x%04x @ 0x%08x): CPSR: %08x, expected CPSR: %08x\n",
							tNum, opcode, base_addr, CPSR, CPSR_final);
					}
				}
			}

			if (testPassed)
				passed++;
			else
			{
				failed++;
				if (failuresShown < maxFailuresToShow)
					failuresShown++;
				else if (failuresShown == maxFailuresToShow)
				{
					printf("  ... (suppressing further failures)\n");
					failuresShown++;
				}
			}

			// Progress
			if (tNum > 0 && tNum % 5000 == 0)
				printf("  Progress: %d/%d... (%d passed, %d failed)\n",
					tNum, numTests, passed, failed);
		}
	}
	printf("\n========================================\n");
	printf("Results: %d passed, %d failed out of %d\n",
		passed, failed, numTests);
	printf("========================================\n");

	fclose(f);
}