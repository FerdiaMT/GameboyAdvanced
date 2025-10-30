#include "DebuggerCPU.h"
#include "CPU.h"
#include <cstdint>
#include <string>
#include <sstream>

#include <iomanip>
#include <iostream>

DebuggerCPU::DebuggerCPU(CPU* cpu)
{
	this->cpu = cpu;
}

void DebuggerCPU::DecodeIns(uint32_t startAddr, uint32_t endAddr)
{
	uint32_t curAddr = startAddr;

	while (curAddr <= endAddr)
	{
        ThumbLineDecode(curAddr);

        curAddr += 2;
	}
}

void DebuggerCPU::ThumbLineDecode(uint32_t curAddr)
{
    uint16_t curInstruction = cpu->read16(curAddr); // this is the instruction in hex
    CPU::thumbInstr curOperation = cpu->decodeThumb(curInstruction); // this turns the instruction into the decoded operation

    printf("PC: 0x%08X, Instruction: 0x%04X, ThumbCode: %s  \n",
        curAddr, curInstruction, thumbToStr(curOperation).c_str()  );

    curAddr += 2;
}


void DebuggerCPU::ArmLineDecode(uint32_t curAddr)
{
	uint32_t curInstruction = cpu->read32(curAddr); // this is the instruction in hex
	CPU::Operation curOperation = cpu->decode(curInstruction); // this turns the instruction into the decoded operation

	printf("PC: 0x%08X, Instruction: 0x%08X, Opcode: %s\n",
		curAddr, curInstruction, cpu->opcodeToString(curOperation));


    curAddr += 4;
	//const char* conditional = checkConditional((curInstruction >> 28) & 0xF); // this will return in string from the conditiona
}



bool needsRd(CPU::thumbOperation type)
{
    switch (type)
    {
    case  CPU::thumbOperation::THUMB_TST_REG:
    case  CPU::thumbOperation::THUMB_CMP_REG:
    case  CPU::thumbOperation::THUMB_CMP_IMM:
    case  CPU::thumbOperation::THUMB_CMN_REG:
    case  CPU::thumbOperation::THUMB_CMP_HI:
    case  CPU::thumbOperation::THUMB_BX:
    case  CPU::thumbOperation::THUMB_BLX_REG:
    case  CPU::thumbOperation::THUMB_B_COND:
    case  CPU::thumbOperation::THUMB_B:
    case  CPU::thumbOperation::THUMB_BL_PREFIX:
    case  CPU::thumbOperation::THUMB_BL_SUFFIX:
    case  CPU::thumbOperation::THUMB_ADD_SP_IMM:

    case CPU::thumbOperation::THUMB_PUSH:
    case CPU::thumbOperation::THUMB_POP:
    case CPU::thumbOperation::THUMB_STMIA:
    case CPU::thumbOperation::THUMB_LDMIA:
    case CPU::thumbOperation::THUMB_SWI:

    return false;
    default:
    return true;
    }
}

bool needsRs(CPU::thumbOperation type)
{
    switch (type)
    {
    case CPU::thumbOperation::THUMB_MOV_IMM:
    case  CPU::thumbOperation::THUMB_CMP_IMM:
    case  CPU::thumbOperation::THUMB_ADD_PC:
    case  CPU::thumbOperation::THUMB_LDR_PC:
    case  CPU::thumbOperation::THUMB_B_COND:
    case  CPU::thumbOperation::THUMB_B:
    case  CPU::thumbOperation::THUMB_BL_PREFIX:
    case  CPU::thumbOperation::THUMB_BL_SUFFIX:
    case  CPU::thumbOperation::THUMB_SWI:
    case  CPU::thumbOperation::THUMB_PUSH:
    case  CPU::thumbOperation::THUMB_POP:
    case  CPU::thumbOperation::THUMB_ADD_SP_IMM:
    return false;
    default:
    return true;
    }
}


inline const char* DebuggerCPU::checkConditional(uint8_t cond)
{

	switch (cond)
	{
	case(0x0):return "Z";					break;
	case(0x1):return "!Z";				break;
	case(0x2):return "C";					break;
	case(0x3):return "!C";				break;
	case(0x4):return "N";					break;
	case(0x5):return "!N";				break;
	case(0x6):return "V";					break;
	case(0x7):return "!V";				break;
	case(0x8):return "(C && !Z)";			break;
	case(0x9):return "(!C || Z)";			break;
	case(0xA):return "(N == V)";			break;
	case(0xB):return "(N != V)";			break;
	case(0xC):return "(!Z && (N == V))";	break;
	case(0xD):return "(Z || (N != V))";	break;
	case(0xE):return "ALWAYS TRUE";				break;
	case(0xF):printf("0XF INVALID CND");break;
	}
}

