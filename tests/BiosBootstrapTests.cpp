#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/Decoder.h"

#include <array>
#include <iostream>

namespace
{
bool expect(bool condition, const char* expression)
{
    if (condition) return true;
    std::cerr << "[FAIL] bios_bootstrap_control_flow: " << expression << '\n';
    return false;
}

#define EXPECT(expression) do { if (!expect((expression), #expression)) return 1; } while (false)

void write16(std::array<uint8_t, 0x4000>& bios, uint32_t address, uint16_t value)
{
    bios[address] = static_cast<uint8_t>(value);
    bios[address + 1] = static_cast<uint8_t>(value >> 8);
}

void write32(std::array<uint8_t, 0x4000>& bios, uint32_t address, uint32_t value)
{
    write16(bios, address, static_cast<uint16_t>(value));
    write16(bios, address + 2, static_cast<uint16_t>(value >> 16));
}
}

int main()
{
    // This is a copyright-free, deliberately reduced form of the real GBA
    // BIOS handoff.  It guards the sequence that transfers from ARM reset
    // code into Thumb initialization, then returns to ARM code.
    Bus bus;
    tdmi7::Decoder decoder;
    tdmi7::CPU cpu(&bus, &decoder);
    std::array<uint8_t, 0x4000> bios{};
    write32(bios, 0x00000000, 0xEA000043); // B 0x114
    write32(bios, 0x00000114, 0xE28F0001); // ADD r0, pc, #1
    write32(bios, 0x00000118, 0xE12FFF10); // BX r0
    write16(bios, 0x0000011C, 0x2142);     // Thumb: MOV r1, #0x42
    write16(bios, 0x0000011E, 0x4770);     // Thumb: BX lr
    write32(bios, 0x00000130, 0xE3A02077); // ARM: MOV r2, #0x77
    write16(bios, 0x000001C0, 0x467B);     // Thumb: MOV r3, pc
    write16(bios, 0x000001C2, 0x4718);     // Thumb: BX r3
    write32(bios, 0x000001C4, 0xE3A04033); // ARM: MOV r4, #0x33
    bus.loadBiosImage(bios.data(), bios.size());
    cpu.reset();
    cpu.pc = 0;
    cpu.lr = 0x130;

    cpu.tick();
    EXPECT(cpu.pc == 0x114 && cpu.T == 0);
    cpu.tick();
    EXPECT(cpu.reg[0] == 0x11D && cpu.pc == 0x118);
    cpu.tick();
    EXPECT(cpu.T == 1 && cpu.pc == 0x11C);
    cpu.tick();
    EXPECT(cpu.reg[1] == 0x42 && cpu.pc == 0x11E);
    cpu.tick();
    EXPECT(cpu.T == 0 && cpu.pc == 0x130);
    cpu.tick();
    EXPECT(cpu.reg[2] == 0x77 && cpu.pc == 0x134);

    // A real BIOS helper uses MOV high-register with r15 followed by BX to
    // switch out of Thumb state.  The source PC must be instruction + 4.
    tdmi7::CPU thumbCpu(&bus, &decoder);
    thumbCpu.reset();
    thumbCpu.T = 1;
    thumbCpu.pc = 0x1C0;
    thumbCpu.tick();
    EXPECT(thumbCpu.reg[3] == 0x1C4 && thumbCpu.pc == 0x1C2);
    thumbCpu.tick();
    EXPECT(thumbCpu.T == 0 && thumbCpu.pc == 0x1C4);
    thumbCpu.tick();
    EXPECT(thumbCpu.reg[4] == 0x33 && thumbCpu.pc == 0x1C8);

    std::cout << "[PASS] bios_bootstrap_control_flow\n";
    return 0;
}
