#pragma once
#include <cstdint>

namespace tdmi7
{
namespace CPUTypes
{

	enum class thumbOperation
	{
		THUMB_MOV_IMM,
		THUMB_ADD_REG,
		THUMB_ADD_IMM,
		THUMB_ADD_IMM3,
		THUMB_SUB_REG,
		THUMB_SUB_IMM,
		THUMB_SUB_IMM3,
		THUMB_CMP_IMM,
		THUMB_LSL_IMM,
		THUMB_LSR_IMM,
		THUMB_ASR_IMM,
		THUMB_AND_REG,
		THUMB_EOR_REG,
		THUMB_LSL_REG,
		THUMB_LSR_REG,
		THUMB_ASR_REG,
		THUMB_ADC_REG,
		THUMB_SBC_REG,
		THUMB_ROR_REG,
		THUMB_TST_REG,
		THUMB_NEG_REG,
		THUMB_CMP_REG,
		THUMB_CMN_REG,
		THUMB_ORR_REG,
		THUMB_MUL_REG,
		THUMB_BIC_REG,
		THUMB_MVN_REG,
		THUMB_ADD_HI,
		THUMB_CMP_HI,
		THUMB_MOV_HI,
		THUMB_BX,
		THUMB_BLX_REG,
		THUMB_LDR_PC,
		THUMB_LDR_REG,
		THUMB_STR_REG,
		THUMB_LDRB_REG,
		THUMB_STRB_REG,
		THUMB_LDRH_REG,
		THUMB_STRH_REG,
		THUMB_LDRSB_REG,
		THUMB_LDRSH_REG,

		THUMB_LDR_IMM,
		THUMB_STR_IMM,
		THUMB_LDRB_IMM,
		THUMB_STRB_IMM,
		THUMB_LDRH_IMM,
		THUMB_STRH_IMM,
		THUMB_LDR_SP,
		THUMB_STR_SP,
		THUMB_ADD_PC,
		THUMB_ADD_SP,
		THUMB_ADD_SP_IMM,
		THUMB_PUSH,
		THUMB_POP,
		THUMB_STMIA,
		THUMB_LDMIA,
		THUMB_B_COND,
		THUMB_B,
		THUMB_BL_PREFIX,
		THUMB_BL_SUFFIX,
		THUMB_SWI,
		THUMB_UNDEFINED,

		COUNT,
	};

	enum class armOperation
	{
		ARM_ADD,
		ARM_SUB,
		ARM_RSB,
		ARM_ADC,
		ARM_SBC,
		ARM_RSC,

		ARM_AND,
		ARM_EOR,
		ARM_ORR,
		ARM_BIC,

		ARM_TST,
		ARM_TEQ,
		ARM_CMP,
		ARM_CMN,

		ARM_MOV,
		ARM_MVN,

		ARM_MUL,
		ARM_MLA,
		ARM_UMULL,
		ARM_UMLAL,
		ARM_SMULL,
		ARM_SMLAL,

		ARM_LDR,
		ARM_STR,
		ARM_LDRH,
		ARM_STRH,
		ARM_LDRSB,
		ARM_LDRSH,

		ARM_LDM,
		ARM_STM,

		ARM_B,
		ARM_BL,
		ARM_BX,

		ARM_MRS,
		ARM_MSR,
		ARM_SWP,

		ARM_SWI,

		ARM_CDP,
		ARM_LDC,
		ARM_STC,
		ARM_MRC,
		ARM_MCR,

		ARM_UNDEFINED,

		COUNT
	};

    	struct thumbInstr
	{
		thumbOperation type;

		uint8_t rd;
		uint8_t rs;
		uint8_t rn;

		uint32_t imm;

		uint8_t cond;

		bool h1;         // hi register f1
		bool h2;         // hi register f2
	};

	struct armInstr
	{
		armOperation type;

		uint8_t rd;
		uint8_t rn;
		uint8_t rs;
		uint8_t rm;

		uint32_t imm;
		uint8_t rotate;

		uint8_t cond;

		bool S;          // set condition codes
		bool L;          // 1 is load, 0 is store
		bool W;          // write back
		bool P;          // 1 is pre index, 0 is post
		bool U;          // 1 is add offs, 0 is sub
		bool B;          // 1 for byte, 0 for word
		bool H;          // is halfword or byte
		bool I;          // immed operand

		// shift info
		uint8_t shift_type;    // 00=LSL, 01=LSR, 10=ASR, 11=ROR
		uint8_t shift_amount;  // immed shift amount (0-31)
		uint8_t shift_reg;     // reg containing shift amount
		bool shift_by_reg;     // true if shift amount in register

		uint16_t reg_list;// reg list (for load multiple etc)
		uint32_t raw; // raw val
	};

};
}
