#ifndef _MEMORY__HPP__
#define _MEMORY__HPP__
#include "Clock.hpp"
#include "Coroutine.hpp"
#include "MBC.hpp"

#include "include_std.hpp"
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <random>
#include <variant>

struct Simple_MBC_tag {
};
struct MBC1_tag {
};

template <typename MBC> struct Tag_to_MBC_convert {
};

template <> struct Tag_to_MBC_convert<Simple_MBC_tag> {
	using type = Simple_MBC;
};

template <> struct Tag_to_MBC_convert<MBC1_tag> {
	using type = MBC1;
};

template <typename MBC>
using Tag_to_MBC_convert_t = typename Tag_to_MBC_convert<MBC>::type;

class Memory {
	std::variant<Simple_MBC, MBC1> m_policy_rw;

	inline static constexpr std::array<std::uint8_t, 47> Scrolling_Nintendo_Graphic{
	    0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00,
	    0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC,
	    0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E,
	    0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};

  public:
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
	Memory() = delete;

	template <typename Memory_Policy_tag>
	Memory(Memory_Policy_tag, std::initializer_list<std::uint8_t> &&rom, size_t ram)
	try : m_policy_rw(Tag_to_MBC_convert_t<Memory_Policy_tag>(rom.begin(), rom.end(),
	                                                          ram)) {
	}
	catch(...) {
	}

	constexpr auto read(std::uint16_t addr) const noexcept -> std::uint8_t
	{
		return std::visit([addr](const auto &visitor) { return visitor.read(addr); },
		                  m_policy_rw);
	}

	constexpr auto write(std::uint16_t addr, std::uint8_t value) noexcept -> void
	{
		std::visit([addr, value](auto &visitor) { return visitor.write(addr, value); },
		           m_policy_rw);
	}
	constexpr auto write_IME(std::uint8_t value) -> void
	{
		write(IME_base, value & 0x1F);
		return;
	}
	constexpr auto IME() const noexcept -> std::uint8_t { return read(IME_base) & 0x1F; }
	constexpr auto IE() const noexcept { return read(0xFF0F); }
};

#endif
