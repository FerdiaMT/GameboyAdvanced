#pragma once
#include "CPU.h"
#include <cstdint>
#include <string>
class DebuggerCPU
{
public:

	CPU* cpu;
	DebuggerCPU(CPU*);

	void DecodeIns(uint32_t startAddr, uint32_t endAddr);
	void ArmLineDecode(uint32_t curAddr);
	void ThumbLineDecode(uint32_t curAddr);

	CPU::Operation decode(uint32_t instruction);
	inline const char* checkConditional(uint8_t cond);

	std::string thumbToStr(CPU::thumbInstr& instr);



	void runAllThumbTests(CPU& cpu);
};

