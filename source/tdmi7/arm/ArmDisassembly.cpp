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

std::string CPU::armToStr(CPU::armInstr& instr)
{
	std::stringstream ss;

	auto regStr = [&](int regNum) -> std::string
		{
			return registerName(regNum);
		};

	auto condStr = [](uint8_t cond) -> const char*
		{
			const char* condNames[] = {
				"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
				"hi", "ls", "ge", "lt", "gt", "le", "", "nv"
			};
			return condNames[cond];
		};

	auto shiftStr = [](uint8_t type) -> const char*
		{
			const char* shifts[] = { "lsl", "lsr", "asr", "ror" };
			return shifts[type & 3];
		};

	auto addCond = [&](const char* mnemonic) -> std::string
		{
			std::string result = mnemonic;
			while (!result.empty() && result.back() == ' ')
			{
				result.pop_back();
			}
			if (instr.cond != 14) result += condStr(instr.cond);
			return result;
		};

	switch (instr.type)
	{

	case armOperation::ARM_ADD:
	case armOperation::ARM_SUB:
	case armOperation::ARM_RSB:
	case armOperation::ARM_ADC:
	case armOperation::ARM_SBC:
	case armOperation::ARM_RSC:
	{
		const char* ops[] = { "add", "sub", "rsb", "adc", "sbc", "rsc" };
		const char* syms[] = { "+", "-", "- (rev)", "+ C", "- !C", "- !C (rev)" };
		int idx = (int)instr.type - (int)armOperation::ARM_ADD;

		if (idx < 0) {
			printf("IDX IS NOT RECOGNIZED, comes out as %d instead, armOp is %0X\n", idx, static_cast<unsigned int>(instr.type));

			idx = 0;
		}

		ss << addCond(ops[idx]) << (instr.S ? "s" : "") << "     ";
		ss << regStr(instr.rd) << ", " << regStr(instr.rn);

		if (instr.I)
		{
			ss << ", #0x" << std::hex << instr.imm << std::dec;
			if (instr.rotate) ss << " ror #" << (instr.rotate * 2);
		}
		else
		{
			ss << ", " << regStr(instr.rm);
			if (instr.shift_amount || instr.shift_by_reg)
			{
				ss << ", " << shiftStr(instr.shift_type) << " ";
				if (instr.shift_by_reg)
					ss << regStr(instr.shift_reg);
				else
					ss << "#" << (int)instr.shift_amount;
			}
		}

		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rn) << " " << syms[idx];
		if (instr.I)
			ss << " #0x" << std::hex << instr.imm << std::dec;
		else
			ss << " " << regStr(instr.rm);
		break;
	}


	case armOperation::ARM_AND:
	case armOperation::ARM_EOR:
	case armOperation::ARM_ORR:
	case armOperation::ARM_BIC:
	{
		const char* ops[] = { "and", "eor", "orr", "bic" };
		const char* syms[] = { "&", "^", "|", "& ~" };
		int idx = (instr.type == armOperation::ARM_AND) ? 0 :
			(instr.type == armOperation::ARM_EOR) ? 1 :
			(instr.type == armOperation::ARM_ORR) ? 2 : 3;

		ss << addCond(ops[idx]) << (instr.S ? "s" : "") << "     ";
		ss << regStr(instr.rd) << ", " << regStr(instr.rn);

		if (instr.I)
		{
			ss << ", #0x" << std::hex << instr.imm << std::dec;
		}
		else
		{
			ss << ", " << regStr(instr.rm);
			if (instr.shift_amount || instr.shift_by_reg)
			{
				ss << ", " << shiftStr(instr.shift_type) << " ";
				if (instr.shift_by_reg)
					ss << regStr(instr.shift_reg);
				else
					ss << "#" << (int)instr.shift_amount;
			}
		}

		ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rn) << " " << syms[idx] << " ";
		if (instr.I)
			ss << "#0x" << std::hex << instr.imm << std::dec;
		else
			ss << regStr(instr.rm);
		break;
	}
	case armOperation::ARM_TST:
	case armOperation::ARM_TEQ:
	case armOperation::ARM_CMP:
	case armOperation::ARM_CMN:
	{
		const char* ops[] = { "tst", "teq", "cmp", "cmn" };
		const char* syms[] = { "&", "^", "-", "+" };
		int idx = (int)instr.type - (int)armOperation::ARM_TST;

		ss << addCond(ops[idx]) << "     ";
		ss << regStr(instr.rn);

		if (instr.I)
		{
			ss << ", #0x" << std::hex << instr.imm << std::dec;
		}
		else
		{
			ss << ", " << regStr(instr.rm);
			if (instr.shift_amount || instr.shift_by_reg)
			{
				ss << ", " << shiftStr(instr.shift_type) << " ";
				if (instr.shift_by_reg)
					ss << regStr(instr.shift_reg);
				else
					ss << "#" << (int)instr.shift_amount;
			}
		}

		ss << "    | flags = " << regStr(instr.rn) << " " << syms[idx] << " ";
		if (instr.I)
			ss << "#0x" << std::hex << instr.imm << std::dec;
		else
			ss << regStr(instr.rm);
		break;
	}


	case armOperation::ARM_MOV:
	case armOperation::ARM_MVN:
	{
		const char* op = (instr.type == armOperation::ARM_MOV) ? "mov" : "mvn";
		const char* prefix = (instr.type == armOperation::ARM_MVN) ? "~" : "";

		ss << addCond(op) << (instr.S ? "s" : "") << "     ";
		ss << regStr(instr.rd);

		if (instr.I)
		{
			ss << ", #0x" << std::hex << instr.imm << std::dec;
		}
		else
		{
			ss << ", " << regStr(instr.rm);
			if (instr.shift_amount || instr.shift_by_reg)
			{
				ss << ", " << shiftStr(instr.shift_type) << " ";
				if (instr.shift_by_reg)
					ss << regStr(instr.shift_reg);
				else
					ss << "#" << (int)instr.shift_amount;
			}
		}

		ss << "    | " << regStr(instr.rd) << " = " << prefix;
		if (instr.I)
			ss << "#0x" << std::hex << instr.imm << std::dec;
		else
			ss << regStr(instr.rm);
		break;
	}

	case armOperation::ARM_MUL:
	ss << addCond("mul") << (instr.S ? "s" : "") << "     ";
	ss << regStr(instr.rd) << ", " << regStr(instr.rm) << ", " << regStr(instr.rs);
	ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rm) << " * " << regStr(instr.rs);
	break;

	case armOperation::ARM_MLA:
	ss << addCond("mla") << (instr.S ? "s" : "") << "     ";
	ss << regStr(instr.rd) << ", " << regStr(instr.rm) << ", " << regStr(instr.rs) << ", " << regStr(instr.rn);
	ss << "    | " << regStr(instr.rd) << " = " << regStr(instr.rm) << " * " << regStr(instr.rs) << " + " << regStr(instr.rn);
	break;

	case armOperation::ARM_UMULL:
	case armOperation::ARM_UMLAL:
	case armOperation::ARM_SMULL:
	case armOperation::ARM_SMLAL:
	{
		const char* ops[] = { "umull", "umlal", "smull", "smlal" };
		int idx = (int)instr.type - (int)armOperation::ARM_UMULL;

		ss << addCond(ops[idx]) << (instr.S ? "s" : "") << " ";
		ss << regStr(instr.rn) << ", " << regStr(instr.rd) << ", " << regStr(instr.rm) << ", " << regStr(instr.rs);
		ss << "    | " << regStr(instr.rn) << ":" << regStr(instr.rd) << " = " << regStr(instr.rm) << " * " << regStr(instr.rs);
		break;
	}

	case armOperation::ARM_LDR:
	case armOperation::ARM_STR:
	{
		const char* op = (instr.type == armOperation::ARM_LDR) ? "ldr" : "str";
		ss << addCond(op) << (instr.B ? "b" : "") << "     ";
		ss << regStr(instr.rd) << ", [" << regStr(instr.rn);

		if (instr.P)
		{
			ss << ", ";
			if (!instr.U) ss << "-";
			if (instr.I)
			{
				ss << regStr(instr.rm);
				if (instr.shift_amount)
				{
					ss << ", " << shiftStr(instr.shift_type) << " #" << (int)instr.shift_amount;
				}
			}
			else
			{
				ss << "#0x" << std::hex << instr.imm << std::dec;
			}
			ss << "]" << (instr.W ? "!" : "");
		}
		else
		{
			ss << "], ";
			if (!instr.U) ss << "-";
			if (instr.I)
			{
				ss << regStr(instr.rm);
			}
			else
			{
				ss << "#0x" << std::hex << instr.imm << std::dec;
			}
		}

		if (instr.type == armOperation::ARM_LDR)
			ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rn) << " �} offset]";
		else
			ss << "    | [" << regStr(instr.rn) << " �} offset] = " << regStr(instr.rd);
		break;
	}


	case armOperation::ARM_LDRH:
	case armOperation::ARM_STRH:
	case armOperation::ARM_LDRSB:
	case armOperation::ARM_LDRSH:
	{
		const char* ops[] = { "ldrh", "strh", "ldrsb", "ldrsh" };
		int idx = (int)instr.type - (int)armOperation::ARM_LDRH;

		ss << addCond(ops[idx]) << "   ";
		ss << regStr(instr.rd) << ", [" << regStr(instr.rn);

		if (instr.P)
		{
			ss << ", ";
			if (!instr.U) ss << "-";
			if (instr.I)
				ss << "#0x" << std::hex << instr.imm << std::dec;
			else
				ss << regStr(instr.rm);
			ss << "]" << (instr.W ? "!" : "");
		}
		else
		{
			ss << "], ";
			if (!instr.U) ss << "-";
			if (instr.I)
				ss << "#0x" << std::hex << instr.imm << std::dec;
			else
				ss << regStr(instr.rm);
		}

		bool isLoad = (idx == 0 || idx == 2 || idx == 3);
		if (isLoad)
			ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rn) << " �} offset]";
		else
			ss << "    | [" << regStr(instr.rn) << " �} offset] = " << regStr(instr.rd);
		break;
	}

	case armOperation::ARM_LDM:
	case armOperation::ARM_STM:
	{
		const char* op = (instr.type == armOperation::ARM_LDM) ? "ldm" : "stm";
		const char* mode = "";


		if (!instr.P && !instr.U) mode = "da";
		else if (!instr.P && instr.U) mode = "ia";
		else if (instr.P && !instr.U) mode = "db";
		else if (instr.P && instr.U) mode = "ib";

		ss << addCond(op) << mode << "   ";
		ss << regStr(instr.rn) << (instr.W ? "!" : "") << ", {";

		bool first = true;
		for (int i = 0; i < 16; i++)
		{
			if (instr.reg_list & (1 << i))
			{
				if (!first) ss << ", ";
				ss << regStr(i);
				first = false;
			}
		}
		ss << "}" << (instr.S ? "^" : "");
		break;
	}

	// Branch
	case armOperation::ARM_B:
	{
		uint32_t target = (pc + 8 + instr.imm) & ~3;
		ss << addCond("b ") << "       0x" << std::hex << target << std::dec;
		ss << "    | pc = 0x" << std::hex << target << std::dec;
		break;
	}

	case armOperation::ARM_BL:
	{
		uint32_t target = (pc + 8 + instr.imm) & ~3;
		ss << addCond("bl ") << "      0x" << std::hex << target << std::dec;
		ss << "    | lr = pc+4, pc = 0x" << std::hex << target << std::dec;
		break;
	}

	case armOperation::ARM_BX:
	ss << addCond("bx ") << "      " << regStr(instr.rm);
	ss << "    | pc = " << regStr(instr.rm) << " & ~1, T = bit0";
	break;

	// PSR Transfer
	case armOperation::ARM_MRS:
	ss << addCond("mrs") << "     " << regStr(instr.rd) << ", " << (instr.B ? "spsr" : "cpsr");
	ss << "    | " << regStr(instr.rd) << " = " << (instr.B ? "spsr" : "cpsr");
	break;

	case armOperation::ARM_MSR:
	ss << addCond("msr") << "     " << (instr.B ? "spsr" : "cpsr") << ", ";
	if (instr.I)
		ss << "#0x" << std::hex << instr.imm << std::dec;
	else
		ss << regStr(instr.rm);
	ss << "    | " << (instr.B ? "spsr" : "cpsr") << " = ";
	if (instr.I)
		ss << "#0x" << std::hex << instr.imm << std::dec;
	else
		ss << regStr(instr.rm);
	break;

	// Special
	case armOperation::ARM_SWP:
	ss << addCond("swp") << "     " << regStr(instr.rd) << ", " << regStr(instr.rm) << ", [" << regStr(instr.rn) << "]";
	ss << "    | " << regStr(instr.rd) << " = [" << regStr(instr.rn) << "], [" << regStr(instr.rn) << "] = " << regStr(instr.rm);
	break;

	case armOperation::ARM_SWI:
	ss << addCond("swi") << "     #0x" << std::hex << instr.imm << std::dec;
	break;

	// Coprocessor
	case armOperation::ARM_CDP:
	ss << addCond("cdp") << "     (coprocessor operation)";
	break;

	case armOperation::ARM_LDC:
	case armOperation::ARM_STC:
	{
		const char* op = (instr.type == armOperation::ARM_LDC) ? "ldc" : "stc";
		ss << addCond(op) << "     c" << (int)instr.rd << ", [" << regStr(instr.rn) << ", #0x" << std::hex << instr.imm << "]" << std::dec;
		break;
	}

	case armOperation::ARM_MRC:
	case armOperation::ARM_MCR:
	{
		const char* op = (instr.type == armOperation::ARM_MRC) ? "mrc" : "mcr";
		ss << addCond(op) << "     " << regStr(instr.rd);
		break;
	}

	case armOperation::ARM_UNDEFINED:
	ss << "undefined";
	break;

	default:
	ss << "unknown";
	break;
	}

	return compactDisassembly(ss.str());
}
//TESTS TO FIX
//CPSR / SPSR

// arm_data_proc_immed


//// COPROCESSOR ( last, NOT REQUIRED FOR GBA AND LIKELY IGNORED)
// "arm_stc_ldc.json.bin"					STORE COPROCESSOR
// "arm_cdp.json.bin"						COPROCESSOR DATA OPERATION
// "arm_mcr_mrc.json.bin"					COPROCESSOR REGISTER TRANSFER

}
