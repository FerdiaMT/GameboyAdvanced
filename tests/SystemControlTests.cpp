#include "GBA.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace
{
bool expect(bool condition, const char* expression, const char* test)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << test << ": " << expression << '\n';
        return false;
    }
    return true;
}
#define EXPECT(test, expression) do { if (!expect((expression), #expression, test)) return false; } while (false)

void installNops(GBA& gba)
{
    gba.cpu.pc = 0x03000000;
    gba.bus.write32(0x03000000, 0xE1A00000);
    gba.bus.write32(0x03000004, 0xE1A00000);
}

bool testInterruptRegistersAndIrqDelivery()
{
    constexpr const char* name = "interrupt_registers_and_irq_delivery";
    GBA gba;
    installNops(gba);
    gba.cpu.I = 0;
    gba.bus.write16(0x04000200, SystemControl::Timer0);
    gba.bus.write16(0x04000208, 1);
    gba.system.requestInterrupt(SystemControl::Timer0);
    EXPECT(name, gba.bus.read16(0x04000202) == SystemControl::Timer0);
    EXPECT(name, gba.tick() == 3);
    EXPECT(name, gba.cpu.pc == 0x18);
    EXPECT(name, gba.cpu.curMode == tdmi7::CPU::mode::IRQ);
    gba.bus.write16(0x04000202, SystemControl::Timer0);
    EXPECT(name, gba.system.interruptFlags() == 0);
    return true;
}

bool testTimerKeypadAndDma()
{
    constexpr const char* name = "timer_keypad_and_dma";
    GBA gba;
    installNops(gba);
    gba.bus.write16(0x04000100, 0xFFFE);
    gba.bus.write16(0x04000102, 0x00C0); // enable + timer IRQ, F/1
    gba.tick();
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::Timer0) == 0);
    gba.tick();
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::Timer0) != 0);

    gba.bus.write16(0x04000132, 0x4001); // keypad IRQ, select A
    gba.setPressedKeys(1);
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::Keypad) != 0);

    gba.bus.write16(0x02000000, 0xBEEF);
    gba.bus.write32(0x040000B0, 0x02000000);
    gba.bus.write32(0x040000B4, 0x02000010);
    gba.bus.write16(0x040000B8, 1);
    gba.bus.write16(0x040000BA, 0xC000); // immediate, 16-bit, IRQ, enable
    EXPECT(name, gba.bus.read16(0x02000010) == 0xBEEF);
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::Dma0) != 0);

    // Repeat DMA with destination reload: used by hardware streaming paths.
    gba.bus.write16(0x02000000, 0x1111);
    gba.bus.write16(0x02000002, 0x2222);
    gba.bus.write32(0x040000BC, 0x02000002);
    gba.bus.write32(0x040000C0, 0x02000020);
    gba.bus.write16(0x040000C4, 2);
    gba.bus.write16(0x040000C6, 0x92E0); // VBlank, repeat, src--, dst reload, enabled
    gba.system.onVBlank();
    EXPECT(name, gba.bus.read16(0x02000020) == 0x2222);
    EXPECT(name, gba.bus.read16(0x02000022) == 0x1111);
    EXPECT(name, gba.bus.read32(0x040000C0) == 0x02000020);

    GBA stalled;
    installNops(stalled);
    stalled.bus.write16(0x02000000, 0xCAFE);
    stalled.bus.write32(0x040000B0, 0x02000000);
    stalled.bus.write32(0x040000B4, 0x02000010);
    stalled.bus.write16(0x040000B8, 1);
    stalled.bus.write16(0x040000BA, 0x8000); // immediate halfword DMA
    EXPECT(name, stalled.tick() == 7); // one CPU cycle plus EWRAM read/write DMA costs

    GBA fifo;
    installNops(fifo);
    fifo.bus.write32(0x02000000, 0x11111111);
    fifo.bus.write32(0x02000004, 0x22222222);
    fifo.bus.write32(0x02000008, 0x33333333);
    fifo.bus.write32(0x0200000C, 0x44444444);
    fifo.bus.write32(0x040000BC, 0x02000000);
    fifo.bus.write32(0x040000C0, 0x040000A0);
    fifo.bus.write16(0x040000C4, 1);     // special DMA ignores programmed count
    fifo.bus.write16(0x040000C6, 0xB200); // DMA1 special + repeat + enabled
    fifo.bus.write16(0x04000100, 0xFFFF);
    fifo.bus.write16(0x04000102, 0x0080);
    fifo.system.advance(1);               // timer 0 overflow triggers special DMA1
    fifo.bus.write16(0x04000102, 0);      // prevent further test-only FIFO requests
    EXPECT(name, fifo.tick() == 29);      // CPU + four EWRAM-to-I/O word transfers
    EXPECT(name, fifo.bus.read32(0x040000A0) == 0x44444444);

    GBA gamePak;
    installNops(gamePak);
    const uint8_t romWord[] = { 0xEF, 0xBE };
    gamePak.bus.loadCartridgeImage(romWord, sizeof(romWord));
    gamePak.bus.write32(0x040000B0, 0x08000000);
    gamePak.bus.write32(0x040000B4, 0x03000020);
    gamePak.bus.write16(0x040000B8, 1);
    gamePak.bus.write16(0x040000BA, 0x8000);
    EXPECT(name, gamePak.tick() == 7); // CPU + Game Pak non-sequential (5) + IWRAM (1)
    EXPECT(name, gamePak.bus.read16(0x03000020) == 0xBEEF);
    return true;
}

