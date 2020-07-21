#ifndef __ISA_HPP__
#define __ISA_HPP__
#include "bit_manipulation.hpp"
#include "include_std.hpp"
#include "memory.hpp"
#include "trait.hpp"
#include <compare>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace ISA {
enum FLAG : uint8_t { NZ, Z, NC, C };
using Imm8_s = std::int8_t;
using Imm8 = std::uint8_t;
using Imm16_s = std::int16_t;
using Imm16 = std::uint16_t;
using Register8 = std::uint8_t;
using Register16 = std::uint16_t;
struct Flag_register {
  private:
	Register8 value;

  public:
	constexpr Flag_register(std::uint8_t val) noexcept : value(val){};
	constexpr Flag_register() noexcept = default;
	constexpr ~Flag_register() noexcept = default;
	constexpr auto operator=(std::uint8_t val) noexcept { value = val; }
	constexpr auto read() const noexcept { return value; }

	constexpr auto reset_flag() { value = 0; }

	constexpr auto zero() const noexcept -> std::uint8_t { return get_bit(value, 7); }
	constexpr auto carry() const noexcept -> std::uint8_t { return get_bit(value, 4); }
	constexpr auto substract() const noexcept -> std::uint8_t
	{
		return get_bit(value, 6);
	}
	constexpr auto half_carry() const noexcept -> std::uint8_t
	{
		return get_bit(value, 5);
	}
	constexpr auto clear_zero() noexcept -> void { clear_bit(value, 7); }
	constexpr auto clear_carry() noexcept -> void { clear_bit(value, 4); }
	constexpr auto clear_substract() noexcept -> void { clear_bit(value, 6); }
	constexpr auto clear_half_carry() noexcept -> void { clear_bit(value, 5); }
	constexpr auto set_zero() noexcept -> void { set_bit(value, 7); }
	constexpr auto set_carry() noexcept -> void { set_bit(value, 4); }
	constexpr auto set_substract() noexcept -> void { set_bit(value, 6); }
	constexpr auto set_half_carry() noexcept -> void { set_bit(value, 5); }
	constexpr auto set_carry(std::uint8_t C) noexcept -> void
	{
		value = set_bit(value, 4, C);
	}
};

struct Register_bank {
	ISA::Register8 A, B, C, D, E;
	ISA::Flag_register F;
	ISA::Register8 H, L;
	ISA::Register16 SP, PC = 0;
	bool interupt_enable;
};
struct Inc_HL {
	inline auto operator()(Register_bank &bank) -> void
	{
		std::tie(bank.H, bank.L) =
		    decompose(static_cast<std::uint16_t>(compose(bank.H, bank.L) + 1));
	}
};
struct Dec_HL {
	inline auto operator()(Register_bank &bank) -> void
	{
		std::tie(bank.H, bank.L) =
		    decompose(static_cast<std::uint16_t>(compose(bank.H, bank.L) - 1));
	}
};

/*************************** Arithmetic *********************************/
template <Unsigned T, Unsigned U = T>
constexpr auto ADD(T lhs, U rhs, Flag_register &F) noexcept -> std::uint8_t
{
	F.clear_substract();
	using gtype = std::conditional_t<sizeof(T) >= sizeof(U), T, U>;
	const Overflow_trait_t<gtype> val = static_cast<gtype>(lhs) + static_cast<gtype>(rhs);

	if(val > 0xFF) F.set_carry();
	if((val & 0xFF) == 0) F.set_zero();
	if(((lhs & 0b1111) + (rhs & 0b1111)) > 0b1111) F.set_half_carry();

	return val;
}

constexpr auto SUB(Register8 lhs, Register8 rhs, Flag_register &F) -> std::uint8_t
{
	F.set_substract();
	const auto val = lhs - rhs;
	if(lhs > rhs) F.set_carry();
	if(val == 0) F.set_zero();
	if((lhs & 0b1111) > (rhs & 0b1111)) F.set_half_carry();
	return val;
}

template <Unsigned T> constexpr auto ADC(T lhs, T rhs, Flag_register &F) noexcept -> T
{
	F.clear_substract();
	const auto val = lhs + rhs + F.carry();

	if(val > 0xFF) F.set_carry();
	if((val & 0xFF) == 0) F.set_zero();
	if(((lhs & 0b111) + (rhs & 0b111)) > 0b111) F.set_half_carry();

	return val;
}

