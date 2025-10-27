#include "GBA.h"
#include "CPU.h"
#include <cstdint>

GBA::GBA(): cpu(&bus)
{

}

void GBA::tick()
{
	uint32_t cycles = cpu.tick();
}