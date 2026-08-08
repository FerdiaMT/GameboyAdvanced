#include "tdmi7/CPU.h"
#include "tdmi7/LegacyTestMemory.h"

namespace tdmi7
{
namespace legacy
{
std::vector<Transaction> transactions;
uint32_t testBaseAddress = 0;
uint16_t testThumbOpcode = 0;
}

uint8_t CPU::read8(uint32_t address, bool)
{
	for (const auto& transaction : tdmi7::legacy::transactions)
	{
		if (transaction.kind == 1 && transaction.addr == address && transaction.size == 1) return static_cast<uint8_t>(transaction.data);
	}
	if (address == tdmi7::legacy::testBaseAddress) return tdmi7::legacy::testThumbOpcode;
	return bus->read8(address);
}

uint16_t CPU::read16(uint32_t address, bool)
{
	for (const auto& transaction : tdmi7::legacy::transactions)
	{
		if (transaction.kind == 1 && transaction.addr == address && transaction.size == 2) return static_cast<uint16_t>(transaction.data);
	}
	if (address == tdmi7::legacy::testBaseAddress) return tdmi7::legacy::testThumbOpcode;
	return bus->read16(address);
}

uint32_t CPU::read32(uint32_t address, bool)
{
	for (const auto& transaction : tdmi7::legacy::transactions)
	{
		if (transaction.kind == 1 && transaction.addr == address && transaction.size == 4) return transaction.data;
	}
	if (address == tdmi7::legacy::testBaseAddress) return tdmi7::legacy::testThumbOpcode;
	return bus->read32(address);
}

void CPU::write8(uint32_t address, uint8_t data) { bus->write8(address, data); }
void CPU::write16(uint32_t address, uint16_t data) { bus->write16(address, data); }
void CPU::write32(uint32_t address, uint32_t data) { bus->write32(address & ~3U, data); }

}
