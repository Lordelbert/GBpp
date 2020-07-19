#include "memory.hpp"
auto Memory::read(uint16_t addr) const noexcept -> task<std::uint8_t>
{
	co_await make_awaiter(m_clock, 1);
	if(addr < IROM1_ul or (addr > IRAM1_base and addr < IRAM1_ul)) {
		co_return m_game->read(addr);
	}
	else {
		co_return m_memory[addr];
	}
}
// use when fetch overlap execute
auto Memory::read_nowait(uint16_t addr) const noexcept -> std::uint8_t
{
	if(addr < IROM1_ul or (addr > IRAM1_base and addr < IRAM1_ul)) {
		return m_game->read(addr);
	}
	else {
		return m_memory[addr];
	}
}

auto Memory::write(uint16_t addr, std::uint8_t value) noexcept -> task<void>
{
	if(addr < IROM1_ul or (addr > IRAM1_base and addr < IRAM1_ul)) {
                m_game->write(addr,value);
	}
	else {
		m_memory[addr] = value;
	}
	co_await make_awaiter(m_clock, 1);
	co_return;
}
auto Memory::write_IME(std::uint8_t value) -> task<void>
{
	co_await write(IME_base, value & 0x1F);
	co_return;
}
