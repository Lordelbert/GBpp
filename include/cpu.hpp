#ifndef __REGISTER_HPP__
#define __REGISTER_HPP__
#include "Clock.hpp"
#include "ISA.hpp"
#include "bit_manipulation.hpp"
#include "include_std.hpp"
#include "memory.hpp"
#include <cstdint>
#include <utility>

class SM83 {
	enum IMMEDIATE : uint8_t {
		NO_IMM = 0,
		IMM8,
		IMM16,
	};

	const Clock_domain &m_clock;

	ISA::Register_bank m_regbank;

  public:
	SM83(Clock_domain &clock) : m_clock(clock){};

	auto dump(std::ostream &cout) -> void;
	[[nodiscard]] auto has_imm(std::uint8_t op) noexcept -> IMMEDIATE;
	[[nodiscard]] auto fetch(const Memory &) noexcept -> Clock_domain::Awaiter;
	[[nodiscard]] auto fetch_imm8(const Memory &) noexcept -> Clock_domain::Awaiter;
	[[nodiscard]] auto fetch_imm16(const Memory &) noexcept -> Clock_domain::Awaiter;

	auto execute(std::uint8_t, std::optional<std::uint16_t>, Memory &) noexcept
	    -> Clock_domain::Awaiter;
	auto extended_set(uint8_t opcode, Memory &memory) noexcept -> void;
	auto run(Memory &) -> Void_task;
};
#endif
