/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ArgumentType.h"
#include <boost/lexical_cast.hpp>
#include <utility>
#include <variant>
#include <string>
#include <cstdint>

namespace ember::commands {

using ArgumentValue = std::variant<
	std::string,
	std::uint8_t,
	std::uint16_t,
	std::uint32_t,
	std::uint64_t,
	std::int8_t,
	std::int16_t,
	std::int32_t,
	std::int64_t,
	float,
	double,
	char
>;

struct ParsedArgument {
	ArgumentValue value;
	ArgumentType type;
	bool required;

	ParsedArgument(ArgumentValue value, ArgumentType type, bool required)
		: value(std::move(value)),
	      type(type),
	      required(required){}
};

} // commands, ember