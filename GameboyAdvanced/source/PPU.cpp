#include "PPU.h"
#include "Bus.h"

#include <algorithm>

namespace
{
int32_t signed28(uint32_t value) { return static_cast<int32_t>(value << 4) >> 4; }
}

PPU::PPU(Bus& busRef) : bus(busRef) {}

void PPU::reset()
{
	cycles = 0;
	vcount = lineCycles = 0;
	hblank = vblank = false;
	displayControl = displayStatus = 0;
	mosaic = blendControl = blendAlpha = blendY = unusedVideoRegister = 0;
	windowHorizontal = windowVertical = {};
	windowInside = 0;
	windowOutside = 0x003F;
	backgrounds = {};
	pixels.fill(0xFF000000U);
	// An all-zero OAM entry is a visible 8x8 sprite.  Initialise entries as
	// disabled so blank OAM cannot unexpectedly cover bitmap display modes.
	for (unsigned object = 0; object < 128; ++object)
		bus.write16(0x07000000U + object * 8U, 0x0200U);
}

void PPU::advance(uint32_t elapsedCycles)
{
	cycles += elapsedCycles;
	while (elapsedCycles != 0)
	{
		const uint16_t edge = hblank ? 1232 : 960;
		const uint32_t untilEdge = edge - lineCycles;
		const uint32_t step = elapsedCycles < untilEdge ? elapsedCycles : untilEdge;
		lineCycles = static_cast<uint16_t>(lineCycles + step);
		elapsedCycles -= step;
		if (lineCycles != edge) continue;
		if (!hblank)
		{
			hblank = true;
			if (vcount < 160)
			{
				renderScanline(vcount);
				for (unsigned background = 2; background < 4; ++background)
				{
					backgrounds[background].affineCurrentX += backgrounds[background].pb;
					backgrounds[background].affineCurrentY += backgrounds[background].pd;
				}
			}
			if (hblankCallback) hblankCallback();
		}
		else
		{
			hblank = false;
			lineCycles = 0;
			vcount = static_cast<uint16_t>((vcount + 1) % 228);
			if (vcount == 160)
			{
				vblank = true;
				if (vblankCallback) vblankCallback();
			}
			else if (vcount == 0)
			{
				vblank = false;
				for (unsigned background = 2; background < 4; ++background)
				{
					backgrounds[background].affineCurrentX = signed28(backgrounds[background].affineX);
					backgrounds[background].affineCurrentY = signed28(backgrounds[background].affineY);
				}
			}
			if (vcount == (displayStatus >> 8) && (displayStatus & 0x0020U) && vcountCallback) vcountCallback();
		}
	}
}

uint64_t PPU::cycleCount() const
{
	return cycles;
}

uint16_t PPU::vcountValue() const { return vcount; }
bool PPU::inHBlank() const { return hblank; }
bool PPU::inVBlank() const { return vblank; }
uint32_t PPU::cyclesUntilNextEvent() const
{
	return (hblank ? 1232U : 960U) - lineCycles;
}
void PPU::setHBlankCallback(std::function<void()> callback) { hblankCallback = std::move(callback); }
void PPU::setVBlankCallback(std::function<void()> callback) { vblankCallback = std::move(callback); }
void PPU::setVCountCallback(std::function<void()> callback) { vcountCallback = std::move(callback); }

