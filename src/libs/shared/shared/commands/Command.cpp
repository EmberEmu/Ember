/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Command.h"
#include <sstream>

namespace ember::commands {

Command::Command(std::string name) {
	args_.emplace_back(std::move(name), false);
}

Command& Command::arg(std::string argument) {
	if(opt_args) {
		throw std::invalid_argument("Required arguments must be placed before optional arguments");
	}
		
	args_.emplace_back(std::move(argument), false);
	++req_args;
	++total_args;
	return *this;
}

Command& Command::optional_arg(std::string argument) {
	args_.emplace_back(std::move(argument), true);
	++opt_args;
	++total_args;
	return *this;
}

Command& Command::handler(CommandHandler handler) {
	handler_ = handler;
	return *this;
}

const std::vector<Argument>& Command::args() const {
	return args_;
}

std::string Command::usage_string() const {
	std::stringstream ss;
	bool first = true;
	
	for(const auto& arg : args_) {
		if(first) {
			ss << arg.value;
			first = false;
		} else {
			if(arg.required) {
				ss << " [" << arg.value << "]";
			} else {
				ss << "(optional) [" << arg.value << "]";
			}
		}
	}

	return ss.str();
}

} // commands, ember