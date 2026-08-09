#pragma once
#include "tdmi7/CPU.h"
#include "Bus.h"
#include "tdmi7/DebuggerCPU.h"
#include "tdmi7/Decoder.h"
#include "PPU.h"
#include "SystemControl.h"

#include <cstdint>
#include <ostream>
#include <vector>

class GBA
{

public:

	Bus bus;
	tdmi7::Decoder decoder;
	PPU ppu;
	tdmi7::CPU cpu;
	SystemControl system;

	uint64_t masterCycleCount() const;
	bool hasExecutedCartridgeCode() const;
	void attachClockedDevice(ClockedDevice& device);
	void setPressedKeys(uint16_t pressedMask);

	GBA();

	struct RunResult
	{
		uint64_t cycles;
		uint64_t steps;
		tdmi7::CPU::TestHalt halt;
	};

	bool loadCartridge(const char* path);
	bool loadBios(const char* path);
	void enableTestSwiHalt(bool enabled);
	void reset();
	// Executes one CPU instruction and returns its elapsed master cycles.
	uint32_t tick();
	RunResult runSteps(uint64_t steps, std::ostream* traceOutput = nullptr);

	private:
	uint64_t masterCycles = 0;
	bool cartridgeCodeExecuted = false;
	std::vector<ClockedDevice*> clockedDevices;
	void advanceDevices(uint32_t cycles);
};
