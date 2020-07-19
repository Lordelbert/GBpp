#include "MBC.hpp"
auto MBC1::select_bank(std::uint16_t addr) const noexcept -> std::uint32_t
{
	if(addr >= 0x4000) {
		const std::uint32_t bank_addr = (m_bank_selector & 0b0111'1111)
		                                << (sizeof(uint16_t) * 8);
		return bank_addr | addr;
	}
	else {
		// Reading 0x0000 - 0x3FFF
		const std::uint8_t mask = (m_bank_selector & 0b1000'0000) ? 0b1100000 : 0b0;
		const std::uint32_t bank_addr = (m_bank_selector & mask)
		                                << (sizeof(uint16_t) * 8);
		return bank_addr | addr;
	}
}

auto MBC1::read_ram(std::uint16_t addr) const noexcept -> std::uint8_t
{
	if(not m_ramg_enable) {
		return m_distrib(m_gen);
	}
	else {
		if(mode()) {
			addr = (addr & 0b0000'1111'1111'1111) | bank2();
		}
		return m_ram_bank[addr-0xA000];
	}
}
auto MBC1::read(std::uint16_t address) const noexcept -> std::uint8_t
{
	return (address < 0xA000) ? (address < 0x4000) ? m_rom_bank[select_bank(address)]
                                                       : m_rom_bank[select_bank(address)]
                                                       : read_ram(address);
}
auto MBC1::write(std::uint16_t address, std::uint8_t value) -> void
{
	if(address < 0x2000) {
		ramg_enable(value);
	}
	else if(address < 0x4000) {
		bank_reg1(value);
	}
	else if(address < 0x6000) {
		bank_reg2(value);
	}
	else if(address < 0x8000) {
		bank_mode(value);
	}
}
