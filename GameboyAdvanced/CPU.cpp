#include "CPU.h"
#include <cstdint>

CPU::CPU(Bus* bus) : bus(bus) , sp(reg[13]) , lr(reg[14]) , pc(reg[15])
{

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

	execute();
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


//MRS AND MSR ARE NOT INCLUDED . THEY ARE DONE BY CERTAIN DATA TRANSFER OPERATIONS
CPU::Operation CPU::decode()
{
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
			//LDRH/STRH/LDRSB/LDRSH
			// load register halfword
			// store register halfword
			// load register sign byte
			// load registersigned word

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