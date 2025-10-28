#include "GBA.h"
#include "CPU.h"
#include <cstdint>

GBA::GBA(): cpu(&bus)
{
	if (!bus.loadROM("armwrestler.gba", 0x08000000))
	{
		printf("error with loading the binary tester");
	}
}

void GBA::tick()
{
	uint32_t cycles = cpu.tick();

}