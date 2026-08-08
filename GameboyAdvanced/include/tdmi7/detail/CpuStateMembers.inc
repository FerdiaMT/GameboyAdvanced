	//current program status registers
	union
	{
		struct
		{
			uint32_t M0 : 1;
			uint32_t M1 : 1;
			uint32_t M2 : 1;
			uint32_t M3 : 1;
			uint32_t M4 : 1;
			uint32_t T : 1;
			uint32_t F : 1;
			uint32_t I : 1;

			uint32_t RESERVED : 20;
			uint32_t V : 1;
			uint32_t C : 1;
			uint32_t Z : 1;
			uint32_t N : 1;
		};
		uint32_t CPSR;
	};

	//SPSR



public:
	uint32_t getHalfWordOffset(CPUTypes::armInstr instr);
	const char* CPSRtoString();
	std::string CPSRtoStringPASSED(uint32_t base, uint32_t final, uint32_t passed);
	uint32_t ThumbToARM(uint16_t thumbInstr, uint32_t pc, uint16_t nextThumbInstr);
	bool checkConditional(uint8_t cond) const;


	uint8_t pcOffset();

	uint8_t read8(uint32_t addr, bool bReadOnly = false);
	uint16_t read16(uint32_t addr, bool bReadOnly = false);
	uint32_t read32(uint32_t addr, bool bReadOnly = false);
	uint16_t fetch16(uint32_t addr);
	uint32_t fetch32(uint32_t addr);

	void write8(uint32_t addr, uint8_t data);
	void write16(uint32_t addr, uint16_t data);
	void write32(uint32_t addr, uint32_t data);

public:
	enum class mode : uint8_t
	{
		User = 0x10,
		FIQ = 0x11,
		IRQ = 0x12,
		Supervisor = 0x13,
		Abort = 0x17,
		Undefined = 0x1B,
		System = 0x1F
	};

public:
	//OPS FOR MODE SWITCHING / EXCEPTION HANDLING



	mode curMode = mode::Supervisor; // curMode should default to Supervisor



	bool isPrivilegedMode(); // used to quickly tell were not in user mode
	uint8_t getModeIndex(CPU::mode mode); // used for register saving

	//reg banking
	void bankRegisters(CPU::mode mode); // save reg val to bank
	void unbankRegisters(CPU::mode mode); // load reg vals from bank

	void switchMode(CPU::mode newMode); // main function used for mode switching, calls bank and unbank register etc
	void saveIntoSpsr(uint8_t index);

	// excpetion handling
	void enterException(CPU::mode newMode, uint32_t vectorAddr, uint32_t returnAddr);
	void returnFromException();

	//SPSR helpers
	uint32_t getSPSR();
	void setSPSR(uint32_t value);

	//cpsr helper

	void writeCPSR(uint32_t value);
	mode CPSRbitToMode(uint8_t modeBits);

	// bank system so when we swap modes, we can store old modes inhere 
	uint32_t r8FIQ[5];   // 8 9 10 11 12 registers stored for just fiq
	uint32_t r8User[5]; // used for swaping back from fiq

	uint32_t r13RegBank[6];  // individual SP for everone except usr/sys which share
	uint32_t r14RegBank[6]; // individual LR for everone except usr/sys which share
	uint32_t spsrBank[6]; // individual LR for everone except usr/sys have 0 , 0 IS SET TO DEADBEEF
