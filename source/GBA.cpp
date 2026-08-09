#include "GBA.h"
#include "tdmi7/CPU.h"
#include <cstdint>
#include <iomanip>
#include <ostream>

GBA::GBA(): ppu(bus), cpu(&bus, &decoder), system(bus, cpu, ppu), clockedDevices{ &ppu, &system }
{
	ppu.setHBlankCallback([this] { system.onHBlank(); });
	ppu.setVBlankCallback([this] { system.onVBlank(); });
	ppu.setVCountCallback([this] { system.onVCount(); });
}

bool GBA::loadCartridge(const char* path)
{
	reset();
	if (!bus.loadROM(path, 0x08000000))
	{
		return false;
	}

	cpu.pc = bus.hasBios() ? 0x00000000U : 0x08000000U;
	return true;
}

bool GBA::loadBios(const char* path)
{
	return bus.loadBios(path);
}

void GBA::reset()
{
	cpu.reset();
	ppu.reset();
	bus.resetSystemState();
	system.reset();
	masterCycles = 0;
	cartridgeCodeExecuted = false;
}

uint32_t GBA::tick()
{
	// A halted ARM7 consumes no instructions.  Advancing it one master cycle at
	// a time made BIOS IntrWait/HALT loops need 280,896 host iterations per
	// frame.  The PPU's next boundary is an observable hardware event, so
	// advance directly to it; device callbacks can assert IRQ and the next tick
	// will perform the normal CPU wake/exception sequence.
	if (cpu.isHalted())
	{
		const uint32_t elapsedCycles = ppu.cyclesUntilNextEvent();
		masterCycles += elapsedCycles;
		advanceDevices(elapsedCycles);
		return elapsedCycles;
	}
	const uint32_t cyclesBefore = static_cast<uint32_t>(cpu.cycleTotal);
	cpu.tick();
	if (cpu.pc >= 0x08000000U && cpu.pc < 0x0E000000U)
		cartridgeCodeExecuted = true;
	const uint32_t elapsedCycles = static_cast<uint32_t>(cpu.cycleTotal) - cyclesBefore;
	masterCycles += elapsedCycles;
	advanceDevices(elapsedCycles);
	const uint32_t dmaCycles = system.takeDmaStallCycles();
	if (dmaCycles != 0)
	{
		masterCycles += dmaCycles;
		advanceDevices(dmaCycles);
	}
	return elapsedCycles + dmaCycles;
}

uint64_t GBA::masterCycleCount() const
{
	return masterCycles;
}

bool GBA::hasExecutedCartridgeCode() const
{
	return cartridgeCodeExecuted;
}

void GBA::attachClockedDevice(ClockedDevice& device)
{
	for (const auto* existing : clockedDevices)
	{
		if (existing == &device)
			return;
	}
	clockedDevices.push_back(&device);
}

void GBA::setPressedKeys(uint16_t pressedMask)
{
	system.setPressedKeys(pressedMask);
}

void GBA::advanceDevices(uint32_t cycles)
{
	for (ClockedDevice* device : clockedDevices)
		device->advance(cycles);
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

	return { masterCycles, completedSteps, cpu.testHalt };

}