constexpr auto SBC(Register8 lhs, Register8 rhs, Flag_register &F) noexcept
    -> std::uint8_t
{
	F.set_substract();
	const unsigned val = lhs - rhs - F.carry();
	if(lhs > rhs) F.set_carry();
	if(val == 0) F.set_zero();
	if((lhs & 0b1111) > (rhs & 0b1111)) F.set_half_carry();

	return val;
}

constexpr auto XOR(Register8 lhs, Register8 rhs, Flag_register &F) noexcept
    -> std::uint8_t
{
	F.clear_substract();
	F.clear_carry();
	F.clear_half_carry();
	const auto tmp = lhs ^ rhs;
	if(tmp == 0) F.set_zero();

	return tmp;
}

constexpr auto OR(Register8 lhs, Register8 rhs, Flag_register &F) noexcept -> std::uint8_t
{
	F.clear_substract();
	F.clear_carry();
	F.clear_half_carry();
	const auto tmp = lhs | rhs;
	if(tmp == 0) F.set_zero();

	return tmp;
}

constexpr auto AND(Register8 lhs, Register8 rhs, Flag_register &F) noexcept
    -> std::uint8_t
{
	F.clear_substract();
	F.clear_carry();
	F.set_half_carry();
	const auto tmp = lhs & rhs;
	if(tmp == 0) F.set_zero();

	return tmp;
}

constexpr auto CP(Register8 lhs, Register8 rhs, Flag_register &F) noexcept -> void
{
	if(lhs == rhs) F.set_zero();
	if(lhs < rhs) F.set_carry();
	if((lhs & 0b1111) < (rhs & 0b1111)) F.set_half_carry();
	F.set_substract();

	return;
}

template <Unsigned T> constexpr auto INC(T source, Flag_register &F) noexcept -> T
{
	F.clear_substract();
	if(((source & 0b1111) + (0b1)) > 0b1111) F.set_half_carry();

	const T tmp = source + 1;
	if(tmp == 0) F.set_zero();
	return tmp;
}
constexpr auto INC(Register16 source, Flag_register &F, const Memory &memory) noexcept
    -> Register8
{
	F.clear_substract();
	if(((source & 0b1111) + (0b1)) > 0b1111) F.set_half_carry();

	const Register8 tmp = (memory.read(source)) + 1;
	if(tmp == 0) F.set_zero();
	return tmp;
}

constexpr auto DEC(Register16 source, Flag_register &F, const Memory &memory) noexcept
    -> Register8
{
	F.clear_substract();
	if(((source & 0b1111) + (0b1)) > 0b1111) F.set_half_carry();

	const Register16 tmp = (memory.read(source)) - 1;
	if(tmp == 0) F.set_zero();
	return tmp;
}

template <Unsigned T> constexpr auto DEC(T source, Flag_register &F) noexcept -> T
{
	if(((source & 0b1111) - (0b1)) < 0) F.set_half_carry();

	const T tmp = source - 1;
	if(tmp == 0) F.set_zero();
	F.set_substract();
	return tmp;
}

constexpr auto SWAP(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const auto [hi, lo] = decompose(source);
	const auto tmp = ((lo << 4) | hi);
	if(tmp == 0) F.set_zero();

	return tmp;
}

constexpr auto CPL(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	F.set_substract();
	F.set_half_carry();
	return ~source;
}

constexpr auto CCF(Flag_register &F) noexcept -> std::uint8_t
{
	F.clear_substract();
	F.clear_half_carry();
	F.set_carry((~(F.carry())) & 0b1);
	return 1;
}
constexpr auto SCF(Flag_register &F) noexcept -> std::uint8_t
{
	F.clear_substract();
	F.clear_half_carry();
	F.set_carry();
	return 1;
}

constexpr auto RRC(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x1);
	const auto tmp = set_bit(static_cast<Register8>(source >> 1), 7, C);
	F.set_carry(C);
	if(tmp == 0) F.set_zero();
	return tmp;
}

