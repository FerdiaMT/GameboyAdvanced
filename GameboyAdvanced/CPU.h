#pragma once
#include "Bus.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>

class CPU
{


public:
	enum class Operation
	{
		// Data processing
		AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC,
		TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN,

		MRS, MSR, // both used for PSR transfer

		// Load/Store
		LDR, STR, // SINGLE DATA TRANSFER
		LDRH, STRH, LDRSB, LDRSH, // halfword and signed data transfer
		LDM, STM,

		// Branch
		B, BL, BX,

		// Multiply
		MUL, MLA, UMULL, UMLAL, SMULL, SMLAL,

		// Special
		SWP, SWPB, SWI,

		//coprocessor

		CDP, LDC, STC, MRC, MCR,

		//general break
		UNKNOWN,
		//more specific breaks
		UNASSIGNED, // this is only given at the start
		CONDITIONALSKIP, // this is only given on conditionalFail
		SINGLEDATATRANSFERUNDEFINED, //theres a single instruction in decode thats the same as executeSingleDataTransfer, but tells is if bit 4 is set, it becomes undefined
		DECODEFAIL, // if it reaches the end of the decoding list and never gets anything it returns this
		//should be some realistically to tell me why it failed

		COUNT // this is just so i have a way of counting how much enums i have in case of future new enum declared
	};

	enum class thumbOperation
	{
		THUMB_MOV_IMM,
		THUMB_ADD_REG,
		THUMB_ADD_IMM,
		THUMB_ADD_IMM3,
		THUMB_SUB_REG,
		THUMB_SUB_IMM,
		THUMB_SUB_IMM3,
		THUMB_CMP_IMM,
		THUMB_LSL_IMM,
		THUMB_LSR_IMM,
		THUMB_ASR_IMM,
		THUMB_AND_REG,
		THUMB_EOR_REG,
		THUMB_LSL_REG,
		THUMB_LSR_REG,
		THUMB_ASR_REG,
		THUMB_ADC_REG,
		THUMB_SBC_REG,
		THUMB_ROR_REG,
		THUMB_TST_REG,
		THUMB_NEG_REG,
		THUMB_CMP_REG,
		THUMB_CMN_REG,
		THUMB_ORR_REG,
		THUMB_MUL_REG,
		THUMB_BIC_REG,
		THUMB_MVN_REG,
		THUMB_ADD_HI,
		THUMB_CMP_HI,
		THUMB_MOV_HI,
		THUMB_BX,
		THUMB_BLX_REG,
		THUMB_LDR_PC,
		THUMB_LDR_REG,
		THUMB_STR_REG,
		THUMB_LDRB_REG,
		THUMB_STRB_REG,
		THUMB_LDRH_REG,
		THUMB_STRH_REG,
		THUMB_LDRSB_REG,
		THUMB_LDRSH_REG,

		THUMB_LDR_IMM,
		THUMB_STR_IMM,
		THUMB_LDRB_IMM,
		THUMB_STRB_IMM,
		THUMB_LDRH_IMM,
		THUMB_STRH_IMM,
		THUMB_LDR_SP,
		THUMB_STR_SP,
		THUMB_ADD_PC,
		THUMB_ADD_SP,
		THUMB_ADD_SP_IMM,
		THUMB_PUSH,
		THUMB_POP,
		THUMB_STMIA,
		THUMB_LDMIA,
		THUMB_B_COND,
		THUMB_B,
		THUMB_BL_PREFIX,
		THUMB_BL_SUFFIX,
		THUMB_SWI,
		THUMB_UNDEFINED,




		COUNT,
	};

	enum class armOperation
	{
		ARM_ADD,
		ARM_SUB,
		ARM_RSB,
		ARM_ADC,
		ARM_SBC,
		ARM_RSC,


		ARM_AND,
		ARM_EOR,
		ARM_ORR,
		ARM_BIC,


		ARM_TST,
		ARM_TEQ,
		ARM_CMP,
		ARM_CMN,

		ARM_MOV,
		ARM_MVN,

		ARM_MUL,
		ARM_MLA,
		ARM_UMULL,
		ARM_UMLAL,
		ARM_SMULL,
		ARM_SMLAL,

		ARM_LDR,
		ARM_STR,
		ARM_LDRH,
		ARM_STRH,
		ARM_LDRSB,
		ARM_LDRSH,

