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
		LDRH , STRH, LDRSB, LDRSH, // halfword and signed data transfer
		LDR , STR, // SINGLE DATA TRANSFER
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
	};
private:
	using OpFunction = void (CPU::*)(void);
	OpFunction data_processing_ops[16];
public:
		

		Bus* bus;
		CPU(Bus*);

		uint32_t tick();

		uint32_t reg[16];

		uint32_t& sp; // stack pointer
		uint32_t& lr; // link register
		uint32_t& pc;

		//15 resevred for pc which i declare seperately
		

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

		uint32_t spsr;
public:
		// instruction to take
		uint32_t instruction;
		//decoded operation
		Operation curOP;

private:


	uint32_t thumbConversion(uint16_t thumbOp);
	Operation decode();
	inline bool checkConditional(uint8_t cond) const;

	uint8_t read8(uint16_t addr, bool bReadOnly = false);
	uint16_t read16(uint16_t addr, bool bReadOnly = false);
	uint32_t read32(uint16_t addr, bool bReadOnly = false);

};

