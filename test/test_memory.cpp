#include "MBC.hpp"
#include "catch.hpp"
#include "memory.hpp"
#include "units.hpp"
#include <iterator>

TEST_CASE("Memory controller test", "[MBC TEST]")
{
	SECTION("MBC1 controller")
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution ramg_addr(0, 0x1FFF);
		std::uniform_int_distribution mbc1_addr(0x2000, 0x3FFF);
		std::uniform_int_distribution mbc2_addr(0x4000, 0x5FFF);
		std::uniform_int_distribution mode_addr(0x6000, 0x7FFF);

		SECTION("Requires more memory than possible")
		{
			// MBC1 handle up to 2MB of ROM and 256 of RAM
			WHEN("Program is too big")
			{
				auto rom = std::vector<std::uint8_t>(3_MB, 0xFF);
				THEN("Constructor throw")
				{
					auto tmp = [&rom] {
						std::vector<std::uint8_t> tmp(64_kB + 3_MB + 32_kB, 0x00);
						std::move(rom.begin(), rom.end(), tmp.begin());
						return tmp;
					}();
					CHECK_THROWS(MBC1(std::begin(tmp), std::end(tmp), 3_MB, 8_kB));
				}
			}
			WHEN("Too much RAM is required")
			{
				auto rom = std::vector<std::uint8_t>(2_MB, 0xFF);
				THEN("Constructor throw")
				{
					auto tmp = [&rom] {
						std::vector<std::uint8_t> tmp(64_kB + 2_MB + 512_kB, 0x00);
						std::move(rom.begin(), rom.end(), tmp.begin());
						return tmp;
					}();
					CHECK_THROWS(MBC1(std::begin(tmp), std::end(tmp), 512_kB, 256_kB));
				}
			}
		}
		SECTION("Writing to bank registers")
		{

			SECTION("RAM:")
			{
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution ramg_enable(0, 0x1FFF);

				auto rom = std::vector<std::uint8_t>(512_kB, 0xFF);
				auto tmp = [&rom] {
					std::vector<std::uint8_t> tmp(64_kB + 512_kB + 32_kB, 0x00);
					std::move(rom.begin(), rom.end(), tmp.begin());
					return tmp;
				}();
				auto memory = MBC1(tmp.begin(), tmp.end(), 512_kB, 32_kB);

				WHEN("write/read back: RAM enable")
				{
					// enable ram
					memory.write(ramg_enable(gen), 0x0A);
					memory.write(0xA000, 0xA5);
					REQUIRE(memory.read(0xA000) == 0xA5);
				}
				WHEN("write/read back: RAM enable and bank!=bank0")
				{
					memory.write(ramg_enable(gen), 0x0A);

					memory.write(0xA000, 0xA5);
					REQUIRE(memory.read(0xA000) == 0xA5);
					memory.write(mode_addr(gen), 0b1);
					memory.write(mbc2_addr(gen), 0b10);
					// TODO change this
					// may fail, since ram is random so we might have 0xA5
					REQUIRE(not(memory.read(0xA000) == 0xA5));
				}

				WHEN("write/read back: RAM disable [stat test may fail][not stable]")
				{
					// TODO Maybe need some works, I'm not 100% sure of me
					memory.write(ramg_enable(gen), 0x00);
					// If we read value in Ram when not enable
					// it will feed us with random value.
					// Which in this emulator are drawn upon a discrete
					// uniform distribution.
					// Therefore chi square is test to check.
					std::array<int, 256> obs{0};
					double proba = 256 * 500 / 256;
					for(size_t i = 0; i < 256 * 500; ++i) {
						obs[memory.read(0xA000)] += 1;
					}
					double acc = std::accumulate(
					    std::begin(obs), std::end(obs), 0, [proba](double acc, int elt) {
						    return acc + (proba - elt) * (proba - elt) / proba;
					    });
					REQUIRE(acc < 219.02528);
				}
			}
			SECTION("ROM:")
			{
				std::vector<std::uint8_t> rom(2_MB, 0xFF);
				std::fill(&rom[0], &rom[0x4000], 0xCA); // read happen in bank 0
				std::fill(&rom[0x080000], &rom[0x084000], 0xA5); // bank0x20
				std::fill(&rom[0x084000], &rom[0x88000], 0xFE);
				std::fill(&rom[0x1FC000], &rom[0x200000], 0x5A);

				auto tmp = [&rom] {
					std::vector<std::uint8_t> tmp(64_kB + 2_MB + 8_kB, 0x00);
					std::move(rom.begin(), rom.end(), tmp.begin());
					return tmp;
				}();
				auto memory = MBC1(tmp.begin(), tmp.end(), 2_MB, 8_kB);

				WHEN("Reading bank 0 and MODE is not set")
				{
					memory.write(mode_addr(gen), 0b0);
					REQUIRE(memory.read(0x0) == 0xCA);
				}
				WHEN("Reading bank 0 and MODE is set")
				{
					memory.write(mode_addr(gen), 0b1);
					memory.write(mbc2_addr(gen), 0b1);
					const std::uint8_t result = memory.read(0x0);
					// if mode is set bank2 is used in address computation
					// of bank0 :fear:
					REQUIRE(result == 0xA5);
				}
				WHEN("Reading bank 7F")
				{
					memory.write(mbc1_addr(gen), 0b1'1111);
					memory.write(mbc2_addr(gen), 0b11);
					// mode bit is not used so we test with both 0 and 1
					// to check if there are any side effect
					WHEN("MODE is not set")
					{
						memory.write(mode_addr(gen), 0b0);
						const std::uint8_t result = memory.read(0x4000);
						REQUIRE(result == 0x5A);
					}
					WHEN("MODE is set")
					{
						memory.write(mode_addr(gen), 0b1);
						const std::uint8_t result = memory.read(0x4000);
						REQUIRE(result == 0x5A);
					}
				}
				WHEN("Reading bank 0x20 from Bank rom")
				{
					memory.write(mbc1_addr(gen), 0b0);
					memory.write(mbc2_addr(gen), 0b1);
					// mode bit is not used so we test with both 0 and 1
					// to check if there are any side effect
					WHEN("MODE is not set")
					{
						memory.write(mode_addr(gen), 0b0);
						const std::uint8_t result = memory.read(0x4000);
						REQUIRE(result == 0xFE);
					}
					WHEN("MODE is set")
					{
						memory.write(mode_addr(gen), 0b1);
						const std::uint8_t result = memory.read(0x4000);
						REQUIRE(result == 0xFE);
					}
				}
			}
		}
	}
	SECTION("MBC2 controller")
	{
		// TODO
	}
	SECTION("MBC3 controller")
	{
		// TODO
	}
	SECTION("MBC5 controller")
	{
		// TODO
	}
	SECTION("Common MBC test")
	{
		SECTION("Checking ROM validity") {}
	}
}

TEST_CASE("Other part of memory", "[MEMORY TEST]") {}
