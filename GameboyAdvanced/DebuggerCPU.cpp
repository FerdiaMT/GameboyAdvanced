#include "DebuggerCPU.h"
#include "CPU.h"
#include <cstdint>
#include <string>
#include <sstream>


DebuggerCPU::DebuggerCPU(CPU* cpu)
{
	this->cpu = cpu;
}

void DebuggerCPU::DecodeIns(uint32_t startAddr, uint32_t endAddr)
{
	uint32_t curAddr = startAddr;

	while (curAddr <= endAddr)
	{
        ThumbLineDecode(curAddr);

        curAddr += 2;
	}
}

void DebuggerCPU::ThumbLineDecode(uint32_t curAddr)
{
    uint16_t curInstruction = cpu->read16(curAddr); // this is the instruction in hex
    CPU::thumbInstr curOperation = cpu->decodeThumb(curInstruction); // this turns the instruction into the decoded operation

    printf("PC: 0x%08X, Instruction: 0x%04X, ThumbCode: %s  \n",
        curAddr, curInstruction, thumbToStr(curOperation).c_str()  );

    curAddr += 2;
}


void DebuggerCPU::ArmLineDecode(uint32_t curAddr)
{
	uint32_t curInstruction = cpu->read32(curAddr); // this is the instruction in hex
	CPU::Operation curOperation = cpu->decode(curInstruction); // this turns the instruction into the decoded operation

	printf("PC: 0x%08X, Instruction: 0x%08X, Opcode: %s\n",
		curAddr, curInstruction, cpu->opcodeToString(curOperation));


    curAddr += 4;
	//const char* conditional = checkConditional((curInstruction >> 28) & 0xF); // this will return in string from the conditiona
}



bool needsRd(CPU::thumbOperation type)
{
    switch (type)
    {
    case  CPU::thumbOperation::THUMB_TST_REG:
    case  CPU::thumbOperation::THUMB_CMP_REG:
    case  CPU::thumbOperation::THUMB_CMP_IMM:
    case  CPU::thumbOperation::THUMB_CMN_REG:
    case  CPU::thumbOperation::THUMB_CMP_HI:
    case  CPU::thumbOperation::THUMB_BX:
    case  CPU::thumbOperation::THUMB_BLX_REG:
    case  CPU::thumbOperation::THUMB_B_COND:
    case  CPU::thumbOperation::THUMB_B:
    case  CPU::thumbOperation::THUMB_BL_PREFIX:
    case  CPU::thumbOperation::THUMB_BL_SUFFIX:
    case  CPU::thumbOperation::THUMB_ADD_SP_IMM:

    case CPU::thumbOperation::THUMB_PUSH:
    case CPU::thumbOperation::THUMB_POP:
    case CPU::thumbOperation::THUMB_STMIA:
    case CPU::thumbOperation::THUMB_LDMIA:
    case CPU::thumbOperation::THUMB_SWI:

    return false;
    default:
    return true;
    }
}

bool needsRs(CPU::thumbOperation type)
{
    switch (type)
    {
    case CPU::thumbOperation::THUMB_MOV_IMM:
    case  CPU::thumbOperation::THUMB_CMP_IMM:
    case  CPU::thumbOperation::THUMB_ADD_PC:
    case  CPU::thumbOperation::THUMB_LDR_PC:
    case  CPU::thumbOperation::THUMB_B_COND:
    case  CPU::thumbOperation::THUMB_B:
    case  CPU::thumbOperation::THUMB_BL_PREFIX:
    case  CPU::thumbOperation::THUMB_BL_SUFFIX:
    case  CPU::thumbOperation::THUMB_SWI:
    case  CPU::thumbOperation::THUMB_PUSH:
    case  CPU::thumbOperation::THUMB_POP:
    case  CPU::thumbOperation::THUMB_ADD_SP_IMM:
    return false;
    default:
    return true;
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

std::string DebuggerCPU::thumbToStr(CPU::thumbInstr& instr)
{
	std::stringstream ss;

	const char* opNames[] = {
		"MOV_IMM", "ADD_REG", "ADD_IMM", "ADD_IMM3", "SUB_REG", "SUB_IMM", "SUB_IMM3", "CMP_IMM",
		"LSL_IMM", "LSR_IMM", "ASR_IMM", "AND_REG", "EOR_REG", "LSL_REG",
		"LSR_REG", "ASR_REG", "ADC_REG", "SBC_REG", "ROR_REG", "TST_REG",
		"NEG_REG", "CMP_REG", "CMN_REG", "ORR_REG", "MUL_REG", "BIC_REG",
		"MVN_REG", "ADD_HI", "CMP_HI", "MOV_HI", "BX", "BLX_REG",
		"LDR_PC", "LDR_REG", "STR_REG", "LDRB_REG", "STRB_REG", "LDRH_REG",
		"STRH_REG", "LDRSB_REG", "LDRSH_REG", "LDR_IMM", "STR_IMM", "LDRB_IMM",
		"STRB_IMM", "LDRH_IMM", "STRH_IMM", "LDR_SP", "STR_SP", "ADD_PC",
		"ADD_SP", "ADD_SP_IMM", "PUSH", "POP", "STMIA", "LDMIA",
		"B_COND", "B", "BL_PREFIX", "BL_SUFFIX", "SWI", "UNDEFINED"
	};

	ss << std::dec << opNames[(int)instr.type];

	if ((instr.rd != NULL)) ss << " Rd=R" << (int)instr.rd;
	if ((instr.rs != NULL)) ss << " Rs=R" << (int)instr.rs;
	if ((instr.rn != NULL)) ss << " Rn=R" << (int)instr.rn;


	if (instr.imm != 0)
	{
		ss << " imm=0x" << std::dec << instr.imm;
	}


	if (instr.type == CPU::thumbOperation::THUMB_B_COND)
	{
		const char* condNames[] = {
			"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
			"HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"
		};
		ss << " cond=" << condNames[instr.cond];
	}

	if (instr.h1) ss << " H1";
	if (instr.h2) ss << " H2";

	return ss.str();
}

