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
private:

	using OpFunction = void (CPU::*)(void);
	OpFunction op_functions[static_cast<int>(Operation::COUNT)];

public:
		
		Bus* bus;
		CPU(Bus*);
		void initializeOpFunctions(); // this is for initing the list of enums to funcs

		uint32_t tick();

		uint32_t reg[16];

		uint32_t& sp; // stack pointer
		uint32_t& lr; // link register
		uint32_t& pc;

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



private:


	uint32_t thumbConversion(uint16_t thumbOp);
	Operation decode();
	void execute();
	inline bool checkConditional(uint8_t cond) const;

	

	uint8_t read8(uint16_t addr, bool bReadOnly = false);
	uint16_t read16(uint16_t addr, bool bReadOnly = false);
	uint32_t read32(uint16_t addr, bool bReadOnly = false);

public:
	void op_AND();
	void op_EOR();
	void op_SUB();
	void op_RSB();
	void op_ADD();
	void op_ADC();
	void op_SBC();
	void op_RSC();
	void op_TST();
	void op_TEQ();
	void op_CMP();
	void op_CMN();
	void op_ORR();
	void op_MOV();
	void op_BIC();
	void op_MVN();

	void op_LDR();
	void op_STR();
	void op_LDRH();
	void op_STRH();
	void op_LDRSB();
	void op_LDRSH();
	void op_LDM();
	void op_STM();

	void op_B();
	void op_BL();
	void op_BX();

	void op_MUL();
	void op_MLA();
	void op_UMULL();
	void op_UMLAL();
	void op_SMULL();
	void op_SMLAL();

	void op_LDRH();
	void op_STRH();
	void op_LDRSB();
	void op_LDRSH();

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

	private: // helper for data rpocessing
		const inline uint8_t DPgetRn();

		const inline uint8_t DPgetRd();

		const inline uint8_t DPgetRm();
		const inline uint8_t DPgetShift();

		const inline uint8_t DPgetImmed();
		const inline uint8_t DPgetRotate();

		const inline bool DPs();
		const inline bool DPi();


};

