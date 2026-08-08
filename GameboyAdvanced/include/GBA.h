#pragma once
#include "tdmi7/CPU.h"
#include "Bus.h"
#include "tdmi7/DebuggerCPU.h"
#include "tdmi7/Decoder.h"

#include <cstdint>
#include <ostream>

class GBA
{

public:

	Bus bus;
	tdmi7::Decoder decoder;
	tdmi7::CPU cpu;
	//DebuggerCPU debuggerCPU;

	GBA();

	struct RunResult
	{
		uint32_t cycles;
		uint64_t steps;
		tdmi7::CPU::TestHalt halt;
	};

	bool loadCartridge(const char* path);
	void enableTestSwiHalt(bool enabled);
	uint32_t tick();
	RunResult runSteps(uint64_t steps, std::ostream* traceOutput = nullptr);
};
