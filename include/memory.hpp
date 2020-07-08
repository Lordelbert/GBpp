#ifndef _MEMORY__HPP__
#define _MEMORY__HPP__
#include "include_std.hpp"
#include <cstdint>
class Memory {
	std::array<uint8_t, 0xFFFF> m_memory;

  public:
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
		EIR_base = IRAM1_ul,
	};
	enum Reserved_Location : uint16_t {
		RST_00 = 0x0000,
		RST_08 = 0x0008,
		RST_10 = 0x0010,
		RST_18 = 0x0018,
		RST_20 = 0x0020,
		RST_28 = 0x0028,
		RST_30 = 0x0030,
		RST_38 = 0x0038,
		Vert_blank_it = 0x0040,
		LCDC_status_it = 0x0048,
		Timer_overflow_it = 0x0050,
		Serial_transfer_completion_it = 0x0058,
		HtoL_P10_P13_it = 0x0060,
	};

	constexpr auto operator[](uint16_t address) const noexcept -> uint8_t
	{
		return m_memory[address];
	}
	constexpr auto operator[](uint16_t address) noexcept -> uint8_t &
	{
		return m_memory[address];
	}

	constexpr auto ROM0() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[0], IRAM0_ul - IRAM0_base};
	}
	constexpr auto ROM1() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[IROM1_base], IROM1_ul - IROM1_base};
	}
	constexpr auto VRAM() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[IROM0_base], VRAM_ul - IROM0_base};
	}
	constexpr auto SWITCHABLE_RAM() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[SWI_RAM_base], SWI_RAM_ul - SWI_RAM_base};
	}
	constexpr auto IRAM0() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[IRAM0_base], IRAM0_ul - IRAM0_base};
	}
	constexpr auto IRAM0_ECHO() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[IRAM0_E_base], IRAM0_E_ul - IRAM0_E_base};
	}
	constexpr auto OAM() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[OAM_base], OAM_ul - OAM_base};
	}
	constexpr auto EMPTY_BEFORE_IO() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[EMPTY_BIO_base], EMPTY_BIO_ul - EMPTY_BIO_base};
	}
	constexpr auto IO() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[IO_base], IO_ul - IO_base};
	}
	constexpr auto EMPTY_AFTER_IO() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[EMPTY_AIO_base], EMPTY_AIO_ul - EMPTY_AIO_base};
	}
	constexpr auto IRAM1() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[IRAM1_base], IRAM1_ul - IRAM1_base};
	}
	constexpr auto INTERUPT_ENABLE_REGISTER() noexcept -> std::span<uint8_t>
	{
		return std::span{&m_memory[EIR_base], 1};
	}
	constexpr auto write_IRAM1(std::uint16_t addr, std::uint8_t value)
	{
		IRAM1()[addr - IRAM1_base] = value;
	}
	constexpr auto write_IRAM0(std::uint16_t addr, std::uint8_t value)
	{
		addr -= (addr > IRAM0_E_base) ? IRAM0_E_base : IRAM0_base;
		IRAM0()[addr] = value;
		IRAM0_ECHO()[addr] = value;
	}
	constexpr auto write_it(std::uint8_t value) { INTERUPT_ENABLE_REGISTER()[0] = value; }
};

#endif
