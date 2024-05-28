/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <utility>

namespace ember {

/*
 * Just a basic type used for validation with Boost Program Options
 */
struct OutputOption {
	std::string option;

	explicit OutputOption(std::string option) : option(std::move(option)) {}
	explicit OutputOption(const char* option) : option(option) {}

	friend bool operator==(const OutputOption& lhs, const std::string& rhs) {
		return lhs.option == rhs;
	}

	friend bool operator==(const OutputOption& lhs, const OutputOption& rhs) {
		return lhs.option == rhs.option;
	}

	friend bool operator>(const OutputOption& lhs, const OutputOption& rhs) {
		return lhs.option > rhs.option;
	}

	friend bool operator<(const OutputOption& lhs, const OutputOption& rhs) {
		return (lhs != rhs && !(lhs > rhs));
	}

	friend bool operator!=(const OutputOption& lhs, const OutputOption& rhs) {
		return !(lhs == rhs);
	}

	friend bool operator<=(const OutputOption& lhs, const OutputOption& rhs) {
		return (lhs < rhs || lhs == rhs);
	}

	friend bool operator>=(const OutputOption& lhs, const OutputOption& rhs) {
		return (lhs > rhs || lhs == rhs);
	}
};

} // ember