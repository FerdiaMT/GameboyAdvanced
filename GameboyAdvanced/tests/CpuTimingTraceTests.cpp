#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/Decoder.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
struct Machine
{
    Bus bus;
    tdmi7::Decoder decoder;
    tdmi7::CPU cpu;

    Machine() : cpu(&bus, &decoder)
    {
        bus.setCpuTimingConfig({ 4, 1 });
        bus.enableCpuTimingTrace(true);
        bus.clearCpuTimingTrace();
    }
};

using Access = Bus::CpuTimingAccess;

struct ExpectedEvent
{
    Access access;
    uint32_t address;
    uint8_t width;
    bool sequential;
    uint32_t cycles;
    Bus::CpuTimingRegion region;
};

bool expect(bool condition, const char* expression, const char* testName)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << testName << ": " << expression << '\n';
        return false;
    }
    return true;
}

#define EXPECT(testName, expression) \
    do { if (!expect((expression), #expression, testName)) return false; } while (false)

bool expectTrace(const char* testName, const std::vector<Bus::CpuTimingEvent>& actual,
    const std::vector<ExpectedEvent>& expected)
{
    EXPECT(testName, actual.size() == expected.size());
    for (size_t index = 0; index < expected.size(); ++index)
    {
        const auto& got = actual[index];
        const auto& want = expected[index];
        if (got.access != want.access || got.address != want.address || got.width != want.width ||
            got.sequential != want.sequential || got.cycles != want.cycles || got.region != want.region)
        {
            std::cerr << "[FAIL] " << testName << ": timing event " << index << " differs"
                << " (got access=" << static_cast<int>(got.access)
                << " addr=0x" << std::hex << got.address << std::dec
                << " width=" << static_cast<int>(got.width)
                << " sequential=" << got.sequential
                << " cycles=" << got.cycles << ")\n";
            return false;
        }
    }
    return true;
}

bool testThumbSequentialFetches()
{
    constexpr const char* testName = "thumb_sequential_fetches";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.T = 1;
    machine.bus.write16(0x100, 0x202A); // MOV r0, #42
    machine.bus.write16(0x102, 0x3001); // ADD r0, #1

    EXPECT(testName, machine.cpu.tick() == 4);
    EXPECT(testName, machine.cpu.tick() == 5);
    EXPECT(testName, machine.cpu.reg[0] == 43);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x100, 2, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::InstructionFetch, 0x102, 2, true, 1, Bus::CpuTimingRegion::Bios },
    });
}

bool testThumbLoadIncludesInternalCycle()
{
    constexpr const char* testName = "thumb_load_includes_internal_cycle";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.T = 1;
    machine.cpu.reg[1] = 0x200;
    machine.bus.write16(0x100, 0x6808); // LDR r0, [r1, #0]
    machine.bus.write32(0x200, 0x12345678);

    EXPECT(testName, machine.cpu.tick() == 9);
    EXPECT(testName, machine.cpu.reg[0] == 0x12345678);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x100, 2, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::DataRead, 0x200, 4, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::Internal, 0, 0, false, 1, Bus::CpuTimingRegion::Other },
    });
}

bool testThumbStoreTrace()
{
    constexpr const char* testName = "thumb_store_trace";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.T = 1;
    machine.cpu.reg[0] = 0xCAFEBABE;
    machine.cpu.reg[1] = 0x200;
    machine.bus.write16(0x100, 0x6008); // STR r0, [r1, #0]

    EXPECT(testName, machine.cpu.tick() == 8);
    EXPECT(testName, machine.bus.read32(0x200) == 0xCAFEBABE);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x100, 2, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::DataWrite, 0x200, 4, false, 4, Bus::CpuTimingRegion::Bios },
    });
}

bool testArmSequentialFetches()
{
    constexpr const char* testName = "arm_sequential_fetches";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.bus.write32(0x100, 0xE3A0002A); // MOV r0, #42
    machine.bus.write32(0x104, 0xE2800001); // ADD r0, r0, #1

    EXPECT(testName, machine.cpu.tick() == 4);
    EXPECT(testName, machine.cpu.tick() == 5);
    EXPECT(testName, machine.cpu.reg[0] == 43);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x100, 4, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::InstructionFetch, 0x104, 4, true, 1, Bus::CpuTimingRegion::Bios },
    });
}

