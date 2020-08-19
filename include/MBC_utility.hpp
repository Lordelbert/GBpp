#ifndef __MBC_UTILITY__
#define __MBC_UTILITY__
#include <concepts>
#include <cstdlib>
#include <type_traits>

#include <algorithm>
#include <utility>

#include <array>
#include <iterator>
#include <span>

#include <stdexcept>

#include "trait.hpp"

template <Integral T> constexpr auto is_pow_of_2(T val) -> bool
{
	return not(val == 0) and ((val & (val - 1)) == 0);
}

template <size_t nbr> concept Pow_of_2 = is_pow_of_2(nbr);

constexpr auto get_pow_2(size_t nbr) -> size_t
{
	size_t i = 0;
	for(; nbr != 0; ++i, nbr >>= 1) {
	}
	return i - 1;
}

template <size_t number>
requires Pow_of_2<number> inline constexpr size_t Pow_2_v =
    std::integral_constant<size_t, get_pow_2(number)>::value;

template <typename Predicate, typename... Args> auto check_pack(Predicate P, Args... args)
{
	return (P(args) && ...);
}

template <typename Accessor, class... List>
auto accumulate(Accessor &&Acc, List &&... list)
{
	return (Acc(list) + ...);
}

template <int I, class... Ts> auto get(Ts &&... ts)
{
	return std::get<I>(std::forward_as_tuple(ts...));
}

template <typename ValueType, size_t mem_size, size_t bin> class Partition {
	static_assert(is_pow_of_2(bin), "bin size is not a power of 2");
	static_assert((mem_size & (bin - 1)) == 0, "mem_size is not a multiple of bin size");
	std::array<std::span<ValueType>, mem_size / bin> lookup;

  public:
	using section = std::span<ValueType, bin>;
	struct descriptor {
		size_t idx;
		size_t offset;
	};

	template <typename Iter>
	explicit constexpr Partition(const Iter begin, const Iter end)
	{
		if(static_cast<size_t>(std::distance(begin, end)) < mem_size) {
			throw std::invalid_argument("memory is too small to be partition");
		}
		size_t i = 0;
		for(auto j = begin; j < end; ++i, j += bin) {
			lookup[i] = section(j, j + bin);
		}
	}
	template <typename Iter, typename... Descriptor>
	constexpr auto precondition(Iter begin, Iter end, Descriptor &&... offsets)
	{
		static_assert(sizeof...(offsets) < bin, "Too much descriptors provided");
		auto is_bin_divisible = [](auto elt) { return (elt.offset & (bin - 1)) == 0; };
		if(static_cast<size_t>(std::distance(begin, end)) < mem_size) {
			throw std::invalid_argument("memory is too small to be partition");
		}
		if(not check_pack(is_bin_divisible, offsets...)) {
			throw std::invalid_argument("offsets must be multiple of 8");
		}
		const auto offset = accumulate([](auto elt) { return elt.offset; }, offsets...);
		if(begin + offset + mem_size > end) {
			throw std::invalid_argument("overflow memory");
		}
	}
	template <typename Iter, typename... Descriptor>
	constexpr Partition(const Iter begin, const Iter end, Descriptor &&... offsets)
	{
		try {
			precondition(begin, end, std::forward<Descriptor>(offsets)...);
		}
		catch(std::invalid_argument &e) {
			throw e;
		};
		std::array descriptions{std::forward<Descriptor>(offsets)...};
		auto it = std::begin(descriptions);
		// create lookup span until last offsets in offfsets list
		for(Iter pmemory = begin;
		    size_t idx : std::views::iota(0) | std::views::take(std::size(lookup))) {
			if(it != std::end(descriptions) and idx == it->idx) {
				pmemory += it->offset;
				it = std::next(it);
			}
			lookup[idx] = section(pmemory, pmemory + bin);
			pmemory += bin;
		}
	}
	constexpr auto operator[](size_t idx) const noexcept -> ValueType
	{
		auto val = lookup[idx >> Pow_2_v<bin>];
		return val[idx & (bin - 1)];
	}
	constexpr auto operator[](size_t idx) noexcept -> ValueType &
	{
		auto val = lookup[idx >> Pow_2_v<bin>];
		return val[idx & (bin - 1)];
	}
	constexpr auto swap(size_t lookup_slot, std::span<ValueType> &&data) noexcept
	{
		std::swap(data, lookup[lookup_slot]);
	}
};
#endif