bool PPU::readIo(uint32_t address, uint8_t& value) const
{
	const uint32_t offset = address & 0x3FFU;
	if (offset >= 0x008 && offset < 0x010)
	{
		const uint16_t control = backgrounds[(offset - 0x008) / 2].control;
		value = static_cast<uint8_t>(control >> ((offset & 1U) * 8));
		return true;
	}
	if (offset >= 0x010 && offset < 0x020)
	{
		const Background& background = backgrounds[(offset - 0x010) / 4];
		const uint16_t scroll = ((offset - 0x010) & 2U) ? background.vofs : background.hofs;
		value = static_cast<uint8_t>(scroll >> ((offset & 1U) * 8));
		return true;
	}
	if ((offset >= 0x020 && offset < 0x030) || (offset >= 0x030 && offset < 0x040))
	{
		const unsigned background = offset >= 0x030 ? 3 : 2;
		const uint32_t affineOffset = offset - (background == 3 ? 0x030U : 0x020U);
		const Background& state = backgrounds[background];
		if (affineOffset < 8)
		{
			const int16_t matrix[] = { state.pa, state.pb, state.pc, state.pd };
			value = static_cast<uint8_t>(static_cast<uint16_t>(matrix[affineOffset / 2]) >> ((affineOffset & 1U) * 8));
		}
		else
		{
			const uint32_t reference = affineOffset < 12 ? state.affineX : state.affineY;
			value = static_cast<uint8_t>(reference >> ((affineOffset & 3U) * 8));
		}
		return true;
	}
	if (offset >= 0x040 && offset < 0x056)
	{
		const uint16_t* registers[] = { &windowHorizontal[0], &windowHorizontal[1], &windowVertical[0], &windowVertical[1],
			&windowInside, &windowOutside, &mosaic, &unusedVideoRegister, &blendControl, &blendAlpha, &blendY };
		const uint32_t registerIndex = (offset - 0x040) / 2;
		if (registerIndex < 11)
		{
			value = static_cast<uint8_t>(*registers[registerIndex] >> ((offset & 1U) * 8));
			return true;
		}
	}
	switch (offset)
	{
	case 0x000: value = static_cast<uint8_t>(displayControl); return true;
	case 0x001: value = static_cast<uint8_t>(displayControl >> 8); return true;
	case 0x004:
		value = static_cast<uint8_t>(displayStatus | (vblank ? 1U : 0U) | (hblank ? 2U : 0U) |
			(vcount == (displayStatus >> 8) ? 4U : 0U)); return true;
	case 0x005: value = static_cast<uint8_t>(displayStatus >> 8); return true;
	case 0x006: value = static_cast<uint8_t>(vcount); return true;
	case 0x007: value = static_cast<uint8_t>(vcount >> 8); return true;
	default: return false;
	}
}

