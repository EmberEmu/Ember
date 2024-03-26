/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Printer.h"
#include <numeric>
#include <stdexcept>
#include <boost/assert.hpp>

namespace ember {

Printer::Printer(const IndentStyle indent_style, const std::uint8_t indent_width)
	: indent_style_(indent_style), indent_width_(indent_width) {}

void Printer::indent() {
	if(indent_level_ == std::numeric_limits<decltype(indent_level_)>::max()) {
		throw std::logic_error("Indent level would overflow");
	}

	++indent_level_;
}

void Printer::outdent() {
	if(indent_level_ == 0) {
		throw std::logic_error("Indent level would underflow");
	}
	
	--indent_level_;
}

void Printer::print(const std::string_view string) {
	char indent_char = 0;

	switch(indent_style_) {
		case IndentStyle::TAB:
			indent_char = '\t';
			break;
		case IndentStyle::SPACES:
			indent_char = ' ';
			break;
		default:
			BOOST_ASSERT_MSG(true, "Unhandled indent style");
	}

	std::string indent(indent_level_ * indent_width_, indent_char);
	stream_ << indent << string << '\n';
}

std::string Printer::output() {
	return stream_.str();
}

} // ember