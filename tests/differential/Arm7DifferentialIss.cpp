#include "Bus.h"
#include "tdmi7/CPU.h"
#include "tdmi7/Decoder.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace
{
bool parseHex(std::string value, uint32_t& result)
{
	if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0)
		value.erase(0, 2);
	const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result, 16);
	return error == std::errc{} && end == value.data() + value.size();
}

bool readWords(const char* path, uint32_t* words, size_t count)
{
	std::ifstream input(path);
	std::string token;
	for (size_t index = 0; index < count; ++index)
	{
		if (!(input >> token) || !parseHex(token, words[index]))
			return false;
	}
	return true;
}
}

int main(int argc, char* argv[])
{
	if (argc != 9 || std::string(argv[1]) != "--state" || std::string(argv[3]) != "--program"
		|| std::string(argv[5]) != "--instructions" || std::string(argv[7]) != "--output")
	{
		std::cerr << "Usage: arm7_differential_iss --state <state.hex> --program <program.hex> "
			"--instructions <count> --output <state.out>\n";
		return 2;
	}

	uint32_t instructionCount = 0;
	const std::string countArgument(argv[6]);
	if (const auto [end, error] = std::from_chars(countArgument.data(),
		countArgument.data() + countArgument.size(), instructionCount);
		error != std::errc{} || end != countArgument.data() + countArgument.size()
		|| instructionCount == 0 || instructionCount > 256)
	{
		std::cerr << "Instruction count must be in [1, 256].\n";
		return 2;
	}

	std::array<uint32_t, 15> initialRegisters{};
	std::array<uint32_t, 256> program{};
	if (!readWords(argv[2], initialRegisters.data(), initialRegisters.size())
		|| !readWords(argv[4], program.data(), program.size()))
	{
		std::cerr << "Could not read differential test input.\n";
		return 2;
	}

	Bus bus;
	tdmi7::Decoder decoder;
	tdmi7::CPU cpu(&bus, &decoder);
	cpu.reset();
	for (size_t index = 0; index < initialRegisters.size(); ++index)
		cpu.reg[index] = initialRegisters[index];
	cpu.pc = 0;
	// The RTL reference only supplies NZCV and starts them clear. Keep the ISS
	// in system mode with the same architecturally visible flags.
	cpu.CPSR = static_cast<uint32_t>(tdmi7::CPU::mode::System);

	for (uint32_t index = 0; index < instructionCount; ++index)
		cpu.armExecute(cpu.decodeArm(program[index]));

	std::ofstream output(argv[8], std::ios::trunc);
	if (!output)
	{
		std::cerr << "Could not write ISS state.\n";
		return 2;
	}
	output << "ARM7_ISS_STATE";
	for (unsigned index = 0; index < initialRegisters.size(); ++index)
		output << " r" << std::dec << index << '=' << std::hex << std::setfill('0')
			<< std::setw(8) << cpu.reg[index];
	output << " cpsr=" << std::setw(8) << cpu.CPSR;
	output << '\n';
	return 0;
}
