/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <utility>

namespace ember::commands {

using ArgumentStore = std::unordered_map<std::string, Argument>;

class Arguments {
	ArgumentStore args_;

public:
	Arguments(ArgumentStore args)
		: args_(std::move(args)) {}

	const Argument& operator[](const std::string& arg_name) {
		return args_.at(arg_name);
	}

	auto begin() {
		return args_.begin();
	}
	
	auto end() {
		return args_.end();
	}
};

} // commands, ember