#include "tdmi7/CPU.h"
#include "tdmi7/LegacyTestMemory.h"

#include <cstdio>
#include <iostream>

namespace tdmi7
{
using namespace CPUTypes;
using tdmi7::legacy::Transaction;
using tdmi7::legacy::testBaseAddress;
using tdmi7::legacy::testThumbOpcode;
using tdmi7::legacy::transactions;

namespace
{
void printBits(uint32_t value)
{
    for (int bit = 31; bit >= 0; --bit)
    {
        std::printf("%d", (value >> bit) & 1U);
    }
    std::printf("\n");
}
}

bool CPU::runIndividualTests(const char* fixturePath)
// this function is for running the individual ARM + THUMB single instruction tests, ensuring every single bound is checked	for the most critical instructions
{
	//loads the single test file in (each is composed of 5000 individual tests on the one instruction)
	const char* str = fixturePath;

	FILE* f = fopen(str, "rb");
	if (!f)
	{
		printf("ERROR: Could not open test file!\n");
		return false;
	}

	int passed = 0;
	int failed = 0;
	constexpr int maxFailuresToShow = 10;
	int failuresShown = 0;

	uint32_t magic, numTests, testSize, stateSize, val;
	fread(&magic, 4, 1, f);
	fread(&numTests, 4, 1, f);


	printf("Magic: 0x%08x\n", magic);
	printf("Number of tests: %d\n\n", numTests);

	//load the arrays with the values from the file
	//there are some junk arrays in here, they serve no purpouse
	//the author of the tests wanted to include buffers
	for (int tNum = 0; tNum < static_cast<int>(numTests); tNum++)
	{
		fread(&testSize, 4, 1, f);
		fread(&stateSize, 4, 1, f);
		fread(&val, 4, 1, f);
		int amtOfTransactions = (testSize - 368) / 24;
		uint32_t R_init[16];
		fread(R_init, 4, 16, f);
		uint32_t R_fiq_init[7];
		fread(R_fiq_init, 4, 7, f);
		uint32_t R_svc_init[2];
		fread(R_svc_init, 4, 2, f);
		uint32_t R_abt_init[2];
		fread(R_abt_init, 4, 2, f);
		uint32_t R_irq_init[2];
		fread(R_irq_init, 4, 2, f);
		uint32_t R_und_init[2];
		fread(R_und_init, 4, 2, f);

		uint32_t CPSR_init;
		fread(&CPSR_init, 4, 1, f);

		uint32_t SPSR_init[5];
		fread(SPSR_init, 4, 5, f);
		uint32_t pipeline_init[2];
		fread(pipeline_init, 4, 2, f);
		uint32_t access_init;
		fread(&access_init, 4, 1, f);

		uint32_t junkA;
		fread(&junkA, 4, 1, f);
		uint32_t junkB;
		fread(&junkB, 4, 1, f);

		uint32_t R_final[16];
		fread(R_final, 4, 16, f);
		uint32_t R_fiq_final[7];
		fread(R_fiq_final, 4, 7, f);
		uint32_t R_svc_final[2];
		fread(R_svc_final, 4, 2, f);
		uint32_t R_abt_final[2];
		fread(R_abt_final, 4, 2, f);
		uint32_t R_irq_final[2];
		fread(R_irq_final, 4, 2, f);
		uint32_t R_und_final[2];
		fread(R_und_final, 4, 2, f);
		uint32_t CPSR_final;
		fread(&CPSR_final, 4, 1, f);
		uint32_t SPSR_final[5]; //WARNING: IF EVER COMPARED AGAINST, INDEX 0 IS 0XDEADBEEF, offset is 1 to deal with user mode being 0 and having no SPSR
		fread(SPSR_final, 4, 5, f);
		uint32_t pipeline_final[2];
		fread(pipeline_final, 4, 2, f);
		uint32_t access_final;
		fread(&access_final, 4, 1, f);;
		fread(&junkA, 4, 1, f);
		fread(&junkB, 4, 1, f);
		uint32_t junkC;
		fread(&junkC, 4, 1, f);

		// TRANSACTIONS 
		// The tests load into certain memory instructions
		// during our testing, if a memory adress not listed here is read, it can be flagged as an error on the memory input address side
		transactions.clear();
		int transactionCounter = 0;
		while (transactionCounter < amtOfTransactions)
		{
			Transaction trans;
			fread(&trans.kind, 4, 1, f);
			fread(&trans.size, 4, 1, f);
			fread(&trans.addr, 4, 1, f);
			fread(&trans.data, 4, 1, f);
			fread(&trans.cycle, 4, 1, f);
			fread(&trans.access, 4, 1, f);
			transactions.push_back(trans);
			transactionCounter++;
		}
		uint32_t junkArr2[3];
		fread(&junkArr2, 4, 2, f);
		uint32_t opcode; //uint16_t opcode; 
		uint32_t base_addr;
		fread(&opcode, 4, 1, f);
		fread(&base_addr, 4, 1, f);


		//////////////
		// LOADS
		////////////

		armInstr decoded = decodeArm(opcode);
		{
			//armInstr decoded = decodeArm(opcode);
			reset();

			for (int r = 0; r < 16; r++)
				reg[r] = R_init[r];

			pc = base_addr + 4;
			CPSR = CPSR_init; //load cspr
			// THIS IS DONE IN A HORRIBLE WAY
			// SINCE THE TEST LOADS IT IN A WONKY WAY

			// SPSR registers in this order: fiq, svc, abt, irq, und
			spsrBank[0] = 0xDEADBEEF;
			spsrBank[getModeIndex(mode::FIQ)] = SPSR_init[0];
			spsrBank[getModeIndex(mode::Supervisor)] = SPSR_init[1];
			spsrBank[getModeIndex(mode::Abort)] = SPSR_init[2];
			spsrBank[getModeIndex(mode::IRQ)] = SPSR_init[3];
			spsrBank[getModeIndex(mode::Undefined)] = SPSR_init[4];

			
			for (int i = 0; i < 5; i++)
				r8FIQ[i] = R_fiq_init[i];  

			r13RegBank[1] = R_fiq_init[5];  
			r14RegBank[1] = R_fiq_init[6];  
			r13RegBank[2] = R_irq_init[0];
			r14RegBank[2] = R_irq_init[1]; 
			r13RegBank[3] = R_svc_init[0]; 
			r14RegBank[3] = R_svc_init[1];  
			r13RegBank[4] = R_abt_init[0];
			r14RegBank[4] = R_abt_init[1];
			r13RegBank[5] = R_und_init[0];
			r14RegBank[5] = R_und_init[1];

			curMode = mode::System;
			switchMode(CPSRbitToMode(CPSR & 0x1F));
			///DECODE / EXECUTE

			//THUMB
			// 
			//thumbInstr decoded = decodeThumb(opcode);
			//std::string decodedStr = thumbToStr(decoded);
			//pc+=2 
			// 
			//ARM

			//armInstr decoded = decodeArm(opcode);
			std::string decodedStr = armToStr(decoded);
			curOpCycles = armExecute(decoded);
			pc += 4; 


			// Check results compares following
			// CPSR
			// Standard Reg
			// FIQ reg
			// fiq 0-6
			// irq 0-1
			// svc 0-1
			// abt 0-1
			// und 0-1

			bool testPassed = true;
			mode failedOnMode = curMode; // remember mode system failed on 


			// cpsr spsr checks
			if (CPSR != CPSR_final)
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED CPSR, opcode:0x%08x  cur:0x%08x , expect:0x%08x , og:0x%08x , |%s| mode: %02x\n", tNum, decoded.raw, CPSR, CPSR_final,CPSR_init, decodedStr.c_str(), static_cast<unsigned int>(failedOnMode));
					printf("org: ");printBits(CPSR_init);
					printf("cur: ");printBits(CPSR);
					printf("exp: ");printBits(CPSR_final);
					printf("     ");printf("NZCV                    IFT43210\n");
				}
			}
			if (failuresShown < maxFailuresToShow && spsrBank[getModeIndex(mode::FIQ)] != SPSR_init[0])			{ printf("Test %d SPSR FAIL og:%08x cr:%08x fn:%08x | FIQ\n", tNum, SPSR_init[0], spsrBank[getModeIndex(mode::FIQ)]			, SPSR_final[0]); }
			if (failuresShown < maxFailuresToShow && spsrBank[getModeIndex(mode::Supervisor)] != SPSR_init[1])	{ printf("Test %d SPSR FAIL og:%08x cr:%08x fn:%08x | SPV\n", tNum, SPSR_init[1], spsrBank[getModeIndex(mode::Supervisor)]	, SPSR_final[1]); }
			if (failuresShown < maxFailuresToShow && spsrBank[getModeIndex(mode::Abort)] != SPSR_init[2])		{ printf("Test %d SPSR FAIL og:%08x cr:%08x fn:%08x | ABT\n", tNum, SPSR_init[2], spsrBank[getModeIndex(mode::Abort)]		, SPSR_final[2]); }
			if (failuresShown < maxFailuresToShow && spsrBank[getModeIndex(mode::IRQ)] != SPSR_init[3])			{ printf("Test %d SPSR FAIL og:%08x cr:%08x fn:%08x | IRQ\n", tNum, SPSR_init[3], spsrBank[getModeIndex(mode::IRQ)]			, SPSR_final[3]); }
			if (failuresShown < maxFailuresToShow && spsrBank[getModeIndex(mode::Undefined)] != SPSR_init[4])	{ printf("Test %d SPSR FAIL og:%08x cr:%08x fn:%08x | UND\n", tNum, SPSR_init[4], spsrBank[getModeIndex(mode::Undefined)]	, SPSR_final[4]); }

			// Swap mode, reg compare
			switchMode(mode::System);
			for (int r = 0; r < 16; r++)
			{
				if (reg[r] != R_final[r])
				{
					testPassed = false;
					if (failuresShown < maxFailuresToShow)
					{ //IRQ = 0x12,Supervisor = 0x13,Abort = 0x17,
						printf("Test %d FAILED  (Opcode 0x%04x @ 0x%08x): r%d = 0x%08x, expected 0x%08x | %s | %s | | %08x\n", 
						       tNum, opcode, base_addr, r, reg[r], R_final[r] , CPSRtoString() , decodedStr.c_str(), static_cast<unsigned int>(failedOnMode));
					}
					//if (failuresShown < maxFailuresToShow)
					//{ //IRQ = 0x12,Supervisor = 0x13,Abort = 0x17,
					//	printf("Test %d FAILED  (Opcode 0x%04x @ 0x%08x): %08x | %s | %s | | %08x\n",
					//	tNum, opcode, base_addr, r ,CPSRtoString(), decodedStr.c_str(), failedOnMode);
					//	printBits(reg[r]);
					//	printBits(R_final[r]);
					//}
				}
			}
			for (int i = 0; i < 5; i++) 
			{
				if (r8FIQ[i] != R_fiq_final[i])
				{
					testPassed = false;
					if (failuresShown < maxFailuresToShow)
					{
						printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r%d_fiq = 0x%08x, expected 0x%08x | %s| %s\n",
							tNum, opcode, base_addr, 8 + i, r8FIQ[i], R_fiq_final[i], CPSRtoString(), decodedStr.c_str() );
					}
				}
			}
			if (r13RegBank[1] != R_fiq_final[5])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r13_fiq = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r13RegBank[1], R_fiq_final[5], CPSRtoString(), decodedStr.c_str() );
				}
			}
			if (r14RegBank[1] != R_fiq_final[6])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r14_fiq = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r14RegBank[1], R_fiq_final[6], CPSRtoString(), decodedStr.c_str() );
				}
			}
			if (r13RegBank[2] != R_irq_final[0])// Check IRQ 
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r13_irq = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r13RegBank[2], R_irq_final[0], CPSRtoString(), decodedStr.c_str());
				}
			}
			if (r14RegBank[2] != R_irq_final[1])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r14_irq = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r14RegBank[2], R_irq_final[1], CPSRtoString(), decodedStr.c_str());
				}
			}
			if (r13RegBank[3] != R_svc_final[0])// Check Supervisor
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r13_svc = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r13RegBank[3], R_svc_final[0], CPSRtoString(), decodedStr.c_str());
				}
			}
			if (r14RegBank[3] != R_svc_final[1])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r14_svc = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r14RegBank[3], R_svc_final[1], CPSRtoString(), decodedStr.c_str());
				}
			}
			// Check Abort
			if (r13RegBank[4] != R_abt_final[0])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r13_abt = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r13RegBank[4], R_abt_final[0], CPSRtoString(), decodedStr.c_str());
				}
			}
			if (r14RegBank[4] != R_abt_final[1])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r14_abt = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r14RegBank[4], R_abt_final[1], CPSRtoString(), decodedStr.c_str());
				}
			}
			// Check Undefined
			if (r13RegBank[5] != R_und_final[0])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r13_und = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r13RegBank[5], R_und_final[0], CPSRtoString(), decodedStr.c_str());
				}
			}
			if (r14RegBank[5] != R_und_final[1])
			{
				testPassed = false;
				if (failuresShown < maxFailuresToShow)
				{
					printf("Test %d FAILED  (opcode 0x%04x @ 0x%08x): r14_und = 0x%08x, expected 0x%08x | %s | %s\n",
						tNum, opcode, base_addr, r14RegBank[5], R_und_final[1], CPSRtoString(), decodedStr.c_str());
				}
			}

			if (testPassed)
				passed++;
			else
			{
				failed++;
				if (failuresShown < maxFailuresToShow)
					failuresShown++;
				else if (failuresShown == maxFailuresToShow)
				{
					printf("  ... (suppressing further failures)\n");
					failuresShown++;
				}
			}
			if (tNum > 0 && tNum % 5000 == 0)
				printf("  Progress: %d/%d... (%d passed, %d failed)\n",
					tNum, numTests, passed, failed);
		}
	}
	printf("\n========================================\n");
	printf("Results: %d passed, %d failed out of %d\n",
		passed, failed, numTests);
	printf("========================================\n");

	fclose(f);
	return failed == 0;
}

}
