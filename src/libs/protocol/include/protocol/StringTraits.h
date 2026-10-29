/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <string_view>

namespace ember::protocol {

template<class T>
struct as_view;

template<class Char, class Traits, class Alloc>
struct as_view<std::basic_string<Char, Traits, Alloc>> {
	using type = std::basic_string_view<Char, Traits>;
};

struct use_strings {
	template<class T>
	using type = T;
};

struct use_views {
	template<class T>
	using type = typename as_view<T>::type;
};

#define IMPORT_STRING_ALIASES                               \
using string    = Ownership::template type<std::string>;    \
using u8string  = Ownership::template type<std::u8string>;  \
using u16string = Ownership::template type<std::u16string>; \
using u32string = Ownership::template type<std::u32string>; 

} // protocol, ember