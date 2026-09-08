/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Common.h>
#include <commands/Commands.h>
#include <cstdint>

namespace ember::blaze {

extern "C" {

struct Command {
	ember::commands::Command* impl;
};

EMBER_EXPORT Command command_create(const CountedString* name, const CountedString* description);
EMBER_EXPORT bool command_destroy(Command command);
EMBER_EXPORT bool command_add_argument(Command command, const CountedString* name, ArgumentType type, bool required);
EMBER_EXPORT bool command_callback(Command command);

} // extern "C"

} // blaze, ember