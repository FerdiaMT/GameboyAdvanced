#pragma once
#include "CPU.h"
#include <cstdint>
class DebuggerCPU
{
public:

	CPU* cpu;
	DebuggerCPU(CPU*);

	void DecodeIns(uint32_t startAddr, uint32_t endAddr);

	CPU::Operation decode(uint32_t instruction);
	inline const char* checkConditional(uint8_t cond);
};

