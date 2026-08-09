#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/Decoder.h"

#include <array>
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

bool testArmAndThumbSwiEntry()
{
    constexpr const char* name = "arm_and_thumb_swi_entry";
    Machine arm;
    arm.cpu.pc = 0x03000100;
    arm.cpu.N = 1;
    arm.cpu.C = 1;
    const uint32_t armCpsr = arm.cpu.CPSR;
    arm.bus.write32(0x03000100, 0xEF000042); // SWI #0x42
    arm.cpu.tick();
    EXPECT(name, arm.cpu.curMode == tdmi7::CPU::mode::Supervisor);
    EXPECT(name, arm.cpu.pc == 0x08);
    EXPECT(name, arm.cpu.lr == 0x03000104);
    EXPECT(name, arm.cpu.getSPSR() == armCpsr);
    EXPECT(name, arm.cpu.I == 1 && arm.cpu.T == 0);
    EXPECT(name, arm.cpu.N == 1 && arm.cpu.C == 1);

    Machine thumb;
    thumb.cpu.pc = 0x03000100;
    thumb.cpu.T = 1;
    thumb.cpu.Z = 1;
    const uint32_t thumbCpsr = thumb.cpu.CPSR;
    thumb.bus.write16(0x03000100, 0xDF42); // SWI #0x42
    thumb.cpu.tick();
    EXPECT(name, thumb.cpu.curMode == tdmi7::CPU::mode::Supervisor);
    EXPECT(name, thumb.cpu.pc == 0x08);
    EXPECT(name, thumb.cpu.lr == 0x03000102);
    EXPECT(name, thumb.cpu.getSPSR() == thumbCpsr);
    EXPECT(name, thumb.cpu.T == 0 && thumb.cpu.Z == 1);
    return true;
}

bool testExceptionVectorsAndReturnAddresses()
{
    constexpr const char* name = "exception_vectors_and_return_addresses";
    struct Case { tdmi7::CPU::Exception exception; tdmi7::CPU::mode mode; uint32_t vector; uint32_t armLr; uint32_t thumbLr; };
    constexpr Case cases[] = {
        { tdmi7::CPU::Exception::Undefined, tdmi7::CPU::mode::Undefined, 0x04, 0x104, 0x102 },
        { tdmi7::CPU::Exception::SoftwareInterrupt, tdmi7::CPU::mode::Supervisor, 0x08, 0x104, 0x102 },
        { tdmi7::CPU::Exception::PrefetchAbort, tdmi7::CPU::mode::Abort, 0x0C, 0x104, 0x104 },
        { tdmi7::CPU::Exception::DataAbort, tdmi7::CPU::mode::Abort, 0x10, 0x108, 0x108 },
        { tdmi7::CPU::Exception::Irq, tdmi7::CPU::mode::IRQ, 0x18, 0x104, 0x104 },
        { tdmi7::CPU::Exception::Fiq, tdmi7::CPU::mode::FIQ, 0x1C, 0x104, 0x104 },
    };

    for (const auto& item : cases)
    {
        Machine arm;
        arm.cpu.CPSR = static_cast<uint32_t>(tdmi7::CPU::mode::Supervisor) | 0xC0000000U;
        const uint32_t savedArmCpsr = arm.cpu.CPSR;
        arm.cpu.raiseException(item.exception, 0x100);
        EXPECT(name, arm.cpu.curMode == item.mode);
        EXPECT(name, arm.cpu.pc == item.vector);
        EXPECT(name, arm.cpu.lr == item.armLr);
        EXPECT(name, arm.cpu.getSPSR() == savedArmCpsr);
        EXPECT(name, arm.cpu.T == 0 && arm.cpu.I == 1);
        EXPECT(name, arm.cpu.N == 1 && arm.cpu.Z == 1);
        EXPECT(name, item.exception != tdmi7::CPU::Exception::Fiq || arm.cpu.F == 1);

        Machine thumb;
        thumb.cpu.CPSR = static_cast<uint32_t>(tdmi7::CPU::mode::Supervisor) | 0x20U;
        const uint32_t savedThumbCpsr = thumb.cpu.CPSR;
        thumb.cpu.raiseException(item.exception, 0x100);
        EXPECT(name, thumb.cpu.curMode == item.mode);
        EXPECT(name, thumb.cpu.pc == item.vector);
        EXPECT(name, thumb.cpu.lr == item.thumbLr);
        EXPECT(name, thumb.cpu.getSPSR() == savedThumbCpsr);
        EXPECT(name, thumb.cpu.T == 0 && thumb.cpu.I == 1);
    }
    return true;
}

