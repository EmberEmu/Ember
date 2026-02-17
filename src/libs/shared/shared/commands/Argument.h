/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/lexical_cast.hpp>
#include <string>
#include <utility>

namespace ember::commands {

struct Argument {
	std::string value;
	bool required;

	Argument(std::string value, bool required)
		: value(std::move(value)),
		  required(required) {}

	template<typename T>
	T as() const {
		return boost::lexical_cast<T>(value);
	}
};

} // commands, ember