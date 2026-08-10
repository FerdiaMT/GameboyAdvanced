#include "GBA.h"

#include <emscripten/emscripten.h>

#include <array>
#include <cstdint>
#include <cstdio>

namespace
{
constexpr uint64_t FrameCycles = 1232U * 228U;
constexpr double GbaClockHz = 16'777'216.0;
constexpr double FrameDurationMs = static_cast<double>(FrameCycles) * 1000.0 / GbaClockHz;
constexpr double MaximumFrameGapMs = 250.0;

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
    Module.gbaCartridgeChangeRequested = false;
    Module.gbaCartridgePath = '/assets/super-mario-advance-4.gba';
    Module.gbaFpsFrameCount = 0;
    Module.gbaFpsLastUpdate = performance.now();

    // A canvas is a replaced element.  Combining a 100%-height rule with a
    // max-width rule lets browser zoom clamp one axis without recalculating
    // the other, stretching the 3:2 GBA image vertically.  Size both axes
    // from the same scale so the display always remains 240:160.
    const screen = canvas.parentElement;
    const resizeCanvas = () => {
        const style = getComputedStyle(screen);
        const width = screen.clientWidth - parseFloat(style.paddingLeft) - parseFloat(style.paddingRight);
        const height = screen.clientHeight - parseFloat(style.paddingTop) - parseFloat(style.paddingBottom);
        const scale = Math.max(0, Math.min(width / 240, height / 160));
        canvas.style.width = `${240 * scale}px`;
        canvas.style.height = `${160 * scale}px`;
    };
    new ResizeObserver(resizeCanvas).observe(screen);
    window.addEventListener('resize', resizeCanvas);
    resizeCanvas();

    const title = document.getElementById('game-title');
    const status = document.getElementById('status');
    const gameSelect = document.getElementById('game-select');
    const romUpload = document.getElementById('rom-upload');
    const setStatus = message => { status.textContent = message; };
    const queueCartridgeChange = (path, name) => {
        Module.gbaCartridgePath = path;
        Module.gbaCartridgeChangeRequested = true;
        Module.gbaKeys = 0;
        title.textContent = name;
        document.title = `${name} - Game Boy Advance`;
        setStatus(`Loading ${name}...`);
    };
    gameSelect.addEventListener('change', () => {
        queueCartridgeChange(gameSelect.value, gameSelect.selectedOptions[0].textContent);
    });
    romUpload.addEventListener('change', async () => {
        const [file] = romUpload.files;
        if (!file) return;
        if (file.size === 0 || file.size > 0x02000000) {
            setStatus('Choose a non-empty GBA ROM no larger than 32 MiB.');
            romUpload.value = null;
            return;
        }
        try {
            setStatus(`Loading ${file.name}...`);
            FS.writeFile('/uploaded.gba', new Uint8Array(await file.arrayBuffer()));
            queueCartridgeChange('/uploaded.gba', file.name);
        } catch (error) {
            console.error('Unable to load uploaded ROM.', error);
            setStatus('Unable to read that ROM file.');
        } finally {
            romUpload.value = null;
        }
    });

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
EM_JS(bool, webTakeCartridgeChangeRequest, (),
{
    const requested = !!Module.gbaCartridgeChangeRequested;
    Module.gbaCartridgeChangeRequested = false;
    return requested;
});
EM_JS(void, webCopyCartridgePath, (char* destination, size_t capacity),
{
    stringToUTF8(Module.gbaCartridgePath, destination, capacity);
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
    const now = performance.now();
    ++Module.gbaFpsFrameCount;
    const elapsed = now - Module.gbaFpsLastUpdate;
    if (elapsed >= 500)
    {
        document.getElementById('fps').textContent = `${Math.round(Module.gbaFpsFrameCount * 1000 / elapsed)} FPS`;
        Module.gbaFpsFrameCount = 0;
        Module.gbaFpsLastUpdate = now;
    }
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
        if (!gba.loadCartridge("/assets/super-mario-advance-4.gba"))
        {
            webSetStatus("Unable to load the bundled Super Mario Advance 4 cartridge.");
            return false;
        }
        nextFrame = gba.masterCycleCount() + FrameCycles;
        resetFramePacing();
        webSetStatus("Playing");
        webPresentFrame(gba.ppu.framebuffer().data());
        return true;
    }

    void frame()
    {
        if (webTakeCartridgeChangeRequest()) loadSelectedCartridge();
        if (webTakeResetRequest()) reset();

        const double now = emscripten_get_now();
        if (webIsPaused())
        {
            resetFramePacing(now);
            return;
        }

        double elapsedMs = now - lastHostTimeMs;
        lastHostTimeMs = now;
        if (elapsedMs < 0.0) elapsedMs = 0.0;

        // requestAnimationFrame runs at the display refresh rate.  Running a
        // GBA frame for every callback makes a 120 Hz display emulate at twice
        // speed, so accumulate real time and only advance complete 59.73 Hz
        // GBA video frames.  Dropping an unusually large gap prevents a
        // backgrounded tab from trying to catch up hundreds of frames at once.
        if (elapsedMs > MaximumFrameGapMs)
        {
            accumulatedFrameTimeMs = 0.0;
            return;
        }
        accumulatedFrameTimeMs += elapsedMs;
        if (accumulatedFrameTimeMs < FrameDurationMs) return;

        gba.setPressedKeys(webPressedKeys());
        do
        {
            while (gba.masterCycleCount() < nextFrame)
                gba.tick();
            nextFrame += FrameCycles;
            while (nextFrame <= gba.masterCycleCount()) nextFrame += FrameCycles;
            accumulatedFrameTimeMs -= FrameDurationMs;
        }
        while (accumulatedFrameTimeMs >= FrameDurationMs);
        webPresentFrame(gba.ppu.framebuffer().data());
    }

private:
    GBA gba;
    uint64_t nextFrame = FrameCycles;
    double lastHostTimeMs = 0.0;
    double accumulatedFrameTimeMs = 0.0;

    void resetFramePacing(double now = emscripten_get_now())
    {
        lastHostTimeMs = now;
        accumulatedFrameTimeMs = 0.0;
    }

    void loadSelectedCartridge()
    {
        std::array<char, 1024> path{};
        webCopyCartridgePath(path.data(), path.size());
        if (!gba.loadCartridge(path.data()))
        {
            webSetStatus("Unable to load the selected cartridge.");
            return;
        }
        nextFrame = gba.masterCycleCount() + FrameCycles;
        resetFramePacing();
        webPresentFrame(gba.ppu.framebuffer().data());
        webSetStatus("Playing");
    }

    void reset()
    {
        gba.reset();
        gba.cpu.pc = gba.bus.hasBios() ? 0x00000000U : 0x08000000U;
        nextFrame = gba.masterCycleCount() + FrameCycles;
        resetFramePacing();
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
