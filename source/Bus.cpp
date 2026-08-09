#define _CRT_SECURE_NO_WARNINGS

#include "Bus.h"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <string>

void Bus::enableCpuTimingTrace(bool enabled)
{
    cpuTimingTraceEnabled = enabled;
}

bool Bus::isCpuTimingTraceEnabled() const
{
    return cpuTimingTraceEnabled;
}

void Bus::setCpuTimingConfig(CpuTimingConfig config)
{
    cpuTimingConfig = config;
}

void Bus::enableGbaRegionTiming(bool enabled)
{
    gbaRegionTimingEnabled = enabled;
}

bool Bus::isGbaRegionTimingEnabled() const
{
    return gbaRegionTimingEnabled;
}

void Bus::setWaitcnt(uint16_t value)
{
    // Bit 15 is read-only on GBA hardware and reads as zero for a GBA cart.
    waitcnt = value & 0x7FFFU;
}

uint16_t Bus::getWaitcnt() const
{
    return waitcnt;
}

void Bus::clearCpuTimingTrace()
{
    cpuTimingEvents.clear();
    hasPreviousCpuAccess = false;
    previousCpuAddress = 0;
    previousCpuWidth = 0;
}

const std::vector<Bus::CpuTimingEvent>& Bus::cpuTimingTrace() const
{
    return cpuTimingEvents;
}

uint64_t Bus::cpuTimingCyclesSince(size_t startIndex) const
{
    uint64_t cycles = 0;
    for (size_t index = startIndex; index < cpuTimingEvents.size(); ++index)
    {
        cycles += cpuTimingEvents[index].cycles;
    }
    return cycles;
}

size_t Bus::cpuTimingExternalAccessCountSince(size_t startIndex) const
{
    size_t count = 0;
    for (size_t index = startIndex; index < cpuTimingEvents.size(); ++index)
    {
        if (cpuTimingEvents[index].access != CpuTimingAccess::Internal)
        {
            ++count;
        }
    }
    return count;
}

void Bus::recordCpuAccess(CpuTimingAccess access, uint32_t address, uint8_t width)
{
    if (!cpuTimingTraceEnabled)
    {
        return;
    }

    const CpuTimingRegion region = cpuTimingRegion(address);
    if (gbaRegionTimingEnabled && isGamePakRegion(region))
    {
        // The Game Pak bus is 16-bit wide.  A CPU word transfer therefore
        // becomes two halfword transfers; the latter is sequential unless it
        // begins a new forced-NonSequential 128 KiB block.
        const uint32_t firstAddress = address & ~1U;
        recordCpuExternalAccess(access, firstAddress, 2, region);
        if (width == 4)
        {
            const uint32_t secondAddress = firstAddress + 2;
            recordCpuExternalAccess(access, secondAddress, 2, region, true);
        }
        return;
    }

    recordCpuExternalAccess(access, address, width, region);
}

void Bus::recordCpuInternal(uint32_t cycles)
{
    if (!cpuTimingTraceEnabled || cycles == 0)
    {
        return;
    }

    cpuTimingEvents.push_back({ CpuTimingAccess::Internal, 0, 0, false, cycles, CpuTimingRegion::Other });
}

Bus::CpuTimingRegion Bus::cpuTimingRegion(uint32_t address) const
{
    if (address <= 0x00003FFFU) return CpuTimingRegion::Bios;
    if (address >= 0x02000000U && address <= 0x0203FFFFU) return CpuTimingRegion::Ewram;
    if (address >= 0x03000000U && address <= 0x03007FFFU) return CpuTimingRegion::Iwram;
    if (address >= 0x04000000U && address <= 0x040003FFU) return CpuTimingRegion::Io;
    if (address >= 0x05000000U && address <= 0x050003FFU) return CpuTimingRegion::Palette;
    if (address >= 0x06000000U && address <= 0x06017FFFU) return CpuTimingRegion::Vram;
    if (address >= 0x07000000U && address <= 0x070003FFU) return CpuTimingRegion::Oam;
    if (address >= 0x08000000U && address <= 0x09FFFFFFU) return CpuTimingRegion::GamePak0;
    if (address >= 0x0A000000U && address <= 0x0BFFFFFFU) return CpuTimingRegion::GamePak1;
    if (address >= 0x0C000000U && address <= 0x0DFFFFFFU) return CpuTimingRegion::GamePak2;
    if (address >= 0x0E000000U && address <= 0x0E00FFFFU) return CpuTimingRegion::Sram;
    return CpuTimingRegion::Other;
}

