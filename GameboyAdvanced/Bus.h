#pragma once
#include <cstdint>
#include <memory>
class Bus
{
private:
	std::unique_ptr<uint8_t[]> biosRom;
	size_t memorySize;
public:

	Bus();

	bool loadROM(const char* filename , uint32_t loadAddr);


	uint8_t read8(uint32_t addr, bool bReadOnly = false);
	uint16_t read16(uint32_t addr,bool bReadOnly = false);
	uint32_t read32(uint32_t addr, bool bReadOnly = false);

	void write8(uint32_t addr, uint8_t data);
	void write16(uint32_t addr, uint16_t data);
	void write32(uint32_t addr, uint32_t data);

};

