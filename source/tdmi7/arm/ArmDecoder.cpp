#include "tdmi7/CPU.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <vector>
#include "tdmi7/Decoder.h"

namespace tdmi7
{
CPU::armInstr Decoder::decodeArm(uint32_t const instr) // this returns a CPU::thumbInstr struct
{
	CPU::armInstr decodedInstr = {}; // creates empty struct for us to fill
	decodedInstr.type = CPU::armOperation::ARM_UNDEFINED;
	decodedInstr.raw = instr;
	decodedInstr.cond = (instr >> 28) & 0xF;

	switch ((instr >> 25) & 0x7)
	{
	case 0b000:  // Data processing, multiply, misc
	{
		// Branch and Exchange: xxxx 0001 0010 1111 1111 1111 0001 xxxx
		if ((instr & 0x0FFFFFF0) == 0x012FFF10)
		{
			decodedInstr.type = CPU::armOperation::ARM_BX;
			decodedInstr.rm = instr & 0xF;
			return decodedInstr;
		}

		// Swap: xxxx 0001 0B00 nnnn dddd 0000 1001 mmmm
		if ((instr & 0x0FB00FF0) == 0x01000090)
		{
			decodedInstr.type = CPU::armOperation::ARM_SWP;
			decodedInstr.B = (instr >> 22) & 1;
			decodedInstr.rn = (instr >> 16) & 0xF;
			decodedInstr.rd = (instr >> 12) & 0xF;
			decodedInstr.rm = instr & 0xF;
			return decodedInstr;
		}
		// Multiply Long: xxxx 0000 1UAS dddd nnnn ssss 1001 mmmm
		if ((instr & 0x0F8000F0) == 0x00800090)
		{
			uint8_t op = (instr >> 21) & 0x3;
			switch (op)
			{
			case 0b00: decodedInstr.type = CPU::armOperation::ARM_UMULL; break;
			case 0b01: decodedInstr.type = CPU::armOperation::ARM_UMLAL; break;
			case 0b10: decodedInstr.type = CPU::armOperation::ARM_SMULL; break;
			case 0b11: decodedInstr.type = CPU::armOperation::ARM_SMLAL; break;
			}
			decodedInstr.S = (instr >> 20) & 1;
			decodedInstr.rd = (instr >> 16) & 0xF;  // RdHi
			decodedInstr.rn = (instr >> 12) & 0xF;  // RdLo
			decodedInstr.rs = (instr >> 8) & 0xF;
			decodedInstr.rm = instr & 0xF;
			return decodedInstr;
		}
		// Multiply: xxxx 0000 00AS dddd nnnn ssss 1001 mmmm
		if ((instr & 0x0FC000F0) == 0x00000090)
		{
			decodedInstr.type = ((instr >> 21) & 1) ? CPU::armOperation::ARM_MLA : CPU::armOperation::ARM_MUL;
			decodedInstr.S = (instr >> 20) & 1;
			decodedInstr.rd = (instr >> 16) & 0xF;
			decodedInstr.rn = (instr >> 12) & 0xF;  // Accumulate register for MLA
			decodedInstr.rs = (instr >> 8) & 0xF;
			decodedInstr.rm = instr & 0xF;
			return decodedInstr;
		}
		// Halfword Transfer: xxxx 000P U0WL nnnn dddd oooo 1SH1 mmmm
		if ((instr & 0x0E000090) == 0x00000090)
		{
			uint8_t SH = (instr >> 5) & 0x3;
			decodedInstr.L = (instr >> 20) & 1;

			if (decodedInstr.L) 
			{
				if (SH == 0b01) decodedInstr.type = CPU::armOperation::ARM_LDRH;
				else if (SH == 0b10) decodedInstr.type = CPU::armOperation::ARM_LDRSB;
				else if (SH == 0b11) decodedInstr.type = CPU::armOperation::ARM_LDRSH;
				else decodedInstr.type = CPU::armOperation::ARM_UNDEFINED;
			}
			else  
			{
				if (SH == 0b01) decodedInstr.type = CPU::armOperation::ARM_STRH;
				else decodedInstr.type = CPU::armOperation::ARM_UNDEFINED;
			}

			decodedInstr.P = (instr >> 24) & 1;
			decodedInstr.U = (instr >> 23) & 1;
			decodedInstr.W = (instr >> 21) & 1;
			decodedInstr.rn = (instr >> 16) & 0xF;
			decodedInstr.rd = (instr >> 12) & 0xF;
			decodedInstr.rm = instr & 0xF;

			if (((instr >> 22) & 1) == 1)  // Immed
			{
				decodedInstr.imm = ((instr >> 4) & 0xF0) | (instr & 0xF);
				decodedInstr.I = true;
			}
			else  // Reg offset
			{
				decodedInstr.I = false;
			}

			return decodedInstr;
		}

		// MRS: xxxx 0001 0R00 1111 dddd 0000 0000 0000
		if ((instr & 0x0FBF0FFF) == 0x010F0000)
		{
			decodedInstr.type = CPU::armOperation::ARM_MRS;
			decodedInstr.rd = (instr >> 12) & 0xF;
			decodedInstr.B = (instr >> 22) & 1;  // Use B flag to indicate SPSR vs CPSR
			return decodedInstr;
		}

		// MSR: xxxx 0001 0R10 1001 1111 0000 0000 mmmm (register)
		//      xxxx 0011 0R10 1000 1111 rrrr iiii iiii (immediate)
		if ((instr & 0x0FB0FFF0) == 0x0120F000 || (instr & 0x0FB0F000) == 0x0320F000)
		{
			decodedInstr.type = CPU::armOperation::ARM_MSR;
			decodedInstr.B = (instr >> 22) & 1;  // SPSR vs CPSR
			if ((instr >> 25) & 1)  // Immediate
			{
				decodedInstr.I = true;
				decodedInstr.imm = instr & 0xFF;
				decodedInstr.rotate = (instr >> 8) & 0xF;
			}
			else 
			{
				decodedInstr.I = false;
				decodedInstr.rm = instr & 0xF;
			}
			return decodedInstr;
		}

		// Data Processing: xxxx 000a aaaa Snnn nddd diii iiii iiii (register)
		//                  xxxx 001a aaaa Snnn nddd drrrr iiii iiii (immediate)
		decodedInstr.S = (instr >> 20) & 1;
		decodedInstr.rn = (instr >> 16) & 0xF;
		decodedInstr.rd = (instr >> 12) & 0xF;

		uint8_t opcode = (instr >> 21) & 0xF;
		switch (opcode)
		{
		case 0x0: decodedInstr.type = CPU::armOperation::ARM_AND; break;
		case 0x1: decodedInstr.type = CPU::armOperation::ARM_EOR; break;
		case 0x2: decodedInstr.type = CPU::armOperation::ARM_SUB; break;
		case 0x3: decodedInstr.type = CPU::armOperation::ARM_RSB; break;
		case 0x4: decodedInstr.type = CPU::armOperation::ARM_ADD; break;
		case 0x5: decodedInstr.type = CPU::armOperation::ARM_ADC; break;
		case 0x6: decodedInstr.type = CPU::armOperation::ARM_SBC; break;
		case 0x7: decodedInstr.type = CPU::armOperation::ARM_RSC; break;
		case 0x8: decodedInstr.type = CPU::armOperation::ARM_TST; break;
		case 0x9: decodedInstr.type = CPU::armOperation::ARM_TEQ; break;
		case 0xA: decodedInstr.type = CPU::armOperation::ARM_CMP; break;
		case 0xB: decodedInstr.type = CPU::armOperation::ARM_CMN; break;
		case 0xC: decodedInstr.type = CPU::armOperation::ARM_ORR; break;
		case 0xD: decodedInstr.type = CPU::armOperation::ARM_MOV; break;
		case 0xE: decodedInstr.type = CPU::armOperation::ARM_BIC; break;
		case 0xF: decodedInstr.type = CPU::armOperation::ARM_MVN; break;
		}

		if ((instr >> 25) & 1)
		{
			decodedInstr.I = true;
			decodedInstr.imm = instr & 0xFF;
			decodedInstr.rotate = (instr >> 8) & 0xF;
		}
		else
		{
			decodedInstr.I = false;
			decodedInstr.rm = instr & 0xF;
			decodedInstr.shift_type = (instr >> 5) & 0x3;

			if ((instr >> 4) & 1)  // Shift by register
			{
				decodedInstr.shift_by_reg = true;
				decodedInstr.shift_reg = (instr >> 8) & 0xF;
			}
			else  // Shift by immediate
			{
				decodedInstr.shift_by_reg = false;
				decodedInstr.shift_amount = (instr >> 7) & 0x1F;
			}
		}

		return decodedInstr;
	}

	case 0b001:  
	{
		if ((instr & 0x0FBF0FFF) == 0x010F0000)
		{
			decodedInstr.type = CPU::armOperation::ARM_MRS;
			decodedInstr.rd = (instr >> 12) & 0xF;
			decodedInstr.B = (instr >> 22) & 1;
			return decodedInstr;
		}

		if ((instr & 0x0FB0F000) == 0x0320F000)
		{
			decodedInstr.type = CPU::armOperation::ARM_MSR;
			decodedInstr.B = (instr >> 22) & 1;
			decodedInstr.I = true;
			decodedInstr.imm = instr & 0xFF;
			decodedInstr.rotate = (instr >> 8) & 0xF;
			return decodedInstr;
		}

		decodedInstr.I = true;
		decodedInstr.S = (instr >> 20) & 1;
		decodedInstr.rn = (instr >> 16) & 0xF;
		decodedInstr.rd = (instr >> 12) & 0xF;
		decodedInstr.imm = instr & 0xFF;
		decodedInstr.rotate = (instr >> 8) & 0xF;

		uint8_t opcode = (instr >> 21) & 0xF;
		switch (opcode)
		{
		case 0x0: decodedInstr.type = CPU::armOperation::ARM_AND; break;
		case 0x1: decodedInstr.type = CPU::armOperation::ARM_EOR; break;
		case 0x2: decodedInstr.type = CPU::armOperation::ARM_SUB; break;
		case 0x3: decodedInstr.type = CPU::armOperation::ARM_RSB; break;
		case 0x4: decodedInstr.type = CPU::armOperation::ARM_ADD; break;
		case 0x5: decodedInstr.type = CPU::armOperation::ARM_ADC; break;
		case 0x6: decodedInstr.type = CPU::armOperation::ARM_SBC; break;
		case 0x7: decodedInstr.type = CPU::armOperation::ARM_RSC; break;
		case 0x8: decodedInstr.type = CPU::armOperation::ARM_TST; break;
		case 0x9: decodedInstr.type = CPU::armOperation::ARM_TEQ; break;
		case 0xA: decodedInstr.type = CPU::armOperation::ARM_CMP; break;
		case 0xB: decodedInstr.type = CPU::armOperation::ARM_CMN; break;
		case 0xC: decodedInstr.type = CPU::armOperation::ARM_ORR; break;
		case 0xD: decodedInstr.type = CPU::armOperation::ARM_MOV; break;
		case 0xE: decodedInstr.type = CPU::armOperation::ARM_BIC; break;
		case 0xF: decodedInstr.type = CPU::armOperation::ARM_MVN; break;
		}

		return decodedInstr;
	}

	case 0b010:  // Load/Store immediate offset
	{
		decodedInstr.L = (instr >> 20) & 1;
		decodedInstr.type = decodedInstr.L ? CPU::armOperation::ARM_LDR : CPU::armOperation::ARM_STR;
		decodedInstr.I = false;  // Immediate offset
		decodedInstr.P = (instr >> 24) & 1;
		decodedInstr.U = (instr >> 23) & 1;
		decodedInstr.B = (instr >> 22) & 1;
		decodedInstr.W = (instr >> 21) & 1;
		decodedInstr.rn = (instr >> 16) & 0xF;
		decodedInstr.rd = (instr >> 12) & 0xF;
		decodedInstr.imm = instr & 0xFFF;
		return decodedInstr;
	}

	case 0b011:  // Load/Store register offset
	{
		if ((instr >> 4) & 1)
		{
			decodedInstr.type = CPU::armOperation::ARM_UNDEFINED;
			return decodedInstr;
		}

		decodedInstr.L = (instr >> 20) & 1;
		decodedInstr.type = decodedInstr.L ? CPU::armOperation::ARM_LDR : CPU::armOperation::ARM_STR;
		decodedInstr.I = true;  // Register offset
		decodedInstr.P = (instr >> 24) & 1;
		decodedInstr.U = (instr >> 23) & 1;
		decodedInstr.B = (instr >> 22) & 1;
		decodedInstr.W = (instr >> 21) & 1;
		decodedInstr.rn = (instr >> 16) & 0xF;
		decodedInstr.rd = (instr >> 12) & 0xF;
		decodedInstr.rm = instr & 0xF;
		decodedInstr.shift_type = (instr >> 5) & 0x3;
		decodedInstr.shift_amount = (instr >> 7) & 0x1F;
		return decodedInstr;
	}

	case 0b100:  // Load/Store multiple
	{
		decodedInstr.L = (instr >> 20) & 1;
		decodedInstr.type = decodedInstr.L ? CPU::armOperation::ARM_LDM : CPU::armOperation::ARM_STM;
		decodedInstr.P = (instr >> 24) & 1;
		decodedInstr.U = (instr >> 23) & 1;
		decodedInstr.S = (instr >> 22) & 1;  // PSR & force user bit
		decodedInstr.W = (instr >> 21) & 1;
		decodedInstr.rn = (instr >> 16) & 0xF;
		decodedInstr.reg_list = instr & 0xFFFF;
		return decodedInstr;
	}

	case 0b101:  // Branch and Branch with Link
	{
		decodedInstr.L = (instr >> 24) & 1;
		decodedInstr.type = decodedInstr.L ? CPU::armOperation::ARM_BL : CPU::armOperation::ARM_B;
		int32_t offset = instr & 0xFFFFFF;
		if (offset & 0x800000)  // Sign bit set
			offset |= 0xFF000000;
		decodedInstr.imm = offset << 2;  // Shift left by 2
		return decodedInstr;
	}

	case 0b110:  // Coprocessor load/store
	{
		decodedInstr.L = (instr >> 20) & 1;
		decodedInstr.type = decodedInstr.L ? CPU::armOperation::ARM_LDC : CPU::armOperation::ARM_STC;
		decodedInstr.P = (instr >> 24) & 1;
		decodedInstr.U = (instr >> 23) & 1;
		decodedInstr.W = (instr >> 21) & 1;
		decodedInstr.rn = (instr >> 16) & 0xF;
		decodedInstr.rd = (instr >> 12) & 0xF;  // CRd
		decodedInstr.imm = (instr & 0xFF) << 2;
		return decodedInstr;
	}

	case 0b111:  // Coprocessor operations and SWI
	{
		if ((instr >> 24) & 1)  // SWI
		{
			decodedInstr.type = CPU::armOperation::ARM_SWI;
			decodedInstr.imm = instr & 0xFFFFFF;
			return decodedInstr;
		}
		else if ((instr >> 4) & 1)  // Coprocessor register transfer
		{
			decodedInstr.L = (instr >> 20) & 1;
			decodedInstr.type = decodedInstr.L ? CPU::armOperation::ARM_MRC : CPU::armOperation::ARM_MCR;
			decodedInstr.rn = (instr >> 16) & 0xF;  // CRn
			decodedInstr.rd = (instr >> 12) & 0xF;
			decodedInstr.rm = instr & 0xF;          // CRm
			return decodedInstr;
		}
		else  // Coprocessor data operation
		{
			decodedInstr.type = CPU::armOperation::ARM_CDP;
			return decodedInstr;
		}
	}
	}

	decodedInstr.type = CPU::armOperation::ARM_UNDEFINED;
	return decodedInstr;
}

}
