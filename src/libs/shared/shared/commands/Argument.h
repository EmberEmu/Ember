/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ArgumentType.h"
#include <string>
#include <utility>

namespace ember::commands {

struct Argument {
	std::string name;
	bool required;
	ArgumentType type;

	Argument(std::string name, bool required, ArgumentType type)
		: name(std::move(name)),
		  required(required),
		  type(type) {}
};

} // commands, ember