bool testArmLoadTrace()
{
    constexpr const char* testName = "arm_load_trace";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.reg[1] = 0x200;
    machine.bus.write32(0x100, 0xE5910000); // LDR r0, [r1]
    machine.bus.write32(0x200, 0x89ABCDEF);

    EXPECT(testName, machine.cpu.tick() == 9);
    EXPECT(testName, machine.cpu.reg[0] == 0x89ABCDEF);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x100, 4, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::DataRead, 0x200, 4, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::Internal, 0, 0, false, 1, Bus::CpuTimingRegion::Other },
    });
}

bool testThumbBranchRefillsFetch()
{
    constexpr const char* testName = "thumb_branch_refills_fetch";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.T = 1;
    machine.bus.write16(0x100, 0xE000); // B +0, skips 0x102
    machine.bus.write16(0x104, 0x202A); // MOV r0, #42

    EXPECT(testName, machine.cpu.tick() == 6);
    EXPECT(testName, machine.cpu.pc == 0x104);
    EXPECT(testName, machine.cpu.tick() == 10);
    EXPECT(testName, machine.cpu.reg[0] == 42);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x100, 2, false, 4, Bus::CpuTimingRegion::Bios },
        { Access::Internal, 0, 0, false, 2, Bus::CpuTimingRegion::Other },
        { Access::InstructionFetch, 0x104, 2, false, 4, Bus::CpuTimingRegion::Bios },
    });
}

bool testGbaEwramWordLoad()
{
    constexpr const char* testName = "gba_ewram_word_load";
    Machine machine;
    machine.bus.enableGbaRegionTiming(true);
    machine.bus.clearCpuTimingTrace();
    machine.cpu.pc = 0x03000000;
    machine.cpu.T = 1;
    machine.cpu.reg[1] = 0x02000000;
    machine.bus.write16(0x03000000, 0x6808); // LDR r0, [r1, #0]
    machine.bus.write32(0x02000000, 0xA5A5A5A5);

    EXPECT(testName, machine.cpu.tick() == 8);
    EXPECT(testName, machine.cpu.reg[0] == 0xA5A5A5A5);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x03000000, 2, false, 1, Bus::CpuTimingRegion::Iwram },
        { Access::DataRead, 0x02000000, 4, false, 6, Bus::CpuTimingRegion::Ewram },
        { Access::Internal, 0, 0, false, 1, Bus::CpuTimingRegion::Other },
    });
}

bool testGamePakWordFetchSplitsIntoHalfwords()
{
    constexpr const char* testName = "gamepak_word_fetch_splits_into_halfwords";
    Machine machine;
    machine.bus.enableGbaRegionTiming(true);
    machine.bus.clearCpuTimingTrace();
    machine.cpu.pc = 0x08000000;
    machine.bus.write32(0x08000000, 0xE3A0002A); // MOV r0, #42
    machine.bus.write32(0x08000004, 0xE2800001); // ADD r0, r0, #1

    EXPECT(testName, machine.cpu.tick() == 8);
    EXPECT(testName, machine.cpu.tick() == 14);
    EXPECT(testName, machine.cpu.reg[0] == 43);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x08000000, 2, false, 5, Bus::CpuTimingRegion::GamePak0 },
        { Access::InstructionFetch, 0x08000002, 2, true, 3, Bus::CpuTimingRegion::GamePak0 },
        { Access::InstructionFetch, 0x08000004, 2, true, 3, Bus::CpuTimingRegion::GamePak0 },
        { Access::InstructionFetch, 0x08000006, 2, true, 3, Bus::CpuTimingRegion::GamePak0 },
    });
}

bool testWaitcntChangesGamePakTiming()
{
    constexpr const char* testName = "waitcnt_changes_gamepak_timing";
    Machine machine;
    machine.bus.enableGbaRegionTiming(true);
    machine.bus.write16(0x04000204, 0x0014); // WS0: 3 waitstates N, 1 waitstate S
    machine.bus.clearCpuTimingTrace();
    machine.cpu.pc = 0x08000000;
    machine.bus.write32(0x08000000, 0xE3A0002A); // MOV r0, #42

    EXPECT(testName, machine.bus.getWaitcnt() == 0x0014);
    EXPECT(testName, machine.cpu.tick() == 6);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x08000000, 2, false, 4, Bus::CpuTimingRegion::GamePak0 },
        { Access::InstructionFetch, 0x08000002, 2, true, 2, Bus::CpuTimingRegion::GamePak0 },
    });
}

