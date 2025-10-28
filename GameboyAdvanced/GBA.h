#pragma once
#include "CPU.h"
#include "Bus.h"
#include "DebuggerCPU.h";

class GBA
{

public:

	Bus bus;
	CPU cpu;
	DebuggerCPU debuggerCPU;

	GBA();

	void tick();
};
