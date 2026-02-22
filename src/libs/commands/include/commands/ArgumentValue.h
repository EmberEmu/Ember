/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <variant>
#include <string>
#include <cstdint>

namespace ember::commands::args {

using Value = std::variant<
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

} // commands, ember