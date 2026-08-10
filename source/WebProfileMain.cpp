#include "GBA.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace
{
bool parseStepCount(const char* value, uint64_t& result)
{
	if (value == nullptr || *value == '-') return false;
    char* end = nullptr;
    result = std::strtoull(value, &end, 10);
    return end && *end == '\0' && result != 0;
}
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::fprintf(stderr, "Usage: %s <bios> <rom> <steps>\n", argv[0]);
        return 2;
    }

    uint64_t steps = 0;
    if (!parseStepCount(argv[3], steps))
    {
        std::fprintf(stderr, "Step count must be a positive integer.\n");
        return 2;
    }

    // The emulated memory arrays are deliberately large; keep them out of
    // Emscripten's relatively small C stack just as the browser frontend does.
    static GBA gba;
    if (!gba.loadBios(argv[1]))
    {
        std::fprintf(stderr, "Unable to load BIOS: %s\n", argv[1]);
        return 1;
    }
    if (!gba.loadCartridge(argv[2]))
    {
        std::fprintf(stderr, "Unable to load cartridge: %s\n", argv[2]);
        return 1;
    }

    for (uint64_t step = 0; step < steps && gba.cpu.testHalt == tdmi7::CPU::TestHalt::None; ++step)
        gba.tick();

    std::printf("Profiled %llu steps (%llu cycles).\n",
        static_cast<unsigned long long>(steps),
        static_cast<unsigned long long>(gba.masterCycleCount()));
    return 0;
}
