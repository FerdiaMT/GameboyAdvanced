#include "GBA.h"
#include "CPU.h"
#include <cstdint>

GBA::GBA(): cpu(&bus) , debuggerCPU(&cpu)
{
	if (!bus.loadROM("armwrestler.gba", 0x08000000))
	{
		printf("error with loading the binary tester");
	}

	debuggerCPU.DecodeIns(0x08000000, 0x08000000+0x40);
}

void GBA::tick()
{
	uint32_t cycles = cpu.tick();

}