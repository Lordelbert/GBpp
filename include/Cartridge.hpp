#ifndef __CARTRIDGE_HPP__
#define __CARTRIDGE_HPP__
#include "bit_manipulation.hpp"
#include "cstdint"
#include "include_std.hpp"
#include <array>
#include <cstddef>
#include <iterator>
#include <random>
// TODO : MBC3 TIMER :baconsweat:
enum Cartridge_type : std::uint16_t {
	ROM_ONLY = 0x0,
	ROM_MBC1 = 0x1,
	ROM_MBC2 = 0x5,
	ROM_MBC3 = 0x11,
	ROM_MBC5 = 0x19,
	Pocket_Camera = 0x1F,
	Bandai_TAMA5 = 0xFD,
	Hudson_HuC_3 = 0xFE,
	Hudson_HuC_1 = 0xFF,
};
enum MBC_type : std::uint16_t {
	ROM_MBC1_RAM = 0x2,
	ROM_MBC1_RAM_BATT = 0x3,
	ROM_MBC2_BATTERY = 0x6,
	ROM_RAM = 0x8,
	ROM_RAM_BATTERY = 0x9,
	ROM_MMM01 = 0xB,
	ROM_MMM01_SRAM = 0xC,
	ROM_MMM01_SRAM_BATT = 0xC,
	ROM_MBC3_TIMER_BATT = 0xD,
	ROM_MBC3_TIMER_RAM_BATT = 0x10,
	ROM_MBC3_RAM = 0x12,
	ROM_MBC3_RAM_BATT = 0x13,
	ROM_MBC5_RAM = 0x1A,
	ROM_MBC5_RAM_BATT = 0x1B,
	ROM_MBC5_RUMBLE = 0x1C,
	ROM_MBC5_RUMBLE_SRAM = 0x1D,
	ROM_MBC5_RUMBLE_SRAM_BATT = 0x1E,
};
template <enum Cartridge_type> class Cartridge {
	static constexpr size_t ROM_SIZE = 0x4000; // Size of one bank
	static constexpr size_t RAM_SIZE = 0x2000; // Size of one bank

	std::vector<std::uint8_t> IRAM;
	std::vector<std::uint8_t> rom_bank;
	std::span<const std::uint8_t, ROM_SIZE * 126> rom_view;

	std::random_device rd;
	mutable std::mt19937 gen;
	mutable std::uniform_int_distribution<std::uint8_t> distrib;

	bool RAMG_enable; // write at 0x0000 0x1FFF
	std::uint8_t bank_selector = 0b1; // (mode 1 bit) | 2 bit | 5 bit

  public:
	Cartridge(std::vector<std::uint8_t> prog) noexcept
	    : IRAM(RAM_SIZE * 4), rom_bank{prog}, rom_view{rom_bank}, gen{rd()}, distrib{0,
	                                                                                 255}
	{
		std::generate(std::begin(IRAM), std::end(IRAM),
		              [this]() { return distrib(gen); });
	}

	constexpr auto read_ram(std::uint16_t addr) const noexcept -> std::uint8_t
	{
		if(not RAMG_enable) {
			return distrib(gen);
		}
		else {
			if(RAM_SIZE >= 0x2000 and MODE()) {
				addr = (addr & 0b0000'1111'1111'1111) | BANK2();
			}
			return IRAM[addr - 0xA000];
		}
	}
	constexpr auto read(std::uint16_t addr) const noexcept -> std::uint8_t
	{
		return (addr < 0xA000) ? rom_view[select_bank(addr)] : read_ram(addr);
	}
	constexpr auto write(std::uint16_t addr, std::uint8_t value) noexcept -> void
	{
		if(addr < 0x2000) {
			ramg_enable(value);
		}
		else if(addr < 0x4000) {
			bank_reg1(value);
		}
		else if(addr < 0x6000) {
			bank_reg2(value);
		}
		else if(addr < 0x8000) {
			bank_mode(value);
		}
	}

	constexpr auto ramg_enable(std::uint8_t value) noexcept -> void;
	constexpr auto bank_reg1(std::uint8_t value) noexcept -> void;
	constexpr auto bank_reg2(std::uint8_t value) noexcept -> void;
	constexpr auto bank_mode(std::uint8_t value) noexcept -> void;
	constexpr auto select_bank(std::uint16_t addr) const noexcept -> std::uint32_t;

  private:
	constexpr auto MODE() const noexcept -> std::uint8_t { return bank_selector >> 7; }
	constexpr auto BANK1() const noexcept -> std::uint8_t
	{
		return bank_selector & 0b1111;
	}
	constexpr auto BANK2() const noexcept -> std::uint8_t
	{
		return (bank_selector >> 5) & 0b11;
	}
};

template <enum Cartridge_type Type>
constexpr auto Cartridge<Type>::ramg_enable(std::uint8_t value) noexcept -> void
{
	RAMG_enable = ((value & 0x0F) == 0x0A) ? true : false;
}

template <>
constexpr auto Cartridge<ROM_MBC5>::ramg_enable(std::uint8_t value) noexcept -> void
{
	RAMG_enable = (value == 0x0A) ? true : false;
}

template <>
constexpr auto Cartridge<ROM_MBC1>::bank_reg1(std::uint8_t value) noexcept -> void
{
	const auto _val = (value & 0b1111);
	bank_selector = (_val == 0) ? 0b1 : _val;
}

template <>
// note can also be used for bank in ram :
constexpr auto Cartridge<ROM_MBC1>::bank_reg2(std::uint8_t value) noexcept -> void
{
	bank_selector = ((value & 0b11) << 5) | bank_selector;
}

template <>
constexpr auto Cartridge<ROM_MBC1>::bank_mode(std::uint8_t value) noexcept -> void
{
	bank_selector = set_bit(bank_selector, 7, static_cast<std::uint8_t>(value & 0b1));
}

template <>
constexpr auto Cartridge<ROM_MBC1>::select_bank(std::uint16_t addr) const noexcept
    -> std::uint32_t
{
	if(addr >= 0x4000) {
		const std::uint32_t bank_addr = (bank_selector & 0b0111'1111)
		                                << (sizeof(uint16_t) * 8);
		return bank_addr | addr;
	}
	else {
		// Reading 0x0000 - 0x3FFF
		const std::uint8_t mask = (bank_selector & 0b1000'0000) ? 0b1100000 : 0b0;
		const std::uint32_t bank_addr = (bank_selector & mask) << (sizeof(uint16_t) * 8);
		return bank_addr | addr;
	}
}

#endif
