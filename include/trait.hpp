#ifndef __TRAIT_HPP__
#define __TRAIT_HPP__
#include <include_std.hpp>

template <typename T> concept Unsigned = std::is_integral_v<T> && !std::is_signed_v<T>;

template <Unsigned T> struct Overflow_trait {
};
template <> struct Overflow_trait<std::uint8_t> {
	using Type = std::uint16_t;
};
template <> struct Overflow_trait<std::uint16_t> {
	using Type = std::uint32_t;
};
template <> struct Overflow_trait<std::uint32_t> {
	using Type = std::uint64_t;
};
template <Unsigned T> using Overflow_trait_t = typename Overflow_trait<T>::Type;

template <Unsigned T> struct Combine_trait {
};
template <> struct Combine_trait<std::uint8_t> {
	using Type = std::uint16_t;
};
template <> struct Combine_trait<std::uint16_t> {
	using Type = std::uint32_t;
};
template <> struct Combine_trait<std::uint32_t> {
	using Type = std::uint64_t;
};
template <Unsigned T> using Combine_trait_t = typename Combine_trait<T>::Type;

#endif
