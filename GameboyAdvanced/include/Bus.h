#pragma once
#include <cstdint>
#include <memory>
#include <vector>

class Bus
{
private:
	std::unique_ptr<uint8_t[]> biosRom;
	size_t memorySize;

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


	uint8_t read8(uint32_t addr, bool bReadOnly = false);
	uint16_t read16(uint32_t addr,bool bReadOnly = false);
	uint32_t read32(uint32_t addr, bool bReadOnly = false);

	void write8(uint32_t addr, uint8_t data);
	void write16(uint32_t addr, uint16_t data);
	void write32(uint32_t addr, uint32_t data);

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

private:
	CpuTimingRegion cpuTimingRegion(uint32_t address) const;
	bool isGamePakRegion(CpuTimingRegion region) const;
	bool isGamePakBoundary(uint32_t address) const;
	uint32_t gbaAccessCycles(CpuTimingRegion region, bool sequential, uint8_t width) const;
	uint32_t gamePakAccessCycles(CpuTimingRegion region, bool sequential) const;
	void recordCpuExternalAccess(CpuTimingAccess access, uint32_t address, uint8_t width,
		CpuTimingRegion region, bool forceSequential = false);

	CpuTimingConfig cpuTimingConfig;
	std::vector<CpuTimingEvent> cpuTimingEvents;

};
