/*
* Copyright (c) 2024 - 2025 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <spark/buffers/Shared.h>
#include <bit>
#include <concepts>
#include <type_traits>

namespace ember::spark::io {

template<typename buf_type>
concept writeable =
	requires(buf_type t, void* v, typename buf_type::size_type s) {
		{ t.write(v, s) } -> std::same_as<void>;
};

template<typename buf_type>
concept seekable = requires(buf_type t) {
	std::is_same_v<typename buf_type::seeking, supported>;
};

template<typename buf_type>
concept contiguous = requires(buf_type t) {
	std::is_same_v<typename buf_type::contiguous, is_contiguous>;
};

template<typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template<typename T>
concept byte_type = sizeof(T) == 1;

template<typename T>
concept byte_oriented = byte_type<typename T::value_type>;

template<typename T>
concept pod = std::is_standard_layout_v<T> && std::is_trivial_v<T>;

template<typename T>
concept has_resize_overwrite =
	requires(T t) {
		{ t.resize_and_overwrite(typename T::size_type(), [](char*, T::size_type) {}) } -> std::same_as<void>;
};

template<typename T>
concept has_resize = 
	requires(T t) {
		{ t.resize(typename T::size_type() ) } -> std::same_as<void>;
};

template<typename T, typename U>
concept has_shl_override =
	requires(T t, U& u) {
		{ t.operator<<(u) } -> std::same_as<U&>;
};

template<typename T, typename U>
concept has_shr_override =
	requires(T t, U& u) {
		{ t.operator>>(u) } -> std::same_as<U&>;
};

} // io, spark, ember