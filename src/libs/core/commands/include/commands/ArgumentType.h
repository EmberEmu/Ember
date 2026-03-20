/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <utility>

namespace ember::commands::args {

enum class Type {
	at_string,
	at_uint8,
	at_uint16,
	at_uint32,
	at_uint64,
	at_int8,
	at_int16,
	at_int32,
	at_int64,
	at_float,
	at_double,
	at_char,
	at_user_data
};

constexpr static auto type_count = std::to_underlying(Type::at_user_data) + 1;

} // args, commands, ember