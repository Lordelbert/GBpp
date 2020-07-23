#ifndef __GAMEBOY_HPP__
#define __GAMEBOY_HPP__

#include "Clock.hpp"
#include "cpu.hpp"
#include "include_std.hpp"
#include "memory.hpp"

class Gameboy {
  public:
	auto run() -> void
	{
		Scheduler clock_domain_handler{&m_clock_cpu};

		m_cpu.run(m_memory);

		while(1) {
			clock_domain_handler();
		}
	}
	Gameboy(std::vector<std::uint8_t> program)
	    : m_clock_cpu{4_Mhz}, m_clock_gpu{4_Mhz}, m_cpu(m_clock_cpu), m_memory(program)
	{
	}
	Gameboy(std::initializer_list<std::uint8_t> program)
	    : m_clock_cpu{4_Mhz}, m_clock_gpu{4_Mhz}, m_cpu(m_clock_cpu), m_memory(program)
	{
	}

  private:
	Clock_domain m_clock_cpu, m_clock_gpu;
	SM83 m_cpu;
	Memory m_memory;
	// Sound_engine
	// Graphic engine
};

#endif
