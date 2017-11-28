/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <type_traits>
#include <cstddef>

/*
 * Utility functions for printing out enum values returned from remote
 * Spark services using the FlatBuffers-based protocol
 */
namespace ember::util {

template<typename T>
auto enum_value(T value) {
	return static_cast<typename std::underlying_type<T>::type>(value);
}

// makes sure converting an enum value to a string isn't going to cause a crash
// in the case of a protocol mismatch
template<typename T>
const char* safe_print(T value, const char** enums) {
	std::size_t length = 0;
	const char* current = enums[0];

	while(current != nullptr) {
		current = enums[length];
		++length;
	}

	auto num_val = enum_value(value);

	// can FlatBuffers handle negative enum value printing? Check, todo!
	if(static_cast<std::size_t>(num_val) < length && enums[num_val] && strlen(enums[num_val]) > 0) {
		return enums[num_val];
	}

	return "<UNKNOWN STATUS STRING>";
}

template<typename T>
std::string fb_status(T value, const char** enums) {
	auto message = safe_print(value, enums);
	return std::string(message) + " (" + std::to_string(enum_value(value)) + ")";
}

} // util, ember