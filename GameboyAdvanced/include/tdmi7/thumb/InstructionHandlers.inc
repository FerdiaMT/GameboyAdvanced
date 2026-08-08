public:

	//////////////////////////////////////////////////////////////////
	//							THUMB STUFF							//
	//////////////////////////////////////////////////////////////////




	CPUTypes::thumbInstr curThumbInstr;

	CPUTypes::thumbInstr debugDecodedInstr(); //used to create a struct full of nulls , usefull for printig debugs

	CPUTypes::thumbInstr decodeThumb(uint16_t instruction); // this returns a CPUTypes::thumbInstr struct
	int thumbExecute(struct CPUTypes::thumbInstr);

	//THUMB HELPERS
	void updateFlagsNZCV_Add(uint32_t result, uint32_t op1, uint32_t op2);

	void updateFlagsNZCV_Sub(uint32_t result, uint32_t op1, uint32_t op2);


public:

	//////////////////////////////////////////////////////////////////
	//							EVERY THUMB FUNC				    //
	//////////////////////////////////////////////////////////////////


	int opT_MOV_IMM(CPUTypes::thumbInstr instr);
	int opT_ADD_REG(CPUTypes::thumbInstr instr);
	int opT_ADD_IMM(CPUTypes::thumbInstr instr);
	int opT_ADD_IMM3(CPUTypes::thumbInstr instr);
	int opT_SUB_REG(CPUTypes::thumbInstr instr);
	int opT_SUB_IMM(CPUTypes::thumbInstr instr);
	int opT_SUB_IMM3(CPUTypes::thumbInstr instr);
	int opT_CMP_IMM(CPUTypes::thumbInstr instr);
	int opT_LSL_IMM(CPUTypes::thumbInstr instr);
	int opT_LSR_IMM(CPUTypes::thumbInstr instr);
	int opT_ASR_IMM(CPUTypes::thumbInstr instr);
	int opT_AND_REG(CPUTypes::thumbInstr instr);
	int opT_EOR_REG(CPUTypes::thumbInstr instr);
	int opT_LSL_REG(CPUTypes::thumbInstr instr);
	int opT_LSR_REG(CPUTypes::thumbInstr instr);
	int opT_ASR_REG(CPUTypes::thumbInstr instr);
	int opT_ADC_REG(CPUTypes::thumbInstr instr);
	int opT_SBC_REG(CPUTypes::thumbInstr instr);
	int opT_ROR_REG(CPUTypes::thumbInstr instr);
	int opT_TST_REG(CPUTypes::thumbInstr instr);
	int opT_NEG_REG(CPUTypes::thumbInstr instr);
	int opT_CMP_REG(CPUTypes::thumbInstr instr);
	int opT_CMN_REG(CPUTypes::thumbInstr instr);
	int opT_ORR_REG(CPUTypes::thumbInstr instr);
	int opT_MUL_REG(CPUTypes::thumbInstr instr);
	int opT_BIC_REG(CPUTypes::thumbInstr instr);
	int opT_MVN_REG(CPUTypes::thumbInstr instr);
	int opT_ADD_HI(CPUTypes::thumbInstr instr);
	int opT_CMP_HI(CPUTypes::thumbInstr instr);
	int opT_MOV_HI(CPUTypes::thumbInstr instr);
	int opT_BX(CPUTypes::thumbInstr instr);
	int opT_BLX_REG(CPUTypes::thumbInstr instr);
	int opT_LDR_PC(CPUTypes::thumbInstr instr);
	int opT_LDR_REG(CPUTypes::thumbInstr instr);
	int opT_STR_REG(CPUTypes::thumbInstr instr);
	int opT_LDRB_REG(CPUTypes::thumbInstr instr);
	int opT_STRB_REG(CPUTypes::thumbInstr instr);
	int opT_LDRH_REG(CPUTypes::thumbInstr instr);
	int opT_STRH_REG(CPUTypes::thumbInstr instr);
	int opT_LDRSB_REG(CPUTypes::thumbInstr instr);
	int opT_LDRSH_REG(CPUTypes::thumbInstr instr);

	int opT_LDR_IMM(CPUTypes::thumbInstr instr);
	int opT_STR_IMM(CPUTypes::thumbInstr instr);
	int opT_LDRB_IMM(CPUTypes::thumbInstr instr);
	int opT_STRB_IMM(CPUTypes::thumbInstr instr);
	int opT_LDRH_IMM(CPUTypes::thumbInstr instr);
	int opT_STRH_IMM(CPUTypes::thumbInstr instr);
	int opT_LDR_SP(CPUTypes::thumbInstr instr);
	int opT_STR_SP(CPUTypes::thumbInstr instr);
	int opT_ADD_PC(CPUTypes::thumbInstr instr);
	int opT_ADD_SP(CPUTypes::thumbInstr instr);
	int opT_ADD_SP_IMM(CPUTypes::thumbInstr instr);
	int opT_PUSH(CPUTypes::thumbInstr instr);
	int opT_POP(CPUTypes::thumbInstr instr);
	int opT_STMIA(CPUTypes::thumbInstr instr);
	int opT_LDMIA(CPUTypes::thumbInstr instr);
	int opT_B_COND(CPUTypes::thumbInstr instr);
	int opT_B(CPUTypes::thumbInstr instr);
	int opT_BL_PREFIX(CPUTypes::thumbInstr instr);
	int opT_BL_SUFFIX(CPUTypes::thumbInstr instr);
	int opT_SWI(CPUTypes::thumbInstr instr);
	int opT_UNDEFINED(CPUTypes::thumbInstr instr);