bool PPU::writeIo(uint32_t address, uint8_t value)
{
	const uint32_t offset = address & 0x3FFU;
	if (offset == 0x000 || offset == 0x001)
	{
		displayControl = static_cast<uint16_t>((displayControl & ~(0xFFU << ((offset & 1U) * 8))) |
			(static_cast<uint16_t>(value) << ((offset & 1U) * 8)));
		return true;
	}
	if (offset == 0x004 || offset == 0x005)
	{
		displayStatus = static_cast<uint16_t>(((displayStatus & ~(0xFFU << ((offset & 1U) * 8))) |
			(static_cast<uint16_t>(value) << ((offset & 1U) * 8))) & 0xFFF8U);
		return true;
	}
	if (offset >= 0x008 && offset < 0x010)
	{
		Background& background = backgrounds[(offset - 0x008) / 2];
		background.control = static_cast<uint16_t>((background.control & ~(0xFFU << ((offset & 1U) * 8))) |
			(static_cast<uint16_t>(value) << ((offset & 1U) * 8)));
		return true;
	}
	if (offset >= 0x010 && offset < 0x020)
	{
		Background& background = backgrounds[(offset - 0x010) / 4];
		uint16_t& scroll = ((offset - 0x010) & 2U) ? background.vofs : background.hofs;
		scroll = static_cast<uint16_t>((scroll & ~(0xFFU << ((offset & 1U) * 8))) |
			(static_cast<uint16_t>(value) << ((offset & 1U) * 8)));
		return true;
	}
	if ((offset >= 0x020 && offset < 0x030) || (offset >= 0x030 && offset < 0x040))
	{
		const unsigned background = offset >= 0x030 ? 3 : 2;
		const uint32_t affineOffset = offset - (background == 3 ? 0x030U : 0x020U);
		Background& state = backgrounds[background];
		if (affineOffset < 8)
		{
			int16_t* matrix[] = { &state.pa, &state.pb, &state.pc, &state.pd };
			uint16_t raw = static_cast<uint16_t>(*matrix[affineOffset / 2]);
			raw = static_cast<uint16_t>((raw & ~(0xFFU << ((affineOffset & 1U) * 8))) |
				(static_cast<uint16_t>(value) << ((affineOffset & 1U) * 8)));
			*matrix[affineOffset / 2] = static_cast<int16_t>(raw);
		}
		else
		{
			uint32_t& reference = affineOffset < 12 ? state.affineX : state.affineY;
			const uint32_t byte = affineOffset & 3U;
			reference = (reference & ~(0xFFU << (byte * 8))) | (static_cast<uint32_t>(value) << (byte * 8));
			reference &= 0x0FFFFFFFU; // BGxX/BGxY are signed 28-bit, 8.8 fixed point.
			if (affineOffset < 12) state.affineCurrentX = signed28(state.affineX);
			else state.affineCurrentY = signed28(state.affineY);
		}
		return true;
	}
	if (offset >= 0x040 && offset < 0x056)
	{
		uint16_t* registers[] = { &windowHorizontal[0], &windowHorizontal[1], &windowVertical[0], &windowVertical[1],
			&windowInside, &windowOutside, &mosaic, &unusedVideoRegister, &blendControl, &blendAlpha, &blendY };
		const uint32_t registerIndex = (offset - 0x040) / 2;
		if (registerIndex < 11)
		{
			uint16_t& reg = *registers[registerIndex];
			reg = static_cast<uint16_t>((reg & ~(0xFFU << ((offset & 1U) * 8))) |
				(static_cast<uint16_t>(value) << ((offset & 1U) * 8)));
			if (registerIndex == 8) reg &= 0x3FFFU;
			if (registerIndex == 9) reg &= 0x1F1FU;
			if (registerIndex == 10) reg &= 0x001FU;
			return true;
		}
	}
	return false;
}

const std::array<uint32_t, 240 * 160>& PPU::framebuffer() const { return pixels; }

uint32_t PPU::color555(uint16_t color) const
{
	const uint32_t red = color & 0x1FU;
	const uint32_t green = (color >> 5) & 0x1FU;
	const uint32_t blue = (color >> 10) & 0x1FU;
	return 0xFF000000U | ((red << 3 | red >> 2) << 16) |
		((green << 3 | green >> 2) << 8) | (blue << 3 | blue >> 2);
}

void PPU::renderScanline(uint16_t line)
{
	uint32_t* output = pixels.data() + line * 240;
	if (displayControl & 0x0080U)
	{
		std::fill(output, output + 240, 0xFFFFFFFFU);
		return;
	}
	const uint16_t mode = displayControl & 7U;
	linePriorities.fill(4);
	lineLayers.fill(0x20U);
	lineObjectWindow.fill(false);
	if (displayControl & 0x8000U) prepareObjectWindow(line);
	if (mode <= 2)
	{
		// In tiled modes palette entry zero is the backdrop colour.  It is not
		// transparent, so clearing to black hides the BIOS/game-selected
		// background behind otherwise-correct text and sprite pixels.
		std::fill(output, output + 240, color555(bus.ppuReadPalette16(0x05000000U)));
		std::array<unsigned, 4> order = { 0, 1, 2, 3 };
		std::sort(order.begin(), order.end(), [this](unsigned left, unsigned right)
		{
			const unsigned leftPriority = backgrounds[left].control & 3U;
			const unsigned rightPriority = backgrounds[right].control & 3U;
			return leftPriority != rightPriority ? leftPriority > rightPriority : left > right;
		});
		for (unsigned background : order)
		{
			if (!(displayControl & (0x0100U << background))) continue;
			const bool affine = (mode == 1 && background == 2) || (mode == 2 && background >= 2);
			const bool valid = mode == 0 || (mode == 1 && background <= 2) || background >= 2;
			if (valid) affine ? renderAffineBackground(background, line) : renderTextBackground(background, line);
		}
		renderObjects(line);
	}
	else if (mode == 3)
	{
		for (uint32_t x = 0; x < 240; ++x)
			plotPixel(x, line, color555(bus.ppuReadVram16(0x06000000U + (line * 240 + x) * 2)), 3, 4);
		renderObjects(line);
	}
	else if (mode == 4)
	{
		const uint32_t page = (displayControl & 0x10U) ? 0xA000U : 0;
		for (uint32_t x = 0; x < 240; ++x)
		{
			const uint8_t index = bus.ppuReadVram8(0x06000000U + page + line * 240 + x);
			plotPixel(x, line, color555(bus.ppuReadPalette16(0x05000000U + index * 2)), 3, 4);
		}
		renderObjects(line);
	}
	else if (mode == 5)
	{
		const uint32_t page = (displayControl & 0x10U) ? 0xA000U : 0;
		for (uint32_t x = 0; x < 240; ++x)
			if (x < 160 && line < 128)
				plotPixel(x, line, color555(bus.ppuReadVram16(0x06000000U + page + (line * 160 + x) * 2)), 3, 4);
		renderObjects(line);
	}
	else std::fill(output, output + 240, color555(bus.ppuReadPalette16(0x05000000U)));
}

