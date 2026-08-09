#include "Bus.h"

#include <cstdint>
#include <iostream>

namespace
{
bool expect(bool condition, const char* expression, const char* test)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << test << ": " << expression << '\n';
        return false;
    }
    return true;
}

#define EXPECT(test, expression) do { if (!expect((expression), #expression, test)) return false; } while (false)

bool testMappedRegionsAndMirrors()
{
    constexpr const char* name = "mapped_regions_and_mirrors";
    Bus bus;
    bus.write8(0x00000100, 0x10);
    bus.write8(0x02000001, 0x20);
    bus.write8(0x03000002, 0x30);
    bus.write8(0x04000003, 0x40);
    bus.write8(0x05000004, 0x50);
    bus.write8(0x06010000, 0x60);
    bus.write8(0x07000006, 0x70);
    bus.write8(0x0E000007, 0x80);

    EXPECT(name, bus.read8(0x00000100) == 0x10);
    EXPECT(name, bus.read8(0x02040001) == 0x20);
    EXPECT(name, bus.read8(0x03008002) == 0x30);
    EXPECT(name, bus.read8(0x04000403) == 0x40);
    EXPECT(name, bus.read8(0x05000404) == 0x50);
    EXPECT(name, bus.read8(0x06018000) == 0x60);
    EXPECT(name, bus.read8(0x07000406) == 0x70);
    EXPECT(name, bus.read8(0x0E010007) == 0x80);
    EXPECT(name, bus.read8(0x01000000) == 0);
    return true;
}

bool testCartridgeIsReadOnlyAndMirrored()
{
    constexpr const char* name = "cartridge_is_read_only_and_mirrored";
    Bus bus;
    const uint8_t image[] = { 0x78, 0x56, 0x34, 0x12 };
    bus.loadCartridgeImage(image, sizeof(image));
    EXPECT(name, bus.read32(0x08000000) == 0x12345678);
    EXPECT(name, bus.read32(0x0A000000) == 0x12345678);
    EXPECT(name, bus.read32(0x0C000000) == 0x12345678);
    bus.write32(0x08000000, 0xDEADBEEF);
    EXPECT(name, bus.read32(0x08000000) == 0x12345678);
    return true;
}

bool testWaitcntLivesInIo()
{
    constexpr const char* name = "waitcnt_lives_in_io";
    Bus bus;
    bus.write16(0x04000204, 0xFFFF);
    EXPECT(name, bus.getWaitcnt() == 0x7FFF);
    EXPECT(name, bus.read16(0x04000204) == 0x7FFF);
    return true;
}

using Test = bool (*)();
bool run(const char* name, Test test)
{
    if (!test()) return false;
    std::cout << "[PASS] " << name << '\n';
    return true;
}
}

int main()
{
    const bool passed =
        run("mapped_regions_and_mirrors", testMappedRegionsAndMirrors) &&
        run("cartridge_is_read_only_and_mirrored", testCartridgeIsReadOnlyAndMirrored) &&
        run("waitcnt_lives_in_io", testWaitcntLivesInIo);
    return passed ? 0 : 1;
}
