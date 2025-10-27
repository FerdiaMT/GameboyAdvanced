#pragma once
#include "CPU.h"
#include "Bus.h"

class GBA
{

public:

	Bus bus;
	CPU cpu;

	GBA();

	void tick();
};
