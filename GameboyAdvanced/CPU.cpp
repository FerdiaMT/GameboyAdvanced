#include "CPU.h"
#include <cstdint>

CPU::CPU(Bus* bus) : bus(bus) , sp(reg[13]) , lr(reg[14]) , pc(reg[15])
{
	for (int i = 0; i < 16; i++)
	{
		reg[i] = 0;
	}
	CPSR = 0;

	instruction = 0;
	curOP = Operation::UNKNOWN;
	curMode = mode::Supervisor;

	initializeOpFunctions();
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
}


//////////////////////////////////////////////////////////////////////////
//				           MODE HELPER FUNCTIONS						//
//////////////////////////////////////////////////////////////////////////


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

	curOP = decode();

	curOpCycles = execute();

	cycleTotal += curOpCycles; // this could be returned and made so the ppu does this many frames too ... 
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

CPU::Operation CPU::decode()
{
	//MRS AND MSR ARE NOT INCLUDED . THEY ARE DONE BY CERTAIN DATA TRANSFER OPERATIONS


	// first we should check if the conditional is valid
	// 
	// 1010 0101 1010 0101 0101 0101 1010 1010
	//  ^ this is conditional bits
	
	uint8_t conditional = (instruction>>28) & 0xF;
	if (!checkConditional(conditional)) return Operation::CONDITIONALSKIP;

	// so now we get bit 27,26 and 25 to tell us what instruction to execute
	switch ((instruction >> 25) & 0x7)
	{
	case(0b000):
	{
		// a few odd cases here
		if ((instruction & 0x0FFFFFF0) == 0x12FFF10) return Operation::BX;
		else if ((instruction & 0x0FB00FF0) == 0x01000090) return Operation::SWP;
		else if ((instruction & 0x0F8000F0) == 0x00800090) // multiplyLong, can be long or accumalate
		{
			// can also be signed or unsigned
			switch ((instruction >> 21) & 0b11)
			{
			case(0b00): return Operation::UMULL;
			case(0b01): return Operation::UMLAL;
			case(0b10): return Operation::SMULL;
			case(0b11): return Operation::SMLAL;
			}
		}
		else if ((instruction & 0x0FC000F0) == 0x00000090) 
		{
			if ((instruction >> 21) & 0b1) return Operation::MLA;
			return Operation::MUL;
		}
		else if ((instruction & 0x0E000090) == 0x00000090) // HalfwordTransfer
		{
			uint8_t SH = (instruction >> 4) & 0b11;

			if (SH == 0) return Operation::SWP;

			SH = (((instruction >> 18) & 100) | SH)&0x7;

			switch (SH)
			{
			case(0b001): return Operation::STRH;
			case(0b101): return Operation::LDRH;
			case(0b110): return Operation::LDRSB;
			case(0b111): return Operation::LDRSH;
			default:printf("ERROR IN HALFWORD TRANSFER DECODING, GOT SH %d", SH);
			}

		}
		else if ((instruction & 0x0FBF0FFF) == 0x010F0000) return Operation::MRS;
		else if ((instruction & 0x0FB0FFF0) == 0x0120F000 || (instruction & 0x0FBF0000) == 0x03200000)return Operation::MSR;
		else {
			// DATA TRANSFER
			uint8_t op = (instruction >> 21) & 0xF;

			//before we return anything, we should 

			switch (op)
			{
			case(0b0000): return Operation::AND;break;
			case(0b0001): return Operation::EOR;break;
			case(0b0010): return Operation::SUB;break;
			case(0b0011): return Operation::RSB;break;
			case(0b0100): return Operation::ADD;break;
			case(0b0101): return Operation::ADC;break;
			case(0b0110): return Operation::SBC;break;
			case(0b0111): return Operation::RSC;break;
			case(0b1001): return Operation::TEQ;break;
			case(0b1010): return Operation::CMP;break;
			case(0b1011): return Operation::CMN;break;
			case(0b1100): return Operation::ORR;break;
			case(0b1101): return Operation::MOV;break;
			case(0b1110): return Operation::BIC;break;
			case(0b1111): return Operation::MVN;break;
			}
		}
	}break;

	case(0b011): {
		if (!((instruction >> 4) & 0b1)) 
		{ 
			//SINGLE DATA TRANSFER
			if ((instruction >> 20) & 0b1) return Operation::LDR;
			return Operation::STR;
		}
		else { return Operation::SINGLEDATATRANSFERUNDEFINED; }; break;
	}
	case(0b010): if ((instruction >> 20) & 0b1) { return Operation::LDR; }else { return Operation::STR; }break; // SINGLE DATA TRANSFER (AGAIN)
	case(0b100): if ((instruction >> 20) & 0b1) { return Operation::LDM; }else { return Operation::STM; }break; //BLOCK DATA TRANSFER
	case(0b101): if ((instruction >> 24) & 0b1) { return Operation::BL; }else { return Operation::B; }break;
	case(0b110): if ((instruction >> 24) & 0b1) { return Operation::LDC; }else { return Operation::STC; }break; // COPROCESSOR DATA TRANSFER
	

	case(0b111): 
	{
		if ((instruction >> 24) & 0b1) return Operation::SWI;
		else if (!((instruction >> 4) & 0b1)) return Operation::CDP;
		else // coprocessor register transfer , MRC , MCR
		{
			if ((instruction >> 20) & 0b1) { return Operation::MRC; }
			else { return Operation::MCR; }break;
		}
	}break;

	}

	return Operation::DECODEFAIL;

}

