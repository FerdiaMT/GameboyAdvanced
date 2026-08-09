#pragma once
#include "ClockedDevice.h"

#include <cstdint>
#include <functional>
#include <array>

class Bus;

class PPU final : public ClockedDevice
{
	Bus& bus;
	uint64_t cycles = 0;
	uint16_t vcount = 0;
	uint16_t lineCycles = 0;
	bool hblank = false;
	bool vblank = false;
	std::function<void()> hblankCallback;
	std::function<void()> vblankCallback;
	std::function<void()> vcountCallback;
	uint16_t displayControl = 0;
	uint16_t displayStatus = 0;
	uint16_t mosaic = 0;
	uint16_t blendControl = 0;
	uint16_t blendAlpha = 0;
	uint16_t blendY = 0;
	std::array<uint16_t, 2> windowHorizontal{};
	std::array<uint16_t, 2> windowVertical{};
	uint16_t windowInside = 0;
	uint16_t windowOutside = 0x003F;
	uint16_t unusedVideoRegister = 0;
	struct Background
	{
		uint16_t control = 0;
		uint16_t hofs = 0;
		uint16_t vofs = 0;
		int16_t pa = 0;
		int16_t pb = 0;
		int16_t pc = 0;
		int16_t pd = 0;
		uint32_t affineX = 0;
		uint32_t affineY = 0;
		int32_t affineCurrentX = 0;
		int32_t affineCurrentY = 0;
	};
	std::array<Background, 4> backgrounds{};
	std::array<uint32_t, 240 * 160> pixels{};
	std::array<unsigned, 240> linePriorities{};
	std::array<uint8_t, 240> lineLayers{};
	std::array<bool, 240> lineObjectWindow{};
	void renderScanline(uint16_t line);
	void renderTextBackground(unsigned background, uint16_t line);
	void renderAffineBackground(unsigned background, uint16_t line);
	void renderObjects(uint16_t line);
	void prepareObjectWindow(uint16_t line);
	void plotPixel(uint32_t x, uint16_t line, uint32_t color, unsigned priority, uint8_t layer, bool semiTransparent = false);
	uint8_t windowMask(uint32_t x, uint16_t line) const;
	uint32_t applyBrightness(uint32_t color) const;
	uint32_t blend(uint32_t source, uint32_t destination) const;
	uint32_t color555(uint16_t color) const;

	public:
	explicit PPU(Bus& bus);
	void reset();
	void advance(uint32_t elapsedCycles) override;
	uint64_t cycleCount() const;
	uint16_t vcountValue() const;
	bool inHBlank() const;
	bool inVBlank() const;
	// Master scheduler aid: the next observable PPU transition is HBlank or
	// the start of the following scanline.
	uint32_t cyclesUntilNextEvent() const;
	void setHBlankCallback(std::function<void()> callback);
	void setVBlankCallback(std::function<void()> callback);
	void setVCountCallback(std::function<void()> callback);
	bool readIo(uint32_t address, uint8_t& value) const;
	bool writeIo(uint32_t address, uint8_t value);
	const std::array<uint32_t, 240 * 160>& framebuffer() const;
};
