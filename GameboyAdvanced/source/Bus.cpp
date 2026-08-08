#define _CRT_SECURE_NO_WARNINGS

#include "Bus.h"
#include <cstdio>
#include <cstring>

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
    memorySize = 0x10000000; 
    biosRom = std::make_unique<uint8_t[]>(memorySize);
    memset(biosRom.get(), 0, memorySize);
}

//====================
// READ FUNCTIONS
//====================
uint32_t Bus::read32(uint32_t addr, bool)
{
    if (addr < memorySize - 3)
    {  // Ensure we can read 4 bytes
        return (biosRom[addr + 3] << 24) |
            (biosRom[addr + 2] << 16) |
            (biosRom[addr + 1] << 8) |
            biosRom[addr];
    }
    return 0;  // Out of bounds
}

uint16_t Bus::read16(uint32_t addr, bool)
{
    if (addr < memorySize - 1)
    {
        return (biosRom[addr + 1] << 8) | biosRom[addr];
    }
    return 0;
}

uint8_t Bus::read8(uint32_t addr, bool)
{
    if (addr < memorySize)
    {
        return biosRom[addr];
    }
    return 0;
}

//====================
// WRITE FUNCTIONS
//====================
void Bus::write8(uint32_t addr, uint8_t data)
{
    if (addr < memorySize)
    {
        biosRom[addr] = data;
    }
    if (addr == 0x04000204U)
    {
        setWaitcnt((waitcnt & 0xFF00U) | data);
    }
    else if (addr == 0x04000205U)
    {
        setWaitcnt((waitcnt & 0x00FFU) | (static_cast<uint16_t>(data) << 8));
    }
}

void Bus::write16(uint32_t addr, uint16_t data)
{
    if (addr < memorySize - 1)
    {
        biosRom[addr] = data & 0xFF;
        biosRom[addr + 1] = (data >> 8) & 0xFF;
    }
    if (addr == 0x04000204U)
    {
        setWaitcnt(data);
    }
}

void Bus::write32(uint32_t addr, uint32_t data) 
{
    if (addr < memorySize - 3)
    {
        biosRom[addr] = data & 0xFF;
        biosRom[addr + 1] = (data >> 8) & 0xFF;
        biosRom[addr + 2] = (data >> 16) & 0xFF;
        biosRom[addr + 3] = (data >> 24) & 0xFF;
    }

    if (addr == 0x04000204U)
    {
        setWaitcnt(static_cast<uint16_t>(data));
    }

    if (addr == 0x03000000) // this is here for the arm tester
    {
        if (data == 0)
        {
            printf(" All tests passed!\n");
        }
        else
        {
            printf("Test failed: %d\n", data);
        }
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

    if (fileSize > memorySize - loadAddr)
    {
        fclose(file);
        return false;
    }

    size_t bytesRead = fread(&biosRom[loadAddr], 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize)
    {
        return false;
    }

    printf("rom loaded\n");
    return true;
}
