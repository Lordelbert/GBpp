#ifndef __UNITS__INCLUDE_HPP__
#define __UNITS__INCLUDE_HPP__

#include <cstdint>

constexpr auto operator"" _MB(unsigned long long int frequency) -> std::size_t
{
	return frequency * 1'048'576;
}
constexpr auto operator"" _kB(unsigned long long int frequency) -> std::size_t
{
	return frequency * 1024;
}

// return µs result
constexpr long double operator"" _Mhz(long double frequency)
{
	return (frequency == 0) ? 0 : (1e3 / frequency);
}
// return µs result
constexpr long double operator"" _Mhz(unsigned long long int frequency)
{
	return (frequency == 0) ? 0 : (1e3 / frequency);
}

#endif