constexpr auto RLC(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x80) >> 7;
	const auto tmp = set_bit(static_cast<Register8>(source << 1), 0, C);
	F.set_carry(C);
	if(tmp == 0) F.set_zero();
	return tmp;
}

constexpr auto RR(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x1);
	const auto tmp = set_bit(static_cast<Register8>(source >> 1), 7, F.carry());
	F.set_carry(C);
	if(source == static_cast<uint8_t>(0)) F.set_zero();
	return tmp;
}

constexpr auto RL(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x80) >> 7;
	const auto tmp = set_bit(static_cast<Register8>(source << 1), 0, F.carry());
	F.set_carry(C);
	if(tmp == 0) F.set_zero();
	return tmp;
}

constexpr auto SLA(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x80) >> 7;
	const std::uint8_t result = (source << 1);
	F.set_carry(C);

	F.clear_substract();
	F.clear_half_carry();
	if(result == 0) F.set_zero();
	return result;
}

constexpr auto SRL(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x1);
	const std::uint8_t result = (source >> 1);
	F.set_carry(C);

	F.clear_substract();
	F.clear_half_carry();
	if(result == 0) F.set_zero();
	return result;
}

constexpr auto SRA(Register8 source, Flag_register &F) noexcept -> std::uint8_t
{
	const std::uint8_t C = (source & 0x1);
	const std::uint8_t MSB = (source & 0x80) >> 7;
	const std::uint8_t result = set_bit(static_cast<Register8>(source >> 1), 7, MSB);
	F.set_carry(C);

	F.clear_substract();
	F.clear_half_carry();
	if(result == 0) F.set_zero();
	return result;
}

constexpr auto RES(size_t idx, Register8 source) noexcept -> std::uint8_t
{
	return source & (~(0x1 << idx));
}

constexpr auto SET(size_t idx, Register8 source) noexcept -> std::uint8_t
{
	return source | (0x1 << idx);
}

constexpr auto BIT(size_t idx, Register8 source, Flag_register &F) noexcept
    -> std::uint8_t
{
	// Flag zero
	F = F.read() | static_cast<Register8>(get_bit(source, idx) << 7);
	return 1;
}

constexpr auto double_dabble(Register8 A) noexcept -> std::pair<uint8_t, bool>
{
	uint8_t result{0};
	if(A > 99) return std::make_pair(result, true);
	for(size_t i = 0; i < 8; ++i) {
		uint8_t val = nibble_lo(result) + 3 * (nibble_lo(result) > 4);
		set_nibble_lo(result, val);
		val = nibble_hi(result) + 3 * (nibble_hi(result) > 4);
		set_nibble_hi(result, val);
		result <<= 1;
		result |= ((A & 0x80) >> 7);
		A <<= 1;
	}
	return std::make_pair(result, false);
}
constexpr auto DAA(Register8 A, Flag_register &F) -> Register8
{
	auto [result, C] = double_dabble(A);
	F.set_carry(C);
	return result & 0xFF;
}
/*************************** Memory *********************************/
constexpr auto LD(Register8 source) noexcept -> Register8 { return source; }
template <class HL_Policy>
auto LD(Register16 source, const Memory &memory, Register_bank &reg_bank) noexcept
    -> std::uint8_t
{
	HL_Policy{}(reg_bank);
	return memory.read(source);
}

template <class HL_Policy>
constexpr auto LD(Register16 source, Register_bank &reg_bank) noexcept
{
	HL_Policy{}(reg_bank);
	return source;
}
constexpr auto LD(Register16 source, const Memory &memory) noexcept -> std::uint8_t
{
	return memory.read(source);
}
constexpr auto LDH(Imm8 value, const Memory &memory) noexcept -> std::uint8_t
{
	return memory.read(compose(static_cast<uint8_t>(0xFF), value));
}

constexpr auto LD(Register16 source) noexcept -> std::uint16_t { return source; }
constexpr auto LD(Register16 source, Imm8 imm) noexcept
{
	return decompose(static_cast<Register16>(source + imm));
}
constexpr auto LD(Register16 source, Imm16 imm) noexcept
{
	return decompose(static_cast<Register16>(source + imm));
}

/*************************** Control Flow *********************************/
constexpr auto JP(Register16 &PC, Register16 destination) noexcept -> void
{
	PC = destination;
	return;
}