bool Bus::isGamePakRegion(CpuTimingRegion region) const
{
    return region == CpuTimingRegion::GamePak0 ||
        region == CpuTimingRegion::GamePak1 ||
        region == CpuTimingRegion::GamePak2;
}

bool Bus::isGamePakBoundary(uint32_t address) const
{
    return (address & 0x1FFFFU) == 0;
}

uint32_t Bus::gamePakAccessCycles(CpuTimingRegion region, bool sequential) const
{
    static constexpr uint32_t firstWaitstates[] = { 4, 3, 2, 8 };
    const uint32_t waitState = region == CpuTimingRegion::GamePak0 ? 0 :
        region == CpuTimingRegion::GamePak1 ? 1 : 2;
    const uint32_t firstBits = (waitcnt >> (2 + waitState * 3)) & 3U;
    const uint32_t sequentialBit = (waitcnt >> (4 + waitState * 3)) & 1U;
    static constexpr uint32_t sequentialWaitstates[3][2] = {
        { 2, 1 },
        { 4, 1 },
        { 8, 1 },
    };
    const uint32_t waitstates = sequential
        ? sequentialWaitstates[waitState][sequentialBit]
        : firstWaitstates[firstBits];
    return 1 + waitstates;
}

uint32_t Bus::gbaAccessCycles(CpuTimingRegion region, bool sequential, uint8_t width) const
{
    switch (region)
    {
    case CpuTimingRegion::Bios:
    case CpuTimingRegion::Iwram:
    case CpuTimingRegion::Io:
        return 1;
    case CpuTimingRegion::Ewram:
        return width == 4 ? 6 : 3;
    case CpuTimingRegion::Palette:
    case CpuTimingRegion::Vram:
    case CpuTimingRegion::Oam:
        return width == 4 ? 2 : 1;
    case CpuTimingRegion::GamePak0:
    case CpuTimingRegion::GamePak1:
    case CpuTimingRegion::GamePak2:
        return gamePakAccessCycles(region, sequential);
    case CpuTimingRegion::Sram:
    {
        static constexpr uint32_t waitstates[] = { 4, 3, 2, 8 };
        return 1 + waitstates[waitcnt & 3U];
    }
    case CpuTimingRegion::Other:
        return sequential ? cpuTimingConfig.sequentialCycles : cpuTimingConfig.nonSequentialCycles;
    }
    return cpuTimingConfig.nonSequentialCycles;
}

uint32_t Bus::dmaAccessCycles(uint32_t address, uint8_t width, bool sequential) const
{
    const CpuTimingRegion region = cpuTimingRegion(address);
    if (isGamePakRegion(region) && width == 4)
        return gamePakAccessCycles(region, sequential) + gamePakAccessCycles(region, true);
    return gbaAccessCycles(region, sequential, width);
}

void Bus::recordCpuExternalAccess(CpuTimingAccess access, uint32_t address, uint8_t width,
    CpuTimingRegion region, bool forceSequential)
{
    const bool contiguous = hasPreviousCpuAccess &&
        region == cpuTimingRegion(previousCpuAddress) &&
        address == previousCpuAddress + previousCpuWidth;
    const bool sequential = (!gbaRegionTimingEnabled || !isGamePakRegion(region) || !isGamePakBoundary(address)) &&
        (forceSequential || contiguous);
    const uint32_t cycles = gbaRegionTimingEnabled
        ? gbaAccessCycles(region, sequential, width)
        : (sequential ? cpuTimingConfig.sequentialCycles : cpuTimingConfig.nonSequentialCycles);

    cpuTimingEvents.push_back({ access, address, width, sequential, cycles, region });
    hasPreviousCpuAccess = true;
    previousCpuAddress = address;
    previousCpuWidth = width;
}

Bus::Bus()
{
    // A freshly erased cartridge save device reads as all ones.  Games use
    // this state to distinguish a new save from a valid existing one.
    save.fill(0xFF);
}