		ARM_LDM,
		ARM_STM,


		ARM_B,
		ARM_BL,
		ARM_BX,

		ARM_MRS,
		ARM_MSR,
		ARM_SWP,


		ARM_SWI,


		ARM_CDP,
		ARM_LDC,
		ARM_STC,
		ARM_MRC,
		ARM_MCR,

		ARM_UNDEFINED,

		COUNT
	};

	struct thumbInstr
	{
		thumbOperation type;

		uint8_t rd;
		uint8_t rs;
		uint8_t rn;

		uint32_t imm;

		uint8_t cond;

		bool h1;         // hi register f1
		bool h2;         // hi register f2
	};

	struct armInstr
	{
		armOperation type;

		uint8_t rd;
		uint8_t rn;
		uint8_t rs;
		uint8_t rm;

		uint32_t imm;
		uint8_t rotate;

		uint8_t cond;

		bool S;          // set condition codes
		bool L;          // 1 is load, 0 is store
		bool W;          // write back
		bool P;          // 1 is pre index, 0 is post
		bool U;          // 1 is add offs, 0 is sub
		bool B;          // 1 for byte, 0 for word
		bool H;          // is halfword or byte
		bool I;          // immed operand

		// shift info
		uint8_t shift_type;    // 00=LSL, 01=LSR, 10=ASR, 11=ROR
		uint8_t shift_amount;  // immed shift amount (0-31)
		uint8_t shift_reg;     // reg containing shift amount
		bool shift_by_reg;     // true if shift amount in register


		uint16_t reg_list;// reg list (for load multiple etc)
	};

	armInstr curArmInstr;

public: // FUNCTION ARRAYS

	using OpAFunction = int (CPU::*)(armInstr);
	OpAFunction opA_functions[static_cast<int>(Operation::COUNT)];

	using OpTFunction = int (CPU::*)(thumbInstr);
	OpTFunction opT_functions[static_cast<int>(thumbOperation::COUNT)];

public:

	Bus* bus;
	CPU(Bus*);
	void reset();

	void initializeOpFunctions(); // this is for initing the list of enums to funcs

	uint16_t curOpCycles; // this is defaulted to 0 every time
	int cycleTotal; // this is how we find out how many cycles have passed

	uint32_t tick();
	//Operation decode(uint32_t passedIns);
	armInstr decodeArm(uint32_t instr);
	int armExecute(armInstr instr);

	uint32_t reg[16];

	uint32_t& sp; // stack pointer ~ points to 13
	uint32_t& lr; // link register ~ points to 14
	uint32_t& pc; //~points to 14

	// instruction to take
	uint32_t instruction;
	//decoded operation
	Operation curOP;

	//current program status registers
	union
	{
		struct
		{
			uint32_t M0 : 1;
			uint32_t M1 : 1;
			uint32_t M2 : 1;
			uint32_t M3 : 1;
			uint32_t M4 : 1;
			uint32_t T : 1;
			uint32_t F : 1;
			uint32_t I : 1;

			uint32_t RESERVED : 20;
			uint32_t V : 1;
			uint32_t C : 1;
			uint32_t Z : 1;
			uint32_t N : 1;
		};
		uint32_t CPSR;
	};

	//SPSR



public:

	const char* CPSRtoString();
	std::string CPSRtoStringPASSED(uint32_t base, uint32_t final, uint32_t passed);
	uint32_t ThumbToARM(uint16_t thumbInstr, uint32_t pc, uint16_t nextThumbInstr);
	inline bool checkConditional(uint8_t cond) const;


	const inline uint8_t pcOffset();

	uint8_t read8(uint32_t addr, bool bReadOnly = false);
	uint16_t read16(uint32_t addr, bool bReadOnly = false);
	uint32_t read32(uint32_t addr, bool bReadOnly = false);

	void write8(uint32_t addr, uint8_t data);
	void write16(uint32_t addr, uint16_t data);
	void write32(uint32_t addr, uint32_t data);

public:
	enum class mode : uint8_t
	{
		User = 0x10,
		FIQ = 0x11,
		IRQ = 0x12,
		Supervisor = 0x13,
		Abort = 0x17,
		Undefined = 0x1B,
		System = 0x1F
	};

public:
	//OPS FOR MODE SWITCHING / EXCEPTION HANDLING



