#ifndef __MBC_HPP__
#define __MBC_HPP__
#include "bit_manipulation.hpp"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <ranges>
#include <span>
#include <stdexcept>

// TODO Change this :thinking:
template <size_t N, typename T, template <class> typename C = std::vector>
constexpr auto window(C<T> &data, std::size_t offset) -> std::span<T, N>
{
	return std::span<T, N>{std::begin(data) + offset, N};
}

/*
 * Memory Controller :
 * Sources :
 *     https://gekkio.fi/files/gb-docs/gbctr.pdf
 *     https://ia601906.us.archive.org/19/items/GameBoyProgManVer1.1/GameBoyProgManVer1.1.pdf
 *
 * On GameBoy Memory Controller is used to control which of the N 32kB ROM bank
 * is currently in use (Note it also controls RAM bank of 8kB).
 *
 * The class MBC models the memory part while derived class MBCn (n in {1,2,3,5}) model
 * the policy for activating a bank.
 *
 * Member m_rom_bank and m_ram_bank represent the whole memory.
 * This memory is accessed using :
 * Membre m_IRAM is the active ram bank R/W.
 * Membre m_IROM0 is the bank0 i.e. 0x0 - 0x4000 adress space RO.
 * Membre m_IROM1 is the active rom bank 0x4000 - 0x8000 RO.
 */
class MBC {
  protected:
	std::vector<std::uint8_t> m_rom_bank; // array of rom banks
	std::vector<std::uint8_t> m_ram_bank; // array of ram banks
	std::random_device rd;

	static inline constexpr size_t g_rom_bank_size = 32_kB;
	static inline constexpr size_t g_ram_bank_size = 8_kB;

	mutable std::mt19937 m_gen;
	mutable std::uniform_int_distribution<std::uint8_t> m_distrib;

  public:
	MBC(std::vector<std::uint8_t> prog, size_t ram)
	    : m_rom_bank(prog), rd(), m_gen(rd()), m_distrib(0, 255)
	{
		if(prog.size() > 2_MB) {
			throw std::out_of_range("MBC1 is unable to manage more than 2MBytes");
		}
		// clang-format off
                m_ram_bank.reserve(ram);
		std::generate(std::begin(m_ram_bank), std::end(m_ram_bank),
                                    [this](){ return m_distrib(m_gen); });
		// clang-format on
		return;
	}
	virtual auto read(std::uint16_t) const noexcept -> std::uint8_t = 0;
	virtual auto write(std::uint16_t, std::uint8_t) -> void = 0;
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
class MBC1 final : public MBC {
	std::uint8_t m_bank_selector = 0b1;
	bool m_ramg_enable = 0;

  public:
	MBC1(std::vector<std::uint8_t> prog, size_t ram) : MBC(prog, ram) {}
	virtual ~MBC1() = default;

	[[nodiscard]] auto mode() const noexcept -> std::uint8_t
	{
		return m_bank_selector >> 7;
	}
	[[nodiscard]] auto bank1() const noexcept -> std::uint8_t
	{
		return m_bank_selector & 0xF;
	}
	[[nodiscard]] auto bank2() const noexcept -> std::uint8_t
	{
		return (m_bank_selector >> 5) & 0b11;
	}

	auto ramg_enable(std::uint8_t value) noexcept -> void
	{
		m_ramg_enable = ((value & 0x0F) == 0x0A) ? true : false;
	}
	auto bank_reg1(std::uint8_t value) noexcept -> void
	{
		const auto _val = (value & 0b1111);
		m_bank_selector = (_val == 0) ? 0b1 : _val;
	}
	// note can also be used for bank in ram :
	auto bank_reg2(std::uint8_t value) noexcept -> void
	{
		m_bank_selector = ((value & 0b11) << 5) | m_bank_selector;
	}
	auto bank_mode(std::uint8_t value) noexcept -> void
	{
		m_bank_selector =
		    set_bit(m_bank_selector, 7, static_cast<std::uint8_t>(value & 0b1));
	}

	// clang-format off
	[[maybe_unused]]
        [[nodiscard]] auto select_bank(std::uint16_t addr) const noexcept -> std::uint32_t;
	// clang-format on

	[[nodiscard]] auto read(std::uint16_t address) const noexcept -> std::uint8_t;
	[[nodiscard]] auto read_ram(std::uint16_t addr) const noexcept -> std::uint8_t;
	auto write(std::uint16_t address, std::uint8_t value) -> void;
};

#endif
