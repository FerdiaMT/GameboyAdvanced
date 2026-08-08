#include "tdmi7/CPU.h"

#include <sstream>
#include <string>

namespace tdmi7
{
using namespace CPUTypes;

namespace
{
std::string registerName(int registerNumber)
{
    return "r" + std::to_string(registerNumber);
}

std::string compactDisassembly(std::string disassembly)
{
    const size_t detailStart = disassembly.find("    | ");
    if (detailStart != std::string::npos)
    {
        disassembly.resize(detailStart);
    }

    std::string compact;
    bool previousWasSpace = false;
    for (const char character : disassembly)
    {
        if (character == ' ' || character == '\t')
        {
            if (!previousWasSpace)
            {
                compact += ' ';
            }
            previousWasSpace = true;
        }
        else
        {
            compact += character;
            previousWasSpace = false;
        }
    }
    if (!compact.empty() && compact.back() == ' ')
    {
        compact.pop_back();
    }
    return compact;
}
}

std::string CPU::thumbToStr(CPU::thumbInstr& instr)
{
	std::stringstream ss;

	auto regStr = [&](int regNum) -> std::string
		{
			return registerName(regNum);
		};

	switch (instr.type)
	{
	case thumbOperation::THUMB_LSL_IMM:
	case thumbOperation::THUMB_LSR_IMM:
	case thumbOperation::THUMB_ASR_IMM:
	{
		const char* op = (instr.type == thumbOperation::THUMB_LSL_IMM) ? "lsl" :
			(instr.type == thumbOperation::THUMB_LSR_IMM) ? "lsr" : "asr";
		const char* sym = (instr.type == thumbOperation::THUMB_LSL_IMM) ? "<<" :
			(instr.type == thumbOperation::THUMB_LSR_IMM) ? ">>" : ">>(s)";
		ss << op << "     " << regStr(instr.rd) << ", " << regStr(instr.rs) << ", #" << instr.imm;
		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs) << " " << sym << " " << instr.imm;
		break;
	}

	case thumbOperation::THUMB_ADD_REG:
	case thumbOperation::THUMB_SUB_REG:
	{
		const char* op = (instr.type == thumbOperation::THUMB_ADD_REG) ? "add" : "sub";
		const char* sym = (instr.type == thumbOperation::THUMB_ADD_REG) ? "+" : "-";
		ss << op << "     " << regStr(instr.rd) << ", " << regStr(instr.rs) << ", " << regStr(instr.rn);
		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs) << " " << sym << " " << regStr(instr.rn);
		break;
	}

