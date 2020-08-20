#include "MBC.hpp"
// TODO change nbr with enum to make code more explicit
auto MBC1::swap_bank_rom_high()
{
	for(size_t i = 0; i < 2; ++i) {
		MBC1_Partition::section mem_part{&m_memory[reg1_2() * 16_kB + i * 8_kB],
		                                 &m_memory[reg1_2() * 16_kB + i * 8_kB + 8_kB]};
		m_memory_partition.swap(i + 2, std::move(mem_part));
	}
	return;
}
auto MBC1::swap_bank_rom_low()
{
	const size_t addr = (mode()) ? (reg2() << 5) * 16_kB : 0b0;
	for(size_t i = 0; i < 2; ++i) {
		MBC1_Partition::section mem_part{&m_memory[addr + i * 8_kB],
		                                 &m_memory[addr + i * 8_kB + 8_kB]};
		m_memory_partition.swap(i, std::move(mem_part));
	}
	return;
}
auto MBC1::swap_bank_ram()
{
	const size_t addr = (mode()) ? reg2() * 8_kB : 0b0;
	MBC1_Partition::section mem_part{&m_memory[addr], &m_memory[addr + 8_kB]};
	m_memory_partition.swap(5, std::move(mem_part));
	return;
}
auto MBC1::ramg_enable(std::uint8_t value) noexcept -> void
{
	m_ramg_enable = ((value & 0x0F) == 0x0A) ? true : false;
}
auto MBC1::bank_reg1(std::uint8_t value) noexcept -> void
{
	const std::uint8_t _val = (value & 0b0001'1111);
	m_bank_selector = (m_bank_selector & 0b1110'0000) | ((_val == 0) ? 0b1 : _val);
	swap_bank_rom_high();
}
// note can also be used for bank in ram :
auto MBC1::bank_reg2(std::uint8_t value) noexcept -> void
{
	m_bank_selector = (m_bank_selector & 0b1001'1111) | ((value & 0b11) << 5);
	swap_bank_rom_high();
	swap_bank_rom_low();
        if(m_ram > 8_kB) swap_bank_ram();
}
auto MBC1::bank_mode(std::uint8_t value) noexcept -> void
{
	m_bank_selector = set_bit(m_bank_selector, 7, static_cast<std::uint8_t>(value & 0b1));
	swap_bank_rom_high();
	swap_bank_rom_low();
        if(m_ram > 8_kB) swap_bank_ram();
}
[[nodiscard]] auto MBC1::read(std::uint16_t addr) const noexcept -> std::uint8_t
{
	if(addr >= SWI_RAM_base and addr < SWI_RAM_ul) {
		return (m_ramg_enable) ? m_memory_partition[addr] : m_distrib(m_gen);
	}
	return m_memory_partition[addr];
}
auto MBC1::write(std::uint16_t addr, std::uint8_t value) noexcept -> void
{
	if(addr < IROM1_ul) {
		if(addr < 0x2000) {
			ramg_enable(value);
			return;
		}
		if(addr < 0x4000) {
			bank_reg1(value);
			return;
		}
		if(addr < 0x6000) {
			bank_reg2(value);
			return;
		}
		if(addr < 0x8000) {
			bank_mode(value);
			return;
		}
	}
	if(addr >= SWI_RAM_base and addr < SWI_RAM_ul) {
		if(m_ramg_enable) {
			m_memory_partition[addr] = value;
		}
	}
	m_memory_partition[addr] = value;
	return;
}