void PPU::renderAffineBackground(unsigned backgroundIndex, uint16_t line)
{
	const Background& background = backgrounds[backgroundIndex];
	const uint32_t size = 128U << ((background.control >> 14) & 3U);
	const uint32_t charBase = ((background.control >> 2) & 3U) * 0x4000U;
	const uint32_t screenBase = ((background.control >> 8) & 0x1FU) * 0x800U;
	const bool wrap = (background.control & 0x2000U) != 0;
	const int32_t referenceX = background.affineCurrentX;
	const int32_t referenceY = background.affineCurrentY;
	for (uint32_t screenX = 0; screenX < 240; ++screenX)
	{
		const uint32_t mosaicX = (background.control & 0x40U) ? screenX - screenX % ((mosaic & 0xFU) + 1U) : screenX;
		int32_t x = (referenceX + background.pa * static_cast<int32_t>(mosaicX)) >> 8;
		int32_t y = (referenceY + background.pc * static_cast<int32_t>(mosaicX)) >> 8;
		if (wrap)
		{
			x &= static_cast<int32_t>(size - 1);
			y &= static_cast<int32_t>(size - 1);
		}
		if (x < 0 || y < 0 || x >= static_cast<int32_t>(size) || y >= static_cast<int32_t>(size)) continue;
		const uint32_t tile = bus.ppuReadVram8(0x06000000U + screenBase + (static_cast<uint32_t>(y) / 8U) * (size / 8U) + static_cast<uint32_t>(x) / 8U);
		const uint8_t colorIndex = bus.ppuReadVram8(0x06000000U + charBase + tile * 64U + (static_cast<uint32_t>(y) & 7U) * 8U + (static_cast<uint32_t>(x) & 7U));
		if (colorIndex != 0) plotPixel(screenX, line, color555(bus.ppuReadPalette16(0x05000000U + colorIndex * 2U)), background.control & 3U, static_cast<uint8_t>(1U << backgroundIndex));
	}
}

