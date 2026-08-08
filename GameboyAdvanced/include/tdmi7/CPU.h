#pragma once
#include "Bus.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include "tdmi7/CPUTypes.h"

namespace tdmi7
{
class Decoder;

class CPU
{

#include "tdmi7/detail/CpuCoreMembers.inc"
#include "tdmi7/detail/CpuStateMembers.inc"
#include "tdmi7/thumb/InstructionHandlers.inc"
#include "tdmi7/arm/InstructionHandlers.inc"

	//debugger help

	std::string thumbToStr(CPUTypes::thumbInstr& instr);
	std::string armToStr(CPUTypes::armInstr& instr);

	const char* opcodeToString(Operation op)
	{
		switch (op)
		{
		case Operation::AND:  return  "AND  ";
		case Operation::EOR:  return  "EOR  ";
		case Operation::SUB:  return  "SUB  ";
		case Operation::RSB:  return  "RSB  ";
		case Operation::ADD:  return  "ADD  ";
		case Operation::ADC:  return  "ADC  ";
		case Operation::SBC:  return  "SBC  ";
		case Operation::RSC:  return  "RSC  ";
		case Operation::TST:  return  "TST  ";
		case Operation::TEQ:  return  "TEQ  ";
		case Operation::CMP:  return  "CMP  ";
		case Operation::CMN:  return  "CMN  ";
		case Operation::ORR:  return  "ORR  ";
		case Operation::MOV:  return  "MOV  ";
		case Operation::BIC:  return  "BIC  ";
		case Operation::MVN:  return  "MVN  ";
		case Operation::MRS:  return  "MRS  ";
		case Operation::MSR:  return  "MSR  ";
		case Operation::LDR:  return  "LDR  ";
		case Operation::STR:  return  "STR  ";
		case Operation::LDRH: return  "LDRH ";
		case Operation::STRH: return  "STRH ";
		case Operation::LDRSB: return "LDRSB";
		case Operation::LDRSH: return "LDRSH";
		case Operation::LDM:  return  "LDM  ";
		case Operation::STM:  return  "STM  ";
		case Operation::B:    return  "B    ";
		case Operation::BL:   return  "BL   ";
		case Operation::BX:   return  "BX   ";
		case Operation::MUL:  return  "MUL  ";
		case Operation::MLA:  return  "MLA  ";
		case Operation::UMULL: return "UMULL";
		case Operation::UMLAL: return "UMLAL";
		case Operation::SMULL: return "SMULL";
		case Operation::SMLAL: return "SMLAL";
		case Operation::SWP:  return  "SWP  ";
		case Operation::SWPB: return  "SWPB ";
		case Operation::SWI:  return  "SWI  ";
		case Operation::CDP:  return  "CDP  ";
		case Operation::LDC:  return  "LDC  ";
		case Operation::STC:  return  "STC  ";
		case Operation::MRC:  return  "MRC  ";
		case Operation::MCR:  return  "MCR  ";
		case Operation::UNKNOWN:return"UNKWN";
		case Operation::UNASSIGNED: return "UNSND";
		case Operation::CONDITIONALSKIP: return "CNDSP";
		case Operation::SINGLEDATATRANSFERUNDEFINED: return "SDTUND";
		case Operation::DECODEFAIL: return "DCDFL";
		default: return "OPINVL";
		}
	}

	bool runIndividualTests(const char* fixturePath);
	void runThumbTestsEXTRADEBUG();
};
}