int CPU::execute()
{
	int op_index = static_cast<int>(curOP);

	if (op_functions[op_index] == nullptr)
	{
		printf("NO FUNCTION MAPPED TO INDEX %d\n", op_index);
		return;
	}

	return (this->*op_functions[op_index])();
}

/////////////////////////////////////////////
///             OPCODE INSTRS()           ///
/////////////////////////////////////////////

// THIS WILL ADD 2 FOR THUMB, ADD 4 OTHERWISE

const inline uint8_t CPU::pcOffset()
{
	return (T)? 2 : 4;
}

//HELPER FUNCTIONS FOR DATA PROCESSING
const inline uint8_t CPU::DPgetRn() {return (instruction >> 16) & 0xF;}
const inline uint8_t CPU::DPgetRd() {return (instruction >> 12) & 0xF;}
const inline uint8_t CPU::DPgetRs() { return (instruction >> 8) & 0xF; }
const inline uint8_t CPU::DPgetRm() { return instruction & 0xF;}
const inline uint8_t CPU::DPgetShift() { return (instruction >> 4) & 0xFF; }
const inline uint8_t CPU::DPgetImmed() { return instruction & 0xFF; }
const inline uint8_t CPU::DPgetRotate() { return (2 * ((instruction >> 8) & 0xF) ); }
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
inline uint32_t CPU::DPgetOp2(bool* carryFlag) {

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
//////////////////////////////////////////////////////////////////////////
//				           CYCLE CALCULATORS							//
//////////////////////////////////////////////////////////////////////////

inline int CPU::dataProcessingCycleCalculator()
{
	int cycles = 1; 

	if (!DPi() && (DPgetShift() & 0b1)) cycles+=1;

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
		pc = newAddr & 0xFFFFFFFE; //clears bit for valid 2 jumping
	}
	else // 0 = arm
	{
		T = 0;
		pc = newAddr & 0xFFFFFFFC; // sets it to a valid num for +4 jumping 
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
	if (800000) offset |= 0xFF000000;
	pc = static_cast<int32_t>(pc) + static_cast<int32_t>(offset << 2);
	return 3;
}
inline int CPU::op_BL() //TODO, make sure this is using correct logic to decide if B or BL
{
	lr = pc + pcOffset(); // this adds 2 in thumb, 4 in ARM

	int32_t offset = (instruction & 0xFFFFFF);
	// FFFFFF
	if (800000) offset |= 0xFF000000;

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
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}

inline int CPU::op_ORR()
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] & DPgetOp2(&isCarry);
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}

inline int CPU::op_EOR()
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] ^ DPgetOp2(&isCarry);
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}

// ADD, SUB, ADDC, SUBC

inline int CPU::op_ADD()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 + op2;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsAdd(res, op1, op2); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_SUB()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 - op2;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsSub(res, op1, op2); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_ADC()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr) + (uint32_t)C;
	uint32_t res = op1 + op2;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsAdd(res, op1, op2); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_SBC()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr) - (uint32_t)C;
	uint32_t res = op1 - op2;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsSub(res, op1, op2); }

	return dataProcessingCycleCalculator();
}

// reverse subtract, reverse subtract with carry

inline int CPU::op_RSB()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op2 - op1;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsSub(res, op1, op2); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_RSC()
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr) - (uint32_t)(!C);
	uint32_t res = op2 - op1;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsSub(res, op1, op2); }

	return dataProcessingCycleCalculator();
}

// test ops, for writing to flag

inline int CPU::op_TST() // AND , but we dont write the val in
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] & DPgetOp2(&isCarry);

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_TEQ() // XOR , but we dont write the val in
{
	bool isCarry = C;

	uint32_t res = reg[DPgetRn()] ^ DPgetOp2(&isCarry);

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_CMP() // SUB , but we dont write the val in
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 - op2;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsSub(res, op1, op2); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_CMN() // ADD , but we dont write the val in
{
	uint32_t op1 = reg[DPgetRn()];
	uint32_t op2 = DPgetOp2(nullptr);
	uint32_t res = op1 + op2;
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagsAdd(res, op1, op2); }

	return dataProcessingCycleCalculator();
}
// ops for writing 
inline int CPU::op_MOV()
{
	bool isCarry = C;

	uint32_t res = DPgetOp2(&isCarry);
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}
inline int CPU::op_MVN()
{
	bool isCarry = C;

	uint32_t res = ~(DPgetOp2(&isCarry));
	reg[DPgetRd()] = res;

	if (DPs()) { setFlagNZC(res, isCarry); }

	return dataProcessingCycleCalculator();
}