void PPU::renderTextBackground(unsigned backgroundIndex, uint16_t line)
{
	const Background& background = backgrounds[backgroundIndex];
	const uint32_t size = (background.control >> 14) & 3U;
	const uint32_t width = (size & 1U) ? 512 : 256;
	const uint32_t height = (size & 2U) ? 512 : 256;
	const uint32_t charBase = ((background.control >> 2) & 3U) * 0x4000U;
	const uint32_t screenBase = ((background.control >> 8) & 0x1FU) * 0x800U;
	const bool color256 = (background.control & 0x80U) != 0;
	for (uint32_t screenX = 0; screenX < 240; ++screenX)
	{
		const uint32_t mosaicX = (background.control & 0x40U) ? screenX - screenX % ((mosaic & 0xFU) + 1U) : screenX;
		const uint16_t mosaicLine = (background.control & 0x40U) ? static_cast<uint16_t>(line - line % (((mosaic >> 4) & 0xFU) + 1U)) : line;
		const uint32_t x = (mosaicX + background.hofs) & (width - 1);
		const uint32_t y = (mosaicLine + background.vofs) & (height - 1);
		const uint32_t block = (x / 256) + (y / 256) * (width / 256);
		const uint32_t mapOffset = screenBase + block * 0x800U + ((y & 255U) / 8) * 64U + ((x & 255U) / 8) * 2U;
		const uint16_t entry = bus.ppuReadVram16(0x06000000U + mapOffset);
		uint32_t tileX = x & 7U;
		uint32_t tileY = y & 7U;
		if (entry & 0x0400U) tileX = 7 - tileX;
		if (entry & 0x0800U) tileY = 7 - tileY;
		const uint32_t tile = entry & 0x03FFU;
		uint8_t colorIndex;
		if (color256)
			colorIndex = bus.ppuReadVram8(0x06000000U + charBase + tile * 64U + tileY * 8U + tileX);
		else
		{
			const uint8_t packed = bus.ppuReadVram8(0x06000000U + charBase + tile * 32U + tileY * 4U + tileX / 2U);
			colorIndex = static_cast<uint8_t>(((tileX & 1U) ? packed >> 4 : packed & 0x0FU) + ((entry >> 12) & 0xFU) * 16U);
		}
		if (colorIndex != 0) plotPixel(screenX, line, color555(bus.ppuReadPalette16(0x05000000U + colorIndex * 2U)), background.control & 3U, static_cast<uint8_t>(1U << backgroundIndex));
	}
}

uint8_t PPU::windowMask(uint32_t x, uint16_t line) const
{
	auto inside = [](uint32_t coordinate, uint8_t first, uint8_t second)
	{
		return first <= second ? coordinate >= first && coordinate < second : coordinate >= first || coordinate < second;
	};
	if ((displayControl & 0x2000U) &&
		inside(x, static_cast<uint8_t>(windowHorizontal[0] >> 8), static_cast<uint8_t>(windowHorizontal[0])) &&
		inside(line, static_cast<uint8_t>(windowVertical[0] >> 8), static_cast<uint8_t>(windowVertical[0])))
		return static_cast<uint8_t>(windowInside);
	if ((displayControl & 0x4000U) &&
		inside(x, static_cast<uint8_t>(windowHorizontal[1] >> 8), static_cast<uint8_t>(windowHorizontal[1])) &&
		inside(line, static_cast<uint8_t>(windowVertical[1] >> 8), static_cast<uint8_t>(windowVertical[1])))
		return static_cast<uint8_t>(windowInside >> 8);
	if ((displayControl & 0x8000U) && lineObjectWindow[x]) return static_cast<uint8_t>(windowOutside >> 8);
	return static_cast<uint8_t>(windowOutside);
}

uint32_t PPU::blend(uint32_t source, uint32_t destination) const
{
	const unsigned eva = std::min<unsigned>(blendAlpha & 0x1FU, 16);
	const unsigned evb = std::min<unsigned>((blendAlpha >> 8) & 0x1FU, 16);
	auto component = [eva, evb](unsigned sourcePart, unsigned destinationPart)
	{
		return std::min(255U, (sourcePart * eva + destinationPart * evb) / 16U);
	};
	return 0xFF000000U | (component(source >> 16 & 0xFFU, destination >> 16 & 0xFFU) << 16) |
		(component(source >> 8 & 0xFFU, destination >> 8 & 0xFFU) << 8) |
		component(source & 0xFFU, destination & 0xFFU);
}

