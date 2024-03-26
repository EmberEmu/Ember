/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <sstream>
#include <string_view>
#include <cstdint>

namespace ember {

class Printer {
public:
	enum class IndentStyle {
		TAB, SPACES
	};

	Printer() = default;
	Printer(IndentStyle indent_style, std::uint8_t indent_width);

	void indent();
	void outdent();
	void print(std::string_view string);
	std::string output();

private:
	const std::uint8_t indent_width_ = 1;
	const IndentStyle indent_style_ = IndentStyle::TAB;

	std::stringstream stream_;
	unsigned int indent_level_ = 0;
};

} // ember