template <FLAG cc>
constexpr auto JP(Register16 &PC, Register16 destination, Flag_register F) noexcept
    -> void
{
	if constexpr(cc == Z)
		if(not F.zero()) return JP(PC, destination);
	if constexpr(cc == C)
		if(not F.carry()) return JP(PC, destination);
	if constexpr(cc == NZ)
		if(F.zero()) return JP(PC, destination);
	if constexpr(cc == NC)
		if(F.carry()) return JP(PC, destination);
}
constexpr auto JR(Register16 &PC, Imm8_s add) noexcept -> void
{
	PC += add;
	return;
}

template <FLAG cc>
constexpr auto JR(Register16 &PC, Imm8_s add, Flag_register F) noexcept -> void
{
	if constexpr(cc == Z)
		if(not F.zero()) return JR(PC, add);
	if constexpr(cc == C)
		if(not F.carry()) return JR(PC, add);
	if constexpr(cc == NZ)
		if(F.zero()) return JR(PC, add);
	if constexpr(cc == NC)
		if(F.carry()) return JR(PC, add);
}
constexpr auto _dec(Register16 &value) noexcept { return --value; }
constexpr auto _inc(Register16 &value) noexcept { return ++value; }

constexpr auto PUSH(Register16 &SP, Memory &memory, Register16 value) noexcept -> void
{
	const auto [hi, lo] = decompose(value);
	memory.write(_dec(SP), hi);
	memory.write(_dec(SP), lo);
	return;
}

constexpr auto POP(Register16 &SP, const Memory &memory) noexcept
    -> std::pair < std::uint8_t, std::uint8_t>
{
	const auto val = std::make_pair(memory.read(SP), memory.read(_inc(SP)));
	++SP;
	return val;
}
constexpr auto CALL(Register16 &PC, Register16 &SP, Imm16 addr, Memory &memory) noexcept
    -> void
{
	PUSH(SP, memory, PC);
	return JP(PC, addr);
}

template <FLAG cc>
auto CALL(Register16 &PC, Register16 &SP, Imm16 addr, Memory &memory,
          Flag_register F) noexcept -> void
{
	if constexpr(cc == NC)
		if(not F.zero()) CALL(PC, SP, addr, memory);
	if constexpr(cc == C)
		if(not F.carry()) CALL(PC, SP, addr, memory);
	if constexpr(cc == NZ)
		if(F.zero()) CALL(PC, SP, addr, memory);
	if constexpr(cc == NC)
		if(F.carry()) CALL(PC, SP, addr, memory);
	return;
}

constexpr auto RST(Register16 &PC, Register16 &SP, Imm8 addr, Memory &memory) noexcept
    -> void
{
	PUSH(SP, memory, PC);
	return JP(PC, addr);
}
constexpr auto RET(Register16 &PC, Register16 &SP, Memory &memory) noexcept -> void
{
	return JP(PC, compose(POP(SP, memory)));
}
template <FLAG cc>
auto RET(Register16 &PC, Register16 &SP, Memory &memory, Flag_register F) noexcept -> void
{
	if constexpr(cc == NC)
		if(not F.zero()) RET(PC, SP, memory);
	if constexpr(cc == C)
		if(not F.carry()) RET(PC, SP, memory);
	if constexpr(cc == NZ)
		if(F.zero()) RET(PC, SP, memory);
	if constexpr(cc == NC)
		if(F.carry()) RET(PC, SP, memory);
	return;
}

constexpr auto EI(Memory &memory) noexcept -> void
{
	memory.write_IME(0x1F);
	return;
}
constexpr auto DI(Memory &memory) noexcept -> void
{
	memory.write_IME(0b0);
	return;
}
constexpr auto RETI(Register16 &PC, Register16 &SP, Memory &memory) noexcept -> void
{
	JP(PC, compose(POP(SP, memory)));
	return EI(memory);
}

/*************************** Other *********************************/
constexpr auto NOOP() noexcept -> void { return; }
inline auto STOP() noexcept -> void
{
	std::cout << "Goodbye" << '\n';
	exit(0);
}
inline auto Unhandle_Instruction() noexcept -> void { exit(-1); }

} // namespace ISA
#endif
