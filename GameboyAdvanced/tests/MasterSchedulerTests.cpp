#include "ClockedDevice.h"
#include "GBA.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
struct Probe final : ClockedDevice
{
    uint64_t cycles = 0;
    std::vector<uint32_t> deltas;
    void advance(uint32_t elapsed) override
    {
        cycles += elapsed;
        deltas.push_back(elapsed);
    }
};

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

bool testCpuDeltasDriveAllDevices()
{
    constexpr const char* name = "cpu_deltas_drive_all_devices";
    GBA gba;
    Probe probe;
    gba.attachClockedDevice(probe);

    gba.cpu.pc = 0x03000000;
    gba.bus.write32(0x03000000, 0xE3A00001); // MOV r0, #1: 1 cycle
    gba.bus.write32(0x03000004, 0xEA000000); // B +0: 3 cycles

    EXPECT(name, gba.tick() == 1);
    EXPECT(name, gba.masterCycleCount() == 1);
    EXPECT(name, gba.ppu.cycleCount() == 1);
    EXPECT(name, probe.cycles == 1 && probe.deltas.size() == 1 && probe.deltas[0] == 1);

    EXPECT(name, gba.tick() == 3);
    EXPECT(name, gba.masterCycleCount() == 4);
    EXPECT(name, gba.ppu.cycleCount() == 4);
    EXPECT(name, probe.cycles == 4 && probe.deltas.size() == 2 && probe.deltas[1] == 3);
    return true;
}

bool testSchedulerResetAndNoDuplicateAttachment()
{
    constexpr const char* name = "scheduler_reset_and_no_duplicate_attachment";
    GBA gba;
    Probe probe;
    gba.attachClockedDevice(probe);
    gba.attachClockedDevice(probe);
    gba.cpu.pc = 0x03000000;
    gba.bus.write32(0x03000000, 0xE3A00000);
    gba.tick();
    EXPECT(name, probe.deltas.size() == 1);
    gba.reset();
    EXPECT(name, gba.masterCycleCount() == 0);
    EXPECT(name, gba.ppu.cycleCount() == 0);
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
        run("cpu_deltas_drive_all_devices", testCpuDeltasDriveAllDevices) &&
        run("scheduler_reset_and_no_duplicate_attachment", testSchedulerResetAndNoDuplicateAttachment);
    return passed ? 0 : 1;
}
