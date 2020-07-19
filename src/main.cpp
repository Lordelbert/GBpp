#include "Gameboy.hpp"

auto main() -> int
{
	std::vector<std::uint8_t> program{0xAF, 0xAF, 0xAF, 0xaf, 0x10};
	Gameboy gb{std::move(program)};
	gb.run();
	return 0;
}