std::string DebuggerCPU::thumbToStr(CPU::thumbInstr& instr)
{
	std::stringstream ss;

	const char* opNames[] = {
		"MOV_IMM", "ADD_REG", "ADD_IMM", "ADD_IMM3", "SUB_REG", "SUB_IMM", "SUB_IMM3", "CMP_IMM",
		"LSL_IMM", "LSR_IMM", "ASR_IMM", "AND_REG", "EOR_REG", "LSL_REG",
		"LSR_REG", "ASR_REG", "ADC_REG", "SBC_REG", "ROR_REG", "TST_REG",
		"NEG_REG", "CMP_REG", "CMN_REG", "ORR_REG", "MUL_REG", "BIC_REG",
		"MVN_REG", "ADD_HI", "CMP_HI", "MOV_HI", "BX", "BLX_REG",
		"LDR_PC", "LDR_REG", "STR_REG", "LDRB_REG", "STRB_REG", "LDRH_REG",
		"STRH_REG", "LDRSB_REG", "LDRSH_REG", "LDR_IMM", "STR_IMM", "LDRB_IMM",
		"STRB_IMM", "LDRH_IMM", "STRH_IMM", "LDR_SP", "STR_SP", "ADD_PC",
		"ADD_SP", "ADD_SP_IMM", "PUSH", "POP", "STMIA", "LDMIA",
		"B_COND", "B", "BL_PREFIX", "BL_SUFFIX", "SWI", "UNDEFINED"
	};

	ss << std::dec << opNames[(int)instr.type];

	if ((instr.rd != NULL)) ss << " Rd=R" << (int)instr.rd;
	if ((instr.rs != NULL)) ss << " Rs=R" << (int)instr.rs;
	if ((instr.rn != NULL)) ss << " Rn=R" << (int)instr.rn;


	if (instr.imm != 0)
	{
		ss << " imm=0x" << std::dec << instr.imm;
	}


	if (instr.type == CPU::thumbOperation::THUMB_B_COND)
	{
		const char* condNames[] = {
			"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
			"HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"
		};
		ss << " cond=" << condNames[instr.cond];
	}

	if (instr.h1) ss << " H1";
	if (instr.h2) ss << " H2";

	return ss.str();
}



// Helper function to print test results
void printTestResult(const char* testName, bool passed)
{
    if (passed)
    {
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << testName << std::endl;
    }
}

// ============================================================
// FORMAT 1: MOVE SHIFTED REGISTER
// ============================================================