	mode curMode = mode::Supervisor; // curMode should default to Supervisor



	bool isPrivilegedMode(); // used to quickly tell were not in user mode
	uint8_t getModeIndex(CPU::mode mode); // used for register saving

	//reg banking
	void bankRegisters(CPU::mode mode); // save reg val to bank
	void unbankRegisters(CPU::mode mode); // load reg vals from bank

	void switchMode(CPU::mode newMode); // main function used for mode switching, calls bank and unbank register etc
	void saveIntoSpsr(uint8_t index);

	// excpetion handling
	void enterException(CPU::mode newMode, uint32_t vectorAddr, uint32_t returnAddr);
	void returnFromException();

	//SPSR helpers
	uint32_t getSPSR();
	void setSPSR(uint32_t value);

	//cpsr helper

	void writeCPSR(uint32_t value);
	mode CPSRbitToMode(uint8_t modeBits);

	// bank system so when we swap modes, we can store old modes inhere 
	uint32_t r8FIQ[5];   // 8 9 10 11 12 registers stored for just fiq
	uint32_t r8User[5]; // used for swaping back from fiq

	uint32_t r13RegBank[6];  // individual SP for everone except usr/sys which share
	uint32_t r14RegBank[6]; // individual LR for everone except usr/sys which share
	uint32_t spsrBank[5]; // individual LR for everone except usr/sys have 0

public:

	//////////////////////////////////////////////////////////////////
	//							THUMB STUFF							//
	//////////////////////////////////////////////////////////////////




	thumbInstr curThumbInstr;

	CPU::thumbInstr debugDecodedInstr(); //used to create a struct full of nulls , usefull for printig debugs

	thumbInstr decodeThumb(uint16_t instruction); // this returns a thumbInstr struct
	int thumbExecute(struct thumbInstr);

	//THUMB HELPERS
	inline void updateFlagsNZCV_Add(uint32_t result, uint32_t op1, uint32_t op2);

	inline void updateFlagsNZCV_Sub(uint32_t result, uint32_t op1, uint32_t op2);


public:

	//////////////////////////////////////////////////////////////////
	//							EVERY THUMB FUNC				    //
	//////////////////////////////////////////////////////////////////


	inline int opT_MOV_IMM(thumbInstr instr);
	inline int opT_ADD_REG(thumbInstr instr);
	inline int opT_ADD_IMM(thumbInstr instr);
	inline int opT_ADD_IMM3(thumbInstr instr);
	inline int opT_SUB_REG(thumbInstr instr);
	inline int opT_SUB_IMM(thumbInstr instr);
	inline int opT_SUB_IMM3(thumbInstr instr);
	inline int opT_CMP_IMM(thumbInstr instr);
	inline int opT_LSL_IMM(thumbInstr instr);
	inline int opT_LSR_IMM(thumbInstr instr);
	inline int opT_ASR_IMM(thumbInstr instr);
	inline int opT_AND_REG(thumbInstr instr);
	inline int opT_EOR_REG(thumbInstr instr);
	inline int opT_LSL_REG(thumbInstr instr);
	inline int opT_LSR_REG(thumbInstr instr);
	inline int opT_ASR_REG(thumbInstr instr);
	inline int opT_ADC_REG(thumbInstr instr);
	inline int opT_SBC_REG(thumbInstr instr);
	inline int opT_ROR_REG(thumbInstr instr);
	inline int opT_TST_REG(thumbInstr instr);
	inline int opT_NEG_REG(thumbInstr instr);
	inline int opT_CMP_REG(thumbInstr instr);
	inline int opT_CMN_REG(thumbInstr instr);
	inline int opT_ORR_REG(thumbInstr instr);
	inline int opT_MUL_REG(thumbInstr instr);
	inline int opT_BIC_REG(thumbInstr instr);
	inline int opT_MVN_REG(thumbInstr instr);
	inline int opT_ADD_HI(thumbInstr instr);
	inline int opT_CMP_HI(thumbInstr instr);
	inline int opT_MOV_HI(thumbInstr instr);
	inline int opT_BX(thumbInstr instr);
	inline int opT_BLX_REG(thumbInstr instr);
	inline int opT_LDR_PC(thumbInstr instr);
	inline int opT_LDR_REG(thumbInstr instr);
	inline int opT_STR_REG(thumbInstr instr);
	inline int opT_LDRB_REG(thumbInstr instr);
	inline int opT_STRB_REG(thumbInstr instr);
	inline int opT_LDRH_REG(thumbInstr instr);
	inline int opT_STRH_REG(thumbInstr instr);
	inline int opT_LDRSB_REG(thumbInstr instr);
	inline int opT_LDRSH_REG(thumbInstr instr);

