#pragma once

#include "Bus.h"
#include "ClockedDevice.h"

#include <array>
#include <cstdint>

namespace tdmi7 { class CPU; }
class PPU;

class SystemControl final : public ClockedDevice
{
public:
    enum class BiosPolicy : uint8_t { RequireRealBios, Hle }; 
    enum Interrupt : uint16_t
    {
        VBlank = 1U << 0, HBlank = 1U << 1, VCounter = 1U << 2,
        Timer0 = 1U << 3, Timer1 = 1U << 4, Timer2 = 1U << 5, Timer3 = 1U << 6,
        Dma0 = 1U << 8, Dma1 = 1U << 9, Dma2 = 1U << 10, Dma3 = 1U << 11,
        Keypad = 1U << 12, GamePak = 1U << 13,
    };

    SystemControl(Bus& bus, tdmi7::CPU& cpu, PPU& ppu);
    void reset();
    void advance(uint32_t cycles) override;
    bool readIo(uint32_t address, uint8_t& value) const;
    bool writeIo(uint32_t address, uint8_t value);
    void requestInterrupt(uint16_t bits);
	void onHBlank();
	void onVBlank();
	void onVCount();
    void setPressedKeys(uint16_t pressedMask);
	// DMA owns the bus while transferring.  GBA consumes this deterministic
	// stall budget into the master clock after device callbacks.
	uint32_t takeDmaStallCycles();
    uint16_t interruptEnable() const;
    uint16_t interruptFlags() const;
    bool interruptMasterEnabled() const;
    void setBiosPolicy(BiosPolicy policy);
    BiosPolicy biosPolicy() const;

private:
    struct Timer { uint16_t reload = 0; uint16_t counter = 0; uint16_t control = 0; uint32_t divider = 0; };
    struct Dma
    {
        uint32_t source = 0;
        uint32_t destination = 0;
        uint32_t initialDestination = 0;
        uint16_t count = 0;
        uint16_t control = 0;
    };
    Bus& bus;
    tdmi7::CPU& cpu;
	PPU& ppu;
    uint16_t ie = 0;
    uint16_t iflags = 0;
    bool ime = false;
    uint16_t keyInput = 0x03FF;
    uint16_t keyControl = 0;
    BiosPolicy policy = BiosPolicy::RequireRealBios;
    std::array<Timer, 4> timers{};
    std::array<Dma, 4> dmas{};
	std::array<uint32_t, 2> dmaFifoWords{};
	uint32_t pendingDmaStallCycles = 0;

    void updateIrqLine();
    void evaluateKeyInterrupt();
    bool incrementTimer(unsigned index);
    void executeDma(unsigned index, bool special = false);
    void triggerSpecialDmas();
	bool handleHleSwi(uint32_t immediate, bool thumb);
    static uint32_t timerPeriod(uint16_t control);
    static uint32_t advanceAddress(uint32_t address, unsigned mode, uint32_t width);
};