	case thumbOperation::THUMB_ADD_IMM:
	case thumbOperation::THUMB_SUB_IMM:
	{
		const char* op = (instr.type == thumbOperation::THUMB_ADD_IMM) ? "add" : "sub";
		const char* sym = (instr.type == thumbOperation::THUMB_ADD_IMM) ? "+" : "-";
		ss << op << "     " << regStr(instr.rd) << ", " << regStr(instr.rs) << ", #" << instr.imm;
		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs) << " " << sym << " #" << instr.imm;
		break;
	}

	case thumbOperation::THUMB_MOV_IMM:
	ss << "mov     " << regStr(instr.rd) << ", #0x" << std::hex << instr.imm << std::dec;
	ss << "    | " << regStr(instr.rd) << " = #0x" << std::hex << instr.imm << std::dec;
	break;

	case thumbOperation::THUMB_CMP_IMM:
	ss << "cmp IMM     " << regStr(instr.rd) << ", #0x" << std::hex << instr.imm << std::dec;
	ss << "    | flags = " << regStr(instr.rd) << " - #0x" << std::hex << instr.imm << std::dec;
	break;

	case thumbOperation::THUMB_ADD_IMM3:
	case thumbOperation::THUMB_SUB_IMM3:
	{
		const char* op = (instr.type == thumbOperation::THUMB_ADD_IMM3) ? "add" : "sub";
		const char* sym = (instr.type == thumbOperation::THUMB_ADD_IMM3) ? "+=" : "-=";
		ss << op << "     " << regStr(instr.rd) << ", #0x" << std::hex << instr.imm << std::dec;
		ss << "    | " << regStr(instr.rd) << " " << sym << " #0x" << std::hex << instr.imm << std::dec;
		break;
	}

	case thumbOperation::THUMB_AND_REG:
	ss << "and     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " &= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_EOR_REG:
	ss << "eor     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " ^= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_LSL_REG:
	ss << "lsl     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " <<= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_LSR_REG:
	ss << "lsr     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " >>= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_ASR_REG:
	ss << "asr     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " >>= (signed) " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_ADC_REG:
	ss << "adc     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " += " << regStr(instr.rs) << " + C";
	break;
	case thumbOperation::THUMB_SBC_REG:
	ss << "sbc     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " -= " << regStr(instr.rs) << " - !C";
	break;
	case thumbOperation::THUMB_ROR_REG:
	ss << "ror     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = ror(" << regStr(instr.rd) << ", " << regStr(instr.rs) << ")";
	break;
	case thumbOperation::THUMB_TST_REG:
	ss << "tst     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " & " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_NEG_REG:
	ss << "neg     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = -" << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_CMP_REG:
	ss << "cmp REG " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " - " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_CMN_REG:
	ss << "cmn     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " + " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_ORR_REG:
	ss << "orr     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " |= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_MUL_REG:
	ss << "mul     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " *= " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_BIC_REG:
	ss << "bic     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " &= ~" << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_MVN_REG:
	ss << "mvn     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = ~" << regStr(instr.rs);
	break;

	case thumbOperation::THUMB_ADD_HI:
	ss << "add  HI " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " += " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_CMP_HI:
	ss << "cmp  HI   " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | flags = " << regStr(instr.rd) << " - " << regStr(instr.rs);
	break;
	case thumbOperation::THUMB_MOV_HI:
	ss << "mov     " << regStr(instr.rd) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rs);
	break;

	case thumbOperation::THUMB_BX:
	ss << "bx      " << regStr(instr.rs);
	ss << "    | pc = " << regStr(instr.rs) << " & ~1, T = bit0";
	break;
	case thumbOperation::THUMB_BLX_REG:
	ss << "blx     " << regStr(instr.rs);
	ss << "    | lr = pc+2, pc = " << regStr(instr.rs) << " & ~1, T = bit0";
	break;

	case thumbOperation::THUMB_LDR_PC:
	{
		uint32_t addr = (pc & ~2)  + instr.imm;
		ss << "ldr  PC " << regStr(instr.rd) << ", [pc, #0x" << std::hex << instr.imm << "]" << std::dec;
		ss << "    | " << regStr(instr.rd) << " = [0x" << std::hex << addr << "]" << std::dec;
		break;
	}

	case thumbOperation::THUMB_STR_REG:
	case thumbOperation::THUMB_STRB_REG:
	case thumbOperation::THUMB_LDR_REG:
	case thumbOperation::THUMB_LDRB_REG:
	case thumbOperation::THUMB_STRH_REG:
	case thumbOperation::THUMB_LDRSB_REG:
	case thumbOperation::THUMB_LDRH_REG:
	case thumbOperation::THUMB_LDRSH_REG:
	{
		const char* opNames[] = {
			"str", "strb", "ldr", "ldrb", "strh", "ldrsb", "ldrh", "ldrsh"
		};
		int idx = (int)instr.type - (int)thumbOperation::THUMB_STR_REG+1;
		bool isLoad = (idx >= 2 && idx != 4);

		ss << opNames[idx] << "    " << regStr(instr.rd) << ", [" << regStr(instr.rs) << ", " << regStr(instr.rn) << "]";
		if (isLoad)
			ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rs) << " + " << regStr(instr.rn) << "]";
		else
			ss << "    | [" << regStr(instr.rs) << " + " << regStr(instr.rn) << "] = " << regStr(instr.rd);
		break;
	}

	//THUMB_LDR_IMM,
	//	THUMB_STR_IMM,
	//	THUMB_LDRB_IMM,
	//	THUMB_STRB_IMM,
	//	THUMB_LDRH_IMM,
	// 
	//	THUMB_STRH_IMM,
	case thumbOperation::THUMB_LDR_IMM:
	case thumbOperation::THUMB_STR_IMM:
	case thumbOperation::THUMB_LDRB_IMM:
	case thumbOperation::THUMB_STRB_IMM:
	case thumbOperation::THUMB_LDRH_IMM:
	case thumbOperation::THUMB_STRH_IMM:
	{
		const char* opNames[] = {
			"ldrIMM", "strIMM","ldrbIMM" , "strbIMM", "ldrhIMM", "strhIMM"
		};
		int idx = (int)instr.type - (int)thumbOperation::THUMB_STR_IMM+1;
		bool isLoad = (idx % 2 == 1);

		ss << opNames[idx] << "     " << regStr(instr.rd) << ", [" << regStr(instr.rs) << ", #0x" << std::hex << instr.imm << "]" << std::dec;
		if (isLoad)
			ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rs) << " + #0x" << std::hex << instr.imm << "]" << std::dec;
		else
			ss << "    | [" << regStr(instr.rs) << " + #0x" << std::hex << instr.imm << "] = " << std::dec << regStr(instr.rd);
		break;
	}

	case thumbOperation::THUMB_STR_SP:
	ss << "str     " << regStr(instr.rd) << ", [sp, #0x" << std::hex << instr.imm << "]" << std::dec;
	ss << "    | [sp + #0x" << std::hex << instr.imm << "] = " << std::dec << regStr(instr.rd);
	break;
	case thumbOperation::THUMB_LDR_SP:
	ss << "ldr     " << regStr(instr.rd) << ", [sp, #0x" << std::hex << instr.imm << "]" << std::dec;
	ss << "    | " << regStr(instr.rd) << " = [sp + #0x" << std::hex << instr.imm << "]" << std::dec;
	break;

	case thumbOperation::THUMB_ADD_PC:
	ss << "add PC  " << regStr(instr.rd) << ", pc, #0x" << std::hex << instr.imm << std::dec;
	ss << "    | " << regStr(instr.rd) << " = pc + #0x" << std::hex << instr.imm << std::dec;
	break;
	case thumbOperation::THUMB_ADD_SP_IMM:
	ss << "addSP IM  " << regStr(instr.rd) << ", sp, #0x" << std::hex << instr.imm << std::dec;
	ss << "    | " << regStr(instr.rd) << " = sp + #0x" << std::hex << instr.imm << std::dec;
	break;

	case thumbOperation::THUMB_ADD_SP:
	if ((int32_t)instr.imm < 0)
	{
		ss << "sub     sp, #0x" << std::hex << (-(int32_t)instr.imm) << std::dec;
		ss << "    | sp -= #0x" << std::hex << (-(int32_t)instr.imm) << std::dec;
	}
	else
	{
		ss << "add     sp, #0x" << std::hex << instr.imm << std::dec;
		ss << "    | sp += #0x" << std::hex << instr.imm << std::dec;
	}
	break;

	case thumbOperation::THUMB_PUSH:
	case thumbOperation::THUMB_POP:
	{
		const char* op = (instr.type == thumbOperation::THUMB_PUSH) ? "push" : "pop";
		ss << op << "    {";
		bool first = true;
		for (int i = 0; i < 16; i++)
		{
			if (instr.imm & (1 << i))
			{
				if (!first) ss << ", ";
				ss << regStr(i);
				first = false;
			}
		}
		ss << "}";
		break;
	}

	case thumbOperation::THUMB_STMIA:
	case thumbOperation::THUMB_LDMIA:
	{
		const char* op = (instr.type == thumbOperation::THUMB_STMIA) ? "stmia" : "ldmia";
		ss << op << "   " << regStr(instr.rs) << "!, {";
		bool first = true;
		for (int i = 0; i < 8; i++)
		{
			if (instr.imm & (1 << i))
			{
				if (!first) ss << ", ";
				ss << regStr(i);
				first = false;
			}
		}
		ss << "}";
		break;
	}

	case thumbOperation::THUMB_B_COND:
	{
		const char* condNames[] = {
			"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
			"hi", "ls", "ge", "lt", "gt", "le", "al", "nv"
		};
		int32_t offset = (int32_t)instr.imm;
		uint32_t target = (pc + 4 + offset) & ~1;
		ss << "b" << condNames[instr.cond] << "     0x" << std::hex << target << std::dec;
		ss << "    | if " << condNames[instr.cond] << " then pc = 0x" << std::hex << target << std::dec;
		break;
	}

	case thumbOperation::THUMB_B:
	{
		int32_t offset = (int32_t)instr.imm;
		uint32_t target = (pc + 4 + offset) & ~1;
		ss << "b       0x" << std::hex << target << std::dec;
		ss << "    | pc = 0x" << std::hex << target << std::dec;
		break;
	}

	case thumbOperation::THUMB_BL_PREFIX:
	ss << "bl_hi   0x" << std::hex << instr.imm << std::dec;
	ss << "    | lr = pc + 0x" << std::hex << instr.imm << std::dec;
	break;
	case thumbOperation::THUMB_BL_SUFFIX:
	{
		uint32_t target = (lr + instr.imm) & ~1;
		ss << "bl_lo   0x" << std::hex << target << std::dec;
		ss << "    | pc = 0x" << std::hex << target << ", lr = 0x" << (pc + 2) << std::dec;
		break;
	}

	case thumbOperation::THUMB_SWI:
	ss << "swi     #0x" << std::hex << (instr.imm & 0xFF) << std::dec;
	break;

	case thumbOperation::THUMB_UNDEFINED:
	ss << "undefined";
	break;

	default:
	ss << "unknown";
	break;
	}

	return compactDisassembly(ss.str());
}


}