bool testInterruptPriorityMaskingAndBankedReturn()
{
    constexpr const char* name = "interrupt_priority_masking_and_banked_return";
    Machine machine;
    machine.cpu.writeCPSR(static_cast<uint32_t>(tdmi7::CPU::mode::System));
    machine.cpu.sp = 0x03007E00;
    machine.cpu.lr = 0x11223344;
    const uint32_t originalCpsr = machine.cpu.CPSR;
    machine.cpu.pc = 0x100;
    machine.cpu.I = 0;
    machine.cpu.F = 0;
    machine.cpu.requestIrq();
    machine.cpu.requestFiq();
    machine.cpu.tick();
    EXPECT(name, machine.cpu.curMode == tdmi7::CPU::mode::FIQ);
    EXPECT(name, machine.cpu.pc == 0x1C);
    EXPECT(name, machine.cpu.lr == 0x104);
    EXPECT(name, machine.cpu.F == 1 && machine.cpu.I == 1);

    machine.cpu.sp = 0xDEADBEEF;
    machine.cpu.returnFromException();
    EXPECT(name, machine.cpu.curMode == tdmi7::CPU::mode::System);
    EXPECT(name, machine.cpu.CPSR == originalCpsr);
    EXPECT(name, machine.cpu.sp == 0x03007E00);
    EXPECT(name, machine.cpu.lr == 0x11223344);
    return true;
}

bool testArmIrqReturnInstruction()
{
    constexpr const char* name = "arm_irq_return_instruction";
    Machine machine;
	std::array<uint8_t, 0x4000> bios{};
	const uint32_t returnInstruction = 0xE25EF004; // SUBS pc, lr, #4
	for (unsigned byte = 0; byte < 4; ++byte)
		bios[0x18 + byte] = static_cast<uint8_t>(returnInstruction >> (byte * 8));
	machine.bus.loadBiosImage(bios.data(), bios.size());
    machine.cpu.writeCPSR(static_cast<uint32_t>(tdmi7::CPU::mode::System));
    machine.cpu.pc = 0x03000100;
    machine.cpu.I = 0;
    const uint32_t originalCpsr = machine.cpu.CPSR;
    machine.cpu.requestIrq();
    machine.cpu.tick();
    EXPECT(name, machine.cpu.pc == 0x18);
    EXPECT(name, machine.cpu.lr == 0x03000104);

    machine.cpu.tick();
    EXPECT(name, machine.cpu.curMode == tdmi7::CPU::mode::System);
    EXPECT(name, machine.cpu.CPSR == originalCpsr);
    EXPECT(name, machine.cpu.pc == 0x03000100);
    return true;
}

bool testUnalignedWordLoadsAndConditionalStore()
{
    constexpr const char* name = "unaligned_word_loads_and_conditional_store";
    Machine machine;
    machine.cpu.pc = 0x03000100;
    machine.cpu.reg[1] = 0x03000200;
    machine.bus.write32(0x03000100, 0xE5910000); // LDR r0, [r1]
    machine.bus.write32(0x03000200, 0x44332211);
    machine.cpu.reg[1] = 0x03000201;
    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == 0x11443322);

    machine.cpu.pc = 0x03000104;
    machine.cpu.Z = 1;
    machine.cpu.reg[0] = 0xA5A5A5A5;
    machine.bus.write32(0x03000104, 0x15010000); // STRNE r0, [r1]
    machine.cpu.tick();
    EXPECT(name, machine.bus.read32(0x03000200) == 0x44332211);
    EXPECT(name, machine.cpu.pc == 0x03000108);
    return true;
}

bool testNeverConditionDoesNotExecute()
{
    constexpr const char* name = "never_condition_does_not_execute";
    Machine machine;
    machine.cpu.N = machine.cpu.Z = machine.cpu.C = machine.cpu.V = 1;
    EXPECT(name, !machine.cpu.checkConditional(0xFU));

    machine.cpu.pc = 0x03000100;
    machine.cpu.reg[0] = 0x12345678;
    // MOV r0, #0 with the ARM7TDMI NV condition. The instruction is skipped.
    machine.bus.write32(machine.cpu.pc, 0xF3A00000U);
    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == 0x12345678U);
    EXPECT(name, machine.cpu.pc == 0x03000104U);
    return true;
}