	inline int opT_LDR_IMM(thumbInstr instr);
	inline int opT_STR_IMM(thumbInstr instr);
	inline int opT_LDRB_IMM(thumbInstr instr);
	inline int opT_STRB_IMM(thumbInstr instr);
	inline int opT_LDRH_IMM(thumbInstr instr);
	inline int opT_STRH_IMM(thumbInstr instr);
	inline int opT_LDR_SP(thumbInstr instr);
	inline int opT_STR_SP(thumbInstr instr);
	inline int opT_ADD_PC(thumbInstr instr);
	inline int opT_ADD_SP(thumbInstr instr);
	inline int opT_ADD_SP_IMM(thumbInstr instr);
	inline int opT_PUSH(thumbInstr instr);
	inline int opT_POP(thumbInstr instr);
	inline int opT_STMIA(thumbInstr instr);
	inline int opT_LDMIA(thumbInstr instr);
	inline int opT_B_COND(thumbInstr instr);
	inline int opT_B(thumbInstr instr);
	inline int opT_BL_PREFIX(thumbInstr instr);
	inline int opT_BL_SUFFIX(thumbInstr instr);
	inline int opT_SWI(thumbInstr instr);
	inline int opT_UNDEFINED(thumbInstr instr);
public:

	//////////////////////////////////////////////////////////////////
	//							EVERY ARM FUNC					    //
	//////////////////////////////////////////////////////////////////

	inline int opA_AND(armInstr instr);
	inline int opA_EOR(armInstr instr);
	inline int opA_SUB(armInstr instr);
	inline int opA_RSB(armInstr instr);
	inline int opA_ADD(armInstr instr);
	inline int opA_ADC(armInstr instr);
	inline int opA_SBC(armInstr instr);
	inline int opA_RSC(armInstr instr);
	inline int opA_TST(armInstr instr);
	inline int opA_TEQ(armInstr instr);
	inline int opA_CMP(armInstr instr);
	inline int opA_CMN(armInstr instr);
	inline int opA_ORR(armInstr instr);
	inline int opA_MOV(armInstr instr);
	inline int opA_BIC(armInstr instr);
	inline int opA_MVN(armInstr instr);
	inline int opA_MRS(armInstr instr);
	inline int opA_MSR(armInstr instr);
	inline int opA_LDR(armInstr instr);
	inline int opA_STR(armInstr instr);
	inline int opA_LDRH(armInstr instr);
	inline int opA_STRH(armInstr instr);
	inline int opA_LDRSB(armInstr instr);
	inline int opA_LDRSH(armInstr instr);
	inline int opA_LDM(armInstr instr);
	inline int opA_STM(armInstr instr);
	inline int opA_B(armInstr instr);
	inline int opA_BL(armInstr instr);
	inline int opA_BX(armInstr instr);
	inline int opA_MUL(armInstr instr);
	inline int opA_MLA(armInstr instr);
	inline int opA_UMULL(armInstr instr);
	inline int opA_UMLAL(armInstr instr);
	inline int opA_SMULL(armInstr instr);
	inline int opA_SMLAL(armInstr instr);
	inline int opA_SWP(armInstr instr);
	inline int opA_SWI(armInstr instr);
	inline int opA_CDP(armInstr instr);
	inline int opA_LDC(armInstr instr);
	inline int opA_STC(armInstr instr);
	inline int opA_MRC(armInstr instr);
	inline int opA_MCR(armInstr instr);
	inline int opA_UNDEFINED(armInstr instr);

public: // helper for data rpocessing

	inline void writeALUResult(uint8_t rdI, uint32_t result, bool s);

	// new arm ops
	inline uint32_t getArmOp2(armInstr instr, bool* carryOut);
	inline uint32_t getArmOffset(armInstr instr);

	const inline uint8_t DPgetRn();
	const inline uint8_t DPgetRd();
	const inline uint8_t DPgetRs();
	const inline uint8_t DPgetRm();

