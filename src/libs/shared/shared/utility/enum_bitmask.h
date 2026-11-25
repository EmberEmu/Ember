/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <concepts>
#include <utility>

namespace ember {

template<typename Enum>
struct EnableBitMask {
	static constexpr bool value = false;
};

template<typename T>
concept bitmask_enum = EnableBitMask<T>::value && std::is_scoped_enum_v<T>;

template<bitmask_enum Enum>
constexpr Enum operator|(Enum lhs, Enum rhs) {
	return static_cast<Enum> (
		std::to_underlying(lhs) |
		std::to_underlying(rhs)
	);
}

template<bitmask_enum Enum>
constexpr Enum operator&(Enum lhs, Enum rhs) {
	return static_cast<Enum> (
		std::to_underlying(lhs) &
		std::to_underlying(rhs)
	);
}

template<bitmask_enum Enum>
constexpr Enum operator^(Enum lhs, Enum rhs) {
	return static_cast<Enum> (
		std::to_underlying(lhs) ^
		std::to_underlying(rhs)
	);
}

template<bitmask_enum Enum>
constexpr Enum operator~(Enum rhs) {
	return static_cast<Enum> (
		~std::to_underlying(rhs)
	);
}

template<bitmask_enum Enum>
constexpr Enum operator|=(Enum &lhs, Enum rhs) {
	lhs = static_cast<Enum> (
		std::to_underlying(lhs) |
		std::to_underlying(rhs)
	);

	return lhs;
}

template<bitmask_enum Enum>
constexpr Enum operator&=(Enum &lhs, Enum rhs) {
	lhs = static_cast<Enum> (
		std::to_underlying(lhs) &
		std::to_underlying(rhs)
	);

	return lhs;
}

template<bitmask_enum Enum>
constexpr Enum operator^=(Enum &lhs, Enum rhs) {
	lhs = static_cast<Enum> (
		std::to_underlying(lhs) ^
		std::to_underlying(rhs)
	);

	return lhs;
}

#define ENABLE_BITMASK(x)                \
template<>                               \
struct EnableBitMask<x>                  \
{                                        \
    static constexpr bool value = true;  \
};

} // ember