bool testLSL_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0x00000010;  // 16

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 5;

    cpu.opT_LSL_IMM(instr);

    if (cpu.reg[0] != 0x00000200)
    {  // 16 << 5 = 512
        std::cout << "  Expected R0=0x200, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLSL_IMM_Carry(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0x80000000;  // MSB set

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 1;

    cpu.opT_LSL_IMM(instr);

    if (cpu.C != 1)
    {
        std::cout << "  C flag should be set (bit shifted out)" << std::endl;
        return false;
    }

    if (cpu.reg[0] != 0x00000000)
    {
        std::cout << "  Expected R0=0, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLSR_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0x000000F0;  // 240

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 4;

    cpu.opT_LSR_IMM(instr);

    if (cpu.reg[0] != 0x0000000F)
    {  // 240 >> 4 = 15
        std::cout << "  Expected R0=0xF, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLSR_IMM_Carry(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0x00000003;  // LSB set

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 1;

    cpu.opT_LSR_IMM(instr);

    if (cpu.C != 1)
    {
        std::cout << "  C flag should be set (bit shifted out)" << std::endl;
        return false;
    }

    return true;
}

bool testASR_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0xFFFFFFF0;  // -16

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 2;

    cpu.opT_ASR_IMM(instr);

    if (cpu.reg[0] != 0xFFFFFFFC)
    {  // -16 >> 2 = -4 (sign extended)
        std::cout << "  Expected R0=0xFFFFFFFC, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    if (cpu.N != 1)
    {
        std::cout << "  N flag should be set (negative result)" << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 2: ADD/SUBTRACT
// ============================================================

bool testADD_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 10;
    cpu.reg[2] = 20;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_ADD_REG(instr);

    if (cpu.reg[0] != 30)
    {
        std::cout << "  Expected R0=30, got " << cpu.reg[0] << std::endl;
        return false;
    }

    if (cpu.Z != 0)
    {
        std::cout << "  Z flag should be clear" << std::endl;
        return false;
    }

    return true;
}

bool testADD_REG_Overflow(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0x7FFFFFFF;  // Max positive
    cpu.reg[2] = 1;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_ADD_REG(instr);

    if (cpu.V != 1)
    {
        std::cout << "  V flag should be set (overflow)" << std::endl;
        return false;
    }

    if (cpu.N != 1)
    {
        std::cout << "  N flag should be set (result negative)" << std::endl;
        return false;
    }

    return true;
}

bool testADD_REG_Carry(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0xFFFFFFFF;
    cpu.reg[2] = 1;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_ADD_REG(instr);

    if (cpu.C != 1)
    {
        std::cout << "  C flag should be set (carry out)" << std::endl;
        return false;
    }

    if (cpu.Z != 1)
    {
        std::cout << "  Z flag should be set (result is 0)" << std::endl;
        return false;
    }

    return true;
}

bool testSUB_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 50;
    cpu.reg[2] = 20;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_SUB_REG(instr);

    if (cpu.reg[0] != 30)
    {
        std::cout << "  Expected R0=30, got " << cpu.reg[0] << std::endl;
        return false;
    }

    if (cpu.Z != 0)
    {
        std::cout << "  Z flag should be clear" << std::endl;
        return false;
    }

    if (cpu.C != 1)
    {
        std::cout << "  C flag should be set (no borrow)" << std::endl;
        return false;
    }

    return true;
}

bool testSUB_REG_Borrow(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 10;
    cpu.reg[2] = 20;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_SUB_REG(instr);

    if (cpu.C != 0)
    {
        std::cout << "  C flag should be clear (borrow)" << std::endl;
        return false;
    }

    if (cpu.N != 1)
    {
        std::cout << "  N flag should be set (negative result)" << std::endl;
        return false;
    }

    return true;
}

bool testADD_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 5;

    cpu.opT_ADD_IMM(instr);

    if (cpu.reg[0] != 15)
    {
        std::cout << "  Expected R0=15, got " << cpu.reg[0] << std::endl;
        return false;
    }

    return true;
}

bool testSUB_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 3;

    cpu.opT_SUB_IMM(instr);

    if (cpu.reg[0] != 7)
    {
        std::cout << "  Expected R0=7, got " << cpu.reg[0] << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 3: MOVE/COMPARE/ADD/SUBTRACT IMMEDIATE
// ============================================================

bool testMOV_IMM(CPU& cpu)
{
    cpu.reset();

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 42;

    cpu.opT_MOV_IMM(instr);

    if (cpu.reg[0] != 42)
    {
        std::cout << "  Expected R0=42, got " << cpu.reg[0] << std::endl;
        return false;
    }

    if (cpu.Z != 0)
    {
        std::cout << "  Z flag should be clear" << std::endl;
        return false;
    }

    return true;
}

bool testMOV_IMM_Zero(CPU& cpu)
{
    cpu.reset();

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 0;

    cpu.opT_MOV_IMM(instr);

    if (cpu.reg[0] != 0)
    {
        std::cout << "  Expected R0=0, got " << cpu.reg[0] << std::endl;
        return false;
    }

    if (cpu.Z != 1)
    {
        std::cout << "  Z flag should be set" << std::endl;
        return false;
    }

    return true;
}

bool testCMP_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 10;

    cpu.opT_CMP_IMM(instr);

    if (cpu.Z != 1)
    {
        std::cout << "  Z flag should be set (equal comparison)" << std::endl;
        return false;
    }

    if (cpu.C != 1)
    {
        std::cout << "  C flag should be set (no borrow)" << std::endl;
        return false;
    }

    return true;
}

bool testCMP_IMM_Less(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 5;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 10;

    cpu.opT_CMP_IMM(instr);

    if (cpu.Z != 0)
    {
        std::cout << "  Z flag should be clear (not equal)" << std::endl;
        return false;
    }

    if (cpu.C != 0)
    {
        std::cout << "  C flag should be clear (borrow)" << std::endl;
        return false;
    }

    if (cpu.N != 1)
    {
        std::cout << "  N flag should be set (negative result)" << std::endl;
        return false;
    }

    return true;
}

bool testADD_IMM8(CPU& cpu)
{
    cpu.reset();
    cpu.reg[3] = 100;

    CPU::thumbInstr instr;
    instr.rd = 3;
    instr.imm = 50;

    cpu.opT_ADD_IMM3(instr);

    if (cpu.reg[3] != 150)
    {
        std::cout << "  Expected R3=150, got " << cpu.reg[3] << std::endl;
        return false;
    }

    return true;
}

bool testSUB_IMM8(CPU& cpu)
{
    cpu.reset();
    cpu.reg[3] = 100;

    CPU::thumbInstr instr;
    instr.rd = 3;
    instr.imm = 25;

    cpu.opT_SUB_IMM3(instr);

    if (cpu.reg[3] != 75)
    {
        std::cout << "  Expected R3=75, got " << cpu.reg[3] << std::endl;
        return false;
    }

    return true;
}

bool testSUB_IMM3(CPU& cpu)
{
    cpu.reset();
    cpu.reg[3] = 100;

    CPU::thumbInstr instr;
    instr.rd = 3;
    instr.imm = 4;

    cpu.opT_SUB_IMM3(instr);

    if (cpu.reg[3] != 96)
    {
        std::cout << "  Expected R3=96, got " << cpu.reg[3] << std::endl;
        return false;
    }

    return true;
}

bool testSUB_IMM3_ZeroFlag(CPU& cpu)
{
    cpu.reset();
    cpu.reg[3] = 4;

    CPU::thumbInstr instr;
    instr.rd = 3;
    instr.imm = 4;

    cpu.opT_SUB_IMM3(instr);

    if (cpu.reg[3] != 0)
    {
        std::cout << "  Expected R3=0, got " << cpu.reg[3] << std::endl;
        return false;
    }

    if (cpu.Z != 1)
    {
        std::cout << "  Z flag should be set when result is 0" << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 4: ALU OPERATIONS
// ============================================================

bool testAND_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0xFF;
    cpu.reg[1] = 0x0F;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_AND_REG(instr);

    if (cpu.reg[0] != 0x0F)
    {
        std::cout << "  Expected R0=0x0F, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testEOR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0xFF;
    cpu.reg[1] = 0x0F;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_EOR_REG(instr);

    if (cpu.reg[0] != 0xF0)
    {
        std::cout << "  Expected R0=0xF0, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLSL_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x01;
    cpu.reg[1] = 4;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_LSL_REG(instr);

    if (cpu.reg[0] != 0x10)
    {
        std::cout << "  Expected R0=0x10, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLSR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x10;
    cpu.reg[1] = 2;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_LSR_REG(instr);

    if (cpu.reg[0] != 0x04)
    {
        std::cout << "  Expected R0=0x04, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testASR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0xFFFFFF00;
    cpu.reg[1] = 4;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_ASR_REG(instr);

    if (cpu.reg[0] != 0xFFFFFFF0)
    {
        std::cout << "  Expected R0=0xFFFFFFF0, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testADC_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 10;
    cpu.reg[1] = 20;
    cpu.C = 1;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_ADC_REG(instr);

    if (cpu.reg[0] != 31)
    {  // 10 + 20 + 1
        std::cout << "  Expected R0=31, got " << cpu.reg[0] << std::endl;
        return false;
    }

    return true;
}

bool testSBC_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 50;
    cpu.reg[1] = 20;
    cpu.C = 1;  // No borrow

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_SBC_REG(instr);

    if (cpu.reg[0] != 30)
    {  // 50 - 20 - 0
        std::cout << "  Expected R0=30, got " << cpu.reg[0] << std::endl;
        return false;
    }

    return true;
}

bool testROR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x80000001;
    cpu.reg[1] = 1;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_ROR_REG(instr);

    if (cpu.reg[0] != 0xC0000000)
    {
        std::cout << "  Expected R0=0xC0000000, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testTST_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0xFF;
    cpu.reg[1] = 0x0F;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_TST_REG(instr);

    // R0 should not change
    if (cpu.reg[0] != 0xFF)
    {
        std::cout << "  R0 should not change, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    if (cpu.Z != 0)
    {
        std::cout << "  Z flag should be clear (result non-zero)" << std::endl;
        return false;
    }

    return true;
}

bool testNEG_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 42;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_NEG_REG(instr);

    if (cpu.reg[0] != (uint32_t)-42)
    {
        std::cout << "  Expected R0=-42, got " << cpu.reg[0] << std::endl;
        return false;
    }

    if (cpu.N != 1)
    {
        std::cout << "  N flag should be set" << std::endl;
        return false;
    }

    return true;
}

bool testCMN_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 10;
    cpu.reg[1] = -10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_CMN_REG(instr);

    if (cpu.Z != 1)
    {
        std::cout << "  Z flag should be set (sum is zero)" << std::endl;
        return false;
    }

    return true;
}

bool testORR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0xF0;
    cpu.reg[1] = 0x0F;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_ORR_REG(instr);

    if (cpu.reg[0] != 0xFF)
    {
        std::cout << "  Expected R0=0xFF, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testMUL_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 6;
    cpu.reg[1] = 7;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_MUL_REG(instr);

    if (cpu.reg[0] != 42)
    {
        std::cout << "  Expected R0=42, got " << cpu.reg[0] << std::endl;
        return false;
    }

    return true;
}

bool testBIC_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[3] = 0xFF;
    cpu.reg[0] = 0x0F;

    CPU::thumbInstr instr;
    instr.rd = 3;
    instr.rs = 0;

    cpu.opT_BIC_REG(instr);

    if (cpu.reg[3] != 0xF0)
    {
        std::cout << "  Expected R3=0xF0, got 0x" << std::hex << cpu.reg[3] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testMVN_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[1] = 0x0F;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;

    cpu.opT_MVN_REG(instr);

    if (cpu.reg[0] != 0xFFFFFFF0)
    {
        std::cout << "  Expected R0=0xFFFFFFF0, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 5: HI REGISTER OPERATIONS
// ============================================================

bool testADD_HI(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 10;
    cpu.reg[8] = 20;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 8;

    cpu.opT_ADD_HI(instr);

    if (cpu.reg[0] != 30)
    {
        std::cout << "  Expected R0=30, got " << cpu.reg[0] << std::endl;
        return false;
    }

    return true;
}

bool testCMP_HI(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 10;
    cpu.reg[8] = 10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 8;

    cpu.opT_CMP_HI(instr);

    if (cpu.Z != 1)
    {
        std::cout << "  Z flag should be set (equal)" << std::endl;
        return false;
    }

    return true;
}

bool testMOV_HI(CPU& cpu)
{
    cpu.reset();
    cpu.reg[15] = 0x08000100;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 15;

    cpu.opT_MOV_HI(instr);

    if (cpu.reg[0] != 0x08000100)
    {
        std::cout << "  Expected R0=0x08000100, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testBX(CPU& cpu)
{
    cpu.reset();
    cpu.reg[5] = 0x08000101;  // Thumb mode (bit 0 set)
    cpu.T = 0;  // Start in ARM mode

    CPU::thumbInstr instr;
    instr.rs = 5;

    cpu.opT_BX(instr);

    if (cpu.T != 1)
    {
        std::cout << "  T flag should be set (switched to Thumb)" << std::endl;
        return false;
    }

    if (cpu.pc != 0x08000100)
    {
        std::cout << "  Expected PC=0x08000100, got 0x" << std::hex << cpu.pc << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 6: PC-RELATIVE LOAD
// ============================================================

bool testLDR_PC(CPU& cpu)
{
    cpu.reset();
    cpu.pc = 0x08000100;

    // Calculate target address: (PC + 2 + 2) & ~2 + offset
    uint32_t targetAddr = 0x0800010C;
    cpu.write32(targetAddr, 0xDEADBEEF);

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 8;

    cpu.opT_LDR_PC(instr);

    if (cpu.reg[0] != 0xDEADBEEF)
    {
        std::cout << "  Expected R0=0xDEADBEEF, got 0x" << std::hex << cpu.reg[0] << std::dec << "Expected addr:0x0800010C , got: "<< std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 7: LOAD/STORE WITH REGISTER OFFSET
// ============================================================

bool testSTR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_STR_REG(instr);

    uint32_t stored = cpu.read32(0x02000010);
    if (stored != 0x12345678)
    {
        std::cout << "  Expected memory=0x12345678, got 0x" << std::hex << stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDR_REG(CPU& cpu)
{
    cpu.reset();
    cpu.write32(0x02000010, 0xABCDEF00);
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1; //// where rs is used instead of rb
    instr.rn = 2; //// where rn is used instead of ro

    cpu.opT_LDR_REG(instr);

    if (cpu.reg[0] != 0xABCDEF00)
    {
        std::cout << "  Expected R0=0xABCDEF00, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testSTRB_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_STRB_REG(instr);

    uint8_t stored = cpu.read8(0x02000010);
    if (stored != 0x78)
    {
        std::cout << "  Expected memory=0x78, got 0x" << std::hex << (int)stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDRB_REG(CPU& cpu)
{
    cpu.reset();
    cpu.write8(0x02000010, 0xAB);
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_LDRB_REG(instr);

    if (cpu.reg[0] != 0xAB)
    {
        std::cout << "  Expected R0=0xAB, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 8: LOAD/STORE SIGN-EXTENDED BYTE/HALFWORD
// ============================================================

bool testSTRH_REG(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_STRH_REG(instr);

    uint16_t stored = cpu.read16(0x02000010);
    if (stored != 0x5678)
    {
        std::cout << "  Expected memory=0x5678, got 0x" << std::hex << stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDRH_REG(CPU& cpu)
{
    cpu.reset();
    cpu.write16(0x02000010, 0xABCD);
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_LDRH_REG(instr);

    if (cpu.reg[0] != 0xABCD)
    {
        std::cout << "  Expected R0=0xABCD, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDSB_REG(CPU& cpu)
{
    cpu.reset();
    cpu.write8(0x02000010, 0xFF);  // -1 as signed byte
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_LDRSB_REG(instr);

    if (cpu.reg[0] != 0xFFFFFFFF)
    {  // Sign-extended
        std::cout << "  Expected R0=0xFFFFFFFF, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDSH_REG(CPU& cpu)
{
    cpu.reset();
    cpu.write16(0x02000010, 0xFFFF);  // -1 as signed halfword
    cpu.reg[1] = 0x02000000;
    cpu.reg[2] = 0x10;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.rn = 2;

    cpu.opT_LDRSH_REG(instr);

    if (cpu.reg[0] != 0xFFFFFFFF)
    {  // Sign-extended
        std::cout << "  Expected R0=0xFFFFFFFF, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 9: LOAD/STORE WITH IMMEDIATE OFFSET
// ============================================================

bool testSTR_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[1] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 0x10;

    cpu.opT_STR_IMM(instr);

    uint32_t stored = cpu.read32(0x02000010);
    if (stored != 0x12345678)
    {
        std::cout << "  Expected memory=0x12345678, got 0x" << std::hex << stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDR_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.write32(0x02000010, 0xABCDEF00);
    cpu.reg[1] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 0x10;

    cpu.opT_LDR_IMM(instr);

    if (cpu.reg[0] != 0xABCDEF00)
    {
        std::cout << "  Expected R0=0xABCDEF00, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testSTRB_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[1] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 0x10;

    cpu.opT_STRB_IMM(instr);

    uint8_t stored = cpu.read8(0x02000010);
    if (stored != 0x78)
    {
        std::cout << "  Expected memory=0x78, got 0x" << std::hex << (int)stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDRB_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.write8(0x02000010, 0xAB);
    cpu.reg[1] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 0x10;

    cpu.opT_LDRB_IMM(instr);

    if (cpu.reg[0] != 0xAB)
    {
        std::cout << "  Expected R0=0xAB, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 10: LOAD/STORE HALFWORD
// ============================================================

bool testSTRH_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[1] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 0x10;

    cpu.opT_STRH_IMM(instr);

    uint16_t stored = cpu.read16(0x02000010);
    if (stored != 0x5678)
    {
        std::cout << "  Expected memory=0x5678, got 0x" << std::hex << stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDRH_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.write16(0x02000010, 0xABCD);
    cpu.reg[1] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.rs = 1;
    instr.imm = 0x10;

    cpu.opT_LDRH_IMM(instr);

    if (cpu.reg[0] != 0xABCD)
    {
        std::cout << "  Expected R0=0xABCD, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 11: SP-RELATIVE LOAD/STORE
// ============================================================

bool testSTR_SP(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x12345678;
    cpu.reg[13] = 0x03007F00;  // SP

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 0x10;

    cpu.opT_STR_SP(instr);

    uint32_t stored = cpu.read32(0x03007F10);
    if (stored != 0x12345678)
    {
        std::cout << "  Expected memory=0x12345678, got 0x" << std::hex << stored << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testLDR_SP(CPU& cpu)
{
    cpu.reset();
    cpu.write32(0x03007F10, 0xABCDEF00);
    cpu.reg[13] = 0x03007F00;  // SP

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 0x10;

    cpu.opT_LDR_SP(instr);

    if (cpu.reg[0] != 0xABCDEF00)
    {
        std::cout << "  Expected R0=0xABCDEF00, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 12: LOAD ADDRESS
// ============================================================

bool testADD_PC(CPU& cpu)
{
    cpu.reset();
    cpu.pc = 0x08000100;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 0x10;

    cpu.opT_ADD_PC(instr);

    // Should be: (PC & ~2) + 4 + imm
    uint32_t expected = (0x08000100 & ~2) + 4 + 0x10;
    if (cpu.reg[0] != expected)
    {
        std::cout << "  Expected R0=0x" << std::hex << expected << ", got 0x" << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testADD_SP_IMM(CPU& cpu)
{
    cpu.reset();
    cpu.reg[13] = 0x03007F00;

    CPU::thumbInstr instr;
    instr.rd = 0;
    instr.imm = 0x10;

    cpu.opT_ADD_SP_IMM(instr);

    if (cpu.reg[0] != 0x03007F10)
    {
        std::cout << "  Expected R0=0x03007F10, got 0x" << std::hex << cpu.reg[0] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 13: ADD OFFSET TO STACK POINTER
// ============================================================

bool testADD_SP(CPU& cpu)
{
    cpu.reset();
    cpu.reg[13] = 0x03007F00;

    CPU::thumbInstr instr;
    instr.imm = 0x10;

    cpu.opT_ADD_SP(instr);

    if (cpu.reg[13] != 0x03007F10)
    {
        std::cout << "  Expected SP=0x03007F10, got 0x" << std::hex << cpu.reg[13] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testSUB_SP(CPU& cpu)
{
    cpu.reset();
    cpu.reg[13] = 0x03007F10;

    CPU::thumbInstr instr;
    instr.imm = 0x10;
    instr.imm = -(int32_t)instr.imm;

    cpu.opT_ADD_SP(instr);

    if (cpu.reg[13] != 0x03007F00)
    {
        std::cout << "  Expected SP=0x03007F00, got 0x" << std::hex << cpu.reg[13] << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 14: PUSH/POP REGISTERS
// ============================================================

bool testPUSH(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x11111111;
    cpu.reg[1] = 0x22222222;
    cpu.reg[13] = 0x03007F10;

    CPU::thumbInstr instr;
    instr.imm = 0x03;  // R0 and R1

    cpu.opT_PUSH(instr);

    if (cpu.reg[13] != 0x03007F08)
    {
        std::cout << "  Expected SP=0x03007F08, got 0x" << std::hex << cpu.reg[13] << std::dec << std::endl;
        return false;
    }

    if (cpu.read32(0x03007F08) != 0x11111111 || cpu.read32(0x03007F0C) != 0x22222222)
    {
        std::cout << "  Memory contents incorrect" << "memA meant to be 0x11111111 got "<< cpu.read32(0x03007F08)<< "memA meant to be 0x22222222 got " << cpu.read32(0x03007F0C)<< std::endl;
        return false;
    }

    return true;
}

bool testPUSH_LR(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x11111111;
    cpu.reg[14] = 0x08000100;
    cpu.reg[13] = 0x03007F10;

    CPU::thumbInstr instr;
    instr.imm = 0x01;  // R0
   // instr.r = 1;  // Include LR

    cpu.opT_PUSH(instr);

    if (cpu.reg[13] != 0x03007F08)
    {
        std::cout << "  Expected SP=0x03007F08, got 0x" << std::hex << cpu.reg[13] << std::dec << std::endl;
        return false;
    }

    if (cpu.read32(0x03007F0C) != 0x08000100)
    {
        std::cout << "  LR not pushed correctly" << std::endl;
        return false;
    }

    return true;
}

bool testPOP(CPU& cpu)
{
    cpu.reset();
    cpu.reg[13] = 0x03007F08;
    cpu.write32(0x03007F08, 0x11111111);
    cpu.write32(0x03007F0C, 0x22222222);

    CPU::thumbInstr instr;
    instr.imm = 0x03;  // R0 and R1

    cpu.opT_POP(instr);

    if (cpu.reg[0] != 0x11111111 || cpu.reg[1] != 0x22222222)
    {
        std::cout << "  Registers not loaded correctly" << std::endl;
        return false;
    }

    if (cpu.reg[13] != 0x03007F10)
    {
        std::cout << "  Expected SP=0x03007F10, got 0x" << std::hex << cpu.reg[13] << std::dec << std::endl;
        return false;
    }

    return true;
}

bool testPOP_PC(CPU& cpu)
{
    cpu.reset();
    cpu.reg[13] = 0x03007F0C;
    cpu.write32(0x03007F0C, 0x08000100);

    CPU::thumbInstr instr;
    instr.imm = 0x00;
    //instr.r = 1;  // Include PC

    cpu.opT_POP(instr);

    if (cpu.pc != 0x08000100)
    {
        std::cout << "  Expected PC=0x08000100, got 0x" << std::hex << cpu.pc << std::dec << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// FORMAT 15: MULTIPLE LOAD/STORE
// ============================================================

bool testSTMIA(CPU& cpu)
{
    cpu.reset();
    cpu.reg[0] = 0x11111111;
    cpu.reg[1] = 0x22222222;
    cpu.reg[2] = 0x02000000;

    CPU::thumbInstr instr;
    instr.rs = 2;
    instr.imm = 0x03;  // R0 and R1

    cpu.opT_STMIA(instr);

    if (cpu.read32(0x02000000) != 0x11111111 || cpu.read32(0x02000004) != 0x22222222)
    {
        std::cout << "  Memory contents incorrect" << std::endl;
        return false;
    }

    if (cpu.reg[2] != 0x02000008)
    {
        std::cout << "  Base register not updated" << std::endl;
        return false;
    }

    return true;
}

bool testLDMIA(CPU& cpu)
{
    cpu.reset();
    cpu.reg[2] = 0x02000000;
    cpu.write32(0x02000000, 0x11111111);
    cpu.write32(0x02000004, 0x22222222);

    CPU::thumbInstr instr;
    instr.rs = 2;
    instr.imm = 0x03;  // R0 and R1

    cpu.opT_LDMIA(instr);

    if (cpu.reg[0] != 0x11111111 || cpu.reg[1] != 0x22222222)
    {
        std::cout << "  Registers not loaded correctly" << std::endl;
        return false;
    }

    if (cpu.reg[2] != 0x02000008)
    {
        std::cout << "  Base register not updated" << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// MAIN TEST RUNNER
// ============================================================

void DebuggerCPU::runAllThumbTests(CPU& cpu)
{
    int passCount = 0;
    int failCount = 0;

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Thumb Instruction Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Format 1: Shift operations
    std::cout << "--- Format 1: Move Shifted Register ---" << std::endl;
    if (testLSL_IMM(cpu)) { printTestResult("LSL_IMM", true); passCount++; }
    else { printTestResult("LSL_IMM", false); failCount++; }
    if (testLSL_IMM_Carry(cpu)) { printTestResult("LSL_IMM_Carry", true); passCount++; }
    else { printTestResult("LSL_IMM_Carry", false); failCount++; }
    if (testLSR_IMM(cpu)) { printTestResult("LSR_IMM", true); passCount++; }
    else { printTestResult("LSR_IMM", false); failCount++; }
    if (testLSR_IMM_Carry(cpu)) { printTestResult("LSR_IMM_Carry", true); passCount++; }
    else { printTestResult("LSR_IMM_Carry", false); failCount++; }
    if (testASR_IMM(cpu)) { printTestResult("ASR_IMM", true); passCount++; }
    else { printTestResult("ASR_IMM", false); failCount++; }

    // Format 2: Add/Subtract
    std::cout << "\n--- Format 2: Add/Subtract ---" << std::endl;
    if (testADD_REG(cpu)) { printTestResult("ADD_REG", true); passCount++; }
    else { printTestResult("ADD_REG", false); failCount++; }
    if (testADD_REG_Overflow(cpu)) { printTestResult("ADD_REG_Overflow", true); passCount++; }
    else { printTestResult("ADD_REG_Overflow", false); failCount++; }
    if (testADD_REG_Carry(cpu)) { printTestResult("ADD_REG_Carry", true); passCount++; }
    else { printTestResult("ADD_REG_Carry", false); failCount++; }
    if (testSUB_REG(cpu)) { printTestResult("SUB_REG", true); passCount++; }
    else { printTestResult("SUB_REG", false); failCount++; }
    if (testSUB_REG_Borrow(cpu)) { printTestResult("SUB_REG_Borrow", true); passCount++; }
    else { printTestResult("SUB_REG_Borrow", false); failCount++; }
    if (testADD_IMM(cpu)) { printTestResult("ADD_IMM", true); passCount++; }
    else { printTestResult("ADD_IMM", false); failCount++; }
    if (testSUB_IMM(cpu)) { printTestResult("SUB_IMM", true); passCount++; }
    else { printTestResult("SUB_IMM", false); failCount++; }

    // Format 3: Move/Compare/Add/Sub immediate
    std::cout << "\n--- Format 3: Move/Compare/Add/Sub Immediate ---" << std::endl;
    if (testMOV_IMM(cpu)) { printTestResult("MOV_IMM", true); passCount++; }
    else { printTestResult("MOV_IMM", false); failCount++; }
    if (testMOV_IMM_Zero(cpu)) { printTestResult("MOV_IMM_Zero", true); passCount++; }
    else { printTestResult("MOV_IMM_Zero", false); failCount++; }
    if (testCMP_IMM(cpu)) { printTestResult("CMP_IMM", true); passCount++; }
    else { printTestResult("CMP_IMM", false); failCount++; }
    if (testCMP_IMM_Less(cpu)) { printTestResult("CMP_IMM_Less", true); passCount++; }
    else { printTestResult("CMP_IMM_Less", false); failCount++; }
    if (testADD_IMM8(cpu)) { printTestResult("ADD_IMM8", true); passCount++; }
    else { printTestResult("ADD_IMM8", false); failCount++; }
    if (testSUB_IMM8(cpu)) { printTestResult("SUB_IMM8", true); passCount++; }
    else { printTestResult("SUB_IMM8", false); failCount++; }
    if (testSUB_IMM3(cpu)) { printTestResult("SUB_IMM3", true); passCount++; }
    else { printTestResult("SUB_IMM3", false); failCount++; }
    if (testSUB_IMM3_ZeroFlag(cpu)) { printTestResult("SUB_IMM3_ZeroFlag", true); passCount++; }
    else { printTestResult("SUB_IMM3_ZeroFlag", false); failCount++; }

    // Format 4: ALU operations
    std::cout << "\n--- Format 4: ALU Operations ---" << std::endl;
    if (testAND_REG(cpu)) { printTestResult("AND_REG", true); passCount++; }
    else { printTestResult("AND_REG", false); failCount++; }
    if (testEOR_REG(cpu)) { printTestResult("EOR_REG", true); passCount++; }
    else { printTestResult("EOR_REG", false); failCount++; }
    if (testLSL_REG(cpu)) { printTestResult("LSL_REG", true); passCount++; }
    else { printTestResult("LSL_REG", false); failCount++; }
    if (testLSR_REG(cpu)) { printTestResult("LSR_REG", true); passCount++; }
    else { printTestResult("LSR_REG", false); failCount++; }
    if (testASR_REG(cpu)) { printTestResult("ASR_REG", true); passCount++; }
    else { printTestResult("ASR_REG", false); failCount++; }
    if (testADC_REG(cpu)) { printTestResult("ADC_REG", true); passCount++; }
    else { printTestResult("ADC_REG", false); failCount++; }
    if (testSBC_REG(cpu)) { printTestResult("SBC_REG", true); passCount++; }
    else { printTestResult("SBC_REG", false); failCount++; }
    if (testROR_REG(cpu)) { printTestResult("ROR_REG", true); passCount++; }
    else { printTestResult("ROR_REG", false); failCount++; }
    if (testTST_REG(cpu)) { printTestResult("TST_REG", true); passCount++; }
    else { printTestResult("TST_REG", false); failCount++; }
    if (testNEG_REG(cpu)) { printTestResult("NEG_REG", true); passCount++; }
    else { printTestResult("NEG_REG", false); failCount++; }
    if (testCMN_REG(cpu)) { printTestResult("CMN_REG", true); passCount++; }
    else { printTestResult("CMN_REG", false); failCount++; }
    if (testORR_REG(cpu)) { printTestResult("ORR_REG", true); passCount++; }
    else { printTestResult("ORR_REG", false); failCount++; }
    if (testMUL_REG(cpu)) { printTestResult("MUL_REG", true); passCount++; }
    else { printTestResult("MUL_REG", false); failCount++; }
    if (testBIC_REG(cpu)) { printTestResult("BIC_REG", true); passCount++; }
    else { printTestResult("BIC_REG", false); failCount++; }
    if (testMVN_REG(cpu)) { printTestResult("MVN_REG", true); passCount++; }
    else { printTestResult("MVN_REG", false); failCount++; }

    // Format 5: Hi register operations
    std::cout << "\n--- Format 5: Hi Register Operations ---" << std::endl;
    if (testADD_HI(cpu)) { printTestResult("ADD_HI", true); passCount++; }
    else { printTestResult("ADD_HI", false); failCount++; }
    if (testCMP_HI(cpu)) { printTestResult("CMP_HI", true); passCount++; }
    else { printTestResult("CMP_HI", false); failCount++; }
    if (testMOV_HI(cpu)) { printTestResult("MOV_HI", true); passCount++; }
    else { printTestResult("MOV_HI", false); failCount++; }
    if (testBX(cpu)) { printTestResult("BX", true); passCount++; }
    else { printTestResult("BX", false); failCount++; }

    // Format 6: PC-relative load
    std::cout << "\n--- Format 6: PC-Relative Load ---" << std::endl;
    if (testLDR_PC(cpu)) { printTestResult("LDR_PC", true); passCount++; }
    else { printTestResult("LDR_PC", false); failCount++; }

    // Format 7: Load/Store with register offset
    std::cout << "\n--- Format 7: Load/Store Register Offset ---" << std::endl;
    if (testSTR_REG(cpu)) { printTestResult("STR_REG", true); passCount++; }
    else { printTestResult("STR_REG", false); failCount++; }
    if (testLDR_REG(cpu)) { printTestResult("LDR_REG", true); passCount++; }
    else { printTestResult("LDR_REG", false); failCount++; }
    if (testSTRB_REG(cpu)) { printTestResult("STRB_REG", true); passCount++; }
    else { printTestResult("STRB_REG", false); failCount++; }
    if (testLDRB_REG(cpu)) { printTestResult("LDRB_REG", true); passCount++; }
    else { printTestResult("LDRB_REG", false); failCount++; }

    // Format 8: Load/Store sign-extended
    std::cout << "\n--- Format 8: Load/Store Sign-Extended ---" << std::endl;
    if (testSTRH_REG(cpu)) { printTestResult("STRH_REG", true); passCount++; }
    else { printTestResult("STRH_REG", false); failCount++; }
    if (testLDRH_REG(cpu)) { printTestResult("LDRH_REG", true); passCount++; }
    else { printTestResult("LDRH_REG", false); failCount++; }
    if (testLDSB_REG(cpu)) { printTestResult("LDSB_REG", true); passCount++; }
    else { printTestResult("LDSB_REG", false); failCount++; }
    if (testLDSH_REG(cpu)) { printTestResult("LDSH_REG", true); passCount++; }
    else { printTestResult("LDSH_REG", false); failCount++; }

    // Format 9: Load/Store with immediate offset
    std::cout << "\n--- Format 9: Load/Store Immediate Offset ---" << std::endl;
    if (testSTR_IMM(cpu)) { printTestResult("STR_IMM", true); passCount++; }
    else { printTestResult("STR_IMM", false); failCount++; }
    if (testLDR_IMM(cpu)) { printTestResult("LDR_IMM", true); passCount++; }
    else { printTestResult("LDR_IMM", false); failCount++; }
    if (testSTRB_IMM(cpu)) { printTestResult("STRB_IMM", true); passCount++; }
    else { printTestResult("STRB_IMM", false); failCount++; }
    if (testLDRB_IMM(cpu)) { printTestResult("LDRB_IMM", true); passCount++; }
    else { printTestResult("LDRB_IMM", false); failCount++; }

    // Format 10: Load/Store halfword
    std::cout << "\n--- Format 10: Load/Store Halfword ---" << std::endl;
    if (testSTRH_IMM(cpu)) { printTestResult("STRH_IMM", true); passCount++; }
    else { printTestResult("STRH_IMM", false); failCount++; }
    if (testLDRH_IMM(cpu)) { printTestResult("LDRH_IMM", true); passCount++; }
    else { printTestResult("LDRH_IMM", false); failCount++; }

    // Format 11: SP-relative load/store
    std::cout << "\n--- Format 11: SP-Relative Load/Store ---" << std::endl;
    if (testSTR_SP(cpu)) { printTestResult("STR_SP", true); passCount++; }
    else { printTestResult("STR_SP", false); failCount++; }
    if (testLDR_SP(cpu)) { printTestResult("LDR_SP", true); passCount++; }
    else { printTestResult("LDR_SP", false); failCount++; }

    // Format 12: Load address
    std::cout << "\n--- Format 12: Load Address ---" << std::endl;
    if (testADD_PC(cpu)) { printTestResult("ADD_PC", true); passCount++; }
    else { printTestResult("ADD_PC", false); failCount++; }
    if (testADD_SP_IMM(cpu)) { printTestResult("ADD_SP_IMM", true); passCount++; }
    else { printTestResult("ADD_SP_IMM", false); failCount++; }

    // Format 13: Add offset to SP
    std::cout << "\n--- Format 13: Add Offset to SP ---" << std::endl;
    if (testADD_SP(cpu)) { printTestResult("ADD_SP", true); passCount++; }
    else { printTestResult("ADD_SP", false); failCount++; }
    if (testSUB_SP(cpu)) { printTestResult("SUB_SP", true); passCount++; }
    else { printTestResult("SUB_SP", false); failCount++; }

    // Format 14: Push/Pop
    std::cout << "\n--- Format 14: Push/Pop Registers ---" << std::endl;
    if (testPUSH(cpu)) { printTestResult("PUSH", true); passCount++; }
    else { printTestResult("PUSH", false); failCount++; }
    if (testPUSH_LR(cpu)) { printTestResult("PUSH_LR", true); passCount++; }
    else { printTestResult("PUSH_LR", false); failCount++; }
    if (testPOP(cpu)) { printTestResult("POP", true); passCount++; }
    else { printTestResult("POP", false); failCount++; }
    if (testPOP_PC(cpu)) { printTestResult("POP_PC", true); passCount++; }
    else { printTestResult("POP_PC", false); failCount++; }

    // Format 15: Multiple load/store
    std::cout << "\n--- Format 15: Multiple Load/Store ---" << std::endl;
    if (testSTMIA(cpu)) { printTestResult("STMIA", true); passCount++; }
    else { printTestResult("STMIA", false); failCount++; }
    if (testLDMIA(cpu)) { printTestResult("LDMIA", true); passCount++; }
    else { printTestResult("LDMIA", false); failCount++; }

    // Print summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "PASSED: " << passCount << std::endl;
    std::cout << "FAILED: " << failCount << std::endl;
    std::cout << "TOTAL:  " << (passCount + failCount) << std::endl;
    std::cout << "========================================" << std::endl;

    if (failCount == 0)
    {
        std::cout << "\n All tests passed yaaay" << std::endl;
    }
    else
    {
        std::cout << "\n " << failCount << " tests failed" << std::endl;
    }
}