bool Bus::loadBios(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    std::array<uint8_t, BiosSize> image{};
    const size_t bytes = fread(image.data(), 1, image.size(), file);
    const bool noExtraBytes = fgetc(file) == EOF;
    fclose(file);
    if (bytes != image.size() || !noExtraBytes) return false;
    bios = image;
    loadedBios = true;
    return true;
}

void Bus::loadBiosImage(const uint8_t* data, size_t size)
{
    if (data == nullptr || size != BiosSize) return;
    std::copy(data, data + BiosSize, bios.begin());
    loadedBios = true;
}

bool Bus::hasBios() const { return loadedBios; }

//====================
// READ FUNCTIONS
//====================
uint32_t Bus::read32(uint32_t addr, bool)
{
    return static_cast<uint32_t>(readMapped8(addr)) |
        (static_cast<uint32_t>(readMapped8(addr + 1)) << 8) |
        (static_cast<uint32_t>(readMapped8(addr + 2)) << 16) |
        (static_cast<uint32_t>(readMapped8(addr + 3)) << 24);
}

uint16_t Bus::read16(uint32_t addr, bool)
{
	if (isEepromAddress(addr)) return readEepromBit();
    return static_cast<uint16_t>(readMapped8(addr)) |
        (static_cast<uint16_t>(readMapped8(addr + 1)) << 8);
}

uint8_t Bus::read8(uint32_t addr, bool)
{
    return readMapped8(addr);
}

//====================
// WRITE FUNCTIONS
//====================
void Bus::write8(uint32_t addr, uint8_t data)
{
	writeMapped8(addr, data);
    if (addr == 0x04000204U)
    {
        setWaitcnt((waitcnt & 0xFF00U) | data);
		io[0x204] = static_cast<uint8_t>(waitcnt);
    }
    else if (addr == 0x04000205U)
    {
        setWaitcnt((waitcnt & 0x00FFU) | (static_cast<uint16_t>(data) << 8));
		io[0x205] = static_cast<uint8_t>(waitcnt >> 8);
    }
}

void Bus::write16(uint32_t addr, uint16_t data)
{
	if (isEepromAddress(addr))
	{
		writeEepromBit(static_cast<uint8_t>(data & 1U));
		return;
	}
	writeMapped8(addr, static_cast<uint8_t>(data));
	writeMapped8(addr + 1, static_cast<uint8_t>(data >> 8));
    if (addr == 0x04000204U)
    {
        setWaitcnt(data);
		io[0x204] = static_cast<uint8_t>(waitcnt);
		io[0x205] = static_cast<uint8_t>(waitcnt >> 8);
    }
}

void Bus::write32(uint32_t addr, uint32_t data) 
{
	writeMapped8(addr, static_cast<uint8_t>(data));
	writeMapped8(addr + 1, static_cast<uint8_t>(data >> 8));
	writeMapped8(addr + 2, static_cast<uint8_t>(data >> 16));
	writeMapped8(addr + 3, static_cast<uint8_t>(data >> 24));

    if (addr == 0x04000204U)
    {
        setWaitcnt(static_cast<uint16_t>(data));
		io[0x204] = static_cast<uint8_t>(waitcnt);
		io[0x205] = static_cast<uint8_t>(waitcnt >> 8);
    }

}


//====================
// LOADROM
//====================

bool Bus::loadROM(const char* filename, uint32_t loadAddr)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("Failed to open ROM file: %s\n", filename);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (loadAddr < 0x08000000U || loadAddr >= 0x0E000000U)
    {
        fclose(file);
        return false;
    }

    const size_t offset = cartridgeOffset(loadAddr);
    if (offset + fileSize > 0x02000000U)
    {
        fclose(file);
        return false;
    }
    cartridgeRom.clear();
    cartridgeRom.resize(offset + fileSize);
    size_t bytesRead = fread(cartridgeRom.data() + offset, 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize)
    {
        return false;
    }

	detectSaveType();

    printf("rom loaded\n");
    return true;
}

