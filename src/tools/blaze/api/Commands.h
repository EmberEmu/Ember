/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Common.h"
#include <commands/Commands.h>
#include <cstdint>

namespace ember::blaze {

extern "C" {

struct Command {
	ember::commands::Command* impl;
};

enum CommandArgType {
	cat_string,
	cat_float,
	cat_double,
	cat_int_8,
	cat_int_16,
	cat_int_32,
	cat_int_64,
	cat_uint_8,
	cat_uint_16,
	cat_uint_32,
	cat_uint_64,
};

EMBER_EXPORT Command command_create(const SizedString* name, const SizedString* description);
EMBER_EXPORT bool command_destroy(Command command);
EMBER_EXPORT bool command_add_argument(Command command, const SizedString* name, std::uint8_t type, bool required);
EMBER_EXPORT bool command_callback(Command command);

} // extern "C"

} // blaze, ember