#define _CRT_SECURE_NO_WARNINGS

#include "Bus.h"


Bus::Bus()
{
    memorySize = 0x10000000; 
    biosRom = std::make_unique<uint8_t[]>(memorySize);
    memset(biosRom.get(), 0, memorySize);
}

//====================
// READ FUNCTIONS
//====================
uint32_t Bus::read32(uint32_t addr, bool bReadOnly)
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

uint16_t Bus::read16(uint32_t addr, bool bReadOnly)
{
    if (addr < memorySize - 1)
    {
        return (biosRom[addr + 1] << 8) | biosRom[addr];
    }
    return 0;
}

uint8_t Bus::read8(uint32_t addr, bool bReadOnly)
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
}

void Bus::write16(uint32_t addr, uint16_t data)
{
    if (addr < memorySize - 1)
    {
        biosRom[addr] = data & 0xFF;
        biosRom[addr + 1] = (data >> 8) & 0xFF;
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