void Bus::loadCartridgeImage(const uint8_t* data, size_t size, uint32_t loadAddr)
{
    if (data == nullptr || loadAddr < 0x08000000U || loadAddr >= 0x0E000000U)
        return;
    const size_t offset = cartridgeOffset(loadAddr);
    if (offset + size > 0x02000000U)
        return;
    cartridgeRom.resize(std::max(cartridgeRom.size(), offset + size));
    std::copy(data, data + size, cartridgeRom.begin() + offset);
	detectSaveType();
}

Bus::SaveType Bus::saveType() const { return detectedSaveType; }
void Bus::setSaveType(SaveType type) { detectedSaveType = type; }
bool Bus::isGamePakPrefetchEnabled() const { return (waitcnt & 0x4000U) != 0; }
void Bus::setIoHandlers(IoReadHandler readHandler, IoWriteHandler writeHandler)
{
	ioReadHandler = std::move(readHandler);
	ioWriteHandler = std::move(writeHandler);
}

void Bus::resetSystemState()
{
	io.fill(0);
	waitcnt = 0;
	resetEepromTransfer();
	clearCpuTimingTrace();
}

uint32_t Bus::vramOffset(uint32_t address)
{
    uint32_t offset = address & 0x1FFFFU;
    // The 96 KiB VRAM block mirrors its final 32 KiB over 0x18000..0x1FFFF.
    if (offset >= VramSize)
        offset = 0x10000U + (offset & 0x7FFFU);
    return offset;
}

uint32_t Bus::cartridgeOffset(uint32_t address)
{
    // Each 32 MiB Game Pak wait-state window exposes the same cartridge
    // address range.
    return address & 0x01FFFFFFU;
}

uint32_t Bus::saveOffset(uint32_t address) const
{
	const uint32_t mask = detectedSaveType == SaveType::Sram ? 0x7FFFU :
		detectedSaveType == SaveType::Flash128 ? 0x1FFFFU : 0xFFFFU;
	return address & mask;
}

bool Bus::isEepromAddress(uint32_t address) const
{
	return (detectedSaveType == SaveType::Eeprom512 || detectedSaveType == SaveType::Eeprom8K) &&
		address >= 0x0D000000U && address < 0x0E000000U;
}

void Bus::resetEepromTransfer()
{
	eepromPhase = EepromPhase::Idle;
	eepromReadCommand = false;
	eepromAddressBits = 0;
	eepromAddress = 0;
	eepromDataBits = 0;
	eepromData = 0;
	eepromReadDummyBits = 0;
	eepromReadDataBits = 0;
	eepromReadAddress = 0;
}

void Bus::writeEepromBit(uint8_t bit)
{
	bit &= 1U;
	// A command always starts with one.  Once a read transfer has completed,
	// the next DMA write begins a fresh command without requiring an explicit
	// reset from the CPU.
	if (eepromReadDummyBits != 0 || eepromReadDataBits != 0)
		resetEepromTransfer();

	const uint8_t addressWidth = detectedSaveType == SaveType::Eeprom8K ? 14 : 6;
	switch (eepromPhase)
	{
	case EepromPhase::Idle:
		if (bit != 0) eepromPhase = EepromPhase::Command;
		break;
	case EepromPhase::Command:
		// Serial commands are 10 (write) and 11 (read); the leading one was
		// consumed while leaving Idle.
		eepromReadCommand = bit != 0;
		eepromAddress = 0;
		eepromAddressBits = 0;
		eepromPhase = EepromPhase::Address;
		break;
	case EepromPhase::Address:
		eepromAddress = static_cast<uint16_t>((eepromAddress << 1) | bit);
		if (++eepromAddressBits == addressWidth)
		{
			eepromPhase = eepromReadCommand ? EepromPhase::Stop : EepromPhase::WriteData;
			eepromData = 0;
			eepromDataBits = 0;
		}
		break;
	case EepromPhase::WriteData:
		eepromData = (eepromData << 1) | bit;
		if (++eepromDataBits == 64) eepromPhase = EepromPhase::Stop;
		break;
	case EepromPhase::Stop:
		if (eepromReadCommand)
		{
			// Reads return four dummy zero bits followed by the stored 64 bits.
			eepromReadAddress = eepromAddress;
			eepromReadDummyBits = 4;
			eepromReadDataBits = 0;
		}
		else
		{
			const size_t offset = static_cast<size_t>(eepromAddress) * 8U;
			for (size_t index = 0; index < 8; ++index)
				save[offset + index] = static_cast<uint8_t>(eepromData >> (56U - index * 8U));
			resetEepromTransfer();
		}
		break;
	}
}

