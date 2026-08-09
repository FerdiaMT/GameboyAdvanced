#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

class Bus
{
public:
	enum class SaveType : uint8_t { Unknown, Sram, Flash64, Flash128, Eeprom512, Eeprom8K };
	using IoReadHandler = std::function<bool(uint32_t address, uint8_t& value)>;
	using IoWriteHandler = std::function<bool(uint32_t address, uint8_t value)>;

private:
	static constexpr size_t BiosSize = 0x4000;
	static constexpr size_t EwramSize = 0x40000;
	static constexpr size_t IwramSize = 0x8000;
	static constexpr size_t IoSize = 0x400;
	static constexpr size_t PaletteSize = 0x400;
	static constexpr size_t VramSize = 0x18000;
	static constexpr size_t OamSize = 0x400;
	static constexpr size_t SaveSize = 0x20000;

	std::array<uint8_t, BiosSize> bios{};
	bool loadedBios = false;
	std::array<uint8_t, EwramSize> ewram{};
	std::array<uint8_t, IwramSize> iwram{};
	std::array<uint8_t, IoSize> io{};
	std::array<uint8_t, PaletteSize> palette{};
	std::array<uint8_t, VramSize> vram{};
	std::array<uint8_t, OamSize> oam{};
	std::array<uint8_t, SaveSize> save{};
	std::vector<uint8_t> cartridgeRom;
	SaveType detectedSaveType = SaveType::Unknown;
	enum class EepromPhase : uint8_t { Idle, Command, Address, WriteData, Stop };
	EepromPhase eepromPhase = EepromPhase::Idle;
	bool eepromReadCommand = false;
	uint8_t eepromAddressBits = 0;
	uint16_t eepromAddress = 0;
	uint8_t eepromDataBits = 0;
	uint64_t eepromData = 0;
	uint8_t eepromReadDummyBits = 0;
	uint8_t eepromReadDataBits = 0;
	uint16_t eepromReadAddress = 0;
	IoReadHandler ioReadHandler;
	IoWriteHandler ioWriteHandler;

	bool cpuTimingTraceEnabled = false;
	bool gbaRegionTimingEnabled = false;
	bool hasPreviousCpuAccess = false;
	uint32_t previousCpuAddress = 0;
	uint8_t previousCpuWidth = 0;
	uint16_t waitcnt = 0;

public:
	enum class CpuTimingAccess : uint8_t
	{
		InstructionFetch,
		DataRead,
		DataWrite,
		Internal,
	};

	enum class CpuTimingRegion : uint8_t
	{
		Bios,
		Ewram,
		Iwram,
		Io,
		Palette,
		Vram,
		Oam,
		GamePak0,
		GamePak1,
		GamePak2,
		Sram,
		Other,
	};

	struct CpuTimingConfig
	{
		uint32_t nonSequentialCycles = 1;
		uint32_t sequentialCycles = 1;
	};

	struct CpuTimingEvent
	{
		CpuTimingAccess access;
		uint32_t address;
		uint8_t width;
		bool sequential;
		uint32_t cycles;
		CpuTimingRegion region;
	};

	Bus();

	bool loadROM(const char* filename , uint32_t loadAddr);
	bool loadBios(const char* filename);
	void loadBiosImage(const uint8_t* data, size_t size);
	bool hasBios() const;
	// Loader/debugger entry point for supplying immutable Game Pak bytes
	// without treating CPU writes to cartridge space as writable memory.
	void loadCartridgeImage(const uint8_t* data, size_t size, uint32_t loadAddr = 0x08000000);
	SaveType saveType() const;
	void setSaveType(SaveType type);
	bool isGamePakPrefetchEnabled() const;
	void setIoHandlers(IoReadHandler readHandler, IoWriteHandler writeHandler);
	void resetSystemState();


	uint8_t read8(uint32_t addr, bool bReadOnly = false);
	uint16_t read16(uint32_t addr,bool bReadOnly = false);
	uint32_t read32(uint32_t addr, bool bReadOnly = false);

	void write8(uint32_t addr, uint8_t data);
	void write16(uint32_t addr, uint16_t data);
	void write32(uint32_t addr, uint32_t data);

	// PPU-only reads bypass the CPU-visible memory map dispatch.  The PPU owns
	// these regions electrically, so this neither changes open-bus behaviour
	// nor produces CPU timing events.
	uint8_t ppuReadVram8(uint32_t address) const
	{
		uint32_t offset = address & 0x1FFFFU;
		if (offset >= VramSize) offset = 0x10000U + (offset & 0x7FFFU);
		return vram[offset];
	}
	uint16_t ppuReadVram16(uint32_t address) const
	{
		return static_cast<uint16_t>(ppuReadVram8(address) | (static_cast<uint16_t>(ppuReadVram8(address + 1U)) << 8));
	}
	uint16_t ppuReadPalette16(uint32_t address) const
	{
		const uint32_t offset = address & (PaletteSize - 1U);
		return static_cast<uint16_t>(palette[offset] | (static_cast<uint16_t>(palette[(offset + 1U) & (PaletteSize - 1U)]) << 8));
	}
	uint16_t ppuReadOam16(uint32_t address) const
	{
		const uint32_t offset = address & (OamSize - 1U);
		return static_cast<uint16_t>(oam[offset] | (static_cast<uint16_t>(oam[(offset + 1U) & (OamSize - 1U)]) << 8));
	}

	// CPU timing instrumentation.  It is intentionally opt-in: normal bus
	// users retain the existing untimed behavior while tests can inspect the
	// exact CPU-visible N/S access stream.
	void enableCpuTimingTrace(bool enabled);
	bool isCpuTimingTraceEnabled() const;
	void setCpuTimingConfig(CpuTimingConfig config);
	void enableGbaRegionTiming(bool enabled);
	bool isGbaRegionTimingEnabled() const;
	void setWaitcnt(uint16_t value);
	uint16_t getWaitcnt() const;
	void clearCpuTimingTrace();
	const std::vector<CpuTimingEvent>& cpuTimingTrace() const;
	uint64_t cpuTimingCyclesSince(size_t startIndex) const;
	size_t cpuTimingExternalAccessCountSince(size_t startIndex) const;
	void recordCpuAccess(CpuTimingAccess access, uint32_t address, uint8_t width);
	void recordCpuInternal(uint32_t cycles);
	uint32_t dmaAccessCycles(uint32_t address, uint8_t width, bool sequential) const;

private:
	CpuTimingRegion cpuTimingRegion(uint32_t address) const;
	bool isGamePakRegion(CpuTimingRegion region) const;
	bool isGamePakBoundary(uint32_t address) const;
	uint32_t gbaAccessCycles(CpuTimingRegion region, bool sequential, uint8_t width) const;
	uint32_t gamePakAccessCycles(CpuTimingRegion region, bool sequential) const;
	void recordCpuExternalAccess(CpuTimingAccess access, uint32_t address, uint8_t width,
		CpuTimingRegion region, bool forceSequential = false);
	uint8_t readMapped8(uint32_t address) const;
	void writeMapped8(uint32_t address, uint8_t data);
	static uint32_t vramOffset(uint32_t address);
	static uint32_t cartridgeOffset(uint32_t address);
	uint32_t saveOffset(uint32_t address) const;
	bool isEepromAddress(uint32_t address) const;
	void writeEepromBit(uint8_t bit);
	uint16_t readEepromBit();
	void resetEepromTransfer();
	void detectSaveType();

	CpuTimingConfig cpuTimingConfig;
	std::vector<CpuTimingEvent> cpuTimingEvents;

};
