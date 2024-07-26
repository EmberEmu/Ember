/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


// todo: delete this file when min. compiler versions are bumped
#if !defined __cpp_lib_print

#include <format>
#include <iostream>
#include <utility>

namespace std {

// crappy polyfill
template<typename... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
	std::cout << std::format(fmt, std::forward<Args>(args)...);
}

} // std
#else
#include <print>
#endif