public:

	//////////////////////////////////////////////////////////////////
	//							EVERY ARM FUNC					    //
	//////////////////////////////////////////////////////////////////

	int opA_AND(CPUTypes::armInstr instr);
	int opA_EOR(CPUTypes::armInstr instr);
	int opA_SUB(CPUTypes::armInstr instr);
	int opA_RSB(CPUTypes::armInstr instr);
	int opA_ADD(CPUTypes::armInstr instr);
	int opA_ADC(CPUTypes::armInstr instr);
	int opA_SBC(CPUTypes::armInstr instr);
	int opA_RSC(CPUTypes::armInstr instr);
	int opA_TST(CPUTypes::armInstr instr);
	int opA_TEQ(CPUTypes::armInstr instr);
	int opA_CMP(CPUTypes::armInstr instr);
	int opA_CMN(CPUTypes::armInstr instr);
	int opA_ORR(CPUTypes::armInstr instr);
	int opA_MOV(CPUTypes::armInstr instr);
	int opA_BIC(CPUTypes::armInstr instr);
	int opA_MVN(CPUTypes::armInstr instr);
	int opA_MRS(CPUTypes::armInstr instr);
	int opA_MSR(CPUTypes::armInstr instr);
	int opA_LDR(CPUTypes::armInstr instr);
	int opA_STR(CPUTypes::armInstr instr);
	int opA_LDRH(CPUTypes::armInstr instr);
	int opA_STRH(CPUTypes::armInstr instr);
	int opA_LDRSB(CPUTypes::armInstr instr);
	int opA_LDRSH(CPUTypes::armInstr instr);
	int opA_LDM(CPUTypes::armInstr instr);
	int opA_STM(CPUTypes::armInstr instr);
	int opA_B(CPUTypes::armInstr instr);
	int opA_BL(CPUTypes::armInstr instr);
	int opA_BX(CPUTypes::armInstr instr);
	int opA_MUL(CPUTypes::armInstr instr);
	int opA_MLA(CPUTypes::armInstr instr);
	int opA_UMULL(CPUTypes::armInstr instr);
	int opA_UMLAL(CPUTypes::armInstr instr);
	int opA_SMULL(CPUTypes::armInstr instr);
	int opA_SMLAL(CPUTypes::armInstr instr);
	int opA_SWP(CPUTypes::armInstr instr);
	int opA_SWI(CPUTypes::armInstr instr);
	int opA_CDP(CPUTypes::armInstr instr);
	int opA_LDC(CPUTypes::armInstr instr);
	int opA_STC(CPUTypes::armInstr instr);
	int opA_MRC(CPUTypes::armInstr instr);
	int opA_MCR(CPUTypes::armInstr instr);
	int opA_UNDEFINED(CPUTypes::armInstr instr);

public: // helper for data rpocessing

	void writeALUResult(uint8_t rdI, uint32_t result, bool s);

	// new arm ops
	uint32_t getArmOp2(CPUTypes::armInstr instr, bool* carryOut);
	uint32_t getArmOffset(CPUTypes::armInstr instr);

	uint8_t DPgetRn();
	uint8_t DPgetRd();
	uint8_t DPgetRs();
	uint8_t DPgetRm();

	uint8_t DPgetShift();

	uint8_t DPgetImmed();
	uint8_t DPgetRotate();

	uint8_t DPgetShiftAmount(uint8_t shift);

	bool DPs();
	bool DPi();

	uint32_t DPgetOp2(bool* carryFlag);

	// cycle calculation helpers

	int dataProcessingCycleCalculator();

	// shift helpers

	uint32_t DPshiftLSL(uint32_t value, uint8_t shift_amount, bool* carry_out);
	uint32_t DPshiftLSR(uint32_t value, uint8_t shift_amount, bool* carry_out);
	uint32_t DPshiftASR(uint32_t value, uint8_t shift_amount, bool* carry_out);
	uint32_t DPshiftROR(uint32_t value, uint8_t shift_amount, bool* carry_out);

	//shift for memory
	uint32_t SDapplyShift(uint32_t rmVal, uint8_t type, uint8_t amount); // singledata apply shift
	uint32_t applyRegisterShift(uint32_t value, uint8_t shift_type, uint8_t shift_amount, bool* carry_out);

	//flag related helper

	void setFlagNZC(uint32_t res, bool isCarry); // no addition or subtraction
	void setFlagsAdd(uint32_t res, uint32_t op1, uint32_t op2);// ADD CHECK
	void setFlagsSub(uint32_t res, uint32_t op1, uint32_t op2); // SUB CHECK
	void setNZ(uint32_t res); // TEST CHECK