inline int CPU::op_BIC()
{
	bool isCarry = C;
	uint32_t res = reg[DPgetRn()] & ~(DPgetOp2(&isCarry));
	reg[DPgetRd()] = res;
	if (DPs()) { setFlagNZC(res, isCarry); }

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

	uint32_t res = (static_cast<uint64_t>(rm) * static_cast<uint64_t>(rs))&0xFFFFFFFF;
	reg[rdI] = res;

	if ((instruction >> 20) & 0b1) // set flags
	{
		N = (res >> 31) & 0b1;
		Z = (res == 0);
	}

	//shortcutting the booths algo here
	uint8_t m = 0;
	if ((rs & 0xFFFFFF00) == 0 || (rs & 0xFFFFFF00) == 0xFFFFFF00) m= 1;
	else if ((rs & 0xFFFF0000) == 0 || (rs & 0xFFFF0000) == 0xFFFF0000) m= 2;
	else if ((rs & 0xFF000000) == 0 || (rs & 0xFF000000) == 0xFF000000) m= 3;
	else m= 4;

	return m + 2;
}
inline int CPU::op_MLA()
{
	uint8_t rm = reg[instruction & 0xF];
	uint8_t rs = reg[(instruction >> 8) & 0xF];
	uint8_t rn = reg[(instruction >> 12) & 0xF];
	uint8_t rdI = (instruction >> 16 & 0xF) ;

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

}
inline int CPU::op_UMLAL()
{

}
inline int CPU::op_SMULL()
{

}
inline int CPU::op_SMLAL()
{

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
		offset = SDapplyShift(reg[(instruction & 0xF )], ((instruction >> 5) & 0x3), ((instruction >> 7) & 0x1F));
	} 



	if (p) newAddr = SDOffset(u, newAddr,offset);

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

	if (b) write8(newAddr , valToStore &0xFF);
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

	int8_t byteVal= read8(newAddr);
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
	uint8_t rnI = (instruction>>16) & 0xF;
	bool w = (instruction >> 21) & 0b1;
	bool s = (instruction >> 22) & 0b1; // 1 ~ load PSR / force user mode, else dont
	bool u = (instruction >> 23) & 0b1;
	bool p = (instruction >> 24) & 0b1;

	// so if a bit is set in registerList, it is transfered
	int numRegs = numOfRegisters(registerList);
	if (numRegs == 0) return; // nothing to transfer

	uint32_t startAddr = reg[rnI];

	if (!u) startAddr -= (numRegs * 4); //if down bit, subtract now

	bool loadPC = (registerList << 15) & 0b1; // save if were gonna load into pc
	bool useUserReg = s && !loadPC; // if s is set, we gotta use user reg EXCEPT FOR v
	bool restoreCPSR = s && loadPC; // we must restore CPSR instead if pc is also target

	uint32_t addr = startAddr; // use this for incrementing through list
	for (uint8_t i = 0; i < 16; i++)
	{
		if ((registerList << i) & 0b0) continue; // skip if not set

		if (p) addr += 4; // pre adress increment

		uint32_t val = read32(addr & ~3);

		if (!useUserReg) reg[i] = val;
		else // if useUserReg true (s and not PC) we store into user modes r13 and r14 instead of our own
		{
			if (i == 13) r13RegBank[getModeIndex(mode::User)] = val;
			if (i == 14) r14RegBank[getModeIndex(mode::User)] = val;
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

}




void op_SWP();
void op_SWPB();
void op_SWI();

void op_LDC();
void op_STC();
void op_CDP();
void op_MRC();
void op_MCR();

void op_UNKNOWN();
void op_UNASSIGNED();
void op_CONDITIONALSKIP();
void op_SINGLEDATATRANSFERUNDEFINED();
void op_DECODEFAIL();

/////////////////////////////////////////////
///             READ FUNCTIONS            ///
/////////////////////////////////////////////

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

/////////////////////////////////////////////
///             WRITE FUNCTIONS           ///
/////////////////////////////////////////////

void CPU::write8(uint16_t addr, uint8_t data)
{
	bus->write8(addr, data);
}
void CPU::write16(uint16_t addr, uint16_t data)
{
	bus->write16(addr, data);
}
void CPU::write32(uint16_t addr, uint32_t data)
{
	bus->write32(addr, data);
}