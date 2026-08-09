#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/Decoder.h"

#include <cstdint>
#include <iostream>

namespace
{
struct Machine
{
    Bus bus;
    tdmi7::Decoder decoder;
    tdmi7::CPU cpu;

    Machine() : cpu(&bus, &decoder) {}
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

bool testBusRoundTrip()
{
    constexpr const char* testName = "bus_round_trip";
    Machine machine;

    machine.bus.write32(0x02000000, 0x12345678);
    EXPECT(testName, machine.bus.read32(0x02000000) == 0x12345678);
    EXPECT(testName, machine.bus.read16(0x02000000) == 0x5678);
    EXPECT(testName, machine.bus.read8(0x02000003) == 0x12);
    return true;
}

bool testResetInitializesBankedStack()
{
    constexpr const char* testName = "reset_initializes_banked_stack";
    Machine machine;
    EXPECT(testName, machine.cpu.curMode == tdmi7::CPU::mode::Supervisor);
    EXPECT(testName, machine.cpu.sp == 0x03007F00);
    return true;
}

bool testArmMovAndAdd()
{
    constexpr const char* testName = "arm_mov_and_add";
    Machine machine;
    // BIOS is ROM; execute the synthetic test program from IWRAM instead.
    machine.cpu.pc = 0x03000000;
    machine.bus.write32(0x03000000, 0xE3A00020); // MOV r0, #0x20
    machine.bus.write32(0x03000004, 0xE2801022); // ADD r1, r0, #0x22

    EXPECT(testName, machine.cpu.tick() == 1);
    EXPECT(testName, machine.cpu.reg[0] == 0x20);
    EXPECT(testName, machine.cpu.pc == 0x03000004);
    EXPECT(testName, machine.cpu.tick() == 2);
    EXPECT(testName, machine.cpu.reg[1] == 0x42);
    EXPECT(testName, machine.cpu.pc == 0x03000008);
    return true;
}

bool testArmConditionalSkip()
{
    constexpr const char* testName = "arm_conditional_skip";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.Z = 1;
    machine.bus.write32(0x100, 0x13A000FF); // MOVNE r0, #0xFF

    machine.cpu.tick();
    EXPECT(testName, machine.cpu.reg[0] == 0);
    EXPECT(testName, machine.cpu.pc == 0x104);
    return true;
}

bool testArmPcRelativeLiteralLoad()
{
    constexpr const char* testName = "arm_pc_relative_literal_load";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.bus.write32(0x100, 0xE59F0000); // LDR r0, [pc, #0] reads from 0x108
    machine.bus.write32(0x108, 0xDEBB20E3);

    machine.cpu.tick();
    EXPECT(testName, machine.cpu.reg[0] == 0xDEBB20E3);
    EXPECT(testName, machine.cpu.pc == 0x104);
    return true;
}

bool testArmTestSwiHalt()
{
    constexpr const char* testName = "arm_test_swi_halt";

    Machine passMachine;
    passMachine.cpu.enableTestSwiHalt(true);
    passMachine.cpu.pc = 0x100;
    passMachine.bus.write32(0x100, 0xEF000000); // SWI #0: pass
    passMachine.cpu.tick();
    EXPECT(testName, passMachine.cpu.testHalt == tdmi7::CPU::TestHalt::Passed);
    EXPECT(testName, passMachine.cpu.pc == 0x104);

    Machine failMachine;
    failMachine.cpu.enableTestSwiHalt(true);
    failMachine.cpu.pc = 0x100;
    failMachine.bus.write32(0x100, 0xEF000001); // SWI #1: fail
    failMachine.cpu.tick();
    EXPECT(testName, failMachine.cpu.testHalt == tdmi7::CPU::TestHalt::Failed);
    EXPECT(testName, failMachine.cpu.pc == 0x104);
    return true;
}

bool testThumbMovAndAdd()
{
    constexpr const char* testName = "thumb_mov_and_add";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.T = 1;
    machine.bus.write16(0x100, 0x202A); // MOV r0, #0x2A
    machine.bus.write16(0x102, 0x3003); // ADD r0, #3

    EXPECT(testName, machine.cpu.tick() == 1);
    EXPECT(testName, machine.cpu.reg[0] == 0x2A);
    EXPECT(testName, machine.cpu.pc == 0x102);
    EXPECT(testName, machine.cpu.tick() == 2);
    EXPECT(testName, machine.cpu.reg[0] == 0x2D);
    EXPECT(testName, machine.cpu.pc == 0x104);
    return true;
}

bool testThumbPcRelativeLiteralLoad()
{
    constexpr const char* testName = "thumb_pc_relative_literal_load";
    Machine machine;
    machine.cpu.pc = 0x100;
    machine.cpu.T = 1;
    machine.bus.write16(0x100, 0x4800); // LDR r0, [pc, #0] reads from 0x104
    machine.bus.write32(0x104, 0x12345678);

    machine.cpu.tick();
    EXPECT(testName, machine.cpu.reg[0] == 0x12345678);
    EXPECT(testName, machine.cpu.pc == 0x102);
    return true;
}

bool testDecoderClassification()
{
    constexpr const char* testName = "decoder_classification";
    Machine machine;

    EXPECT(testName, machine.cpu.decodeArm(0xE3A00020).type == tdmi7::CPU::armOperation::ARM_MOV);
    EXPECT(testName, machine.cpu.decodeArm(0xE2801022).type == tdmi7::CPU::armOperation::ARM_ADD);
    EXPECT(testName, machine.cpu.decodeThumb(0x202A).type == tdmi7::CPU::thumbOperation::THUMB_MOV_IMM);
    EXPECT(testName, machine.cpu.decodeThumb(0x3003).type == tdmi7::CPU::thumbOperation::THUMB_ADD_IMM3);
    return true;
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
        runTest("bus_round_trip", testBusRoundTrip) &&
		runTest("reset_initializes_banked_stack", testResetInitializesBankedStack) &&
        runTest("arm_mov_and_add", testArmMovAndAdd) &&
        runTest("arm_conditional_skip", testArmConditionalSkip) &&
		runTest("arm_pc_relative_literal_load", testArmPcRelativeLiteralLoad) &&
		runTest("arm_test_swi_halt", testArmTestSwiHalt) &&
        runTest("thumb_mov_and_add", testThumbMovAndAdd) &&
		runTest("thumb_pc_relative_literal_load", testThumbPcRelativeLiteralLoad) &&
        runTest("decoder_classification", testDecoderClassification);

    return passed ? 0 : 1;
}
