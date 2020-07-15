#include "cpu.hpp"
#include "Clock.hpp"
#include "Coroutine.hpp"
#include "ISA.hpp"
#include "bit_manipulation.hpp"
#include "memory.hpp"
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iterator>
#include <optional>
#include <utility>
using namespace ISA;
using namespace std;

auto SM83::dump(ostream &cout) -> void
{
	cout << "Register A:" << static_cast<int>(m_regbank.A) << '\n';
	cout << "Register B:" << static_cast<int>(m_regbank.B) << '\n';
	cout << "Register C:" << static_cast<int>(m_regbank.C) << '\n';
	cout << "Register D:" << static_cast<int>(m_regbank.D) << '\n';
	cout << "Register E:" << static_cast<int>(m_regbank.E) << '\n';
	cout << "Register F:" << static_cast<int>(m_regbank.F.read()) << '\n';
	cout << "Register PC:" << static_cast<int>(m_regbank.PC) << '\n';
	cout << "Register SP:" << static_cast<int>(m_regbank.SP) << '\n';
}

[[nodiscard]] auto SM83::fetch(const Memory &memory) -> task<uint8_t>
{
	co_return co_await memory.read(m_regbank.PC++);
}
[[nodiscard]] auto SM83::fetch_ovelap(const Memory &memory) noexcept -> uint8_t
{
	return memory.read_nowait(m_regbank.PC++);
}

[[nodiscard]] auto SM83::fetch_imm8(const Memory &memory) -> task<uint8_t>
{
	co_return co_await memory.read(m_regbank.PC++);
}

[[nodiscard]] auto SM83::fetch_imm16(const Memory &memory) -> task<uint16_t>
{
	const uint8_t lo = co_await memory.read(m_regbank.PC++);
	const uint16_t hi = co_await memory.read(m_regbank.PC++);
	co_return(hi << 4) | lo;
}
auto SM83::interrupt_handler(Memory &memory) -> task<void>
{
	// disable it
	memory.write_IME(0x0);
	co_await PUSH(m_regbank.SP, memory, m_regbank.PC);
	co_await make_awaiter(m_clock, 1);
	JP(m_regbank.PC, ((memory.IE() & 5) * 0x8) + 0x40);
	co_await make_awaiter(m_clock, 1);
	co_return;
}

auto SM83::run(Memory &memory) -> Dummy_coro
{
	std::uint8_t opcode = co_await fetch(memory);
	while(1) {
		co_await execute(opcode, memory);
		// fetch overlap interrup check
		// fetch overlap execute
		opcode = fetch_ovelap(memory);
		if((memory.IME() & memory.IE())) {
			co_await interrupt_handler(memory);
			opcode = co_await fetch(memory);
		}
	}
}

