#include <ISA.hpp>
namespace ISA {
namespace {
constexpr auto _dec(Register16 &value) noexcept { return --value; }
constexpr auto _inc(Register16 &value) noexcept { return ++value; }
} // namespace
auto INC(Register16 source, Flag_register &F, const Memory &memory) noexcept
    -> task<Register8>
{
	F.clear_substract();
	if(((source & 0b1111) + (0b1)) > 0b1111) F.set_half_carry();

	const Register8 tmp = (co_await memory.read(source)) + 1;
	if(tmp == 0) F.set_zero();
	co_return tmp;
}

auto DEC(Register16 source, Flag_register &F, const Memory &memory) noexcept
    -> task<Register8>
{
	F.clear_substract();
	if(((source & 0b1111) + (0b1)) > 0b1111) F.set_half_carry();

	const Register16 tmp = (co_await memory.read(source)) - 1;
	if(tmp == 0) F.set_zero();
	co_return tmp;
}

auto PUSH(Register16 &SP, Memory &memory, Register16 value) noexcept -> task<void>
{
	const auto [hi, lo] = decompose(value);
	co_await memory.write(_dec(SP), hi);
	co_await memory.write(_dec(SP), lo);
	co_return;
}

auto POP(Register16 &SP, const Memory &memory) noexcept
    -> task<std::pair<std::uint8_t, std::uint8_t>>
{
	const auto val =
	    std::make_pair(co_await memory.read(SP), co_await memory.read(_inc(SP)));
	++SP;
	co_return val;
}
auto CALL(Register16 &PC, Register16 &SP, Imm16 addr, Memory &memory) noexcept
    -> task<void>
{
	co_await PUSH(SP, memory, PC);
	co_return JP(PC, addr);
}

auto RST(Register16 &PC, Register16 &SP, Imm8 addr, Memory &memory) noexcept -> task<void>
{
	co_await PUSH(SP, memory, PC);
	co_return JP(PC, addr);
}
auto RET(Register16 &PC, Register16 &SP, Memory &memory) noexcept -> task<void>
{
	co_return JP(PC, compose(co_await POP(SP, memory)));
}

auto EI(Memory &memory) noexcept -> task<void>
{
	co_await memory.write_IME(0x1F);
	co_return;
}
auto DI(Memory &memory) noexcept -> task<void>
{
	co_await memory.write_IME(0b0);
	co_return;
}
auto RETI(Register16 &PC, Register16 &SP, Memory &memory) noexcept -> task<void>
{
	JP(PC, compose(co_await POP(SP, memory)));
	co_return co_await EI(memory);
}
} // namespace ISA
