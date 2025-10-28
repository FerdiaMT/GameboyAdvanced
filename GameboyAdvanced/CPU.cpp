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

uint32_t CPU::thumbConversion(uint16_t thumbOp)
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

	curOP = decode();

	curOpCycles = execute();

	//cycle total += curOpCycles
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
			case(0b1000): return Operation::TST;break;
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

		if (DPgetRm() == 15) rmVal += 4;  // special case if we are shifting the pc

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

}

//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				             BRANCH / BRANCH LINK			            //
//////////////////////////////////////////////////////////////////////////

inline int CPU::op_B()
{

}
inline int CPU::op_BL()
{

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
inline int CPU::op_MRS()
{

}
inline int CPU::op_MSR()
{

}


//////////////////////////////////////////////////////////////////////////
//				              OPERATIONS								//
//////////////////////////////////////////////////////////////////////////
//				      MULTIPLY and MULT-ACC              				//
//////////////////////////////////////////////////////////////////////////

inline int CPU::op_MUL()
{

}
inline int CPU::op_MLA()
{

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

inline int CPU::op_LDR()
{

}
inline int CPU::op_STR()
{

}
inline int CPU::op_LDRH()
{

}
inline int CPU::op_STRH()
{

}
inline int CPU::op_LDRSB()
{

}
inline int CPU::op_LDRSH()
{

}
inline int CPU::op_LDM()
{

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