	const inline uint8_t DPgetShift();

	const inline uint8_t DPgetImmed();
	const inline uint8_t DPgetRotate();

	const inline uint8_t DPgetShiftAmount(uint8_t shift);

	const inline bool DPs();
	const inline bool DPi();

	inline uint32_t DPgetOp2(bool* carryFlag);

	// cycle calculation helpers

	inline int dataProcessingCycleCalculator();

	// shift helpers

	inline uint32_t DPshiftLSL(uint32_t value, uint8_t shift_amount, bool* carry_out);
	inline uint32_t DPshiftLSR(uint32_t value, uint8_t shift_amount, bool* carry_out);
	inline uint32_t DPshiftASR(uint32_t value, uint8_t shift_amount, bool* carry_out);
	inline uint32_t DPshiftROR(uint32_t value, uint8_t shift_amount, bool* carry_out);

	//shift for memory
	inline uint32_t SDapplyShift(uint32_t rmVal, uint8_t type, uint8_t amount); // singledata apply shift

	//flag related helper

	inline void setFlagNZC(uint32_t res, bool isCarry); // no addition or subtraction
	inline void setFlagsAdd(uint32_t res, uint32_t op1, uint32_t op2);// ADD CHECK
	inline void setFlagsSub(uint32_t res, uint32_t op1, uint32_t op2); // SUB CHECK
	inline void setNZ(uint32_t res); // TEST CHECK

	//debugger help

	std::string thumbToStr(CPU::thumbInstr& instr);
	std::string armToStr(CPU::armInstr& instr);

	const char* opcodeToString(Operation op)
	{
		switch (op)
		{
		case Operation::AND:  return  "AND  ";
		case Operation::EOR:  return  "EOR  ";
		case Operation::SUB:  return  "SUB  ";
		case Operation::RSB:  return  "RSB  ";
		case Operation::ADD:  return  "ADD  ";
		case Operation::ADC:  return  "ADC  ";
		case Operation::SBC:  return  "SBC  ";
		case Operation::RSC:  return  "RSC  ";
		case Operation::TST:  return  "TST  ";
		case Operation::TEQ:  return  "TEQ  ";
		case Operation::CMP:  return  "CMP  ";
		case Operation::CMN:  return  "CMN  ";
		case Operation::ORR:  return  "ORR  ";
		case Operation::MOV:  return  "MOV  ";
		case Operation::BIC:  return  "BIC  ";
		case Operation::MVN:  return  "MVN  ";
		case Operation::MRS:  return  "MRS  ";
		case Operation::MSR:  return  "MSR  ";
		case Operation::LDR:  return  "LDR  ";
		case Operation::STR:  return  "STR  ";
		case Operation::LDRH: return  "LDRH ";
		case Operation::STRH: return  "STRH ";
		case Operation::LDRSB: return "LDRSB";
		case Operation::LDRSH: return "LDRSH";
		case Operation::LDM:  return  "LDM  ";
		case Operation::STM:  return  "STM  ";
		case Operation::B:    return  "B    ";
		case Operation::BL:   return  "BL   ";
		case Operation::BX:   return  "BX   ";
		case Operation::MUL:  return  "MUL  ";
		case Operation::MLA:  return  "MLA  ";
		case Operation::UMULL: return "UMULL";
		case Operation::UMLAL: return "UMLAL";
		case Operation::SMULL: return "SMULL";
		case Operation::SMLAL: return "SMLAL";
		case Operation::SWP:  return  "SWP  ";
		case Operation::SWPB: return  "SWPB ";
		case Operation::SWI:  return  "SWI  ";
		case Operation::CDP:  return  "CDP  ";
		case Operation::LDC:  return  "LDC  ";
		case Operation::STC:  return  "STC  ";
		case Operation::MRC:  return  "MRC  ";
		case Operation::MCR:  return  "MCR  ";
		case Operation::UNKNOWN:return"UNKWN";
		case Operation::UNASSIGNED: return "UNSND";
		case Operation::CONDITIONALSKIP: return "CNDSP";
		case Operation::SINGLEDATATRANSFERUNDEFINED: return "SDTUND";
		case Operation::DECODEFAIL: return "DCDFL";
		default: return "OPINVL";
		}
	}

	void runThumbTests();
	void runThumbTestsEXTRADEBUG();
};

