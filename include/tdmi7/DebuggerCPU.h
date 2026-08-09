#pragma once
#include <cstdint>
#include <string>
#include "tdmi7/CPUTypes.h"

namespace tdmi7
{
class CPU;

class DebuggerCPU
{
public:

	CPU* cpu;
	DebuggerCPU(CPU*);

	void DecodeIns(uint32_t startAddr, uint32_t endAddr);
	void ArmLineDecode(uint32_t curAddr);
	void ThumbLineDecode(uint32_t curAddr);

	//CPU::Operation decode(uint32_t instruction);
	const char* checkConditional(uint8_t cond);

	std::string thumbToStr(CPUTypes::thumbInstr& instr);



	bool runAllThumbTests(CPU& cpu);
};
}
