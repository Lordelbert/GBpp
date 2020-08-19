#include "Gameboy.hpp"

auto main() -> int
{
	std::initializer_list<std::uint8_t> program{0xAF, 0x0A, 0xAF, 0xaf, 0x10};
	Gameboy gb{std::move(program)};
	gb.run();
	return 0;
}