bool testWaitcntSaveDetectionAndBiosPolicy()
{
    constexpr const char* name = "waitcnt_save_detection_and_bios_policy";
    GBA gba;
    gba.bus.write16(0x04000204, 0x4000);
    EXPECT(name, gba.bus.isGamePakPrefetchEnabled());
    const uint8_t flashId[] = { 'F','L','A','S','H','1','M','_','V','1','0','2' };
    gba.bus.loadCartridgeImage(flashId, sizeof(flashId));
    EXPECT(name, gba.bus.saveType() == Bus::SaveType::Flash128);
    gba.system.setBiosPolicy(SystemControl::BiosPolicy::Hle);
    EXPECT(name, gba.system.biosPolicy() == SystemControl::BiosPolicy::Hle);
	// Reset hardware-visible control state without discarding the loaded cart.
    gba.reset();
    EXPECT(name, gba.bus.getWaitcnt() == 0);
    EXPECT(name, gba.system.interruptFlags() == 0);
    EXPECT(name, gba.bus.saveType() == Bus::SaveType::Flash128);
    return true;
}

bool testBiosBootAndHleSwi()
{
    constexpr const char* name = "bios_boot_and_hle_swi";
    GBA gba;
    std::array<uint8_t, 0x4000> bios{};
    bios[0] = 0x78;
    bios[0x3FFF] = 0x56;
    gba.bus.loadBiosImage(bios.data(), bios.size());
    EXPECT(name, gba.bus.hasBios());
    EXPECT(name, gba.bus.read8(0) == 0x78);
    EXPECT(name, gba.bus.read8(0x3FFF) == 0x56);
    gba.system.setBiosPolicy(SystemControl::BiosPolicy::Hle);
    gba.cpu.pc = 0x03000000;
    gba.cpu.reg[0] = 17;
    gba.cpu.reg[1] = 5;
    gba.bus.write32(0x03000000, 0xEF000006); // swi 6: Div
    gba.tick();
    EXPECT(name, gba.cpu.reg[0] == 3);
    EXPECT(name, gba.cpu.reg[1] == 2);
    EXPECT(name, gba.cpu.reg[3] == 3);
    EXPECT(name, gba.cpu.pc == 0x03000004);

    gba.cpu.pc = 0x03000004;
    gba.cpu.reg[0] = 81;
    gba.bus.write32(0x03000004, 0xEF000008); // swi 8: Sqrt
    gba.tick();
    EXPECT(name, gba.cpu.reg[0] == 9);

    gba.cpu.pc = 0x03000008;
    gba.bus.write16(0x02000000, 0x1234);
    gba.bus.write16(0x02000002, 0x5678);
    gba.cpu.reg[0] = 0x02000000;
    gba.cpu.reg[1] = 0x02000010;
    gba.cpu.reg[2] = 2;
    gba.bus.write32(0x03000008, 0xEF00000B); // swi 0B: CpuSet halfword copy
    gba.tick();
    EXPECT(name, gba.bus.read16(0x02000010) == 0x1234);
    EXPECT(name, gba.bus.read16(0x02000012) == 0x5678);

    gba.cpu.T = 1;
    gba.cpu.pc = 0x0300000C;
    gba.cpu.reg[0] = 144;
    gba.bus.write16(0x0300000C, 0xDF08); // Thumb swi 8: Sqrt
    gba.tick();
    EXPECT(name, gba.cpu.reg[0] == 12);
    EXPECT(name, gba.cpu.pc == 0x0300000E);
    return true;
}

