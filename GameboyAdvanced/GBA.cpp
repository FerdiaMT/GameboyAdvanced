#include "GBA.h"
#include "CPU.h"
#include <cstdint>

GBA::GBA(): cpu(&bus) , debuggerCPU(&cpu)
{
	if (!bus.loadROM("ferdiaTest.bin", 0x00000000))
	{
		printf("error with loading the binary tester");
		return;
	}

	debuggerCPU.DecodeIns(0x00000000, 0x0000200);
}

void GBA::tick()
{
	//uint32_t cycles = cpu.tick();

}