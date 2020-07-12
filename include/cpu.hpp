#ifndef __REGISTER_HPP__
#define __REGISTER_HPP__
#include "Clock.hpp"
#include "Coroutine.hpp"
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
	[[nodiscard]] auto fetch(const Memory &memory) -> sink<uint8_t>;
	[[nodiscard]] auto fetch_ovelap(const Memory &memory) noexcept -> uint8_t;
	[[nodiscard]] auto fetch_imm8(const Memory &memory) -> sink<uint8_t>;
	[[nodiscard]] auto fetch_imm16(const Memory &memory) -> sink<uint16_t>;
	auto interrupt_handler(Memory &memory) -> sink<void>;

	auto execute(std::uint8_t, Memory &) noexcept -> sink<void>;
	auto extended_set(uint8_t opcode, Memory &memory) noexcept -> void;
	auto run(Memory &) -> Dummy_coro;
};
#endif