bool testScanlineEdgesAndBlankDma()
{
    constexpr const char* name = "scanline_edges_and_blank_dma";
    GBA gba;
    gba.bus.write16(0x04000200, SystemControl::HBlank | SystemControl::VBlank);
    gba.bus.write16(0x04000208, 1);
	 gba.bus.write16(0x04000004, 0x0018); // enable HBlank and VBlank DISPSTAT IRQs
    gba.bus.write16(0x02000000, 0xCAFE);
    gba.bus.write32(0x040000B0, 0x02000000);
    gba.bus.write32(0x040000B4, 0x02000010);
    gba.bus.write16(0x040000B8, 1);
    gba.bus.write16(0x040000BA, 0x9000); // VBlank start, 16-bit, enabled

    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.inHBlank());
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::HBlank) != 0);
    gba.ppu.advance(272 + 1232 * 159);
    EXPECT(name, gba.ppu.vcountValue() == 160 && gba.ppu.inVBlank());
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::VBlank) != 0);
    EXPECT(name, gba.bus.read16(0x02000010) == 0xCAFE);

    GBA vblankHblank;
	 vblankHblank.ppu.advance(1232 * 160); // enter VBlank before arming HBlank DMA
    vblankHblank.bus.write16(0x02000000, 0x1234);
    vblankHblank.bus.write32(0x040000B0, 0x02000000);
    vblankHblank.bus.write32(0x040000B4, 0x02000010);
    vblankHblank.bus.write16(0x040000B8, 1);
    vblankHblank.bus.write16(0x040000BA, 0xA200); // HBlank, repeat, enabled
    vblankHblank.ppu.advance(960);        // VBlank HBlank must not trigger DMA
    EXPECT(name, vblankHblank.bus.read16(0x02000010) == 0);
    return true;
}

bool testBitmapVideoModes()
{
    constexpr const char* name = "bitmap_video_modes";
    GBA gba;
    gba.bus.write16(0x04000000, 3);
    gba.bus.write16(0x06000000, 0x001F); // red mode 3 pixel
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 4);
    gba.bus.write16(0x05000002, 0x03E0); // palette[1] green
    gba.bus.write8(0x06000000, 1);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 5);
    gba.bus.write16(0x06000000, 0x7C00); // blue mode 5 pixel
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF0000FFU);
    EXPECT(name, gba.ppu.framebuffer()[200] == 0xFF000000U);
    return true;
}

bool testMode0TextBackground()
{
    constexpr const char* name = "mode0_text_background";
    GBA gba;
    gba.bus.write16(0x04000000, 0x0100); // mode 0, BG0 enabled
    gba.bus.write16(0x04000008, 0x1F00); // screen base block 31, 4bpp tiles
    gba.bus.write16(0x0600F800, 1);      // map entry: tile 1
    gba.bus.write8(0x06000020, 0x01);    // tile 1 pixel (0,0): palette index 1
    gba.bus.write16(0x05000002, 0x7C00); // palette index 1: blue
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF0000FFU);
    EXPECT(name, gba.ppu.framebuffer()[1] == 0xFF000000U);
    return true;
}

bool testMode0BackgroundPriority()
{
    constexpr const char* name = "mode0_background_priority";
    GBA gba;
    gba.bus.write16(0x04000000, 0x0300); // mode 0, BG0/BG1
    gba.bus.write16(0x04000008, 0x1F01); // BG0 screen block 31, priority 1
    gba.bus.write16(0x0400000A, 0x1E00); // BG1 screen block 30, priority 0
    gba.bus.write16(0x0600F800, 1);
    gba.bus.write16(0x0600F000, 2);
    gba.bus.write8(0x06000020, 0x01);
    gba.bus.write8(0x06000040, 0x22);
    gba.bus.write16(0x05000002, 0x001F); // red
    gba.bus.write16(0x05000004, 0x03E0); // green
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF00FF00U);
    return true;
}