uint32_t PPU::applyBrightness(uint32_t color) const
{
	const unsigned factor = std::min<unsigned>(blendY & 0x1FU, 16);
	const unsigned effect = (blendControl >> 6) & 3U;
	auto alter = [factor, effect](unsigned component)
	{
		return effect == 2 ? component + (255U - component) * factor / 16U : component - component * factor / 16U;
	};
	return 0xFF000000U | (alter(color >> 16 & 0xFFU) << 16) |
		(alter(color >> 8 & 0xFFU) << 8) | alter(color & 0xFFU);
}

void PPU::plotPixel(uint32_t x, uint16_t line, uint32_t color, unsigned priority, uint8_t layer, bool semiTransparent)
{
	// The overwhelmingly common case (including the BIOS logo) has neither
	// windows nor colour effects enabled.  Avoid calculating two window masks
	// and blend state for every source pixel in that case.
	if ((displayControl & 0xE000U) == 0 && blendControl == 0 && !semiTransparent)
	{
		if (priority > linePriorities[x]) return;
		pixels[line * 240U + x] = color;
		linePriorities[x] = priority;
		lineLayers[x] = layer;
		return;
	}
	if ((windowMask(x, line) & layer) == 0 || priority > linePriorities[x]) return;
	uint32_t* output = pixels.data() + line * 240;
	const uint8_t mask = windowMask(x, line);
	const bool effects = (mask & 0x20U) != 0;
	const unsigned effect = (blendControl >> 6) & 3U;
	const bool sourceTarget = (blendControl & layer) != 0;
	const bool destinationTarget = (blendControl & (lineLayers[x] << 8)) != 0;
	if (effects && (semiTransparent || (effect == 1 && sourceTarget && destinationTarget)))
		color = destinationTarget ? blend(color, output[x]) : color;
	else if (effects && sourceTarget && (effect == 2 || effect == 3)) color = applyBrightness(color);
	output[x] = color;
	linePriorities[x] = priority;
	lineLayers[x] = layer;
}

void PPU::prepareObjectWindow(uint16_t line)
{
	static constexpr uint8_t widths[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } };
	static constexpr uint8_t heights[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };
	const bool oneDimensional = (displayControl & 0x0040U) != 0;
	const uint32_t objBase = (displayControl & 7U) >= 3 ? 0x06014000U : 0x06010000U;
	for (unsigned object = 0; object < 128; ++object)
	{
		const uint32_t address = 0x07000000U + object * 8U;
		const uint16_t attr0 = bus.ppuReadOam16(address);
		const uint16_t attr1 = bus.ppuReadOam16(address + 2U);
		const uint16_t attr2 = bus.ppuReadOam16(address + 4U);
		const bool affine = (attr0 & 0x0100U) != 0;
		if (((attr0 >> 10) & 3U) != 2 || (!affine && (attr0 & 0x0200U))) continue;
		const unsigned shape = (attr0 >> 14) & 3U;
		if (shape == 3) continue;
		const unsigned width = widths[shape][(attr1 >> 14) & 3U];
		const unsigned height = heights[shape][(attr1 >> 14) & 3U];
		const unsigned displayWidth = affine && (attr0 & 0x0200U) ? width * 2U : width;
		const unsigned displayHeight = affine && (attr0 & 0x0200U) ? height * 2U : height;
		unsigned localY = (line - (attr0 & 0xFFU)) & 0xFFU;
		if (localY >= displayHeight) continue;
		if (!affine && (attr1 & 0x2000U)) localY = height - 1U - localY;
		int spriteX = attr1 & 0x1FFU;
		if (spriteX >= 240) spriteX -= 512;
		const bool color256 = (attr0 & 0x2000U) != 0;
		const unsigned baseTile = attr2 & 0x3FFU;
		for (unsigned localX = 0; localX < displayWidth; ++localX)
		{
			const int screenX = spriteX + static_cast<int>(localX);
			if (screenX < 0 || screenX >= 240) continue;
			int sourceX = static_cast<int>(localX);
			int sourceY = static_cast<int>(localY);
			if (affine)
			{
				const unsigned matrix = (attr1 >> 9) & 0x1FU;
				const uint32_t matrixBase = 0x07000000U + matrix * 32U;
				const int pa = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 6U));
				const int pb = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 14U));
				const int pc = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 22U));
				const int pd = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 30U));
				const int dx = sourceX - static_cast<int>(displayWidth / 2U);
				const int dy = sourceY - static_cast<int>(displayHeight / 2U);
				sourceX = ((pa * dx + pb * dy) >> 8) + static_cast<int>(width / 2U);
				sourceY = ((pc * dx + pd * dy) >> 8) + static_cast<int>(height / 2U);
			}
			else if (attr1 & 0x1000U) sourceX = static_cast<int>(width) - 1 - sourceX;
			if (sourceX < 0 || sourceY < 0 || sourceX >= static_cast<int>(width) || sourceY >= static_cast<int>(height)) continue;
			const unsigned rowTiles = oneDimensional ? (width / 8U) * (color256 ? 2U : 1U) : 32U;
			const unsigned tile = baseTile + (static_cast<unsigned>(sourceY) / 8U) * rowTiles + (static_cast<unsigned>(sourceX) / 8U) * (color256 ? 2U : 1U);
			uint8_t index;
			if (color256) index = bus.ppuReadVram8(objBase + tile * 32U + (static_cast<unsigned>(sourceY) & 7U) * 8U + (static_cast<unsigned>(sourceX) & 7U));
			else
			{
				const uint8_t packed = bus.ppuReadVram8(objBase + tile * 32U + (static_cast<unsigned>(sourceY) & 7U) * 4U + (static_cast<unsigned>(sourceX) & 7U) / 2U);
				index = (sourceX & 1) ? packed >> 4 : packed & 0xFU;
			}
			if (index != 0) lineObjectWindow[static_cast<unsigned>(screenX)] = true;
		}
	}
}

