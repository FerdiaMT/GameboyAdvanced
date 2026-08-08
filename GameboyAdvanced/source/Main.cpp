#include "GBA.h"
#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/DebuggerCPU.h"
#include "tdmi7/Decoder.h"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace
{
void printUsage(const char* program)
{
	std::cout << "Usage:\n"
		<< "  " << program << " run <cartridge.gba> [--steps N] [--trace [trace-file]] [--test-swi]\n"
		<< "  " << program << " test thumb\n"
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

	if (argc == 3 && std::string(argv[1]) == "test" && std::string(argv[2]) == "thumb")
	{
		Bus bus;
		tdmi7::Decoder decoder;
		tdmi7::CPU cpu(&bus, &decoder);
		tdmi7::DebuggerCPU debugger(&cpu);
		return debugger.runAllThumbTests(cpu) ? 0 : 1;
	}

	if (argc == 4 && std::string(argv[1]) == "test" && std::string(argv[2]) == "arm")
	{
		Bus bus;
		tdmi7::Decoder decoder;
		tdmi7::CPU cpu(&bus, &decoder);
		return cpu.runIndividualTests(argv[3]) ? 0 : 1;
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
			else
			{
				std::cerr << "Unknown run option: " << option << '\n';
				return 1;
			}
		}

		GBA gba;
		gba.enableTestSwiHalt(testSwiHalt);
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

		const auto result = gba.runSteps(steps, traceFile ? &traceFile : nullptr);
		std::cout << "Completed " << result.steps << " CPU steps (" << result.cycles << " cycles).\n";
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