auto SM83::has_imm(uint8_t opcode) noexcept -> IMMEDIATE
{
	// d16 : 01 11 21 31
	// d8  : 06 16 26 36 C6 D6 E6 F6
	//       0E 1E 2E 3E CE DE EE FE
	// a8  : E0 F0
	// a16 : C2 C3 C4 CA CC CD
	//       D2 D4 DA DC
	//       08
	//       EA
	//       FA
	IMMEDIATE predicate = NO_IMM;
	switch(opcode >> 4) {
	case 0x0:
		if((opcode & 0b1111) == 0x8) {
			predicate = IMM16;
			break;
		}
	case 0x1:
	case 0x2:
	case 0x3:
		switch(opcode & 0b1111) {
		case 0x1:
			predicate = IMM16;
			break;
		case 0x6:
		case 0xE:
			predicate = IMM8;
			break;
		default:
			break;
		}
		break;
	case 0xC:
		switch(opcode & 0b1111) {
		case 0x1:
		case 0x3:
		case 0x4:
		case 0x6:
		case 0xA:
		case 0xC:
		case 0xD:
		case 0xE:
			predicate = IMM16;
			break;
		default:
			break;
		}
		break;
	case 0xD:
		switch(opcode & 0b1111) {
		case 0x2:
		case 0x4:
		case 0xA:
		case 0xC:
			predicate = IMM16;
			break;
		case 0x6:
		case 0xE:
			predicate = IMM8;
			break;
		default:
			break;
		}

		break;
	case 0xE:
		switch(opcode & 0b1111) {
		case 0x6:
		case 0xE:
			predicate = IMM8;
			break;
		case 0xA:
			predicate = IMM16;
			break;
		default:
			break;
		}
		break;
	case 0xF:
		switch(opcode & 0b1111) {
		case 0xA:
			predicate = IMM16;
			break;
		case 0x6:
		case 0xE:
			predicate = IMM8;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return predicate;
}
namespace {
constexpr auto nbr_to_reg(std::uint8_t nibble_lo, Register_bank &regbank,
                          Memory &memory) noexcept -> std::uint8_t *
{
	switch(nibble_lo % 7) {
	case 0x0:
		return &regbank.B;
	case 0x1:
		return &regbank.C;
	case 0x2:
		return &regbank.D;
	case 0x3:
		return &regbank.E;
	case 0x4:
		return &regbank.H;
	case 0x5:
		return &regbank.L;
		//	case 0x6:
		//		return &memory[compose(regbank.H, regbank.L)];
	case 0x7:
		return &regbank.A;
	}
	return nullptr;
}
} // namespace
auto SM83::extended_set(uint8_t opcode, Memory &memory) noexcept -> void
{
	const auto [hi, lo] = decompose(opcode);
	auto *lhs = nbr_to_reg(lo, m_regbank, memory);
	switch(hi) {
	case 0x0:
		*lhs = (lo < 8) ? RLC(*lhs, m_regbank.F) : RRC(*lhs, m_regbank.F);
		break;
	case 0x1:
		*lhs = (lo < 8) ? RL(*lhs, m_regbank.F) : RR(*lhs, m_regbank.F);
		break;
	case 0x2:
		*lhs = (lo < 8) ? SLA(*lhs, m_regbank.F) : SRA(*lhs, m_regbank.F);
		break;
	case 0x3:
		*lhs = (lo < 8) ? SWAP(*lhs, m_regbank.F) : SWAP(*lhs, m_regbank.F);
		break;
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x8:
		*lhs = BIT(lo & 7, *lhs, m_regbank.F);
		break;
	case 0x9:
	case 0xA:
	case 0xB:
		*lhs = RES(lo & 7, *lhs);
		break;
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		*lhs = SET(lo & 7, *lhs);
		break;
	}
}

auto SM83::execute(uint8_t opcode, Memory &memory) noexcept -> task<void>
{
	switch(opcode) {
	case 0x00: {
		NOOP();
		break;
	}
	case 0x01: {
		tie(m_regbank.B, m_regbank.C) = decompose(LD(co_await fetch_imm16(memory)));
		break;
	}
	case 0x02: {
		co_await memory.write(compose(m_regbank.B, m_regbank.C), LD(m_regbank.A));
		break;
	}
	case 0x03: {
		const Register16 BC = compose(m_regbank.B, m_regbank.C);
		tie(m_regbank.B, m_regbank.C) = decompose(INC(BC, m_regbank.F));
		break;
	}
	case 0x04: {
		m_regbank.B = INC(m_regbank.B, m_regbank.F);
		break;
	}
	case 0x05: {
		m_regbank.B = DEC(m_regbank.B, m_regbank.F);
		break;
	}
	case 0x06: {
		m_regbank.B = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x07: {
		m_regbank.A = RLC(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x08: {
		const Register16 imm16 = co_await fetch_imm16(memory);
		const auto [hi, lo] = decompose(LD(m_regbank.SP));
		co_await memory.write(imm16, hi);
		co_await memory.write(imm16 + 1, lo);
		break;
	}
	case 0x09: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		const Register16 BC = compose(m_regbank.B, m_regbank.C);
		tie(m_regbank.H, m_regbank.L) = decompose(ADD(HL, BC, m_regbank.F));
		break;
	}
	case 0x0a: {
		m_regbank.A = co_await LD(compose(m_regbank.B, m_regbank.C), memory);
		break;
	}
	case 0x0b: {
		const Register16 BC = compose(m_regbank.B, m_regbank.C);
		tie(m_regbank.B, m_regbank.C) = decompose(DEC(BC, m_regbank.F));
		break;
	}
	case 0x0c: {
		m_regbank.C = INC(m_regbank.C, m_regbank.F);
		break;
	}
	case 0x0d: {
		m_regbank.C = DEC(m_regbank.C, m_regbank.F);
		break;
	}
	case 0x0e: {
		m_regbank.C = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x0f: {
		m_regbank.A = RRC(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x10: {
		STOP();
		break;
	}
	case 0x11: {
		tie(m_regbank.D, m_regbank.E) = decompose(LD(co_await fetch_imm16(memory)));
		break;
	}
	case 0x12: {
		co_await memory.write(compose(m_regbank.D, m_regbank.E), LD(m_regbank.A));
		break;
	}
	case 0x13: {
		const Register16 DE = compose(m_regbank.D, m_regbank.E);
		tie(m_regbank.D, m_regbank.E) = decompose(INC(DE, m_regbank.F));
		break;
	}
	case 0x14: {
		m_regbank.D = INC(m_regbank.D, m_regbank.F);
		break;
	}
	case 0x15: {
		m_regbank.D = DEC(m_regbank.D, m_regbank.F);
		break;
	}
	case 0x16: {
		m_regbank.D = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x17: {
		m_regbank.A = RL(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x18: {
		JR(m_regbank.PC, co_await fetch_imm8(memory));
		break;
	}
	case 0x19: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		const Register16 DE = compose(m_regbank.D, m_regbank.E);
		tie(m_regbank.H, m_regbank.L) = decompose(ADD(HL, DE, m_regbank.F));
		break;
	}
	case 0x1a: {
		m_regbank.A = co_await LD(compose(m_regbank.D, m_regbank.E), memory);
		break;
	}
	case 0x1b: {
		const Register16 DE = compose(m_regbank.D, m_regbank.E);
		tie(m_regbank.D, m_regbank.E) = decompose(DEC(DE, m_regbank.F));
		break;
	}
	case 0x1c: {
		m_regbank.E = INC(m_regbank.E, m_regbank.F);
		break;
	}
	case 0x1d: {
		m_regbank.E = DEC(m_regbank.E, m_regbank.F);
		break;
	}
	case 0x1e: {
		m_regbank.E = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x1f: {
		m_regbank.A = RR(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x20: {
		JR<NZ>(m_regbank.PC, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0x21: {
		tie(m_regbank.H, m_regbank.L) = decompose(LD(co_await fetch_imm16(memory)));
		break;
	}
	case 0x22: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		co_await memory.write(HL, LD<Inc_HL>(m_regbank.A, m_regbank));
		break;
	}
	case 0x23: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		tie(m_regbank.H, m_regbank.L) = decompose(INC(HL, m_regbank.F));
		break;
	}
	case 0x24: {
		m_regbank.H = INC(m_regbank.H, m_regbank.F);
		break;
	}
	case 0x25: {
		m_regbank.H = DEC(m_regbank.H, m_regbank.F);
		break;
	}
	case 0x26: {
		m_regbank.H = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x27: {
		m_regbank.A = DAA(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x28: {
		JR<Z>(m_regbank.PC, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0x29: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		tie(m_regbank.H, m_regbank.L) = decompose(ADD(HL, HL, m_regbank.F));
		break;
	}
	case 0x2a: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		m_regbank.A = LD<Inc_HL>(HL, m_regbank);
		break;
	}
	case 0x2b: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		tie(m_regbank.H, m_regbank.L) = decompose(DEC(HL, m_regbank.F));
		break;
	}
	case 0x2c: {
		m_regbank.L = INC(m_regbank.L, m_regbank.F);
		break;
	}
	case 0x2d: {
		m_regbank.L = DEC(m_regbank.L, m_regbank.F);
		break;
	}
	case 0x2e: {
		m_regbank.L = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x2f: {
		m_regbank.A = CPL(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x30: {
		JR<NC>(m_regbank.PC, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0x31: {
		m_regbank.SP = LD(co_await fetch_imm16(memory));
		break;
	}
	case 0x32: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		co_await memory.write(HL, LD<Dec_HL>(m_regbank.A, m_regbank));
		break;
	}
	case 0x33: {
		m_regbank.SP = INC(m_regbank.SP, m_regbank.F);
		break;
	}
	case 0x34: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		tie(m_regbank.H, m_regbank.L) = decompose(co_await INC(HL, m_regbank.F, memory));
		break;
	}
	case 0x35: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		tie(m_regbank.H, m_regbank.L) = decompose(co_await DEC(HL, m_regbank.F, memory));
		break;
	}
	case 0x36: {
		tie(m_regbank.H, m_regbank.L) =
		    decompose<uint16_t>(LD(co_await fetch_imm8(memory)));
		break;
	}
	case 0x37: {
		SCF(m_regbank.F);
		break;
	}
	case 0x38: {
		JR<C>(m_regbank.PC, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0x39: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		tie(m_regbank.H, m_regbank.L) = decompose(ADD(HL, m_regbank.SP, m_regbank.F));
		break;
	}
	case 0x3a: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		co_await memory.write(HL, LD<Inc_HL>(m_regbank.A, m_regbank));
		break;
	}
	case 0x3b: {
		m_regbank.SP = DEC(m_regbank.SP, m_regbank.F);
		break;
	}
	case 0x3c: {
		m_regbank.A = INC(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x3d: {
		m_regbank.A = DEC(m_regbank.A, m_regbank.F);
		break;
	}
	case 0x3e: {
		m_regbank.A = LD(co_await fetch_imm8(memory));
		break;
	}
	case 0x3f: {
		CCF(m_regbank.F);
		break;
	}
	case 0x40: {
		m_regbank.B = LD(m_regbank.B);
		break;
	}
	case 0x41: {
		m_regbank.B = LD(m_regbank.C);
		break;
	}
	case 0x42: {
		m_regbank.B = LD(m_regbank.D);
		break;
	}
	case 0x43: {
		m_regbank.B = LD(m_regbank.E);
		break;
	}
	case 0x44: {
		m_regbank.B = LD(m_regbank.H);
		break;
	}
	case 0x45: {
		m_regbank.B = LD(m_regbank.L);
		break;
	}
	case 0x46: {
		m_regbank.B = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x47: {
		m_regbank.B = LD(m_regbank.A);
		break;
	}
	case 0x48: {
		m_regbank.C = LD(m_regbank.B);
		break;
	}
	case 0x49: {
		m_regbank.C = LD(m_regbank.C);
		break;
	}
	case 0x4a: {
		m_regbank.C = LD(m_regbank.D);
		break;
	}
	case 0x4b: {
		m_regbank.C = LD(m_regbank.E);
		break;
	}
	case 0x4c: {
		m_regbank.C = LD(m_regbank.H);
		break;
	}
	case 0x4d: {
		m_regbank.C = LD(m_regbank.L);
		break;
	}
	case 0x4e: {
		m_regbank.C = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x4f: {
		m_regbank.C = LD(m_regbank.A);
		break;
	}
	case 0x50: {
		m_regbank.D = LD(m_regbank.B);
		break;
	}
	case 0x51: {
		m_regbank.D = LD(m_regbank.C);
		break;
	}
	case 0x52: {
		m_regbank.D = LD(m_regbank.D);
		break;
	}
	case 0x53: {
		m_regbank.D = LD(m_regbank.E);
		break;
	}
	case 0x54: {
		m_regbank.D = LD(m_regbank.H);
		break;
	}
	case 0x55: {
		m_regbank.D = LD(m_regbank.L);
		break;
	}
	case 0x56: {
		m_regbank.D = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x57: {
		m_regbank.D = LD(m_regbank.A);
		break;
	}
	case 0x58: {
		m_regbank.E = LD(m_regbank.B);
		break;
	}
	case 0x59: {
		m_regbank.E = LD(m_regbank.C);
		break;
	}
	case 0x5a: {
		m_regbank.E = LD(m_regbank.D);
		break;
	}
	case 0x5b: {
		m_regbank.E = LD(m_regbank.E);
		break;
	}
	case 0x5c: {
		m_regbank.E = LD(m_regbank.H);
		break;
	}
	case 0x5d: {
		m_regbank.E = LD(m_regbank.L);
		break;
	}
	case 0x5e: {
		m_regbank.E = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x5f: {
		m_regbank.E = LD(m_regbank.A);
		break;
	}
	case 0x60: {
		m_regbank.H = LD(m_regbank.B);
		break;
	}
	case 0x61: {
		m_regbank.H = LD(m_regbank.C);
		break;
	}
	case 0x62: {
		m_regbank.H = LD(m_regbank.D);
		break;
	}
	case 0x63: {
		m_regbank.H = LD(m_regbank.E);
		break;
	}
	case 0x64: {
		m_regbank.H = LD(m_regbank.H);
		break;
	}
	case 0x65: {
		m_regbank.H = LD(m_regbank.L);
		break;
	}
	case 0x66: {
		m_regbank.H = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x67: {
		m_regbank.H = LD(m_regbank.A);
		break;
	}
	case 0x68: {
		m_regbank.L = LD(m_regbank.B);
		break;
	}
	case 0x69: {
		m_regbank.L = LD(m_regbank.C);
		break;
	}
	case 0x6a: {
		m_regbank.L = LD(m_regbank.D);
		break;
	}
	case 0x6b: {
		m_regbank.L = LD(m_regbank.E);
		break;
	}
	case 0x6c: {
		m_regbank.L = LD(m_regbank.H);
		break;
	}
	case 0x6d: {
		m_regbank.L = LD(m_regbank.L);
		break;
	}
	case 0x6e: {
		m_regbank.L = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x6f: {
		m_regbank.L = LD(m_regbank.A);
		break;
	}
	case 0x70: {
		co_await memory.write(compose(m_regbank.H, m_regbank.L), LD(m_regbank.B));
		break;
	}
	case 0x71: {
		co_await memory.write(compose(m_regbank.H, m_regbank.L), LD(m_regbank.C));
		break;
	}
	case 0x72: {
		co_await memory.write(compose(m_regbank.H, m_regbank.L), LD(m_regbank.D));
		break;
	}
	case 0x73: {
		co_await memory.write(compose(m_regbank.H, m_regbank.L), LD(m_regbank.E));
		break;
	}
	case 0x74: {
		co_await memory.write(compose(m_regbank.H, m_regbank.L), LD(m_regbank.H));
		break;
	}
	case 0x75: {
		co_await memory.write(compose(m_regbank.H, m_regbank.L), LD(m_regbank.L));
		break;
	}
	case 0x76: {
		const Register16 HL = compose(m_regbank.H, m_regbank.L);
		co_await memory.write(HL, co_await LD(HL, memory));
		break;
	}
	case 0x77: {
		m_regbank.B = LD(m_regbank.A);
		break;
	}
	case 0x78: {
		m_regbank.A = LD(m_regbank.B);
		break;
	}
	case 0x79: {
		m_regbank.A = LD(m_regbank.C);
		break;
	}
	case 0x7a: {
		m_regbank.A = LD(m_regbank.D);
		break;
	}
	case 0x7b: {
		m_regbank.A = LD(m_regbank.E);
		break;
	}
	case 0x7c: {
		m_regbank.A = LD(m_regbank.H);
		break;
	}
	case 0x7d: {
		m_regbank.A = LD(m_regbank.L);
		break;
	}
	case 0x7e: {
		m_regbank.A = co_await LD(compose(m_regbank.H, m_regbank.L), memory);
		break;
	}
	case 0x7f: {
		m_regbank.A = LD(m_regbank.A);
		break;
	}
	case 0x80: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0x81: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0x82: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0x83: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0x84: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0x85: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0x86: {
		m_regbank.A =
		    ADD(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		        m_regbank.F);
		break;
	}
	case 0x87: {
		m_regbank.A = ADD(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0x88: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0x89: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0x8a: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0x8b: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0x8c: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0x8d: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0x8e: {
		m_regbank.A =
		    ADC(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		        m_regbank.F);
		break;
	}
	case 0x8f: {
		m_regbank.A = ADC(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0x90: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0x91: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0x92: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0x93: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0x94: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0x95: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0x96: {
		m_regbank.A =
		    SUB(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		        m_regbank.F);
		break;
	}
	case 0x97: {
		m_regbank.A = SUB(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0x98: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0x99: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0x9a: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0x9b: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0x9c: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0x9d: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0x9e: {
		m_regbank.A =
		    SBC(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		        m_regbank.F);
		break;
	}
	case 0x9f: {
		m_regbank.A = SBC(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0xa0: {
		m_regbank.A = AND(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0xa1: {
		m_regbank.A = AND(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0xa2: {
		m_regbank.A = AND(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0xa3: {
		m_regbank.A = AND(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0xa4: {
		m_regbank.A = AND(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0xa5: {
		m_regbank.A = AND(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0xa6: {
		m_regbank.A =
		    AND(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		        m_regbank.F);
		break;
	}
	case 0xa7: {
		m_regbank.A = AND(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0xa8: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0xa9: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0xaa: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0xab: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0xac: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0xad: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0xae: {
		m_regbank.A =
		    XOR(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		        m_regbank.F);
		break;
	}
	case 0xaf: {
		m_regbank.A = XOR(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0xb0: {
		m_regbank.A = OR(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0xb1: {
		m_regbank.A = OR(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0xb2: {
		m_regbank.A = OR(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0xb3: {
		m_regbank.A = OR(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0xb4: {
		m_regbank.A = OR(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0xb5: {
		m_regbank.A = OR(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0xb6: {
		m_regbank.A =
		    OR(m_regbank.A, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		       m_regbank.F);
		break;
	}
	case 0xb7: {
		m_regbank.A = OR(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0xb8: {
		CP(m_regbank.A, m_regbank.B, m_regbank.F);
		break;
	}
	case 0xb9: {
		CP(m_regbank.A, m_regbank.C, m_regbank.F);
		break;
	}
	case 0xba: {
		CP(m_regbank.A, m_regbank.D, m_regbank.F);
		break;
	}
	case 0xbb: {
		CP(m_regbank.A, m_regbank.E, m_regbank.F);
		break;
	}
	case 0xbc: {
		CP(m_regbank.A, m_regbank.H, m_regbank.F);
		break;
	}
	case 0xbd: {
		CP(m_regbank.A, m_regbank.L, m_regbank.F);
		break;
	}
	case 0xbe: {
		CP(m_regbank.A, compose(m_regbank.H, m_regbank.L), m_regbank.F);
		break;
	}
	case 0xbf: {
		CP(m_regbank.A, m_regbank.A, m_regbank.F);
		break;
	}
	case 0xc0: {
		co_await RET<NZ>(m_regbank.PC, m_regbank.SP, memory, m_regbank.F);
		break;
	}
	case 0xc1: {
		tie(m_regbank.B, m_regbank.C) = co_await POP(m_regbank.SP, memory);
		break;
	}
	case 0xc2: {
		JP<NZ>(m_regbank.PC, co_await fetch_imm16(memory), m_regbank.F);
		break;
	}
	case 0xc3: {
		JP(m_regbank.PC, co_await fetch_imm16(memory));
		break;
	}
	case 0xc4: {
		co_await CALL<NZ>(m_regbank.PC, m_regbank.SP, co_await fetch_imm16(memory),
		                  memory, m_regbank.F);
		break;
	}
	case 0xc5: {
		co_await PUSH(m_regbank.SP, memory, compose(m_regbank.B, m_regbank.C));
		break;
	}
	case 0xc6: {
		m_regbank.A = ADD(m_regbank.A, co_await fetch_imm16(memory), m_regbank.F);
		break;
	}
	case 0xc7: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x00, memory);
		break;
	}
	case 0xc8: {
		co_await RET<Z>(m_regbank.PC, m_regbank.SP, memory, m_regbank.F);
		break;
	}
	case 0xc9: {
		co_await RET(m_regbank.PC, m_regbank.SP, memory);
		break;
	}
	case 0xca: {
		JP<Z>(m_regbank.PC, co_await fetch_imm16(memory), m_regbank.F);
		break;
	}
	case 0xcb: {
		extended_set(opcode, memory);
		break;
	}
	case 0xcc: {
		co_await CALL<Z>(m_regbank.PC, m_regbank.SP, co_await fetch_imm16(memory), memory,
		                 m_regbank.F);
		break;
	}
	case 0xcd: {
		co_await CALL(m_regbank.PC, m_regbank.SP, co_await fetch_imm16(memory), memory);
		break;
	}
	case 0xce: {
		m_regbank.A = ADC(m_regbank.A, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xcf: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x08, memory);
		break;
	}
	case 0xd0: {
		co_await RET<NC>(m_regbank.PC, m_regbank.SP, memory, m_regbank.F);
		break;
	}
	case 0xd1: {
		tie(m_regbank.D, m_regbank.E) = co_await POP(m_regbank.SP, memory);
		break;
	}
	case 0xd2: {
		JP<NC>(m_regbank.PC, co_await fetch_imm16(memory), m_regbank.F);
		break;
	}
	case 0xd3: {
		Unhandle_Instruction();
		break;
	}
	case 0xd4: {
		co_await CALL<NC>(m_regbank.PC, m_regbank.SP, co_await fetch_imm16(memory),
		                  memory, m_regbank.F);
		break;
	}
	case 0xd5: {
		co_await PUSH(m_regbank.SP, memory, compose(m_regbank.D, m_regbank.E));
		break;
	}
	case 0xd6: {
		m_regbank.A = SUB(m_regbank.A, co_await fetch_imm16(memory), m_regbank.F);
		break;
	}
	case 0xd7: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x10, memory);
		break;
	}
	case 0xd8: {
		co_await RET<C>(m_regbank.PC, m_regbank.SP, memory, m_regbank.F);
		break;
	}
	case 0xd9: {
		co_await RETI(m_regbank.PC, m_regbank.SP, memory);
		break;
	}
	case 0xda: {
		JP<C>(m_regbank.PC, co_await fetch_imm16(memory), m_regbank.F);
		break;
	}
	case 0xdb: {
		Unhandle_Instruction();
		break;
	}
	case 0xdc: {
		co_await CALL<C>(m_regbank.PC, m_regbank.SP, co_await fetch_imm8(memory), memory,
		                 m_regbank.F);
		break;
	}
	case 0xdd: {
		Unhandle_Instruction();
		break;
	}
	case 0xde: {
		m_regbank.A = SBC(m_regbank.A, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xdf: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x18, memory);
		break;
	}
	case 0xe0: {
		const uint16_t addr =
		    compose(static_cast<uint8_t>(0xFF), co_await fetch_imm8(memory));
		co_await memory.write(addr, LD(m_regbank.A));
		break;
	}
	case 0xe1: {
		tie(m_regbank.H, m_regbank.L) = co_await POP(m_regbank.SP, memory);
		break;
	}
	case 0xe2: {
		const uint16_t addr = compose(static_cast<uint8_t>(0xFF), m_regbank.C);
		co_await memory.write(addr, LD(m_regbank.A));
		break;
	}
	case 0xe3: {
		Unhandle_Instruction();
		break;
	}
	case 0xe4: {
		Unhandle_Instruction();
		break;
	}
	case 0xe5: {
		co_await PUSH(m_regbank.SP, memory, compose(m_regbank.H, m_regbank.L));
		break;
	}
	case 0xe6: {
		m_regbank.A = AND(m_regbank.A, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xe7: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x20, memory);
		break;
	}
	case 0xe8: {
		m_regbank.A = ADD(m_regbank.SP, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xe9: {
		JP<Z>(m_regbank.PC, co_await memory.read(compose(m_regbank.H, m_regbank.L)),
		      m_regbank.F);
		break;
	}
	case 0xea: {
		co_await memory.write(co_await fetch_imm16(memory), LD(m_regbank.A));
		break;
	}
	case 0xeb: {
		Unhandle_Instruction();
		break;
	}
	case 0xec: {
		Unhandle_Instruction();
		break;
	}
	case 0xed: {
		Unhandle_Instruction();
		break;
	}
	case 0xee: {
		m_regbank.A = XOR(m_regbank.A, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xef: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x28, memory);
		break;
	}
	case 0xf0: {
		m_regbank.A = co_await LDH(co_await fetch_imm8(memory), memory);
		break;
	}
	case 0xf1: {
		tie(m_regbank.A, m_regbank.F) = co_await POP(m_regbank.SP, memory);
		break;
	}
	case 0xf2: {

		m_regbank.A = co_await LDH(m_regbank.C, memory);
		break;
	}
	case 0xf3: {
		co_await DI(memory);
		break;
	}
	case 0xf4: {
		Unhandle_Instruction();
		break;
	}
	case 0xf5: {
		co_await PUSH(m_regbank.SP, memory, compose(m_regbank.A, m_regbank.F.read()));
		break;
	}
	case 0xf6: {
		m_regbank.A = OR(m_regbank.A, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xf7: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x30, memory);
		break;
	}
	case 0xf8: {
		tie(m_regbank.H, m_regbank.L) = LD(m_regbank.SP, co_await fetch_imm8(memory));
		break;
	}
	case 0xf9: {
		m_regbank.SP = LD(compose(m_regbank.H, m_regbank.L));
		break;
	}
	case 0xfa: {
		// TODO fix the flag
		m_regbank.A = co_await LD(co_await fetch_imm16(memory), memory);
		break;
	}
	case 0xfb: {
		co_await EI(memory);
		break;
	}
	case 0xfc: {
		Unhandle_Instruction();
		break;
	}
	case 0xfd: {
		Unhandle_Instruction();
		break;
	}
	case 0xfe: {
		CP(m_regbank.A, co_await fetch_imm8(memory), m_regbank.F);
		break;
	}
	case 0xff: {
		co_await RST(m_regbank.PC, m_regbank.SP, 0x38, memory);
		break;
	}
	}
	co_await make_awaiter(m_clock, 1);
	co_return;
}