void PPU::renderObjects(uint16_t line)
{
	static constexpr uint8_t widths[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } };
	static constexpr uint8_t heights[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };
	const bool oneDimensional = (displayControl & 0x0040U) != 0;
	const uint32_t objBase = (displayControl & 7U) >= 3 ? 0x06014000U : 0x06010000U;
	std::array<bool, 128> visibleObjects{};
	unsigned objectCount = 0;
	// Hardware evaluates at most 32 OBJ entries on one scanline, in OAM order.
	for (unsigned object = 0; object < 128 && objectCount < 32; ++object)
	{
		const uint32_t address = 0x07000000U + object * 8U;
		const uint16_t attr0 = bus.ppuReadOam16(address);
		const uint16_t attr1 = bus.ppuReadOam16(address + 2U);
		const bool affine = (attr0 & 0x0100U) != 0;
		if ((!affine && (attr0 & 0x0200U)) || ((attr0 >> 10) & 3U) == 2) continue;
		const unsigned shape = (attr0 >> 14) & 3U;
		if (shape == 3) continue;
		const unsigned size = (attr1 >> 14) & 3U;
		const unsigned height = heights[shape][size] * (affine && (attr0 & 0x0200U) ? 2U : 1U);
		if (((line - (attr0 & 0xFFU)) & 0xFFU) < height) visibleObjects[object] = true, ++objectCount;
	}
	unsigned objectPixels = 0;
	for (int object = 127; object >= 0; --object)
	{
		if (!visibleObjects[static_cast<unsigned>(object)]) continue;
		const uint32_t address = 0x07000000U + static_cast<uint32_t>(object) * 8U;
		const uint16_t attr0 = bus.ppuReadOam16(address);
		const uint16_t attr1 = bus.ppuReadOam16(address + 2U);
		const uint16_t attr2 = bus.ppuReadOam16(address + 4U);
		const bool affine = (attr0 & 0x0100U) != 0;
		if ((!affine && (attr0 & 0x0200U)) || ((attr0 >> 10) & 3U) == 2) continue;
		const unsigned shape = (attr0 >> 14) & 3U;
		if (shape == 3) continue;
		const unsigned size = (attr1 >> 14) & 3U;
		const unsigned width = widths[shape][size];
		const unsigned height = heights[shape][size];
		const unsigned displayWidth = affine && (attr0 & 0x0200U) ? width * 2U : width;
		const unsigned displayHeight = affine && (attr0 & 0x0200U) ? height * 2U : height;
		const unsigned spriteY = attr0 & 0xFFU;
		unsigned localY = (line - spriteY) & 0xFFU;
		if (localY >= displayHeight) continue;
		if (!affine && (attr1 & 0x2000U)) localY = height - 1U - localY;
		int spriteX = attr1 & 0x1FFU;
		if (spriteX >= 240) spriteX -= 512;
		const bool color256 = (attr0 & 0x2000U) != 0;
		const bool useMosaic = (attr0 & 0x1000U) != 0;
		const unsigned priority = (attr2 >> 10) & 3U;
		const unsigned palette = (attr2 >> 12) & 0xFU;
		const unsigned baseTile = attr2 & 0x3FFU;
		for (unsigned localX = 0; localX < displayWidth; ++localX)
		{
			const int screenX = spriteX + static_cast<int>(localX);
			if (screenX < 0 || screenX >= 240) continue;
			int sourceX = static_cast<int>(localX);
			int sourceY = static_cast<int>(localY);
			if (useMosaic)
			{
				sourceX -= sourceX % static_cast<int>(((mosaic >> 8) & 0xFU) + 1U);
				sourceY -= sourceY % static_cast<int>(((mosaic >> 12) & 0xFU) + 1U);
			}
			if (affine)
			{
				const unsigned matrix = (attr1 >> 9) & 0x1FU;
				const uint32_t matrixBase = 0x07000000U + matrix * 32U;
				const int pa = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 6U));
				const int pb = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 14U));
				const int pc = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 22U));
				const int pd = static_cast<int16_t>(bus.ppuReadOam16(matrixBase + 30U));
				const int dx = sourceX - static_cast<int>(displayWidth / 2U);
				const int dy = sourceY - static_cast<int>(displayHeight / 2U);
				sourceX = ((pa * dx + pb * dy) >> 8) + static_cast<int>(width / 2U);
				sourceY = ((pc * dx + pd * dy) >> 8) + static_cast<int>(height / 2U);
			}
			else if (attr1 & 0x1000U) sourceX = static_cast<int>(width) - 1 - sourceX;
			if (sourceX < 0 || sourceY < 0 || sourceX >= static_cast<int>(width) || sourceY >= static_cast<int>(height)) continue;
			const unsigned rowTiles = oneDimensional ? (width / 8U) * (color256 ? 2U : 1U) : 32U;
			const unsigned tile = baseTile + (static_cast<unsigned>(sourceY) / 8U) * rowTiles + (static_cast<unsigned>(sourceX) / 8U) * (color256 ? 2U : 1U);
			uint8_t index;
			if (color256) index = bus.ppuReadVram8(objBase + tile * 32U + (static_cast<unsigned>(sourceY) & 7U) * 8U + (static_cast<unsigned>(sourceX) & 7U));
			else
			{
				const uint8_t packed = bus.ppuReadVram8(objBase + tile * 32U + (static_cast<unsigned>(sourceY) & 7U) * 4U + (static_cast<unsigned>(sourceX) & 7U) / 2U);
				index = static_cast<uint8_t>(((sourceX & 1U) ? packed >> 4 : packed & 0xFU) + palette * 16U);
			}
			if (index != 0)
			{
				if (objectPixels++ >= 960) return; // 960 OBJ pixels per scanline.
				plotPixel(static_cast<uint32_t>(screenX), line, color555(bus.ppuReadPalette16(0x05000200U + index * 2U)), priority, 0x10U, ((attr0 >> 10) & 3U) == 1);
			}
		}
	}
}
