public:

	Bus* bus;
	Decoder* decoder;
	CPU(Bus*, Decoder*);
	void reset();

	using thumbInstr = CPUTypes::thumbInstr;
	using armInstr = CPUTypes::armInstr;
	using thumbOperation = CPUTypes::thumbOperation;
	using armOperation = CPUTypes::armOperation;

	enum class Operation
	{
		// Data processing
		AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC,
		TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN,

		MRS, MSR, // both used for PSR transfer

		// Load/Store
		LDR, STR, // SINGLE DATA TRANSFER
		LDRH, STRH, LDRSB, LDRSH, // halfword and signed data transfer
		LDM, STM,

		// Branch
		B, BL, BX,

		// Multiply
		MUL, MLA, UMULL, UMLAL, SMULL, SMLAL,

		// Special
		SWP, SWPB, SWI,

		//coprocessor

		CDP, LDC, STC, MRC, MCR,

		//general break
		UNKNOWN,
		//more specific breaks
		UNASSIGNED, // this is only given at the start
		CONDITIONALSKIP, // this is only given on conditionalFail
		SINGLEDATATRANSFERUNDEFINED, //theres a single instruction in decode thats the same as executeSingleDataTransfer, but tells is if bit 4 is set, it becomes undefined
		DECODEFAIL, // if it reaches the end of the decoding list and never gets anything it returns this
		//should be some realistically to tell me why it failed

		COUNT // this is just so i have a way of counting how much enums i have in case of future new enum declared
	};

	CPUTypes::armInstr curArmInstr;

public: // FUNCTION ARRAYS

	using OpAFunction = int (CPU::*)(CPUTypes::armInstr);
	OpAFunction opA_functions[static_cast<int>(Operation::COUNT)];

	using OpTFunction = int (CPU::*)(CPUTypes::thumbInstr);
	OpTFunction opT_functions[static_cast<int>(CPUTypes::thumbOperation::COUNT)];

public:


	void initializeOpFunctions(); // this is for initing the list of enums to funcs

	uint16_t curOpCycles; // this is defaulted to 0 every time
	int cycleTotal; // this is how we find out how many cycles have passed

	uint32_t tick();

	// External hardware raises IRQ/FIQ between instructions.  The bus and
	// peripherals will drive these once the system scheduler is in place.
	void requestIrq();
	void requestFiq();
	void setIrqLine(bool asserted);
	void setHalted(bool value) { halted = value; }
	bool isHalted() const { return halted; }

	// Opt-in test ABI for raw instruction binaries.  When enabled, SWI #0
	// reports pass and SWI #1 reports fail instead of entering the SWI vector.
	enum class TestHalt : uint8_t
	{
		None,
		Passed,
		Failed,
	};

	void enableTestSwiHalt(bool enabled);
	void setHleSwiHandler(std::function<bool(uint32_t, bool)> handler) { hleSwiHandler = std::move(handler); }
	bool handleTestSwi(uint32_t immediate);
	TestHalt testHalt = TestHalt::None;
	std::function<bool(uint32_t, bool)> hleSwiHandler;
	//Operation decode(uint32_t passedIns);
	CPUTypes::armInstr decodeArm(uint32_t instr);
	int armExecute(CPUTypes::armInstr instr);

	uint32_t reg[16];

	uint32_t& sp; // stack pointer ~ points to 13
	uint32_t& lr; // link register ~ points to 14
	uint32_t& pc; // program counter ~ points to 15

	// instruction to take
	uint32_t instruction;
	//decoded operation
	Operation curOP;

private:
	bool testSwiHaltEnabled = false;
	bool irqPending = false;
	bool fiqPending = false;
	bool halted = false;

public:
