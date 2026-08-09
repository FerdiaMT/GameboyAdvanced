#include "SystemControl.h"
#include "tdmi7/CPU.h"
#include "PPU.h"

namespace
{
uint8_t byteOf(uint32_t value, unsigned byte) { return static_cast<uint8_t>(value >> (byte * 8)); }
void writeByte(uint32_t& value, unsigned byte, uint8_t data)
{
    value = (value & ~(0xFFU << (byte * 8))) | (static_cast<uint32_t>(data) << (byte * 8));
}
}

SystemControl::SystemControl(Bus& busRef, tdmi7::CPU& cpuRef, PPU& ppuRef) : bus(busRef), cpu(cpuRef), ppu(ppuRef)
{
    bus.setIoHandlers(
        [this](uint32_t address, uint8_t& value) { return readIo(address, value); },
        [this](uint32_t address, uint8_t value) { return writeIo(address, value); });
	cpu.setHleSwiHandler([this](uint32_t immediate, bool thumb) { return handleHleSwi(immediate, thumb); });
}

void SystemControl::reset()
{
    ie = iflags = keyControl = 0;
    ime = false;
    keyInput = 0x03FF;
    timers = {};
    dmas = {};
	dmaFifoWords = {};
	pendingDmaStallCycles = 0;
    updateIrqLine();
}

bool SystemControl::readIo(uint32_t address, uint8_t& value) const
{
	if (ppu.readIo(address, value)) return true;
    const uint32_t offset = address & 0x3FFU;
    if (offset >= 0x100 && offset < 0x110)
    {
        const unsigned index = (offset - 0x100) / 4;
        const unsigned part = (offset - 0x100) % 4;
        value = part < 2 ? byteOf(timers[index].counter, part) : byteOf(timers[index].control, part - 2);
        return true;
    }
    if (offset >= 0x0B0 && offset < 0x0E0)
    {
        const unsigned index = (offset - 0x0B0) / 12;
        const unsigned part = (offset - 0x0B0) % 12;
        if (part < 4) value = byteOf(dmas[index].source, part);
        else if (part < 8) value = byteOf(dmas[index].destination, part - 4);
        else if (part < 10) value = byteOf(dmas[index].count, part - 8);
        else value = byteOf(dmas[index].control, part - 10);
        return true;
    }
	if (offset >= 0x0A0 && offset < 0x0A8)
	{
		value = byteOf(dmaFifoWords[(offset - 0x0A0) / 4], (offset - 0x0A0) & 3U);
		return true;
	}
    switch (offset)
    {
    case 0x130: value = byteOf(keyInput, 0); return true;
    case 0x131: value = byteOf(keyInput, 1); return true;
    case 0x132: value = byteOf(keyControl, 0); return true;
    case 0x133: value = byteOf(keyControl, 1); return true;
    case 0x200: value = byteOf(ie, 0); return true;
    case 0x201: value = byteOf(ie, 1); return true;
    case 0x202: value = byteOf(iflags, 0); return true;
    case 0x203: value = byteOf(iflags, 1); return true;
    case 0x208: value = ime ? 1 : 0; return true;
    case 0x209: value = 0; return true;
    default: return false;
    }
}

bool SystemControl::writeIo(uint32_t address, uint8_t value)
{
	if (ppu.writeIo(address, value)) return true;
    const uint32_t offset = address & 0x3FFU;
    if (offset >= 0x100 && offset < 0x110)
    {
        const unsigned index = (offset - 0x100) / 4;
        const unsigned part = (offset - 0x100) % 4;
        Timer& timer = timers[index];
        if (part < 2)
        {
            timer.reload = static_cast<uint16_t>((timer.reload & ~(0xFFU << (part * 8))) |
                (static_cast<uint16_t>(value) << (part * 8)));
            if ((timer.control & 0x80U) == 0) timer.counter = timer.reload;
        }
        else
        {
            const bool enabled = (timer.control & 0x80U) != 0;
            uint32_t control = timer.control;
            writeByte(control, part - 2, value);
            timer.control = static_cast<uint16_t>(control & 0x00C7U);
            if (!enabled && (timer.control & 0x80U)) { timer.counter = timer.reload; timer.divider = 0; }
        }
        return true;
    }
    if (offset >= 0x0B0 && offset < 0x0E0)
    {
        const unsigned index = (offset - 0x0B0) / 12;
        const unsigned part = (offset - 0x0B0) % 12;
        Dma& dma = dmas[index];
        if (part < 4) writeByte(dma.source, part, value);
        else if (part < 8) writeByte(dma.destination, part - 4, value);
        else if (part < 10) { uint32_t count = dma.count; writeByte(count, part - 8, value); dma.count = static_cast<uint16_t>(count); }
        else
        {
            const bool wasEnabled = (dma.control & 0x8000U) != 0;
            uint32_t control = dma.control;
            writeByte(control, part - 10, value);
            dma.control = static_cast<uint16_t>(control & 0xFFE0U);
            if (!wasEnabled && (dma.control & 0x8000U))
            {
                dma.count &= static_cast<uint16_t>(index == 3 ? 0xFFFFU : 0x3FFFU);
                dma.initialDestination = dma.destination;
                if (((dma.control >> 12) & 3U) == 0) executeDma(index);
            }
        }
        return true;
    }
	if (offset >= 0x0A0 && offset < 0x0A8)
	{
		writeByte(dmaFifoWords[(offset - 0x0A0) / 4], (offset - 0x0A0) & 3U, value);
		return true;
	}
    switch (offset)
    {
	case 0x301:
		// HALTCNT bit 7 selects STOP; STOP is outside the current no-audio
		// scope, while value 0 is the BIOS's normal HALT request.
		if ((value & 0x80U) == 0) cpu.setHalted(true);
		return true;
    case 0x132: case 0x133:
    {
        uint32_t control = keyControl; writeByte(control, offset - 0x132, value);
        keyControl = static_cast<uint16_t>(control & 0xC3FFU); evaluateKeyInterrupt(); return true;
    }
    case 0x200: case 0x201:
    {
        uint32_t enabled = ie; writeByte(enabled, offset - 0x200, value); ie = static_cast<uint16_t>(enabled & 0x3FFFU); updateIrqLine(); return true;
    }
    case 0x202: case 0x203:
        iflags &= static_cast<uint16_t>(~(static_cast<uint16_t>(value) << ((offset - 0x202) * 8))); updateIrqLine(); return true;
    case 0x208: ime = (value & 1U) != 0; updateIrqLine(); return true;
    default: return false;
    }
}

void SystemControl::requestInterrupt(uint16_t bits) { iflags |= bits & 0x3FFFU; updateIrqLine(); }
void SystemControl::onHBlank()
{
	uint8_t displayStatus = 0;
	ppu.readIo(0x04000004U, displayStatus);
	if (displayStatus & 0x10U) requestInterrupt(HBlank);
	if (ppu.inVBlank()) return;
    for (unsigned index = 0; index < dmas.size(); ++index)
        if ((dmas[index].control & 0x8000U) && ((dmas[index].control >> 12) & 3U) == 2) executeDma(index);
}
void SystemControl::onVBlank()
{
	uint8_t displayStatus = 0;
	ppu.readIo(0x04000004U, displayStatus);
	if (displayStatus & 0x08U) requestInterrupt(VBlank);
    for (unsigned index = 0; index < dmas.size(); ++index)
        if ((dmas[index].control & 0x8000U) && ((dmas[index].control >> 12) & 3U) == 1) executeDma(index);
}
void SystemControl::onVCount() { requestInterrupt(VCounter); }
void SystemControl::setPressedKeys(uint16_t pressedMask) { keyInput = static_cast<uint16_t>(~pressedMask) & 0x03FFU; evaluateKeyInterrupt(); }
uint32_t SystemControl::takeDmaStallCycles()
{
    const uint32_t cycles = pendingDmaStallCycles;
    pendingDmaStallCycles = 0;
    return cycles;
}
uint16_t SystemControl::interruptEnable() const { return ie; }
uint16_t SystemControl::interruptFlags() const { return iflags; }
bool SystemControl::interruptMasterEnabled() const { return ime; }
void SystemControl::setBiosPolicy(BiosPolicy newPolicy) { policy = newPolicy; }
SystemControl::BiosPolicy SystemControl::biosPolicy() const { return policy; }

bool SystemControl::handleHleSwi(uint32_t immediate, bool)
{
	if (policy != BiosPolicy::Hle)
		return false;

	switch (immediate)
	{
	case 0x06: // Div
	{
		const int32_t numerator = static_cast<int32_t>(cpu.reg[0]);
		const int32_t denominator = static_cast<int32_t>(cpu.reg[1]);
		if (denominator == 0)
			return false;
		const int32_t quotient = numerator / denominator;
		const int32_t remainder = numerator % denominator;
		cpu.reg[0] = static_cast<uint32_t>(quotient);
		cpu.reg[1] = static_cast<uint32_t>(remainder);
		const int64_t wideQuotient = quotient;
		cpu.reg[3] = static_cast<uint32_t>(wideQuotient < 0 ? -wideQuotient : wideQuotient);
		return true;
	}
	case 0x08: // Sqrt
	{
		const uint32_t value = cpu.reg[0];
		uint32_t root = 0;
		for (uint32_t bit = 1U << 15; bit != 0; bit >>= 1)
		{
			const uint32_t candidate = root | bit;
			if (candidate <= value / candidate) root = candidate;
		}
		cpu.reg[0] = root;
		return true;
	}
	case 0x0B: // CpuSet
	case 0x0C: // CpuFastSet
	{
		const uint32_t control = cpu.reg[2];
		const bool fast = immediate == 0x0C;
		const bool word = fast || (control & (1U << 26)) != 0;
		const bool fill = (control & (1U << 24)) != 0;
		const uint32_t width = word ? 4U : 2U;
		uint32_t count = control & 0x1FFFFFU;
		if (fast) count *= 8U;
		if (count == 0) return true;

		uint32_t source = cpu.reg[0] & ~(width - 1U);
		uint32_t destination = cpu.reg[1] & ~(width - 1U);
		const uint32_t fillValue = word ? bus.read32(source) : bus.read16(source);
		for (uint32_t index = 0; index < count; ++index)
		{
			const uint32_t value = fill ? fillValue : (word ? bus.read32(source) : bus.read16(source));
			if (word) bus.write32(destination, value);
			else bus.write16(destination, static_cast<uint16_t>(value));
			if (!fill) source += width;
			destination += width;
		}
		return true;
	}
	default:
		return false;
	}
}

void SystemControl::updateIrqLine()
{
	const bool enabledPending = (ie & iflags) != 0;
	if (enabledPending) cpu.setHalted(false);
	cpu.setIrqLine(ime && enabledPending);
}
void SystemControl::evaluateKeyInterrupt()
{
    const uint16_t selected = keyControl & 0x03FFU;
    const uint16_t pressed = static_cast<uint16_t>(~keyInput) & selected;
    const bool all = (keyControl & 0x8000U) != 0;
    const bool matched = selected != 0 && (all ? pressed == selected : pressed != 0);
    if ((keyControl & 0x4000U) && matched) requestInterrupt(Keypad);
}

uint32_t SystemControl::timerPeriod(uint16_t control)
{
    static constexpr uint32_t periods[] = { 1, 64, 256, 1024 };
    return periods[control & 3U];
}

bool SystemControl::incrementTimer(unsigned index)
{
    Timer& timer = timers[index];
    ++timer.counter;
    if (timer.counter != 0) return false;
    timer.counter = timer.reload;
	if (index == 0) triggerSpecialDmas();
    if (timer.control & 0x40U) requestInterrupt(static_cast<uint16_t>(Timer0 << index));
    if (index + 1 < timers.size() && (timers[index + 1].control & 0x84U) == 0x84U)
        incrementTimer(index + 1);
    return true;
}

void SystemControl::advance(uint32_t cycles)
{
    for (unsigned index = 0; index < timers.size(); ++index)
    {
        Timer& timer = timers[index];
        if ((timer.control & 0x80U) == 0 || (index != 0 && (timer.control & 4U))) continue;
        timer.divider += cycles;
        const uint32_t period = timerPeriod(timer.control);
        while (timer.divider >= period) { timer.divider -= period; incrementTimer(index); }
    }
    updateIrqLine();
}

uint32_t SystemControl::advanceAddress(uint32_t address, unsigned mode, uint32_t width)
{
    return mode == 1 ? address - width : mode == 2 ? address : address + width;
}

void SystemControl::triggerSpecialDmas()
{
	for (unsigned index = 1; index <= 2; ++index)
	{
		const Dma& dma = dmas[index];
		const uint32_t fifo = dma.destination & ~3U;
		if ((dma.control & 0x8000U) && ((dma.control >> 12) & 3U) == 3 &&
			(fifo == 0x040000A0U || fifo == 0x040000A4U)) executeDma(index, true);
	}
}

void SystemControl::executeDma(unsigned index, bool special)
{
    Dma& dma = dmas[index];
    const uint32_t width = special ? 4U : ((dma.control & 0x0400U) ? 4U : 2U);
    const uint32_t transfers = special ? 4U : (dma.count ? dma.count : (index == 3 ? 0x10000U : 0x4000U));
    uint32_t source = dma.source & ~(width - 1U);
    uint32_t destination = dma.destination & ~(width - 1U);
    const unsigned sourceMode = (dma.control >> 7) & 3U;
    const unsigned destinationMode = special ? 2U : ((dma.control >> 5) & 3U);
    for (uint32_t count = 0; count < transfers; ++count)
    {
		const bool sourceSequential = count != 0 && sourceMode == 0;
		const bool destinationSequential = count != 0 && destinationMode == 0;
		pendingDmaStallCycles += bus.dmaAccessCycles(source, static_cast<uint8_t>(width), sourceSequential);
		pendingDmaStallCycles += bus.dmaAccessCycles(destination, static_cast<uint8_t>(width), destinationSequential);
        if (width == 4) bus.write32(destination, bus.read32(source));
        else bus.write16(destination, bus.read16(source));
        source = advanceAddress(source, sourceMode == 3 ? 0 : sourceMode, width);
        destination = advanceAddress(destination, destinationMode == 3 ? 0 : destinationMode, width);
    }
    dma.source = source;
    // Destination control 3 increments during the transfer and reloads for
    // the next trigger.  This is essential for sound/video style repeated DMA.
    dma.destination = destinationMode == 3 || special ? dma.initialDestination : destination;
    if (dma.control & 0x4000U) requestInterrupt(static_cast<uint16_t>(Dma0 << index));
    if ((dma.control & 0x0200U) == 0) dma.control &= ~0x8000U;
}
