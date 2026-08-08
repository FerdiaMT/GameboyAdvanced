#include "GBA.h"
#include "tdmi7/CPU.h"
#include <cstdint>
#include <iomanip>
#include <ostream>

GBA::GBA(): cpu(&bus, &decoder)
{
}

bool GBA::loadCartridge(const char* path)
{
	cpu.reset();
	if (!bus.loadROM(path, 0x08000000))
	{
		return false;
	}

	cpu.pc = 0x08000000;
	return true;
}

uint32_t GBA::tick()
{
	return cpu.tick();
}

void GBA::enableTestSwiHalt(bool enabled)
{
	cpu.enableTestSwiHalt(enabled);
}

GBA::RunResult GBA::runSteps(uint64_t steps, std::ostream* traceOutput)
{
	uint64_t completedSteps = 0;
	for (; completedSteps < steps && cpu.testHalt == tdmi7::CPU::TestHalt::None; ++completedSteps)
	{
		if (traceOutput)
		{
			const uint32_t address = cpu.pc;
			(*traceOutput) << "inst: [" << std::setw(5) << (completedSteps + 1)
				<< "] CPU  0 <v:0x" << std::hex << std::setfill('0') << std::setw(8) << address
				<< "> [...] ";

			if (cpu.T)
			{
				const uint16_t instruction = bus.read16(address, true);
				auto decoded = cpu.decodeThumb(instruction);
				(*traceOutput) << std::setw(4) << instruction << ' ' << cpu.thumbToStr(decoded);
			}
			else
			{
				const uint32_t instruction = bus.read32(address, true);
				auto decoded = cpu.decodeArm(instruction);
				(*traceOutput) << std::setw(8) << instruction << ' ' << cpu.armToStr(decoded);
			}

			(*traceOutput) << std::dec << std::setfill(' ') << '\n';
		}

		tick();
	}

	return { static_cast<uint32_t>(cpu.cycleTotal), completedSteps, cpu.testHalt };

}