bool testPcOperandsShiftEdgesAndMultiplyTiming()
{
    constexpr const char* name = "pc_operands_shift_edges_and_multiply_timing";
    Machine machine;
    constexpr uint32_t code = 0x03000100;
    constexpr uint32_t data = 0x03000200;
    machine.cpu.pc = code;
    machine.cpu.reg[0] = data;
    machine.bus.write32(code, 0xE580F000); // STR r15, [r0]
    machine.cpu.tick();
    EXPECT(name, machine.bus.read32(data) == code + 8);

    machine.cpu.pc = code + 4;
    machine.cpu.reg[1] = 0x80000001;
    machine.cpu.C = 0;
    machine.bus.write32(code + 4, 0xE1B00021); // MOVS r0, r1, LSR #32
    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == 0);
    EXPECT(name, machine.cpu.C == 1);

    machine.cpu.pc = code + 8;
    machine.cpu.reg[1] = 1;
    machine.cpu.reg[2] = 32;
    machine.bus.write32(code + 8, 0xE1B00211); // MOVS r0, r1, LSL r2
    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == 0);
    EXPECT(name, machine.cpu.C == 1);

    machine.cpu.pc = code + 12;
    machine.cpu.reg[1] = 7;
    machine.cpu.reg[2] = 1;
    machine.cpu.reg[3] = 9;
    machine.bus.write32(code + 12, 0xE0000391); // MUL r0, r1, r3
    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == 63);
    EXPECT(name, machine.cpu.curOpCycles == 3); // 1S + 1I for one-byte multiplier
    return true;
}

bool testArmPcOperandThumbBootstrap()
{
    constexpr const char* name = "arm_pc_operand_thumb_bootstrap";
    Machine machine;
    constexpr uint32_t code = 0x03000100;
    machine.cpu.pc = code;
    machine.bus.write32(code, 0xE28F0001); // ADD r0, pc, #1
    machine.bus.write32(code + 4, 0xE12FFF10); // BX r0
    machine.bus.write16(code + 8, 0x2007);     // Thumb: MOV r0, #7

    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == code + 9);
    EXPECT(name, machine.cpu.pc == code + 4);
    machine.cpu.tick();
    EXPECT(name, machine.cpu.T == 1);
    EXPECT(name, machine.cpu.pc == code + 8);
    machine.cpu.tick();
    EXPECT(name, machine.cpu.reg[0] == 7);
    EXPECT(name, machine.cpu.pc == code + 10);
    return true;
}

bool testArmBranchLinkReturnAddress()
{
    constexpr const char* name = "arm_branch_link_return_address";
    Machine machine;
    constexpr uint32_t code = 0x03000100;
    machine.cpu.pc = code;
    machine.bus.write32(code, 0xEB000000); // BL to code + 8

    machine.cpu.tick();
    EXPECT(name, machine.cpu.lr == code + 4);
    EXPECT(name, machine.cpu.pc == code + 8);

    machine.bus.write32(code + 8, 0xE12FFF1E); // BX lr
    machine.cpu.tick();
    EXPECT(name, machine.cpu.pc == code + 4);
    return true;
}

bool testArmLoadsToProgramCounterDoNotFallThrough()
{
    constexpr const char* name = "arm_loads_to_program_counter_do_not_fall_through";
    Machine machine;
    constexpr uint32_t code = 0x03000100;
    constexpr uint32_t data = 0x03000200;
    machine.cpu.pc = code;
    machine.cpu.reg[0] = data;
    machine.bus.write32(code, 0xE590F000); // LDR pc, [r0]
    machine.bus.write32(data, code + 0x204);
    machine.cpu.tick();
    EXPECT(name, machine.cpu.pc == code + 0x204);

    machine.cpu.pc = code + 8;
    machine.cpu.reg[0] = data + 4;
    machine.bus.write32(code + 8, 0xE8908000); // LDMIA r0, {pc}
    machine.bus.write32(data + 4, code + 0x308);
    machine.cpu.tick();
    EXPECT(name, machine.cpu.pc == code + 0x308);
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
        run("arm_and_thumb_swi_entry", testArmAndThumbSwiEntry) &&
        run("exception_vectors_and_return_addresses", testExceptionVectorsAndReturnAddresses) &&
        run("interrupt_priority_masking_and_banked_return", testInterruptPriorityMaskingAndBankedReturn) &&
		run("arm_irq_return_instruction", testArmIrqReturnInstruction) &&
		run("never_condition_does_not_execute", testNeverConditionDoesNotExecute) &&
		run("unaligned_word_loads_and_conditional_store", testUnalignedWordLoadsAndConditionalStore) &&
        run("pc_operands_shift_edges_and_multiply_timing", testPcOperandsShiftEdgesAndMultiplyTiming) &&
        run("arm_pc_operand_thumb_bootstrap", testArmPcOperandThumbBootstrap) &&
        run("arm_branch_link_return_address", testArmBranchLinkReturnAddress) &&
        run("arm_loads_to_program_counter_do_not_fall_through", testArmLoadsToProgramCounterDoNotFallThrough);
    return passed ? 0 : 1;
}
