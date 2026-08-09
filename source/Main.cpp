#include "GBA.h"
#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/DebuggerCPU.h"
#include "tdmi7/Decoder.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#if defined(GBA_HAS_SDL2)
#include <SDL2/SDL.h>
#endif

namespace
{
void printUsage(const char* program)
{
	std::cout << "Usage:\n"
		<< "  " << program << " run <cartridge.gba> [--bios <bios.bin>] [--hle-bios] [--steps N] [--trace [trace-file]] [--dump-state] [--test-swi] [--display] [--fps]\n"
		<< "  " << program << " test thumb [fixture.json.bin]\n"
		<< "  " << program << " test arm <fixture.json.bin>\n"
		<< "  " << program << " --help\n";
}

bool parseSteps(const char* value, uint64_t& steps)
{
	const std::string input(value);
	const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), steps);
	return error == std::errc{} && end == input.data() + input.size();
}

bool isRepositoryRelativePath(const std::filesystem::path& path)
{
	if (path.empty() || path.is_absolute())
	{
		return false;
	}

	const auto normalized = path.lexically_normal();
	return normalized.begin() == normalized.end() || *normalized.begin() != "..";
}

#if defined(GBA_HAS_SDL2)
class SdlDisplay final
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;

public:
	static std::optional<uint16_t> gbaKeyForSdlKey(SDL_Keycode key)
	{
		switch (key)
		{
		case SDLK_z: return 1U << 0;          // A
		case SDLK_x: return 1U << 1;          // B
		case SDLK_BACKSPACE: return 1U << 2;  // Select
		case SDLK_RETURN: return 1U << 3;     // Start
		case SDLK_RIGHT: return 1U << 4;
		case SDLK_LEFT: return 1U << 5;
		case SDLK_UP: return 1U << 6;
		case SDLK_DOWN: return 1U << 7;
		case SDLK_s: return 1U << 8;          // R
		case SDLK_a: return 1U << 9;          // L
		default: return std::nullopt;
		}
	}

	bool open(bool visible = true)
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
		window = SDL_CreateWindow("Game Boy Advance", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			480, 320, visible ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN);
		if (!window) return false;
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
		if (!renderer) return false;
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 240, 160);
		return texture != nullptr;
	}

	bool present(const std::array<uint32_t, 240 * 160>& frame)
	{
		if (!texture || SDL_UpdateTexture(texture, nullptr, frame.data(), 240 * sizeof(uint32_t)) != 0) return false;
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
		return true;
	}

	bool pollInput(uint16_t& pressedKeys, bool& resetRequested) const
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) return true;
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F5) resetRequested = true;
			if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
			{
				const auto gbaKey = gbaKeyForSdlKey(event.key.keysym.sym);
				if (gbaKey)
				{
					if (event.type == SDL_KEYDOWN) pressedKeys |= *gbaKey;
					else pressedKeys &= static_cast<uint16_t>(~*gbaKey);
				}
			}
		}
		return false;
	}

	~SdlDisplay()
	{
		if (texture) SDL_DestroyTexture(texture);
		if (renderer) SDL_DestroyRenderer(renderer);
		if (window) SDL_DestroyWindow(window);
		SDL_Quit();
	}
};
#endif
}

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		printUsage(argv[0]);
		return 0;
	}

	if (argc == 2 && std::string(argv[1]) == "--help")
	{
		printUsage(argv[0]);
		return 0;
	}

	if (argc == 2 && std::string(argv[1]) == "--screen-smoke")
	{
#if defined(GBA_HAS_SDL2)
		GBA gba;
		gba.bus.write16(0x04000000, 3);
		gba.bus.write16(0x06000000, 0x001F);
		gba.ppu.advance(960);
		SdlDisplay display;
		if (!display.open(false) || !display.present(gba.ppu.framebuffer()))
		{
			std::cerr << "SDL framebuffer smoke test failed: " << SDL_GetError() << '\n';
			return 1;
		}
		return 0;
#else
		std::cerr << "SDL2 support was not enabled at build time. Install SDL2-devel and reconfigure.\n";
		return 1;
#endif
	}

	if (argc == 2 && std::string(argv[1]) == "--input-smoke")
	{
#if defined(GBA_HAS_SDL2)
		const auto a = SdlDisplay::gbaKeyForSdlKey(SDLK_z);
		const auto right = SdlDisplay::gbaKeyForSdlKey(SDLK_RIGHT);
		const auto shoulder = SdlDisplay::gbaKeyForSdlKey(SDLK_s);
		if (!a || !right || !shoulder || *a != 1U || *right != (1U << 4) || *shoulder != (1U << 8))
		{
			std::cerr << "SDL input mapping smoke test failed.\n";
			return 1;
		}
		return 0;
#else
		std::cerr << "SDL2 support was not enabled at build time.\n";
		return 1;
#endif
	}

	if (argc == 3 && std::string(argv[1]) == "test" && std::string(argv[2]) == "thumb")
	{
		Bus bus;
		tdmi7::Decoder decoder;
		tdmi7::CPU cpu(&bus, &decoder);
		tdmi7::DebuggerCPU debugger(&cpu);
		return debugger.runAllThumbTests(cpu) ? 0 : 1;
	}

	if (argc == 4 && std::string(argv[1]) == "test" && std::string(argv[2]) == "thumb")
	{
		Bus bus;
		tdmi7::Decoder decoder;
		tdmi7::CPU cpu(&bus, &decoder);
		return cpu.runIndividualTests(argv[3], true) ? 0 : 1;
	}

	if (argc == 4 && std::string(argv[1]) == "test" && std::string(argv[2]) == "arm")
	{
		Bus bus;
		tdmi7::Decoder decoder;
		tdmi7::CPU cpu(&bus, &decoder);
		return cpu.runIndividualTests(argv[3], false) ? 0 : 1;
	}

	if (argc >= 3)
	{
		if (std::string(argv[1]) != "run")
		{
			printUsage(argv[0]);
			return 1;
		}

		uint64_t steps = 10'000;
		bool testSwiHalt = false;
		bool displayRequested = false;
		bool dumpState = false;
		bool showFps = false;
		bool hleBios = false;
		std::optional<std::filesystem::path> biosPath;
		std::optional<std::filesystem::path> tracePath;
		for (int argument = 3; argument < argc; ++argument)
		{
			const std::string option(argv[argument]);
			if (option == "--steps")
			{
				if (++argument == argc || !parseSteps(argv[argument], steps))
				{
					std::cerr << "Invalid --steps value.\n";
					return 1;
				}
			}
			else if (option == "--trace")
			{
				if (argument + 1 < argc && std::string(argv[argument + 1]).rfind("--", 0) != 0)
				{
					tracePath = argv[++argument];
				}
				else
				{
					tracePath = std::filesystem::path("traces") /
						(std::filesystem::path(argv[2]).stem().string() + ".trace");
				}
			}
			else if (option == "--test-swi")
			{
				testSwiHalt = true;
			}
			else if (option == "--dump-state")
			{
				dumpState = true;
			}
			else if (option == "--display")
			{
				displayRequested = true;
			}
			else if (option == "--fps")
			{
				showFps = true;
			}
			else if (option == "--bios")
			{
				if (++argument == argc)
				{
					std::cerr << "Missing --bios path.\n";
					return 1;
				}
				biosPath = argv[argument];
			}
			else if (option == "--hle-bios")
			{
				hleBios = true;
			}
			else
			{
				std::cerr << "Unknown run option: " << option << '\n';
				return 1;
			}
		}

		GBA gba;
		if (hleBios && biosPath)
		{
			std::cerr << "--bios and --hle-bios cannot be used together.\n";
			return 1;
		}
		gba.enableTestSwiHalt(testSwiHalt);
		if (hleBios) gba.system.setBiosPolicy(SystemControl::BiosPolicy::Hle);
		if (biosPath && !gba.loadBios(biosPath->c_str()))
		{
			std::cerr << "Could not load an exact 16 KiB BIOS image: " << biosPath->string() << '\n';
			return 1;
		}
		if (displayRequested && tracePath)
		{
			std::cerr << "--display and --trace cannot currently be used together.\n";
			return 1;
		}
		if (!gba.loadCartridge(argv[2]))
		{
			return 1;
		}

		std::ofstream traceFile;
		if (tracePath)
		{
			if (!isRepositoryRelativePath(*tracePath))
			{
				std::cerr << "Trace path must be relative to the repository root.\n";
				return 1;
			}

			if (!tracePath->parent_path().empty())
			{
				std::filesystem::create_directories(tracePath->parent_path());
			}
			traceFile.open(*tracePath, std::ios::trunc);
			if (!traceFile)
			{
				std::cerr << "Could not open trace file: " << tracePath->string() << '\n';
				return 1;
			}
		}

		GBA::RunResult result{};
		if (displayRequested)
		{
#if defined(GBA_HAS_SDL2)
			SdlDisplay display;
			if (!display.open())
			{
				std::cerr << "Could not create SDL display: " << SDL_GetError() << '\n';
				return 1;
			}
			constexpr uint64_t frameCycles = 1232U * 228U;
			constexpr double gbaClockHz = 16'777'216.0;
			const auto frameDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
				std::chrono::duration<double>(static_cast<double>(frameCycles) / gbaClockHz));
			uint64_t nextFrame = gba.masterCycleCount() + frameCycles;
			auto nextPresentation = std::chrono::steady_clock::now() + frameDuration;
			uint64_t completedSteps = 0;
			uint64_t presentedFrames = 0;
			auto fpsStart = std::chrono::steady_clock::now();
			bool quit = false;
			uint16_t pressedKeys = 0;
			auto resetEmulator = [&]
			{
				gba.reset();
				gba.cpu.pc = gba.bus.hasBios() ? 0x00000000U : 0x08000000U;
				gba.setPressedKeys(pressedKeys);
				nextFrame = gba.masterCycleCount() + frameCycles;
				nextPresentation = std::chrono::steady_clock::now() + frameDuration;
			};
			bool resetRequested = false;
			quit = display.pollInput(pressedKeys, resetRequested);
			if (resetRequested) resetEmulator();
			std::cout << "Controls: Z=A, X=B, Enter=Start, Backspace=Select, arrows=D-pad, A=L, S=R, F5=reset, Escape=quit.\n";
			while (completedSteps < steps && gba.cpu.testHalt == tdmi7::CPU::TestHalt::None && !quit)
			{
				gba.setPressedKeys(pressedKeys);
				gba.tick();
				++completedSteps;
				if (gba.masterCycleCount() >= nextFrame)
				{
					display.present(gba.ppu.framebuffer());
					++presentedFrames;
					if (showFps)
					{
						const auto now = std::chrono::steady_clock::now();
						const auto elapsed = std::chrono::duration<double>(now - fpsStart).count();
						if (elapsed >= 1.0)
						{
							std::cout << "Display: " << static_cast<double>(presentedFrames) / elapsed << " FPS\n";
							presentedFrames = 0;
							fpsStart = now;
						}
					}
					std::this_thread::sleep_until(nextPresentation);
					const auto now = std::chrono::steady_clock::now();
					nextPresentation += frameDuration;
					// Do not accumulate a multi-frame debt after a debugger stop or
					// a slow host frame: resume pacing from the current wall clock.
					if (now > nextPresentation + frameDuration * 2) nextPresentation = now + frameDuration;
					while (nextFrame <= gba.masterCycleCount()) nextFrame += frameCycles;
					// Host input is asynchronous, while the GBA keypad state is held
					// between samples. Pump SDL once per displayed frame instead of
					// once per emulated instruction.
					resetRequested = false;
					quit = display.pollInput(pressedKeys, resetRequested);
					if (resetRequested) resetEmulator();
				}
			}
			display.present(gba.ppu.framebuffer());
			result = { gba.masterCycleCount(), completedSteps, gba.cpu.testHalt };
#else
			std::cerr << "This build has no SDL2 support. Install SDL2-devel, reconfigure, and rebuild.\n";
			return 1;
#endif
		}
		// A default-constructed ofstream is in a good state even before it is
		// opened, so testing traceFile here would enable instruction tracing for
		// every normal run. tracePath is engaged only by the --trace option.
		else result = gba.runSteps(steps, tracePath ? &traceFile : nullptr);
		std::cout << "Completed " << result.steps << " CPU steps (" << result.cycles << " cycles).\n";
		if (dumpState)
		{
			std::cout << std::hex << std::setfill('0')
				<< "CPU state: PC=0x" << std::setw(8) << gba.cpu.pc
				<< " CPSR=0x" << std::setw(8) << gba.cpu.CPSR
				<< " SP=0x" << std::setw(8) << gba.cpu.sp
				<< " LR=0x" << std::setw(8) << gba.cpu.lr
				<< " IE=0x" << std::setw(4) << gba.system.interruptEnable()
				<< " IF=0x" << std::setw(4) << gba.system.interruptFlags()
				<< " IME=" << (gba.system.interruptMasterEnabled() ? 1 : 0)
				<< " cart=" << (gba.hasExecutedCartridgeCode() ? 1 : 0)
				<< " VCOUNT=" << std::setw(3) << gba.ppu.vcountValue()
				<< std::dec << std::setfill(' ') << '\n';
		}
		if (tracePath)
		{
			std::cout << "Wrote trace to " << tracePath->string() << ".\n";
		}
		if (result.halt == tdmi7::CPU::TestHalt::Passed)
		{
			std::cout << "Test passed (SWI #0).\n";
			return 0;
		}
		if (result.halt == tdmi7::CPU::TestHalt::Failed)
		{
			std::cerr << "Test failed (SWI #1).\n";
			return 2;
		}
		if (testSwiHalt)
		{
			std::cerr << "Test did not halt within the step limit.\n";
			return 3;
		}
		return 0;
	}

	printUsage(argv[0]);
	return 1;
}
