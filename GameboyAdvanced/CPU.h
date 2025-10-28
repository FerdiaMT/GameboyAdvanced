#pragma once
#include "Bus.h"
#include <cstdint>
#include <map>
#include <unordered_map>
class CPU
{


public:
	enum class Operation
	{
		// Data processing
		AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC,
		TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN,

		MRS,MSR, // both used for PSR transfer

		// Load/Store
		LDR, STR, // SINGLE DATA TRANSFER
		LDRH , STRH, LDRSB, LDRSH, // halfword and signed data transfer
		LDM, STM,

		// Branch
		B, BL, BX,

		// Multiply
		MUL, MLA, UMULL, UMLAL, SMULL, SMLAL,

		// Special
		SWP, SWPB, SWI,

		//coprocessor

		CDP,LDC,STC,MRC,MCR,

		//general break
		UNKNOWN,
		//more specific breaks
		UNASSIGNED, // this is only given at the start
		CONDITIONALSKIP, // this is only given on conditionalFail
		SINGLEDATATRANSFERUNDEFINED, //theres a single instruction in decode thats the same as executeSingleDataTransfer, but tells is if bit 4 is set, it becomes undefined
		DECODEFAIL , // if it reaches the end of the decoding list and never gets anything it returns this
		//should be some realistically to tell me why it failed

		COUNT // this is just so i have a way of counting how much enums i have in case of future new enum declared
	};

	const char* opcodeToString(Operation op)
	{
		switch (op)
		{
			case Operation::AND:  return "AND";
			case Operation::EOR:  return "EOR";
			case Operation::SUB:  return "SUB";
			case Operation::RSB:  return "RSB";
			case Operation::ADD:  return "ADD";
			case Operation::ADC:  return "ADC";
			case Operation::SBC:  return "SBC";
			case Operation::RSC:  return "RSC";
			case Operation::TST:  return "TST";
			case Operation::TEQ:  return "TEQ";
			case Operation::CMP:  return "CMP";
			case Operation::CMN:  return "CMN";
			case Operation::ORR:  return "ORR";
			case Operation::MOV:  return "MOV";
			case Operation::BIC:  return "BIC";
			case Operation::MVN:  return "MVN";
			case Operation::MRS:  return "MRS";
			case Operation::MSR:  return "MSR";
			case Operation::LDR:  return "LDR";
			case Operation::STR:  return "STR";
			case Operation::LDRH: return "LDRH";
			case Operation::STRH: return "STRH";
			case Operation::LDRSB: return "LDRSB";
			case Operation::LDRSH: return "LDRSH";
			case Operation::LDM:  return "LDM";
			case Operation::STM:  return "STM";
			case Operation::B:    return "B";
			case Operation::BL:   return "BL";
			case Operation::BX:   return "BX";
			case Operation::MUL:  return "MUL";
			case Operation::MLA:  return "MLA";
			case Operation::UMULL: return "UMULL";
			case Operation::UMLAL: return "UMLAL";
			case Operation::SMULL: return "SMULL";
			case Operation::SMLAL: return "SMLAL";
			case Operation::SWP:  return "SWP";
			case Operation::SWPB: return "SWPB";
			case Operation::SWI:  return "SWI";
			case Operation::CDP:  return "CDP";
			case Operation::LDC:  return "LDC";
			case Operation::STC:  return "STC";
			case Operation::MRC:  return "MRC";
			case Operation::MCR:  return "MCR";
			case Operation::UNKNOWN: return "UNKNOWN";
			case Operation::UNASSIGNED: return "UNASSIGNED";
			case Operation::CONDITIONALSKIP: return "CONDITIONALSKIP";
			case Operation::SINGLEDATATRANSFERUNDEFINED: return "SINGLEDATATRANSFERUNDEFINED";
			case Operation::DECODEFAIL: return "DECODEFAIL";
			default: return "INVALID_OPCODE";
		}
	}
public:

	using OpFunction = int (CPU::*)(void);
	OpFunction op_functions[static_cast<int>(Operation::COUNT)];

public:
		
		Bus* bus;
		CPU(Bus*);
		void reset();

		void initializeOpFunctions(); // this is for initing the list of enums to funcs
		

		uint16_t curOpCycles; // this is defaulted to 0 every time
		int cycleTotal; // this is how we find out how many cycles have passed

		uint32_t tick();
		Operation decode(uint32_t passedIns);
		int execute();

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
			{ //declared from bit 0 onwards
				bool M0;
				bool M1;
				bool M2;
				bool M3;
				bool M4;
				bool T;
				bool F;
				bool I;//7
				//JUNK FOR 8 TO 27
				uint16_t RESERVED;//8 - 23
				bool RESERVED1, RESERVED2, RESERVED3, RESERVED4;//24,25,26,27
				bool V; // overflow flag
				bool C; // carry flag
				bool Z; // zero flag
				bool N; // sign flag

			};
			uint32_t CPSR;
		};
		//SPSR



public:


	uint32_t thumbConversion(uint16_t thumbOp);
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



	mode curMode= mode::Supervisor; // curMode should default to Supervisor

	

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
	uint32_t r13RegBank[6];  // individual SP for everone except usr/sys which share
	uint32_t r14RegBank[6]; // individual LR for everone except usr/sys which share
	uint32_t spsrBank[5]; // individual LR for everone except usr/sys have 0



public:
	inline int op_AND();
	inline int op_EOR();
	inline int op_SUB();
	inline int op_RSB();
	inline int op_ADD();
	inline int op_ADC();
	inline int op_SBC();
	inline int op_RSC();
	inline int op_TST();
	inline int op_TEQ();
	inline int op_CMP();
	inline int op_CMN();
	inline int op_ORR();
	inline int op_MOV();
	inline int op_BIC();
	inline int op_MVN();

	inline int op_MRS();
	inline int op_MSR();

	inline int op_LDR();
	inline int op_STR();
	inline int op_LDM();
	inline int op_STM();

	inline int op_B();
	inline int op_BL();
	inline int op_BX();

	inline int op_MUL();
	inline int op_MLA();
	inline int op_UMULL();
	inline int op_UMLAL();
	inline int op_SMULL();
	inline int op_SMLAL();

	inline int op_LDRH();
	inline int op_STRH();
	inline int op_LDRSB();
	inline int op_LDRSH();

	inline int op_SWP();
	inline int op_SWPB();
	inline int op_SWI();

	inline int op_LDC();
	inline int op_STC();
	inline int op_CDP();
	inline int op_MRC();
	inline int op_MCR();

	inline int op_UNKNOWN();
	inline int op_UNASSIGNED();
	inline int op_CONDITIONALSKIP();
	inline int op_SINGLEDATATRANSFERUNDEFINED();
	inline int op_DECODEFAIL();

	public: // helper for data rpocessing

		inline void writeALUResult(uint8_t rdI, uint32_t result, bool s);

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
};

