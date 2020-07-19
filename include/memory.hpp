#ifndef _MEMORY__HPP__
#define _MEMORY__HPP__
#include "Clock.hpp"
#include "Coroutine.hpp"
#include "MBC.hpp"
#include "include_std.hpp"
#include <cstdint>
#include <memory>
#include <initializer_list>
#include <iterator>

class Memory {
	std::array<std::uint8_t, 0xFFFF> m_memory;
	std::unique_ptr<MBC> m_game;
	const Clock_domain &m_clock;

	inline static constexpr std::array<std::uint8_t, 47> Scrolling_Nintendo_Graphic{
	    0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00,
	    0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC,
	    0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E,
	    0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

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
		IME_base = IRAM1_ul,
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

	Memory(const Clock_domain &clock, std::vector<std::uint8_t> rom) : m_clock{clock}
	{
		m_game = std::make_unique<MBC1>(rom, 32_kB);
		// Nintendo graphics
		std::move(std::begin(Scrolling_Nintendo_Graphic),
		          std::end(Scrolling_Nintendo_Graphic), &m_memory[0x0104]);
		// name of the game if the name < static arry => 0
		std::fill(&m_memory[0x0134], &m_memory[0x142], 0x0);
		// color GB or not ?
		m_memory[0x143] = 0x80;
		// Has super Fonction
		m_memory[0x146] = 0x00;
		// Destination code Japanese (0) or not (1)
		m_memory[0x14A] = 0;
		// Licence code
		// 33 -> ck 0144/0145 for Licensee code. (Super GB won't work)
		// 79 -> Accolade
		// A4 -> Konami
		m_memory[0x14B] = 33;
	}

	auto read(uint16_t addr) const noexcept -> task<std::uint8_t>;
	// use when fetch overlap execute
	auto read_nowait(uint16_t addr) const noexcept -> std::uint8_t;
	auto write(uint16_t addr, std::uint8_t value) noexcept -> task<void>;
	auto write_IME(std::uint8_t value) -> task<void>;

	constexpr auto VRAM() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[IROM0_base], VRAM_ul - IROM0_base};
	}
	constexpr auto SWITCHABLE_RAM() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[SWI_RAM_base], SWI_RAM_ul - SWI_RAM_base};
	}
	constexpr auto IRAM0() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[IRAM0_base], IRAM0_ul - IRAM0_base};
	}
	constexpr auto IRAM0_ECHO() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[IRAM0_E_base], IRAM0_E_ul - IRAM0_E_base};
	}
	constexpr auto OAM() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[OAM_base], OAM_ul - OAM_base};
	}
	constexpr auto EMPTY_BEFORE_IO() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[EMPTY_BIO_base], EMPTY_BIO_ul - EMPTY_BIO_base};
	}
	constexpr auto IO() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[IO_base], IO_ul - IO_base};
	}
	constexpr auto EMPTY_AFTER_IO() noexcept -> std::span<std::uint8_t>
	{
		return std::span{&m_memory[EMPTY_AIO_base], EMPTY_AIO_ul - EMPTY_AIO_base};
	}
	constexpr auto IME() const noexcept -> std::uint8_t
	{
		return m_memory[IME_base] & 0x1F;
	}
	constexpr auto IE() const noexcept { return m_memory[0xFF0F]; }
};

#endif
