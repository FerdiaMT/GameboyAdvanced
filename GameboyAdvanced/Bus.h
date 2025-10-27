#pragma once
#include <cstdint>
#include <memory>
class Bus
{
public:

	Bus();

	std::unique_ptr<uint8_t[]> biosRom;

	uint8_t read8(uint16_t addr);
	uint16_t read16(uint16_t addr);
	uint32_t read32(uint16_t addr);


	void read8(uint16_t addr, uint8_t data);
	void read16(uint16_t addr, uint16_t data);
};

