#ifndef __MBC_HPP__
#define __MBC_HPP__
#include "MBC_utility.hpp"
#include "bit_manipulation.hpp"
#include "include_std.hpp"
#include "units.hpp"
#include <algorithm>
#include <exception>
#include <random>
#include <vector>
enum Memory_map : uint16_t {
	IROM0_ul = 0x4000,
	IROM1_ul = 0x8000,
	VRAM_ul = 0xA000,
	SWI_RAM_ul = 0xC000,
	IRAM0_ul = 0xE000,
	IRAM0_E_ul = 0xFE00,
	OAM_ul = 0xFEA0,
	EMPTY_BIO_ul = 0xFF00,
	IO_ul = 0xFF4C,
	EMPTY_AIO_ul = 0xFF80,
	IRAM1_ul = 0xFFFF,

	IROM0_base = 0x0,
	IROM1_base = IROM0_ul,
	VRAM_base = IROM1_ul,
	SWI_RAM_base = VRAM_ul,
	IRAM0_base = SWI_RAM_ul,
	IRAM0_E_base = IRAM0_ul,
	OAM_base = IRAM0_E_ul,
	EMPTY_BIO_base = OAM_ul,
	IO_base = EMPTY_BIO_ul,
	EMPTY_AIO_base = IO_ul,
	IRAM1_base = EMPTY_AIO_ul,
	IME_base = IRAM1_ul,
};

// EMPTY MBC for GameBoy
class Simple_MBC {
	// when we have constexpr vector would this class be constexpr-able ?
	std::span<std::uint8_t> m_memory_view;

  public:
	constexpr Simple_MBC() = delete;
	template <typename Iter>
	constexpr Simple_MBC(const Iter begin, const Iter end) : m_memory_view(begin, end)
	{
		if(std::distance(begin, end) > 64_kB) {
			throw std::invalid_argument("Either program is too big or ram");
		}
	}
	[[nodiscard]] auto read(std::uint16_t addr) const noexcept -> std::uint8_t
	{
		return m_memory_view[addr];
	}
	auto write(std::uint16_t address, std::uint8_t value) noexcept -> void
	{
		if(address > IROM1_ul) m_memory_view[address] = value;
	}
};

/*
 *  MBC1 controller:
 *      There are two registre to control which rom bank is active:
 *              - ROM bank code
 *              - upper ROM bank code
 *      One registre to activate the ram: RAMCS gate data.
 *      Writing 0x0A in this register enable both reading and writing RAM.
 *      Otherwise the memory return undefined value on reading.
 *
 *      One regitre control both ROM and RAM banking -> Mode Ragistre encoded on 1bit
 *      Note writing 0b1 in this registre has a side effect on ROM0 access.
 *
 *      All rom related registre can be encoded on a 8 bits registre.
 *      m_bank_selector<8> : MODE
 *      m_bank_selector<7-6> : upper ROM bank code
 *      m_bank_selector<5-0> : ROM bank code
 *
 *      ROM addressing :
 *      ____________________________________________________________________
 *      | Accessed addr |            Bank nbr           |Adress within bank|
 *      | in            |                               |                  |
 *      --------------------------------------------------------------------
 *      |               |      <20-19>     |   <18-14>  |       <13-0>     |
 *      --------------------------------------------------------------------
 *      | ROM0 Mode=0b0 |        0b0       |     0b0    |      A<13-0>     |
 *      --------------------------------------------------------------------
 *      | ROM0 Mode=0b1 |  upper ROM Code  |     0b0    |      A<13-0>     |
 *      --------------------------------------------------------------------
 *      | ROM1          |  upper ROM Code  |  ROM code  |      A<13-0>     |
 *      --------------------------------------------------------------------
 *
 *      RAM addressing :
 *      _______________________________________________________
 *      | Accessed addr |      Bank nbr    |Adress within bank|
 *      -------------------------------------------------------
 *      |               |      <14-13>     |       <12-0>     |
 *      -------------------------------------------------------
 *      | ROM0 Mode=0b0 |        0b0       |      A<12-0>     |
 *      -------------------------------------------------------
 *      | ROM0 Mode=0b1 |  upper ROM Code  |      A<12-0>     |
 *      -------------------------------------------------------
 */

class MBC1 {
	std::uint8_t m_bank_selector = 0b1;
	bool m_ramg_enable = 0;
        // TODO move rom and ram size as template parameter ?
        size_t m_ram;
	std::span<std::uint8_t> m_memory;
	using MBC1_Partition = Partition<std::uint8_t, 64_kB, 8_kB>;
	MBC1_Partition m_memory_partition;

	std::random_device m_rd;
	mutable std::mt19937 m_gen;
	mutable std::uniform_int_distribution<std::uint8_t> m_distrib;

  public:
	// TODO cleanup, a bit messy
	template <typename Iter>
	explicit MBC1(const Iter begin, const Iter end, size_t rom, size_t ram)
	    : m_ram(ram), m_memory(begin, end),
	      m_memory_partition(m_memory.begin(), m_memory.end(),
	                         MBC1_Partition::descriptor{4, rom - 16_kB},
	                         MBC1_Partition::descriptor{6, ram - 8_kB}),
	      m_rd(), m_gen(m_rd()), m_distrib(0, 255)
	{
		if(rom> 2_MB or ram > 32_kB or
                   (rom<=512_kB and ram > 32_kB) or
                   (rom>=1_MB and rom <= 2_MB and ram>8_kB))
                {
			throw std::invalid_argument("Either program or ram is too big");
		}
		// after rom 8kB of Vram follow by swi ram
		const auto ram_offset = rom + 8_kB;
		std::generate(&m_memory[ram_offset], &m_memory[ram_offset + ram],
		              [this] { return this->m_distrib(this->m_gen); });
	}
	auto mode() const -> uint8_t { return m_bank_selector >> 7; }
	auto reg1() const -> uint8_t { return m_bank_selector & 0b1'1111; }
	auto reg2() const -> uint8_t { return (m_bank_selector & 0b0110'0000) >> 5; }
	auto reg1_2() const -> uint8_t { return m_bank_selector & 0b111'1111; }

	auto swap_bank_rom_high();
	auto swap_bank_rom_low();
	auto swap_bank_ram();
	auto ramg_enable(std::uint8_t value) noexcept -> void;
	auto bank_reg1(std::uint8_t value) noexcept -> void;
	// note can also be used for bank in ram :
	auto bank_reg2(std::uint8_t value) noexcept -> void;
	auto bank_mode(std::uint8_t value) noexcept -> void;
	[[nodiscard]] auto read(std::uint16_t addr) const noexcept -> std::uint8_t;
	auto write(std::uint16_t addr, std::uint8_t value) noexcept -> void;
};

#endif
