#include "Clock.hpp"
#include "ISA.hpp"
#include "bit_manipulation.hpp"
#include "cpu.hpp"
#include "include_std.hpp"
#include "memory.hpp"
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>

using namespace ISA;

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("FLAG Register behaviour", "[Flag]")
{
	Flag_register F = 0;
	SECTION("CARRY")
	{
		F.set_carry();
		REQUIRE(F.carry());
		F.clear_carry();
		REQUIRE(not F.carry());
	}
	SECTION("HALF CARRY")
	{
		F.set_half_carry();
		REQUIRE(F.half_carry());
		F.clear_half_carry();
		REQUIRE(not F.half_carry());
	}
	SECTION("ZERO")
	{
		F.set_zero();
		REQUIRE(F.zero());
		F.clear_zero();
		REQUIRE(not F.zero());
	}
	SECTION("SUBSTRACT")
	{
		F.set_substract();
		REQUIRE(F.substract());
		F.clear_substract();
		REQUIRE(not F.substract());
	}
}

TEST_CASE("ARTIHMETIC_INSTRUCTION", "[Arithmetic]")
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution distrib(0, 127);
	SECTION("ADD_SUB")
	{
		SECTION("ADD")
		{
			Flag_register F = 0;
			Register8 A = distrib(gen);
			Register8 B = distrib(gen);
			const std::uint8_t result = ADD(A, B, F);
			REQUIRE(result == (A + B));
			WHEN("ADD F.carry overflow")
			{
				F.reset_flag();
				const Register8 A = 255;
				const Register8 B = 2;
				ADD(A, B, F);
				THEN("F.carry flag is set") { REQUIRE(F.carry()); }
			}
			WHEN("ADD Half F.carry overflow")
			{
				F.reset_flag();
				const Register8 A = 0b1111;
				const Register8 B = 0b1;
				ADD(A, B, F);
				THEN("Half F.carry flag is set") { REQUIRE(F.half_carry()); }
			}
			WHEN("ADD F.zero")
			{
				F.reset_flag();
				const Register8 A = distrib(gen);
				const Register8 B = -A;
				ADD(A, B, F);
				THEN("F.zero is set") { REQUIRE(F.zero()); }
			}
			WHEN("ADD F.substract flag")
			{
				F.reset_flag();
				const Register8 A = distrib(gen);
				const Register8 B = distrib(gen);
				ADD(A, B, F);
				// clang-format off
                THEN("F.substract flag is not set")
                { REQUIRE(not F.substract()); }
                //clang-format on
            }
        }
        SECTION("SUB")
        {
            Flag_register F;
            const Register8 A = distrib(gen);
            const Register8 B = distrib(gen);
            const std::uint8_t result = SUB(A, B, F);
            REQUIRE(static_cast<std::int8_t>(result) == (A - B));

            WHEN("SUB F.carry overflow")
            {
                F.reset_flag();
                const Register8 A = 255;
                const Register8 B = 1;
                SUB(A, B, F);
                THEN("F.carry flag is set") { REQUIRE(F.carry()); }
            }
            WHEN("SUB Half F.carry overflow")
            {
                F.reset_flag();
                const Register8 A = 0b1111;
                const Register8 B = 0b1;
                SUB(A, B, F);
                THEN("Half F.carry flag is set") { REQUIRE(F.half_carry()); }
            }
            WHEN("SUB F.zero")
            {
                F.reset_flag();
                const Register8 A = distrib(gen);
                const Register8 B = A;
                SUB(A, B, F);
                THEN("F.zero is set") { REQUIRE(F.zero()); }
            }
            WHEN("SUB F.substract flag")
            {
                F.reset_flag();
                const Register8 A = distrib(gen);
                const Register8 B = distrib(gen);
                SUB(A, B, F);
                THEN("F.substract flag is set") { REQUIRE(F.substract()); }
            }
        }
        SECTION("ADC")
        {
            Flag_register F=0;
            F.set_carry();
            Register8 A = distrib(gen);
            Register8 B = distrib(gen);
            const std::uint8_t result =  ADC(A, B, F);
            REQUIRE(result == (A + B + 1));
            WHEN("ADC F.carry overflow")
            {
                F.reset_flag();
                const Register8 A = 255;
                const Register8 B = 2;
                ADC(A, B, F);
                THEN("F.carry flag is set") { REQUIRE(F.carry()); }
            }
            WHEN("ADC Half F.carry overflow")
            {
                F.reset_flag();
                const Register8 A = 0b1111;
                const Register8 B = 0b1;
                ADC(A, B, F);
                THEN("Half F.carry flag is set") { REQUIRE(F.half_carry()); }
            }
            WHEN("ADC F.zero")
            {
                F.reset_flag();
                const Register8 A = distrib(gen);
                const Register8 B = -A;
                ADC(A, B, F);
                THEN("F.zero is set") { REQUIRE(F.zero()); }
            }
            WHEN("ADC F.substract flag")
            {
                F.reset_flag();
                const Register8 A = distrib(gen);
                const Register8 B = distrib(gen);
                ADC(A, B, F);
                // clang-format off
                THEN("F.substract flag is not set")
                { REQUIRE(not F.substract()); }
                //clang-format on
            }
        }
        SECTION("SBC")
        {
            Flag_register F=0;
            F.reset_flag();
            F.set_carry();
            const Register8 A = distrib(gen);
            const Register8 B = distrib(gen);
            const std::uint8_t result = SBC(A, B, F);
            REQUIRE(static_cast<std::int8_t>(result) == (A - B - 1));

            WHEN("SUB F.carry overflow")
            {
                F.reset_flag();
                const Register8 A = 255;
                const Register8 B = 1;
                SBC(A, B, F);
                THEN("F.carry flag is set") { REQUIRE(F.carry()); }
            }
            WHEN("SUB Half F.carry overflow")
            {
                F.reset_flag();
                const Register8 A = 0b1111;
                const Register8 B = 0b1;
                SBC(A, B, F);
                THEN("Half F.carry flag is set") { REQUIRE(F.half_carry()); }
            }
            WHEN("SUB F.zero")
            {
                F.reset_flag();
                const Register8 A = distrib(gen);
                const Register8 B = A;
                SBC(A, B, F);
                THEN("F.zero is set") { REQUIRE(F.zero()); }
            }
            WHEN("SUB F.substract flag")
            {
                F.reset_flag();
                const Register8 A = distrib(gen);
                const Register8 B = distrib(gen);
                SBC(A, B, F);
                THEN("F.substract flag is set") { REQUIRE(F.substract()); }
            }
        }
    }
    SECTION("Increment - Decrement")
    {
        SECTION("Increment")
        {
            Flag_register F=0;
            const Register8 A = distrib(gen);
            const std::uint8_t result = INC(A, F);
            REQUIRE(result == A + 1);
            REQUIRE(not(F.substract()));
            WHEN("INC Half F.carry occurs")
            {
                const Register8 A = 0b1111;
                INC(A, F);
                THEN("Half F.carry is set") { REQUIRE(F.half_carry()); }
            }
            WHEN("INC F.zero occurs")
            {
                const Register8 A = -1;
                INC(A, F);
                THEN("F.zero is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Decrement")
        {
            Flag_register F=0;
            F.reset_flag();
            const Register8 A = distrib(gen);
            const std::uint8_t result = DEC(A, F);
            REQUIRE(result == static_cast<std::uint8_t>(A - 1));
            REQUIRE(F.substract());

            WHEN("DEC Half F.carry occurs")
            {
                const Register8 A = 0;
                DEC(A, F);
                THEN("Half F.carry is set") { REQUIRE(F.half_carry()); }
            }
            WHEN("DEC F.zero occurs")
            {
                const Register8 A = 1;
                DEC(A, F);
                THEN("Half F.carry is set") { REQUIRE(F.zero()); }
            }
        }
    }
    SECTION("Logic")
    {

        // TODO  special test case for F.zero ?
        SECTION("AND")
        {
            Flag_register F=0;
            const Register8 A = distrib(gen);
            const Register8 B = distrib(gen);
            const std::uint8_t result = AND(A, B, F);
            REQUIRE(result == (A & B));
            REQUIRE(F.half_carry());
            REQUIRE(not(F.carry()));
            REQUIRE(not(F.substract()));
        }
        SECTION("OR")
        {
            Flag_register F=0;
            const Register8 A = distrib(gen);
            const Register8 B = distrib(gen);
            const std::uint8_t result = OR(A, B, F);
            REQUIRE(result == (A | B));
            REQUIRE(not(F.half_carry()));
            REQUIRE(not(F.carry()));
            REQUIRE(not(F.substract()));
        }
        SECTION("XOR")
        {
            Flag_register F=0;
            const Register8 A = distrib(gen);
            const Register8 B = distrib(gen);
            const std::uint8_t result = XOR(A, B, F);
            REQUIRE(result == (A ^ B));
            REQUIRE(not(F.half_carry()));
            REQUIRE(not(F.carry()));
            REQUIRE(not(F.substract()));
        }
        SECTION("Complementary CPL")
        {
            Flag_register F=0;
            F.reset_flag();
            const Register8 A = 0xA5;
            const std::uint8_t result = CPL(A, F);
            REQUIRE(result == static_cast<std::uint8_t>(~A));
            REQUIRE(F.substract());
            REQUIRE(F.half_carry());
        }
    }
    SECTION("0xCB extension")
    {
        SECTION("Swap")
        {
            Flag_register F=0;
            const Register8 A = 0xA5;
            const std::uint8_t result = SWAP(A, F);
            REQUIRE(result == 0x5A);
            F.reset_flag();
            SWAP(0, F);
            REQUIRE(F.zero());
        }
        SECTION("Rotate Left with F.carry")
        {
            Flag_register F=0;
            WHEN("F.carry bit is set AND Bit 7 is set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RLC(A, F);
                REQUIRE(result == 0b10010011);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is set AND Bit 7 is not set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b01001001;
                const std::uint8_t result = RLC(A, F);
                REQUIRE(result == 0b10010010);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 7 is set") {
                F.reset_flag();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RLC(A, F);
                REQUIRE(result == 0b10010011);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 7 is not set") {
                F.reset_flag();
                const Register8 A = 0b01001001;
                const std::uint8_t result = RLC(A, F);
                REQUIRE(result == 0b10010010);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }

            WHEN("Shift lead to a F.zero value") {
                F.reset_flag();
                RLC(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Rotate Left")
        {
            Flag_register F=0;
            WHEN("F.carry bit is set AND Bit 7 is not set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b01001001;
                const std::uint8_t result = RL(A, F);
                REQUIRE(result == 0b10010011);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is set AND Bit 7 is set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RL(A, F);
                REQUIRE(result == 0b10010011);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 7 is set") {
                F.reset_flag();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RL(A, F);
                REQUIRE(result == 0b10010010);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 7 is not set") {
                F.reset_flag();
                const Register8 A = 0b01001001;
                const std::uint8_t result = RL(A, F);
                REQUIRE(result == 0b10010010);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("Shift lead to a F.zero value") {
                RL(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Rotate Right with F.carry")
        {
            Flag_register F=0;
            WHEN("F.carry bit is set AND Bit 0 is set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RRC(A, F);
                REQUIRE(result ==  0b11100100);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is set AND Bit 0 is not set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b11001000;
                const std::uint8_t result = RRC(A, F);
                REQUIRE(result ==  0b01100100);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 0 is set") {
                F.reset_flag();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RRC(A, F);
                REQUIRE(result ==  0b11100100);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 0 is not set") {
                F.reset_flag();
                const Register8 A = 0b11001000;
                const std::uint8_t result = RRC(A, F);
                REQUIRE(result ==  0b01100100);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("Shift lead to a F.zero value") {
                F.reset_flag();
                RRC(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Rotate Right")
        {
            Flag_register F=0;
            WHEN("F.carry bit is set AND Bit 0 is not set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b11001000;
                const std::uint8_t result = RR(A, F);
                REQUIRE(result ==  0b11100100);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is set AND Bit 0 is set") {
                F.reset_flag();
                F.set_carry();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RR(A, F);
                REQUIRE(result ==  0b11100100);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 0 is set") {
                F.reset_flag();
                const Register8 A = 0b11001001;
                const std::uint8_t result = RR(A, F);
                REQUIRE(result ==  0b01100100);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("F.carry bit is not set AND Bit 0 is not set") {
                F.reset_flag();
                const Register8 A = 0b11001000;
                const std::uint8_t result = RR(A, F);
                REQUIRE(result ==  0b01100100);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
            }
            WHEN("Shift lead to a F.zero value") {
                F.reset_flag();
                RR(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Shift Left SLA")
        {
            Flag_register F=0;
            const Register8 A = 0xFF;
            const std::uint8_t result = SLA(A, F);
            REQUIRE(result == 0xFE);
            REQUIRE(F.carry());
            REQUIRE(not F.half_carry());
            REQUIRE(not F.substract());
            WHEN("Shift lead to a F.zero value") {
                F.reset_flag();
                SLA(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Shift Right SRL")
        {
            Flag_register F=0;
            const Register8 A = 0xFF;
            const std::uint8_t result = SRL(A, F);
            REQUIRE(result == 0x7F);
            REQUIRE(F.carry());
            REQUIRE(not F.half_carry());
            REQUIRE(not F.substract());

            WHEN("Shift lead to a F.zero value") {
                F.reset_flag();
                SRA(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Shift Right same MSB SRA")
        {
            Flag_register F=0;
            const Register8 A = 0b10010101;
            const std::uint8_t result = SRA(A, F);
            REQUIRE(result ==  0b11001010);
            REQUIRE(F.carry());
            REQUIRE(not F.half_carry());
            REQUIRE(not F.substract());

            WHEN("Shift lead to a F.zero value") {
                SRL(0, F);
                THEN("F.zero Flag is set") { REQUIRE(F.zero()); }
            }
        }
        SECTION("Reset bit")
        {
            Flag_register F=0;
            const size_t idx = distrib(gen)&7;
            const Register8 A = 0xFF;
            const std::uint8_t result = RES(idx, A);
            REQUIRE(((result >> idx)&1) == 0b0);
        }
        SECTION("Set bit")
        {
            Flag_register F=0;
            const size_t idx = distrib(gen)&7;
            const std::uint8_t result = SET(idx, 0);
            REQUIRE(((result >> idx)&1) == 0b1);
        }
    }
    // todo ADD getter for flag
    SECTION("Flag Manipulation")
    {
        SECTION("Complementary carry Flag CCF") {
            Flag_register F=0;
            WHEN("Flag: half_carry substract are set") {
                F.reset_flag();
                F.set_carry();
                F.set_half_carry();
                F.substract();
                CCF(F);
                REQUIRE(not F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
                // checking if there are some unwanted side effect
                REQUIRE(not F.zero());

                CCF(F);
                REQUIRE(F.carry());
                REQUIRE(not F.half_carry());
                REQUIRE(not F.substract());
                // checking if there are some unwanted side effect
                REQUIRE(not F.zero());
            }
        }
        SECTION("Set carry Flag SCF") {
            Flag_register F=0;
            WHEN("Flag: carry half_carry substract are set") {
                F.set_half_carry();
                F.substract();

                SCF(F);
                THEN("Flag carry, half_carry, substract are clear") {
                    REQUIRE(F.carry());
                    REQUIRE(not F.half_carry());
                    REQUIRE(not F.substract());
                    // checking if there are some unwanted side effect
                    REQUIRE(not F.zero());
                }
            }
        }
    }
    SECTION("BCD Representation")
    {
        WHEN("No overflow A < 99")  {
            Flag_register F=0;
            Register8 A = 13;
            A = DAA(A, F);
            REQUIRE(A==0b00010011);
            REQUIRE(not F.carry());
        }
        WHEN("Overflow A>99")
        {
            Flag_register F=0;
            Register8 A = 0xFF;
            A = DAA(A, F);
            REQUIRE(F.carry());
        }
    }
}
TEST_CASE("LOAD_INSTRUCTION", "[LOAD]")
{
    Memory memory{};
    const auto rom = memory.ROM0();
    std::fill(std::begin(rom), std::end(rom), 0xFF);
    Clock_domain clock_cpu{1_Mhz};

    SECTION("LD Register8 to Register")
    {
        Register8 source = 0xA5;
        Register8 destination = LD(source);
        REQUIRE(destination == source);
    }
    SECTION("LD Register8 to Memory")
    {
        Register16 source = 0x0000;
        Register8 destination = LD(source, memory);
        REQUIRE(destination == 0xFF);
    }
    SECTION("LD Register8 16 value")
    {
        Register16 source = 0xA55A;
        auto [hi, lo] = decompose(LD(source));
        REQUIRE(lo == 0x5A);
        REQUIRE(hi == 0xA5);
    }
}
TEST_CASE("Control_Flow_INSTRUCTION", "[CF]") {
    SECTION("Direct Stack Manipulation i.e. PUSH/POP") {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution distrib(0, 0xFFFF);

        SECTION("PUSH") {
            Memory memory{};
            const auto ram = memory.IRAM0();
            std::fill(std::begin(ram), std::end(ram), 0xFFFF);

            Register16 SP = Memory::IRAM0_ul;
            const Register16 value = distrib(gen);
            const auto [hi, lo] = decompose(value);
            PUSH(SP, memory, value);

            REQUIRE(SP == Memory::IRAM0_ul-2);
            REQUIRE(memory[Memory::IRAM0_ul-1] == lo);
            REQUIRE(memory[Memory::IRAM0_ul-2] == hi);
        }
        SECTION("POP") {
            Memory memory{};
            const auto ram = memory.IRAM0();
            std::fill(std::begin(ram), std::end(ram), 0xA5);

            Register16 SP = Memory::IRAM0_ul-2;
            const auto value = POP(SP, memory);

            REQUIRE(SP == Memory::IRAM0_ul);
            REQUIRE(compose(value) == 0xA5A5);
        }
    }
    SECTION("JUMP")
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution distrib(0, 0xFFFF);

        SECTION("JP") {
            const auto offset = distrib(gen);
            Register16 PC = 0x0;
            JP(PC, offset);
            REQUIRE(PC==offset);

            WHEN("NZ Flag") {
                const auto offset = distrib(gen);
                Register16 PC = 0x0;
                Flag_register F=0;
                JP<NZ>(PC, offset, F);
                REQUIRE(not (PC==offset));

                F.set_zero();
                JP<NZ>(PC, offset, F);
                REQUIRE(PC==offset);
            }
            WHEN("C Flag") {
                const auto offset = distrib(gen);
                Register16 PC = 0x0;
                Flag_register F=0;
                F.set_zero();
                JP<Z>(PC, offset, F);
                REQUIRE(not (PC==offset));

                F.clear_zero();
                JP<Z>(PC, offset, F);
                REQUIRE(PC==offset);
            }
            WHEN("NC Flag") {
                const auto offset = distrib(gen);
                Register16 PC = 0x0;
                Flag_register F=0;
                JP<NC>(PC, offset, F);
                REQUIRE(not (PC==offset));

                F.set_carry();
                JP<NC>(PC, offset, F);
                REQUIRE(PC==offset);
            }
            WHEN("C Flag") {
                const auto offset = distrib(gen);
                Register16 PC = 0x0;
                Flag_register F=0;
                F.set_carry();
                JP<C>(PC, offset, F);
                REQUIRE(not (PC==offset));

                F.clear_carry();
                JP<C>(PC, offset, F);
                REQUIRE(PC==offset);
            }
        }
        SECTION("JR") {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution distrib(-128, 111);
            Register16 PC = 0x16;

            const std::int8_t offset = distrib(gen);
            JR(PC, offset);
            REQUIRE(PC==static_cast<uint16_t>(offset+0x16));

            WHEN("NZ Flag") {
                const std::int8_t offset = distrib(gen);
                Register16 PC = 0x16;
                Flag_register F=0;
                JR<NZ>(PC, offset, F);
                REQUIRE(not (PC==static_cast<uint16_t>(offset+0x16)));

                PC = 0x16;
                F.set_zero();
                JR<NZ>(PC, offset, F);
                REQUIRE(PC==static_cast<uint16_t>(offset+0x16));
            }
            WHEN("C Flag") {
                const std::int8_t offset= distrib(gen);
                Register16 PC = 0x16;
                Flag_register F=0;
                F.set_zero();
                JR<Z>(PC, offset, F);
                REQUIRE(not (PC==static_cast<uint16_t>(offset+0x16)));

                PC = 0x16;
                F.clear_zero();
                JR<Z>(PC, offset, F);
                REQUIRE(PC==static_cast<uint16_t>(offset+0x16));
            }
            WHEN("NC Flag") {
                const std::int8_t offset= distrib(gen);
                Register16 PC = 0x16;
                Flag_register F=0;
                JR<NC>(PC, offset, F);
                REQUIRE(not (PC==static_cast<uint16_t>(offset+0x16)));

                PC = 0x16;
                F.set_carry();
                JR<NC>(PC, offset, F);
                REQUIRE(PC==static_cast<uint16_t>(offset+0x16));
            }
            WHEN("C Flag") {
                const std::int8_t offset= distrib(gen);
                Register16 PC = 0x16;
                Flag_register F=0;
                F.set_carry();
                JR<C>(PC, offset, F);
                REQUIRE(not (PC==static_cast<uint16_t>(offset+0x16)));

                PC = 0x16;
                F.clear_carry();
                JR<C>(PC, offset, F);
                REQUIRE(PC==static_cast<uint16_t>(offset+0x16));
            }
        }
    }
}
