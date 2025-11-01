#include "GBA.h"
#include "CPU.h"
#include <cstdint>

//BUGS TO FIX WITH DECODER

// MSR NOT PROPERLY APPEARING FOR 3RD CASE

//LOAD / STORE BITS NOT PROPERLY HANDLED

//const char* rom = "thumb.gba";
const char* rom = "gba_bios.bin";

GBA::GBA(): cpu(&bus) , debuggerCPU(&cpu)
{
	if (!bus.loadROM(rom, 0x00000000))
	{
		printf("error with loading the binary tester");
		return;
	}

	//debuggerCPU.runAllThumbTests(cpu);

	//debuggerCPU.DecodeIns(0x00000000, 0x000120);

	cpu.runThumbTests();
}

void GBA::tick()
{
	//uint32_t cycles = cpu.tick();

}