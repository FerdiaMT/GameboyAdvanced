#include "GBA.h"

#include <iostream>

namespace
{
bool expect(bool condition, const char* expression)
{
    if (!condition)
    {
        std::cerr << "[FAIL] mode3_gradient_program: " << expression << '\n';
        return false;
    }
    return true;
}
}

bool testGradientProgram()
{
    GBA gba;
    if (!gba.loadCartridge("tests/assembly/build/mode3_gradient.bin"))
    {
        std::cerr << "[FAIL] mode3_gradient_program: unable to load generated ROM\n";
        return 1;
    }

    // Four instructions per pixel plus setup, then allow a complete further
    // frame for the PPU to scan out the fully populated VRAM.
    gba.runSteps(300000);
    const auto& frame = gba.ppu.framebuffer();
    if (!expect(frame[0] == 0xFFFF0000U, "frame[0] == red")) return 1;
    if (!expect(frame[1] != frame[0], "frame[1] differs from frame[0]")) return 1;
    if (!expect(frame[239] != 0xFF000000U, "last pixel of first row is visible")) return 1;
    if (!expect(frame[159 * 240] != 0xFF000000U, "first pixel of bottom row is visible")) return 1;
    std::cout << "[PASS] mode3_gradient_program\n";
    return true;
}

bool testRgbCheckerboardProgram()
{
    GBA gba;
    if (!gba.loadCartridge("tests/assembly/build/mode3_rgb_checkerboard.bin"))
    {
        std::cerr << "[FAIL] mode3_rgb_checkerboard_program: unable to load generated ROM\n";
        return false;
    }
    gba.runSteps(300000);
    const auto& frame = gba.ppu.framebuffer();
    constexpr uint32_t red = 0xFFFF0000U;
    constexpr uint32_t green = 0xFF00FF00U;
    constexpr uint32_t blue = 0xFF0000FFU;
    if (!expect(frame[0] == red, "checkerboard (0,0) is red")) return false;
    if (!expect(frame[8] == green, "checkerboard (8,0) is green")) return false;
    if (!expect(frame[16] == blue, "checkerboard (16,0) is blue")) return false;
    if (!expect(frame[8 * 240] == green, "checkerboard (0,8) is green")) return false;
    if (!expect(frame[8 * 240 + 8] == blue, "checkerboard (8,8) is blue")) return false;
    std::cout << "[PASS] mode3_rgb_checkerboard_program\n";
    return true;
}

bool testRgbScrollProgram()
{
    GBA gba;
    if (!gba.loadCartridge("tests/assembly/build/mode3_rgb_scroll.bin"))
    {
        std::cerr << "[FAIL] mode3_rgb_scroll_program: unable to load generated ROM\n";
        return false;
    }
    gba.runSteps(300000);
    const auto& frame = gba.ppu.framebuffer();
    if (!expect(frame[0] != frame[8], "scroll pattern has distinct horizontal tiles")) return false;
    if (!expect(frame[0] != frame[8 * 240], "scroll pattern has distinct vertical tiles")) return false;
    if (!expect(frame[159 * 240] != 0xFF000000U, "scroll pattern reaches bottom row")) return false;
    std::cout << "[PASS] mode3_rgb_scroll_program\n";
    return true;
}

bool testBackgroundScrollProgram()
{
    GBA gba;
    if (!gba.loadCartridge("tests/assembly/build/mode0_background_scroll.bin"))
    {
        std::cerr << "[FAIL] mode0_background_scroll_program: unable to load generated ROM\n";
        return false;
    }
    gba.runSteps(300000);
    const auto& firstFrame = gba.ppu.framebuffer();
    const uint32_t firstPixel = firstFrame[0];
    if (!expect(firstPixel != 0xFF000000U, "background scroll has visible pixels")) return false;
    if (!expect(firstFrame[8] != firstPixel, "background scroll has tiled horizontal colours")) return false;
    if (!expect(gba.bus.read16(0x04000010) != 0, "BG0HOFS is updated")) return false;
    if (!expect(gba.bus.read16(0x04000012) != 0, "BG0VOFS is updated")) return false;
    gba.runSteps(3000000);
    if (!expect(gba.ppu.framebuffer()[0] != firstPixel, "BG0HOFS changes the displayed screen position")) return false;
    std::cout << "[PASS] mode0_background_scroll_program\n";
    return true;
}

bool testObjShowcaseProgram()
{
    GBA gba;
    if (!gba.loadCartridge("tests/assembly/build/mode0_obj_showcase.bin"))
    {
        std::cerr << "[FAIL] mode0_obj_showcase_program: unable to load generated ROM\n";
        return false;
    }
    gba.runSteps(300000);
    const auto& frame = gba.ppu.framebuffer();
    if (!expect(frame[0] == 0xFF00FF00U, "showcase regular green OBJ")) return false;
    if (!expect(frame[16] == 0xFFFF0000U, "showcase regular red OBJ")) return false;
    if (!expect(frame[32] == 0xFF00FF00U, "showcase affine OBJ")) return false;
    if (!expect(frame[48] == 0xFF0000FFU, "showcase BG behind OBJs")) return false;
    std::cout << "[PASS] mode0_obj_showcase_program\n";
    return true;
}

bool testAlphaBackgroundBlendsOverLowerPriorityObj()
{
    GBA gba;
    gba.reset();

    // BG0 priority 0 is a red alpha layer. OBJ0 priority 1 is blue beneath
    // it.  The GBA priority pipeline must submit the OBJ before BG0 so the
    // latter can use it as its alpha-blend destination.
    gba.bus.write16(0x04000000, 0x1100); // mode 0, BG0 and OBJ enabled
    gba.bus.write16(0x04000008, 0x0100); // BG0: priority 0, screen base 1
    gba.bus.write16(0x04000050, 0x1041); // BG0 target A, alpha, OBJ target B
    gba.bus.write16(0x04000052, 0x0808); // 50% / 50%

    gba.bus.write16(0x05000002, 0x001F); // BG palette entry 1: red
    gba.bus.write16(0x05000204, 0x7C00); // OBJ palette entry 2: blue
    for (uint32_t offset = 0; offset < 32; ++offset)
    {
        gba.bus.write8(0x06000000 + offset, 0x11); // BG tile 0, colour 1
        gba.bus.write8(0x06010000 + offset, 0x22); // OBJ tile 0, colour 2
    }
    gba.bus.write16(0x06000800, 0); // BG map entry: tile 0
    gba.bus.write16(0x07000000, 0); // OBJ0 at (0, 0), regular 4bpp
    gba.bus.write16(0x07000002, 0);
    gba.bus.write16(0x07000004, 0x0400); // OBJ priority 1

    gba.ppu.advance(960); // render scanline 0 at HBlank
    return expect(gba.ppu.framebuffer()[0] == 0xFF7F007FU,
        "alpha BG blends over lower-priority OBJ");
}

int main()
{
    return testGradientProgram() && testRgbCheckerboardProgram() && testRgbScrollProgram() &&
        testBackgroundScrollProgram() && testObjShowcaseProgram() &&
        testAlphaBackgroundBlendsOverLowerPriorityObj() ? 0 : 1;
}
