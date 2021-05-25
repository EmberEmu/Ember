/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Printer.h"

namespace ember {

Printer::Printer(const IndentStyle indent_style, const std::uint8_t indent_width)
	: indent_style_(indent_style), indent_width_(indent_width) {}

// todo, bounds
void Printer::indent() {
	++indent_level_;
}

// todo, bounds
void Printer::outdent() {
	--indent_level_;
}

void Printer::print() {

}

} // ember