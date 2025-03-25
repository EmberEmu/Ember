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
#include <cstdint>

namespace ember::spark::io::endian {

enum class Conversion {
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
constexpr auto conditional_reverse_inplace(arithmetic auto& value) {
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

constexpr auto conditional_reverse_inplace(arithmetic auto& value, std::endian from, std::endian to) {
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

template<Conversion conversion>
constexpr auto convert(arithmetic auto value) -> decltype(value) {
	switch(conversion) {
		case Conversion::big_to_native:
			return big_to_native(value);
			break;
		case Conversion::native_to_big:
			return native_to_big(value);
			break;
		case Conversion::little_to_native:
			return little_to_native(value);
			break;
		case Conversion::native_to_little:
			return native_to_little(value);
			break;
#if defined(_MSC_VER) && (_MSC_VER >= 1940) // todo: drop in next version of VS
		default:
			static_assert(false, "Unhandled conversion");
#endif
	};
}

} // endian, io, spark, ember