bool testGamePakBoundaryForcesNonSequential()
{
    constexpr const char* testName = "gamepak_boundary_forces_nonsequential";
    Machine machine;
    machine.bus.enableGbaRegionTiming(true);
    machine.bus.clearCpuTimingTrace();
    machine.cpu.pc = 0x0801FFFE;
    machine.bus.write32(0x0801FFFE, 0xE3A0002A); // MOV r0, #42 across a 128 KiB boundary

    EXPECT(testName, machine.cpu.tick() == 10);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x0801FFFE, 2, false, 5, Bus::CpuTimingRegion::GamePak0 },
        { Access::InstructionFetch, 0x08020000, 2, false, 5, Bus::CpuTimingRegion::GamePak0 },
    });
}

bool testGamePakMirrorsUseIndependentWaitstates()
{
    constexpr const char* testName = "gamepak_mirrors_use_independent_waitstates";

    Machine waitState1;
    waitState1.bus.enableGbaRegionTiming(true);
    waitState1.bus.clearCpuTimingTrace();
    waitState1.cpu.pc = 0x0A000000;
    waitState1.bus.write32(0x0A000000, 0xE3A0002A); // MOV r0, #42
    EXPECT(testName, waitState1.cpu.tick() == 10); // WS1: 5N + 5S by default
    EXPECT(testName, waitState1.bus.cpuTimingTrace()[0].region == Bus::CpuTimingRegion::GamePak1);
    EXPECT(testName, waitState1.bus.cpuTimingTrace()[1].cycles == 5);

    Machine waitState2;
    waitState2.bus.enableGbaRegionTiming(true);
    waitState2.bus.clearCpuTimingTrace();
    waitState2.cpu.pc = 0x0C000000;
    waitState2.bus.write32(0x0C000000, 0xE3A0002A); // MOV r0, #42
    EXPECT(testName, waitState2.cpu.tick() == 14); // WS2: 5N + 9S by default
    EXPECT(testName, waitState2.bus.cpuTimingTrace()[0].region == Bus::CpuTimingRegion::GamePak2);
    EXPECT(testName, waitState2.bus.cpuTimingTrace()[1].cycles == 9);
    return true;
}

bool testGamePakDataLoadFromIwram()
{
    constexpr const char* testName = "gamepak_data_load_from_iwram";
    Machine machine;
    machine.bus.enableGbaRegionTiming(true);
    machine.bus.clearCpuTimingTrace();
    machine.cpu.pc = 0x03000000;
    machine.cpu.T = 1;
    machine.cpu.reg[1] = 0x08000000;
    machine.bus.write16(0x03000000, 0x6808); // LDR r0, [r1, #0]
    machine.bus.write32(0x08000000, 0x12345678);

    EXPECT(testName, machine.cpu.tick() == 9);
    EXPECT(testName, machine.cpu.reg[0] == 0x12345678);
    return expectTrace(testName, machine.bus.cpuTimingTrace(), {
        { Access::InstructionFetch, 0x03000000, 2, false, 1, Bus::CpuTimingRegion::Iwram },
        { Access::DataRead, 0x08000000, 2, false, 5, Bus::CpuTimingRegion::GamePak0 },
        { Access::DataRead, 0x08000002, 2, true, 3, Bus::CpuTimingRegion::GamePak0 },
    });
}

using Test = bool (*)();

bool runTest(const char* name, Test test)
{
    if (!test())
    {
        return false;
    }
    std::cout << "[PASS] " << name << '\n';
    return true;
}
}

int main()
{
    const bool passed =
        runTest("thumb_sequential_fetches", testThumbSequentialFetches) &&
        runTest("thumb_load_includes_internal_cycle", testThumbLoadIncludesInternalCycle) &&
        runTest("thumb_store_trace", testThumbStoreTrace) &&
        runTest("arm_sequential_fetches", testArmSequentialFetches) &&
        runTest("arm_load_trace", testArmLoadTrace) &&
        runTest("thumb_branch_refills_fetch", testThumbBranchRefillsFetch) &&
        runTest("gba_ewram_word_load", testGbaEwramWordLoad) &&
        runTest("gamepak_word_fetch_splits_into_halfwords", testGamePakWordFetchSplitsIntoHalfwords) &&
        runTest("waitcnt_changes_gamepak_timing", testWaitcntChangesGamePakTiming) &&
        runTest("gamepak_boundary_forces_nonsequential", testGamePakBoundaryForcesNonSequential) &&
        runTest("gamepak_mirrors_use_independent_waitstates", testGamePakMirrorsUseIndependentWaitstates) &&
        runTest("gamepak_data_load_from_iwram", testGamePakDataLoadFromIwram);

    return passed ? 0 : 1;
}