bool testAffineBackgrounds()
{
    constexpr const char* name = "affine_backgrounds";
    GBA gba;
    gba.bus.write16(0x04000000, 0x0402); // mode 2, BG2 enabled
    gba.bus.write16(0x0400000C, 0x1F00); // BG2: screen base block 31
    gba.bus.write16(0x04000020, 0x0100); // PA = 1.0
    gba.bus.write16(0x04000026, 0x0100); // PD = 1.0
    gba.bus.write8(0x0600F800, 1);       // affine map entry: tile 1
    gba.bus.write8(0x06000040, 1);       // tile 1 pixel (0,0): palette index 1
    gba.bus.write16(0x05000002, 0x03E0); // green
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x0402); // mode 2, BG2 enabled
    gba.bus.write16(0x0400000C, 0x1F00); // wrapping disabled
    gba.bus.write16(0x04000020, 0x0100);
    gba.bus.write16(0x04000026, 0x0100);
    gba.bus.write32(0x04000028, 0xFFFF0000); // X = -256 pixels
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF000000U);
    return true;
}

bool testRasterEffectsAndVCount()
{
    constexpr const char* name = "raster_effects_and_vcount";
    GBA gba;
    gba.bus.write16(0x04000200, SystemControl::VCounter);
    gba.bus.write16(0x04000208, 1);
    gba.bus.write16(0x04000004, 0x0520); // VCOUNT=5, VCOUNT IRQ enabled
    gba.ppu.advance(1232 * 5);
    EXPECT(name, gba.ppu.vcountValue() == 5);
    EXPECT(name, (gba.system.interruptFlags() & SystemControl::VCounter) != 0);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x0402); // Mode 2, BG2
    gba.bus.write16(0x0400000C, 0x1F00);
    gba.bus.write8(0x0600F800, 1);
    gba.bus.write8(0x06000040, 1);       // tile 1 y=0, index 1
    gba.bus.write8(0x06000048, 2);       // tile 1 y=1, index 2
    gba.bus.write16(0x05000002, 0x001F);
    gba.bus.write16(0x05000004, 0x03E0);
    gba.bus.write16(0x04000026, 0x0100); // BG2PD: advance source y one pixel/line
    EXPECT(name, gba.bus.read16(0x04000026) == 0x0100);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);
    gba.ppu.advance(272 + 960);
    EXPECT(name, gba.ppu.framebuffer()[240] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x0100);
    gba.bus.write16(0x04000008, 0x1F40); // BG0 mosaic, screen block 31
    gba.bus.write16(0x0600F800, 1);
    gba.bus.write8(0x06000020, 0x21);    // red then green source pixels
    gba.bus.write16(0x05000002, 0x001F);
    gba.bus.write16(0x05000004, 0x03E0);
    gba.bus.write16(0x0400004C, 0x0001); // 2-pixel BG mosaic horizontally
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);
    EXPECT(name, gba.ppu.framebuffer()[1] == 0xFFFF0000U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 3);
    gba.bus.write16(0x06000000, 0x0008); // dark red bitmap pixel
    gba.bus.write16(0x04000050, 0x0084); // BG2 first target, brighten
    gba.bus.write16(0x04000054, 8);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] != 0xFF420000U);
    return true;
}

void installSimpleRedBackground(GBA& gba)
{
    gba.bus.write16(0x04000008, 0x1F01); // BG0, screen block 31, priority 1
    gba.bus.write16(0x0600F800, 1);
    gba.bus.write8(0x06000020, 0x01);
    gba.bus.write16(0x05000002, 0x001F);
}

void installGreenObject(GBA& gba, uint16_t attr0, uint16_t priority)
{
    gba.bus.write16(0x07000000, attr0);
    gba.bus.write16(0x07000002, 0);
    gba.bus.write16(0x07000004, priority << 10);
    gba.bus.write8(0x06010000, 0x01);
    gba.bus.write16(0x05000202, 0x03E0);
}

