#include "GBA.h"

#include <emscripten/emscripten.h>

#include <cstdint>
#include <cstdio>

namespace
{
constexpr uint64_t FrameCycles = 1232U * 228U;

EM_JS(void, webSetup, (),
{
    const canvas = document.getElementById('screen');
    const context = canvas.getContext('2d', { alpha: false });
    context.imageSmoothingEnabled = false;
    Module.gbaCanvas = canvas;
    Module.gbaContext = context;
    Module.gbaImage = context.createImageData(240, 160);
    Module.gbaImageWords = new Uint32Array(Module.gbaImage.data.buffer);
    Module.gbaKeys = 0;
    Module.gbaPaused = false;
    Module.gbaResetRequested = false;

    const keyMap = {
        KeyZ: 1 << 0, KeyX: 1 << 1, Backspace: 1 << 2, Enter: 1 << 3,
        ArrowRight: 1 << 4, ArrowLeft: 1 << 5, ArrowUp: 1 << 6,
        ArrowDown: 1 << 7, KeyS: 1 << 8, KeyA: 1 << 9,
    };
    const updateKey = (event, pressed) => {
        const gbaKey = keyMap[event.code];
        if (gbaKey === undefined) return;
        event.preventDefault();
        Module.gbaKeys = pressed ? Module.gbaKeys | gbaKey : Module.gbaKeys & ~gbaKey;
    };
    window.addEventListener('keydown', event => updateKey(event, true));
    window.addEventListener('keyup', event => updateKey(event, false));
    window.addEventListener('blur', () => { Module.gbaKeys = 0; });
    canvas.addEventListener('click', () => canvas.focus());
    document.getElementById('pause').addEventListener('click', () => {
        Module.gbaPaused = !Module.gbaPaused;
        document.getElementById('pause').textContent = Module.gbaPaused ? 'Resume' : 'Pause';
    });
    document.getElementById('reset').addEventListener('click', () => {
        Module.gbaResetRequested = true;
        Module.gbaKeys = 0;
    });
});

EM_JS(uint16_t, webPressedKeys, (), { return Module.gbaKeys | 0; });
EM_JS(bool, webIsPaused, (), { return !!Module.gbaPaused; });
EM_JS(bool, webTakeResetRequest, (),
{
    const requested = !!Module.gbaResetRequested;
    Module.gbaResetRequested = false;
    return requested;
});
EM_JS(void, webSetStatus, (const char* message),
{
    document.getElementById('status').textContent = UTF8ToString(message);
});
EM_JS(void, webPresentFrame, (const uint32_t* pixels),
{
    const image = Module.gbaImage;
    const source = HEAPU32.subarray(pixels >>> 2, (pixels >>> 2) + 240 * 160);
    const destination = Module.gbaImageWords;
    for (let index = 0; index < source.length; ++index)
    {
        // The PPU uses 0xAARRGGBB pixels. WebAssembly and browsers are
        // little-endian, while ImageData expects RGBA bytes, so convert each
        // word to its little-endian 0xAABBGGRR representation before upload.
        const pixel = source[index];
        destination[index] = (pixel & 0xFF00FF00) |
            ((pixel & 0x00FF0000) >>> 16) | ((pixel & 0x000000FF) << 16);
    }
    Module.gbaContext.putImageData(image, 0, 0);
});

class WebEmulator
{
public:
    bool initialize()
    {
        webSetup();
        if (!gba.loadBios("/assets/gba_bios.bin"))
        {
            webSetStatus("Unable to load the bundled BIOS.");
            return false;
        }
        if (!gba.loadCartridge("/assets/sma.gba"))
        {
            webSetStatus("Unable to load the bundled Super Mario Advance cartridge.");
            return false;
        }
        nextFrame = gba.masterCycleCount() + FrameCycles;
        webSetStatus("Playing");
        webPresentFrame(gba.ppu.framebuffer().data());
        return true;
    }

    void frame()
    {
        if (webTakeResetRequest()) reset();
        if (webIsPaused()) return;

        gba.setPressedKeys(webPressedKeys());
        // Execute a full hardware video frame per browser animation frame.
        // No wall-clock sleeping is used: requestAnimationFrame owns pacing in
        // the browser and the loop remains deterministic between frames.
        while (gba.masterCycleCount() < nextFrame)
            gba.tick();
        webPresentFrame(gba.ppu.framebuffer().data());
        nextFrame += FrameCycles;
        while (nextFrame <= gba.masterCycleCount()) nextFrame += FrameCycles;
    }

private:
    GBA gba;
    uint64_t nextFrame = FrameCycles;

    void reset()
    {
        gba.reset();
        gba.cpu.pc = gba.bus.hasBios() ? 0x00000000U : 0x08000000U;
        nextFrame = gba.masterCycleCount() + FrameCycles;
        webSetStatus("Playing");
    }
};

void runFrame(void* context)
{
    static_cast<WebEmulator*>(context)->frame();
}
}

int main()
{
    static WebEmulator emulator;
    if (!emulator.initialize()) return 1;
    emscripten_set_main_loop_arg(runFrame, &emulator, 0, true);
    return 0;
}
