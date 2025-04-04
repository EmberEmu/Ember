/*
 * Copyright (c) 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Concepts.h>
#include <bit>
#include <type_traits>
#include <utility>
#include <cstdint>

namespace ember::spark::io::endian {

enum class conversion {
	big_to_native,
	native_to_big,
	little_to_native,
	native_to_little
};

constexpr auto conditional_reverse(arithmetic auto value, std::endian from, std::endian to) {
	using type = decltype(value);

	if(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}

	return value;
}

template<std::endian from, std::endian to>
constexpr auto conditional_reverse(arithmetic auto value) {
	using type = decltype(value);

	if constexpr(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}

	return value;
}

constexpr auto little_to_native(arithmetic auto value) {
	return conditional_reverse<std::endian::little, std::endian::native>(value);
}

constexpr auto big_to_native(arithmetic auto value) {
	return conditional_reverse<std::endian::big, std::endian::native>(value);
}

constexpr auto native_to_little(arithmetic auto value) {
	return conditional_reverse<std::endian::native, std::endian::little>(value);
}

constexpr auto native_to_big(arithmetic auto value) {
	return conditional_reverse<std::endian::native, std::endian::big>(value);
}

template<std::endian from, std::endian to>
constexpr void conditional_reverse_inplace(arithmetic auto& value) {
	using type = std::remove_reference_t<decltype(value)>;

	if constexpr(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}
}

constexpr void conditional_reverse_inplace(arithmetic auto& value, std::endian from, std::endian to) {
	using type = std::remove_reference_t<decltype(value)>;

	if(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}
}

constexpr void little_to_native_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::little, std::endian::native>(value);
}

constexpr void big_to_native_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::big, std::endian::native>(value);
}

constexpr void native_to_little_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::native, std::endian::little>(value);
}

constexpr void native_to_big_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::native, std::endian::big>(value);
}

template<conversion _conversion>
constexpr auto convert(arithmetic auto value) -> decltype(value) {
	switch(_conversion) {
		case conversion::big_to_native:
			return big_to_native(value);
		case conversion::native_to_big:
			return native_to_big(value);
		case conversion::little_to_native:
			return little_to_native(value);
		case conversion::native_to_little:
			return native_to_little(value);
		default:
			std::unreachable();
	};
}

struct adaptor_tag_t {};

#define ENDIAN_ADAPTOR(name, func_to, func_from) \
template<arithmetic T>                           \
struct name final : adaptor_tag_t {              \
	T& value;                                    \
                                                 \
    name(T& t) : value(t) {}                     \
    name(T&& t) : value(t) {}                    \
                                                 \
	auto to() -> T {                             \
		return func_to(value);                   \
	}                                            \
	auto from() -> T {                           \
		return func_from(value);                 \
	}                                            \
};

ENDIAN_ADAPTOR(be, native_to_big,    big_to_native)
ENDIAN_ADAPTOR(le, native_to_little, little_to_native)

struct storage_tag {};
struct as_big_t final : storage_tag {};
struct as_little_t final : storage_tag {};
struct as_native_t final : storage_tag {};

[[maybe_unused]] constexpr static as_big_t big {};
[[maybe_unused]] constexpr static as_little_t little {};
[[maybe_unused]] constexpr static as_native_t native {};

inline auto storage_in(const arithmetic auto& value, as_native_t) {
	return value;
}

inline auto storage_in(const arithmetic auto& value, as_little_t) {
	return native_to_little(value);
}

inline auto storage_in(const arithmetic auto& value, as_big_t) {
	return native_to_big(value);
}

inline void storage_out(arithmetic auto& value, as_native_t) {}

inline void storage_out(arithmetic auto& value, as_little_t) {
	return little_to_native_inplace(value);
}

inline void storage_out(arithmetic auto& value, as_big_t) {
	return big_to_native_inplace(value);
}


} // endian, io, spark, ember