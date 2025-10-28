#include "DebuggerCPU.h"
#include "CPU.h"
#include <cstdint>


DebuggerCPU::DebuggerCPU(CPU* cpu)
{
	this->cpu = cpu;
}

void DebuggerCPU::DecodeIns(uint32_t startAddr, uint32_t endAddr)
{
	uint32_t curAddr = startAddr;

	while (curAddr <= endAddr)
	{
		uint32_t curInstruction = cpu->read32(curAddr); // this is the instruction in hex

		const char* conditional = checkConditional((curInstruction >> 28) & 0xF); // this will return in string from the conditional

		CPU::Operation curOperation = decode(curInstruction); // this turns the instruction into the decoded operation

		
		printf("PC: 0x%08X, Instruction: 0x%08X, Opcode: %s\n",
			curAddr , curInstruction, cpu->opcodeToString(curOperation));


		curAddr += 4;
	}
}

inline const char* DebuggerCPU::checkConditional(uint8_t cond)
{

	switch (cond)
	{
	case(0x0):return "Z";					break;
	case(0x1):return "!Z";				break;
	case(0x2):return "C";					break;
	case(0x3):return "!C";				break;
	case(0x4):return "N";					break;
	case(0x5):return "!N";				break;
	case(0x6):return "V";					break;
	case(0x7):return "!V";				break;
	case(0x8):return "(C && !Z)";			break;
	case(0x9):return "(!C || Z)";			break;
	case(0xA):return "(N == V)";			break;
	case(0xB):return "(N != V)";			break;
	case(0xC):return "(!Z && (N == V))";	break;
	case(0xD):return "(Z || (N != V))";	break;
	case(0xE):return "ALWAYS TRUE";				break;
	case(0xF):printf("0XF INVALID CND");break;
	}
}

CPU::Operation DebuggerCPU::decode(uint32_t instruction)
{

	uint8_t conditional = (instruction >> 28) & 0xF;
	if (!cpu->checkConditional(conditional)) return CPU::Operation::CONDITIONALSKIP;

	// so now we get bit 27,26 and 25 to tell us what instruction to execute
	switch ((instruction >> 25) & 0x7)
	{
	case(0b000):
	{
		// a few odd cases here
		if ((instruction & 0x0FFFFFF0) == 0x12FFF10) return CPU::Operation::BX;
		else if ((instruction & 0x0FB00FF0) == 0x01000090) return CPU::Operation::SWP;
		else if ((instruction & 0x0F8000F0) == 0x00800090) // multiplyLong, can be long or accumalate
		{
			// can also be signed or unsigned
			switch ((instruction >> 21) & 0b11)
			{
			case(0b00): return CPU::Operation::UMULL;
			case(0b01): return CPU::Operation::UMLAL;
			case(0b10): return CPU::Operation::SMULL;
			case(0b11): return CPU::Operation::SMLAL;
			}
		}
		else if ((instruction & 0x0FC000F0) == 0x00000090)
		{
			if ((instruction >> 21) & 0b1) return CPU::Operation::MLA;
			return CPU::Operation::MUL;
		}
		else if ((instruction & 0x0E000090) == 0x00000090) // HalfwordTransfer
		{
			uint8_t SH = (instruction >> 4) & 0b11;

			if (SH == 0) return CPU::Operation::SWP;

			SH = (((instruction >> 18) & 100) | SH) & 0x7;

			switch (SH)
			{
			case(0b001): return CPU::Operation::STRH;
			case(0b101): return CPU::Operation::LDRH;
			case(0b110): return CPU::Operation::LDRSB;
			case(0b111): return CPU::Operation::LDRSH;
			default:printf("ERROR IN HALFWORD TRANSFER DECODING, GOT SH %d", SH);
			}

		}
		else if ((instruction & 0x0FBF0FFF) == 0x010F0000) return CPU::Operation::MRS;
		else if ((instruction & 0x0FB0FFF0) == 0x0120F000 || (instruction & 0x0FBF0000) == 0x03200000)return CPU::Operation::MSR;
		else
		{
			// DATA TRANSFER
			uint8_t op = (instruction >> 21) & 0xF;

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

	case(0b011): {
		if (!((instruction >> 4) & 0b1))
		{
			//SINGLE DATA TRANSFER
			if ((instruction >> 20) & 0b1) return CPU::Operation::LDR;
			return CPU::Operation::STR;
		}
		else { return CPU::Operation::SINGLEDATATRANSFERUNDEFINED; }; break;
	}
	case(0b010): if ((instruction >> 20) & 0b1) { return CPU::Operation::LDR; }
			   else { return CPU::Operation::STR; }break; // SINGLE DATA TRANSFER (AGAIN)
	case(0b100): if ((instruction >> 20) & 0b1) { return CPU::Operation::LDM; }
			   else { return CPU::Operation::STM; }break; //BLOCK DATA TRANSFER
	case(0b101): if ((instruction >> 24) & 0b1) { return CPU::Operation::BL; }
			   else { return CPU::Operation::B; }break;
	case(0b110): if ((instruction >> 24) & 0b1) { return CPU::Operation::LDC; }
			   else { return CPU::Operation::STC; }break; // COPROCESSOR DATA TRANSFER


	case(0b111):
	{
		if ((instruction >> 24) & 0b1) return CPU::Operation::SWI;
		else if (!((instruction >> 4) & 0b1)) return CPU::Operation::CDP;
		else // coprocessor register transfer , MRC , MCR
		{
			if ((instruction >> 20) & 0b1) { return CPU::Operation::MRC; }
			else { return CPU::Operation::MCR; }break;
		}
	}break;

	}

	return CPU::Operation::DECODEFAIL;

}