uint16_t Bus::readEepromBit()
{
	if (eepromReadDummyBits != 0)
	{
		--eepromReadDummyBits;
		return 0;
	}
	if (eepromReadDataBits >= 64) return 1;
	const size_t byteOffset = static_cast<size_t>(eepromReadAddress) * 8U + eepromReadDataBits / 8U;
	const uint16_t bit = (save[byteOffset] >> (7U - (eepromReadDataBits & 7U))) & 1U;
	++eepromReadDataBits;
	return bit;
}

void Bus::detectSaveType()
{
	// Save-library identifiers may appear anywhere in the Game Pak image.
	// Scanning separately for every identifier made loading SMA take four full
	// passes over the ROM. Inspect each possible identifier once, then apply the
	// same precedence as the former ordered searches.
	bool hasFlash1M = false;
	bool hasFlash = false;
	bool hasEeprom = false;
	bool hasSram = false;
	const auto matches = [this](size_t offset, const char* id, size_t length)
	{
		return offset + length <= cartridgeRom.size() &&
			std::memcmp(cartridgeRom.data() + offset, id, length) == 0;
	};

	for (size_t offset = 0; offset < cartridgeRom.size(); ++offset)
	{
		switch (cartridgeRom[offset])
		{
		case 'F':
			hasFlash1M = hasFlash1M || matches(offset, "FLASH1M_V", 9);
			hasFlash = hasFlash || matches(offset, "FLASH512_V", 10) || matches(offset, "FLASH_V", 7);
			break;
		case 'E': hasEeprom = hasEeprom || matches(offset, "EEPROM_V", 8); break;
		case 'S': hasSram = hasSram || matches(offset, "SRAM_V", 6); break;
		default: break;
		}
	}

	detectedSaveType = hasFlash1M ? SaveType::Flash128 :
		hasFlash ? SaveType::Flash64 :
		hasEeprom ? SaveType::Eeprom512 :
		hasSram ? SaveType::Sram : SaveType::Unknown;
}

uint8_t Bus::readMapped8(uint32_t address) const
{
    switch (address >> 24)
    {
    case 0x00: return address < BiosSize ? bios[address] : 0;
    case 0x02: return ewram[address & (EwramSize - 1)];
    case 0x03: return iwram[address & (IwramSize - 1)];
    case 0x04:
    {
        uint8_t value = 0;
        if (ioReadHandler && ioReadHandler(address, value)) return value;
        return io[address & (IoSize - 1)];
    }
    case 0x05: return palette[address & (PaletteSize - 1)];
    case 0x06: return vram[vramOffset(address)];
    case 0x07: return oam[address & (OamSize - 1)];
    case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
    {
        const uint32_t offset = cartridgeOffset(address);
        return offset < cartridgeRom.size() ? cartridgeRom[offset] : 0;
    }
    case 0x0E: return save[saveOffset(address)];
    default: return 0;
    }
}

void Bus::writeMapped8(uint32_t address, uint8_t data)
{
    switch (address >> 24)
    {
    // The 16 KiB BIOS mapping is ROM.  In particular, allowing a cartridge
    // DMA/write to alter the exception vectors makes the first game IRQ loop
    // forever instead of entering the real BIOS dispatcher.
    case 0x00: break;
    case 0x02: ewram[address & (EwramSize - 1)] = data; break;
    case 0x03: iwram[address & (IwramSize - 1)] = data; break;
    case 0x04:
        io[address & (IoSize - 1)] = data;
        if (ioWriteHandler) ioWriteHandler(address, data);
        break;
    case 0x05: palette[address & (PaletteSize - 1)] = data; break;
    case 0x06: vram[vramOffset(address)] = data; break;
    case 0x07: oam[address & (OamSize - 1)] = data; break;
    case 0x0E: save[saveOffset(address)] = data; break;
    default: break; // Cartridge ROM and unmapped regions are read-only/open bus.
    }
}