bool testObjectsWindowsAndEffects()
{
    constexpr const char* name = "objects_windows_and_effects";
    GBA gba;
    gba.bus.write16(0x04000000, 0x1100); // Mode 0, BG0 + OBJ
    installSimpleRedBackground(gba);
    installGreenObject(gba, 0, 0);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x1100);
    installSimpleRedBackground(gba);
    installGreenObject(gba, 0, 2); // Lower priority than BG0
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x2300); // BG0/BG1 plus WIN0
    installSimpleRedBackground(gba);
    gba.bus.write16(0x0400000A, 0x1E00); // BG1 priority 0, green
    gba.bus.write16(0x0600F000, 2);
    gba.bus.write8(0x06000040, 0x22);
    gba.bus.write16(0x05000004, 0x03E0);
    gba.bus.write16(0x04000040, 0x0001); // WIN0 x=[0,1)
    gba.bus.write16(0x04000044, 0x00A0); // WIN0 covers all visible y
    gba.bus.write16(0x04000048, 0x0001); // inside: BG0 only
    gba.bus.write16(0x0400004A, 0x0002); // outside: BG1 only
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);
    EXPECT(name, gba.ppu.framebuffer()[1] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x1100);
    installSimpleRedBackground(gba);
    installGreenObject(gba, 0x0400, 0); // Semi-transparent OBJ
    gba.bus.write16(0x04000050, 0x0150); // OBJ first target, BG0 second, alpha blend
    gba.bus.write16(0x04000052, 0x0808);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF7F7F00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x0083); // Forced blank in Mode 3
    gba.bus.write16(0x06000000, 0x001F);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFFFFFFU);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x1100); // BG0 + OBJ
    installSimpleRedBackground(gba);
    installGreenObject(gba, 0x0100, 0); // affine OBJ, matrix 0
    gba.bus.write16(0x07000006, 0x0100); // PA
    gba.bus.write16(0x0700001E, 0x0100); // PD
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x8100); // BG0 plus OBJ-window enable
    installSimpleRedBackground(gba);
    gba.bus.write16(0x07000000, 0x0800); // regular object-window at (0,0)
    gba.bus.write16(0x07000002, 0);
    gba.bus.write16(0x07000004, 0);
    gba.bus.write8(0x06010000, 0x11);
    gba.bus.write16(0x0400004A, 0x0100); // OBJ window: BG0; outside: nothing
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);
    EXPECT(name, gba.ppu.framebuffer()[8] == 0xFF000000U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x8100); // BG0 plus affine OBJ-window
    installSimpleRedBackground(gba);
    gba.bus.write16(0x07000000, 0x0900); // affine object-window at (0,0)
    gba.bus.write16(0x07000002, 0);
    gba.bus.write16(0x07000004, 0);
    gba.bus.write16(0x07000006, 0x0100); // PA
    gba.bus.write16(0x0700001E, 0x0100); // PD
    gba.bus.write8(0x06010000, 0x11);
    gba.bus.write16(0x0400004A, 0x0100);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFFFF0000U);
    EXPECT(name, gba.ppu.framebuffer()[8] == 0xFF000000U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x1003); // Mode 3 + OBJ
    gba.bus.write16(0x06000000, 0x001F); // bitmap red
    installGreenObject(gba, 0, 0);
    gba.bus.write8(0x06010000, 0);       // mode 3 OBJ tiles start at 0x6014000
    gba.bus.write8(0x06014000, 0x11);
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF00FF00U);

    gba.ppu.reset();
    gba.bus.write16(0x04000000, 0x1000); // Mode 0 + OBJ
    gba.bus.write8(0x06010020, 0x22);    // tile 1 would be a red sprite
    gba.bus.write16(0x05000204, 0x001F);
    for (unsigned object = 0; object < 33; ++object)
    {
        const uint32_t address = 0x07000000U + object * 8U;
        gba.bus.write16(address, 0);
        gba.bus.write16(address + 2U, 0);
        gba.bus.write16(address + 4U, object == 32 ? 1 : 0); // 33rd object is red
    }
    gba.ppu.advance(960);
    EXPECT(name, gba.ppu.framebuffer()[0] == 0xFF000000U); // first 32 transparent OBJs consume the line limit
    return true;
}

using Test = bool (*)();
bool run(const char* name, Test test)
{
    if (!test()) return false;
    std::cout << "[PASS] " << name << '\n';
    return true;
}
}

int main()
{
    const bool passed =
        run("interrupt_registers_and_irq_delivery", testInterruptRegistersAndIrqDelivery) &&
        run("timer_keypad_and_dma", testTimerKeypadAndDma) &&
        run("waitcnt_save_detection_and_bios_policy", testWaitcntSaveDetectionAndBiosPolicy) &&
		run("bios_boot_and_hle_swi", testBiosBootAndHleSwi) &&
        run("scanline_edges_and_blank_dma", testScanlineEdgesAndBlankDma) &&
        run("bitmap_video_modes", testBitmapVideoModes) &&
        run("mode0_text_background", testMode0TextBackground) &&
        run("mode0_background_priority", testMode0BackgroundPriority) &&
        run("affine_backgrounds", testAffineBackgrounds) &&
        run("raster_effects_and_vcount", testRasterEffectsAndVCount) &&
        run("objects_windows_and_effects", testObjectsWindowsAndEffects);
    return passed ? 0 : 1;
}
