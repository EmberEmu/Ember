/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <functional>
#include <string>
#include <typeinfo>
#include <utility>

namespace ember::commands {

struct Argument {
	std::string name;
	std::reference_wrapper<const std::type_info> type; // this is okay, type_info lives for the duration of the program
	bool required;

	Argument(std::string name, bool required, const std::type_info& type_info) noexcept
		: name(std::move(name))
		, required(required)
		, type(type_info) {}
};

} // commands, ember