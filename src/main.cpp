#include "Gameboy.hpp"

auto main() -> int
{
	const std::vector<std::uint8_t> program{0xAF, 0x0A, 0xAF, 0xaf, 0x10};
	Gameboy gb{program};
	gb.run();
	return 0;
}
