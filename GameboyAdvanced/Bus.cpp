#include "Bus.h"


Bus::Bus()
{
	biosRom = std::make_unique<uint8_t[]>(0x3FFF);

	memset(biosRom.get(), 0, 0x3FFF);
}

//====================
// READ FUNCTIONS
//====================

uint32_t Bus::read32(uint16_t addr, bool bReadOnly = false)
{
	if (addr >= 0x3FFF)
	{
		return (*(uint8_t*)&biosRom[addr + 3] << 8) | (*(uint8_t*)&biosRom[addr + 2] << 8) | (*(uint8_t*)&biosRom[addr + 1] << 8) | *(uint8_t*)&biosRom[addr];
	}
}

uint16_t Bus::read16(uint16_t addr, bool bReadOnly = false)
{
	if (addr >= 0x3FFF)
	{
		return (*(uint8_t*)&biosRom[addr + 1] << 8) | *(uint8_t*)&biosRom[addr];
	}
}


uint8_t Bus::read8(uint16_t addr, bool bReadOnly = false)
{
	if (addr >= 0x3FFF)
	{
		return *(uint8_t*)&biosRom[addr];
	}
}

//====================
// WRITE FUNCTIONS
//====================

void Bus::write8(uint16_t addr, uint8_t data)
{
	if (addr >= 0x3FFF)
	{
		biosRom[addr] = data;
	}
}

void Bus::write16(uint16_t addr, uint16_t data)
{
	if (addr >= 0x3FFF)
	{
		biosRom[addr] = data & 0xFF;
		biosRom[addr+1] = (data>>8) & 0xFF ;
	}
}

void Bus::write32(uint16_t addr, uint16_t data)
{
	if (addr >= 0x3FFF)
	{
		biosRom[addr] = data & 0xFF;
		biosRom[addr + 1] = (data >> 8) & 0xFF;
		biosRom[addr + 2] = (data >> 16) & 0xFF;
		biosRom[addr + 3] = (data >> 24) & 0xFF;
	}
}