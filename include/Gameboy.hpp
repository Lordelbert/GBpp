#ifndef __GAMEBOY_HPP__
#define __GAMEBOY_HPP__

#include "Clock.hpp"
#include "cpu.hpp"
#include "include_std.hpp"
#include "memory.hpp"
#include <cstdint>
#include <initializer_list>

class Gameboy {
  public:
	auto run() -> void
	{
		Scheduler clock_domain_handler{&m_clock_cpu};

		m_cpu.run(m_memory);

		for([[maybe_unused]] auto _ : m_memory.ROM0()) {
			clock_domain_handler();
		}
	}
	Gameboy(std::vector<std::uint8_t> &&program)
	    : m_clock_cpu{4_Mhz}, m_clock_gpu{4_Mhz}, m_cpu(m_clock_cpu)
	{
		auto rom = m_memory.ROM0();
		std::move(program.begin(), program.end(), rom.begin());
	}
	Gameboy(std::initializer_list<std::uint8_t> &&program)
	    : m_clock_cpu{4_Mhz}, m_clock_gpu{4_Mhz}, m_cpu(m_clock_cpu)
	{
		auto rom = m_memory.ROM0();
		std::move(program.begin(), program.end(), rom.begin());
	}

  private:
	Clock_domain m_clock_cpu, m_clock_gpu;
	SM83 m_cpu;
	Memory m_memory;
	// Sound_engine
	// Graphic engine
};

#endif
