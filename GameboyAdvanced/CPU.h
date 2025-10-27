#pragma once
#include "Bus.h"
#include <cstdint>
class CPU
{
	public:
		Bus* bus;
		CPU(Bus*);

		uint32_t tick();

		uint32_t reg[14];

		//15 resevred for pc which i declare seperately
		uint32_t pc;

		//current program status registers
		union
		{
			struct
			{ //declared from bit 0 onwards
				bool M0;
				bool M1;
				bool M2;
				bool M3;
				bool M4;
				bool T;
				bool F;
				bool I;//7
				//JUNK FOR 8 TO 27
				uint16_t RESERVED;//8 - 23
				bool RESERVED1, RESERVED2, RESERVED3, RESERVED4;//24,25,26,27
				bool V; // overflow flag
				bool C; // carry flag
				bool Z; // zero flag
				bool N; // sign flag

			};
			uint32_t CPSR;
		};

		uint32_t spsr;
};

