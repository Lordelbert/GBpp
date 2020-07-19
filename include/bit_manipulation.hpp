#ifndef __BIT_MANIPULATION_HPP__
#define __BIT_MANIPULATION_HPP__
#include "include_std.hpp"
#include "trait.hpp"
#include <utility>

constexpr size_t operator"" _MB(unsigned long long int frequency)
{
	return frequency * 1'048'576;
}
constexpr size_t operator"" _kB(unsigned long long int frequency)
{
	return frequency * 1024;
}

template <Unsigned T> constexpr auto get_bit(T val, size_t pos) noexcept -> std::uint8_t
{
	return (val >> pos) & 0b1;
}

template <Unsigned T>
constexpr auto compose(std::pair<T, T> word) noexcept -> Combine_trait_t<T>
{
	const unsigned N = (sizeof(T) * 8);
	if constexpr(sizeof(T) == 4) {
		return static_cast<std::uint64_t>((word.first << N)) | word.second;
	}
	if constexpr(sizeof(T) == 2) {
		return static_cast<std::uint32_t>((word.first << N)) | word.second;
	}
	if constexpr(sizeof(T) == 1) {
		return static_cast<std::uint16_t>((word.first << N)) | word.second;
	}
}
template <Unsigned T> constexpr auto compose(T hi, T lo) noexcept -> Combine_trait_t<T>
{
	return compose(std::make_pair(hi, lo));
}

template <Unsigned T> constexpr auto decompose(T opcode) noexcept
{
	const unsigned N = (sizeof(T) * 8) / 2;
	if constexpr(sizeof(T) == 8) {
		return std::make_pair(static_cast<std::uint32_t>(opcode >> N),
		                      static_cast<std::uint32_t>(opcode & 0xFFFF'FFFF));
	}
	if constexpr(sizeof(T) == 4) {
		return std::make_pair(static_cast<std::uint16_t>(opcode >> N),
		                      static_cast<std::uint16_t>(opcode & 0xFFFF));
	}
	if constexpr(sizeof(T) == 2) {
		return std::make_pair(static_cast<std::uint8_t>(opcode >> N),
		                      static_cast<std::uint8_t>(opcode & 0xFF));
	}
	if constexpr(sizeof(T) == 1) {
		return std::make_pair(static_cast<std::uint8_t>(opcode >> N),
		                      static_cast<std::uint8_t>(opcode & 0xF));
	}
}

template <Unsigned T> constexpr auto set_bit(T &val, size_t pos) noexcept -> void
{
	val |= (0x1 << pos);
}

template <Unsigned T> constexpr auto set_bit(T val, size_t pos, T content) noexcept -> T
{
	const std::uint8_t mask = ~(1 << pos);
	return (val & mask) | (content << pos);
}

template <Unsigned T> constexpr auto clear_bit(T &val, size_t pos) noexcept -> void
{
	val &= (~(0x1 << pos));
}

constexpr uint8_t nibble_hi(std::uint8_t a) { return (a & 0xF0) >> 4; }

constexpr uint8_t nibble_lo(std::uint8_t a) { return (a & 0x0F); }

constexpr uint8_t set_nibble_hi(std::uint8_t &a, std::uint8_t val)
{
	a &= 0x0F;
	a |= (val << 4);
	return a;
}

constexpr uint8_t set_nibble_lo(std::uint8_t &a, std::uint8_t val)
{
	a &= 0xF0;
	a |= (val & 0x0F);
	return a;
}

#endif
