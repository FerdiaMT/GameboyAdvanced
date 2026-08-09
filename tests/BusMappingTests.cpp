#include "Bus.h"

#include <array>
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
    std::array<uint8_t, 0x4000> bios{};
    bios[0x100] = 0x10;
    bus.loadBiosImage(bios.data(), bios.size());
    bus.write8(0x02000001, 0x20);
    bus.write8(0x03000002, 0x30);
    bus.write8(0x04000003, 0x40);
    bus.write8(0x05000004, 0x50);
    bus.write8(0x06010000, 0x60);
    bus.write8(0x07000006, 0x70);
    bus.write8(0x0E000007, 0x80);

    EXPECT(name, bus.read8(0x00000100) == 0x10);
    bus.write8(0x00000100, 0x99);
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

bool testSaveTypeDetection()
{
    constexpr const char* name = "save_type_detection";
    const auto detect = [](const char* image, size_t size)
    {
        Bus bus;
        bus.loadCartridgeImage(reinterpret_cast<const uint8_t*>(image), size);
        return bus.saveType();
    };

    EXPECT(name, detect("FLASH_V123", 10) == Bus::SaveType::Flash64);
    EXPECT(name, detect("FLASH512_V123", 13) == Bus::SaveType::Flash64);
    EXPECT(name, detect("EEPROM_V123", 11) == Bus::SaveType::Eeprom512);
    EXPECT(name, detect("SRAM_V123", 9) == Bus::SaveType::Sram);
    // Preserve the former global precedence rather than choosing the first
    // identifier encountered while scanning the ROM.
    EXPECT(name, detect("SRAM_V123FLASH1M_V123", 21) == Bus::SaveType::Flash128);
    return true;
}

bool testEepromSerialProtocol()
{
    constexpr const char* name = "eeprom_serial_protocol";
    Bus bus;
    const uint8_t eepromId[] = { 'E','E','P','R','O','M','_','V','1','2','0' };
    bus.loadCartridgeImage(eepromId, sizeof(eepromId));
    EXPECT(name, bus.saveType() == Bus::SaveType::Eeprom512);

    constexpr uint32_t port = 0x0DFFFF00;
    constexpr uint8_t address = 0x15;
    constexpr uint64_t value = 0x0123456789ABCDEFULL;
    auto send = [&](uint8_t bit) { bus.write16(port, bit); };
    send(1); send(0); // write command: 10
    for (int bit = 5; bit >= 0; --bit) send(address >> bit);
    for (int bit = 63; bit >= 0; --bit) send(static_cast<uint8_t>(value >> bit));
    send(0); // stop bit

    send(1); send(1); // read command: 11
    for (int bit = 5; bit >= 0; --bit) send(address >> bit);
    send(0); // stop bit
    for (int count = 0; count < 4; ++count) EXPECT(name, bus.read16(port) == 0);
    uint64_t received = 0;
    for (int count = 0; count < 64; ++count) received = (received << 1) | bus.read16(port);
    EXPECT(name, received == value);
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
        run("waitcnt_lives_in_io", testWaitcntLivesInIo) &&
		run("save_type_detection", testSaveTypeDetection) &&
        run("eeprom_serial_protocol", testEepromSerialProtocol);
    return passed ? 0 : 1;
}
