/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <type_traits>

namespace ember {

template<typename Enum>
struct EnableBitMask {
	static constexpr bool enable = false;
};

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type
operator|(Enum lhs, Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
	);
}

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type
operator&(Enum lhs, Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
	);
}

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type
operator^(Enum lhs, Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) ^
		static_cast<underlying>(rhs)
	);
}

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type
operator~(Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		~static_cast<underlying>(rhs)
	);
}

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type &
operator|=(Enum &lhs, Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
	);

	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type &
operator&=(Enum &lhs, Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
	);

	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMask<Enum>::enable, Enum>::type &
operator^=(Enum &lhs, Enum rhs) {
	using underlying = typename std::underlying_type<Enum>::type;

	lhs = static_cast<Enum> (
		static_cast<underlying>(lhs) ^
		static_cast<underlying>(rhs)
	);

	return lhs;
}

#define ENABLE_BITMASK(x)                \
template<>                               \
struct EnableBitMask<x>                  \
{                                        \
    static constexpr bool enable = true; \
};

}; // ember
