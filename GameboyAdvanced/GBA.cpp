#include "GBA.h"
#include "CPU.h"
#include <cstdint>

//BUGS TO FIX WITH DECODER

// MSR NOT PROPERLY APPEARING FOR 3RD CASE

//LOAD / STORE BITS NOT PROPERLY HANDLED


GBA::GBA(): cpu(&bus) , debuggerCPU(&cpu)
{
	if (!bus.loadROM("armwrestler.gba", 0x08000000))
	{
		printf("error with loading the binary tester");
		return;
	}

	//debuggerCPU.DecodeIns(0x08000000, 0x08001000);
}

void GBA::tick()
{
	uint32_t cycles = cpu.tick();

}