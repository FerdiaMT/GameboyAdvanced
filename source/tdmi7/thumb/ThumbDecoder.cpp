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
CPU::thumbInstr Decoder::decodeThumb(uint16_t instr) // this returns a CPU::thumbInstr struct
{
	CPU::thumbInstr decodedInstr = {}; // creates empty struct for us to fill
	decodedInstr.type = CPU::thumbOperation::THUMB_UNDEFINED;

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
			case(0):decodedInstr.type = CPU::thumbOperation::THUMB_LSL_IMM; break;
			case(1):decodedInstr.type = CPU::thumbOperation::THUMB_LSR_IMM; break;
			case(2):decodedInstr.type = CPU::thumbOperation::THUMB_ASR_IMM; break;
			}
		}
		else // Add/Subtract
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111;

			switch ((instr >> 9) & 0b11)
			{// 00 reg and, 01, reg sub, 10 immed and, 11 immed sub
			case(0b00): decodedInstr.rn = (instr >> 6) & 0b111; decodedInstr.type = CPU::thumbOperation::THUMB_ADD_REG; break;
			case(0b01): decodedInstr.rn = (instr >> 6) & 0b111; decodedInstr.type = CPU::thumbOperation::THUMB_SUB_REG; break;
			case(0b10): decodedInstr.imm = (instr >> 6) & 0b111;decodedInstr.type = CPU::thumbOperation::THUMB_ADD_IMM; break;
			case(0b11): decodedInstr.imm = (instr >> 6) & 0b111;decodedInstr.type = CPU::thumbOperation::THUMB_SUB_IMM; break;
			}
		}
	}break;
	case(0b001): // Move/compare/add/ subtract immediate
	{
		decodedInstr.imm = (instr) & 0xFF;
		decodedInstr.rd = (instr >> 8) & 0b111;

		switch ((instr >> 11) & 0b11)
		{
		case(0):decodedInstr.type = CPU::thumbOperation::THUMB_MOV_IMM; break;
		case(1):decodedInstr.type = CPU::thumbOperation::THUMB_CMP_IMM; break;
		case(2):decodedInstr.type = CPU::thumbOperation::THUMB_ADD_IMM3; break;
		case(3):decodedInstr.type = CPU::thumbOperation::THUMB_SUB_IMM3; break;
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
			case(0b0000):decodedInstr.type = CPU::thumbOperation::THUMB_AND_REG; break;
			case(0b0001):decodedInstr.type = CPU::thumbOperation::THUMB_EOR_REG; break;
			case(0b0010):decodedInstr.type = CPU::thumbOperation::THUMB_LSL_REG; break;
			case(0b0011):decodedInstr.type = CPU::thumbOperation::THUMB_LSR_REG; break;
			case(0b0100):decodedInstr.type = CPU::thumbOperation::THUMB_ASR_REG; break;
			case(0b0101):decodedInstr.type = CPU::thumbOperation::THUMB_ADC_REG; break;
			case(0b0110):decodedInstr.type = CPU::thumbOperation::THUMB_SBC_REG; break;
			case(0b0111):decodedInstr.type = CPU::thumbOperation::THUMB_ROR_REG; break;
			case(0b1000):decodedInstr.type = CPU::thumbOperation::THUMB_TST_REG; break;
			case(0b1001):decodedInstr.type = CPU::thumbOperation::THUMB_NEG_REG; break;
			case(0b1010):decodedInstr.type = CPU::thumbOperation::THUMB_CMP_REG; break;
			case(0b1011):decodedInstr.type = CPU::thumbOperation::THUMB_CMN_REG; break;
			case(0b1100):decodedInstr.type = CPU::thumbOperation::THUMB_ORR_REG; break;
			case(0b1101):decodedInstr.type = CPU::thumbOperation::THUMB_MUL_REG; break;
			case(0b1110):decodedInstr.type = CPU::thumbOperation::THUMB_BIC_REG; break;
			case(0b1111):decodedInstr.type = CPU::thumbOperation::THUMB_MVN_REG; break;
			}
		}
		else if (((instr >> 10) & 0b111) == 0b001) //Hi register operations/branch exchange
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111;
			decodedInstr.h1 = ((instr >> 7) & 0b1)==1;
			decodedInstr.h2 = ((instr >> 6) & 0b1)==1;


			if ((((instr >> 8) & 0b11) == 0b00 || ((instr >> 8) & 0b11) == 0b01 || ((instr >> 8) & 0b11) == 0b10) && (!decodedInstr.h1 && !decodedInstr.h2))
			{
				decodedInstr.type = CPU::thumbOperation::THUMB_UNDEFINED;
			}


			if (decodedInstr.h1) decodedInstr.rd += 8;
			if (decodedInstr.h2) decodedInstr.rs += 8;

			//The action of H1 = 0, H2 = 0 for Op = 00 (ADD), Op = 01 (CMP) and Op = 10 (MOV)is
			//	undefined, and should not be used



			switch ((instr >> 8) & 0b11)
			{
			case 0b00: decodedInstr.type = CPU::thumbOperation::THUMB_ADD_HI; break;
			case 0b01: decodedInstr.type = CPU::thumbOperation::THUMB_CMP_HI; break;
			case 0b10: decodedInstr.type = CPU::thumbOperation::THUMB_MOV_HI; break;
			case 0b11:
			if (decodedInstr.h1)
			{
				decodedInstr.type = CPU::thumbOperation::THUMB_BX; //BLX was apparntly a figment of my imagination
			}
			else decodedInstr.type = CPU::thumbOperation::THUMB_BX; // ; 
			break;
			}
		}
		else if (((instr >> 11) & 0b11) == 0b01) // pc relative load
		{
			decodedInstr.imm = ((instr) & 0xFF) << 2;
			decodedInstr.rd = (instr >> 8) & 0b111;

			decodedInstr.type = CPU::thumbOperation::THUMB_LDR_PC;
		}
		else if (((instr >> 12) & 0b1) == 1 && ((instr >> 9) & 0b1) == 0) // load store w reg offset
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111; // where rs is used instead or rb for rbase
			decodedInstr.rn = (instr >> 6) & 0b111; // where rn is used instad or r0

			switch ((instr >> 10) & 0b11)
			{
			case 0b00: decodedInstr.type = CPU::thumbOperation::THUMB_STR_REG; break;
			case 0b01: decodedInstr.type = CPU::thumbOperation::THUMB_STRB_REG; break;
			case 0b10: decodedInstr.type = CPU::thumbOperation::THUMB_LDR_REG; break;
			case 0b11: decodedInstr.type = CPU::thumbOperation::THUMB_LDRB_REG; break;
			}
		}
		else if (((instr >> 12) & 0b1) == 1 && ((instr >> 9) & 0b1) == 1) // load store w sign-extended byte / halfwor
		{
			decodedInstr.rd = (instr) & 0b111;
			decodedInstr.rs = (instr >> 3) & 0b111; // where rs is used instead or rb for rbase
			decodedInstr.rn = (instr >> 6) & 0b111; // where rn is used instad or r0

			switch ((instr >> 10) & 0b11)
			{
			case 0b00: decodedInstr.type = CPU::thumbOperation::THUMB_STRH_REG; break;
			case 0b10: decodedInstr.type = CPU::thumbOperation::THUMB_LDRH_REG; break;
			case 0b01: decodedInstr.type = CPU::thumbOperation::THUMB_LDRSB_REG; break;
			case 0b11: decodedInstr.type = CPU::thumbOperation::THUMB_LDRSH_REG; break;
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
		case 0b00: decodedInstr.type = CPU::thumbOperation::THUMB_STR_IMM; decodedInstr.imm = decodedInstr.imm << 2; break;
		case 0b01: decodedInstr.type = CPU::thumbOperation::THUMB_LDR_IMM; decodedInstr.imm = decodedInstr.imm << 2; break;
		case 0b10: decodedInstr.type = CPU::thumbOperation::THUMB_STRB_IMM; break;
		case 0b11: decodedInstr.type = CPU::thumbOperation::THUMB_LDRB_IMM; break;
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
			case 0b0: decodedInstr.type = CPU::thumbOperation::THUMB_STRH_IMM; break;
			case 0b1: decodedInstr.type = CPU::thumbOperation::THUMB_LDRH_IMM; break;
			}
		}
		else // (SP-relative load/store)
		{
			decodedInstr.imm = (instr & 0xFF) << 2;
			decodedInstr.rd = (instr >> 8) & 0b111;
			decodedInstr.rs = 13;

			switch ((instr >> 11) & 0b1)
			{
			case 0b0: decodedInstr.type = CPU::thumbOperation::THUMB_STR_SP; break;
			case 0b1: decodedInstr.type = CPU::thumbOperation::THUMB_LDR_SP; break;
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
			case 0b0: decodedInstr.type = CPU::thumbOperation::THUMB_ADD_PC; break;
			case 0b1: decodedInstr.type = CPU::thumbOperation::THUMB_ADD_SP; break;
			}
		}
		else if (((instr >> 8) & 0b11111) == 0b10000) //  (add ofs to sp)
		{
			decodedInstr.imm = (instr & 0b1111111) << 2;

			if ((instr >> 7) & 0b1) decodedInstr.imm = -(int32_t)decodedInstr.imm;
			decodedInstr.type = CPU::thumbOperation::THUMB_ADD_SP_IMM;
		}
		else //  (push/pop reg)
		{
			decodedInstr.imm = (instr & 0xFF);
			bool rBit = ((instr >> 8) & 0b1);
			bool lBit = ((instr >> 11) & 0b1);

			if (!lBit) // push
			{
				if (rBit) decodedInstr.imm |= (1 << 14);  // include lr
				decodedInstr.type = CPU::thumbOperation::THUMB_PUSH;
			}
			else // pop
			{
				if (rBit) decodedInstr.imm |= (1 << 15);  // include pc
				decodedInstr.type = CPU::thumbOperation::THUMB_POP;
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
			case(0b0):decodedInstr.type = CPU::thumbOperation::THUMB_STMIA;break;
			case(0b1):decodedInstr.type = CPU::thumbOperation::THUMB_LDMIA;break;
			}
		}
		else if (((instr >> 8) & 0b11111) != 0b11111) //  conditional branch (done by making sure it isnt SWI first)
		{
			decodedInstr.cond = (instr >> 8) & 0b1111;
			int8_t offset8 = (instr & 0xFF);
			decodedInstr.imm = (int32_t)offset8 << 1;

			decodedInstr.type = CPU::thumbOperation::THUMB_B_COND;
		}
		else // SWI
		{
			decodedInstr.imm = instr & 0xFF;
			decodedInstr.type = CPU::thumbOperation::THUMB_SWI;
		}
	}break;
	case(0b111): // (uncond branch) or (long branch w/link)
	{
		if (((instr >> 12) & 0b1) == 0b0) // (uncond branch)
		{
			int16_t offset11 = (instr & 0x7FF);
			if (offset11 & 0x400) offset11 |= 0xF800;
			decodedInstr.imm = (int32_t)offset11 << 1;

			decodedInstr.type = CPU::thumbOperation::THUMB_B;
		}
		else // (long branch w/link)
		{
			decodedInstr.imm = (instr & 0x7FF);
			if (!((instr >> 11) & 0b1))
			{
				int32_t offset11 = (instr & 0x7FF); 
				if (offset11 & 0x400) offset11 |= 0xFFFFF800; 
				decodedInstr.imm = offset11 << 12;

				decodedInstr.type = CPU::thumbOperation::THUMB_BL_PREFIX;
			}
			else
			{
				decodedInstr.imm = decodedInstr.imm << 1;
				decodedInstr.type = CPU::thumbOperation::THUMB_BL_SUFFIX;
			}
		}

	}break;
	}

	return decodedInstr;
}


}
