#include "CPU.h"
#include <cstdint>

CPU::CPU(Bus* bus) : bus(bus)
{

}

uint32_t CPU::tick()
{
	fetch();
}