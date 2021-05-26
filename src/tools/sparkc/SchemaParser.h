/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Printer.h"
#include <vector>
#include <cstdint>

namespace ember {

class SchemaParser {
public:
	explicit SchemaParser(std::vector<std::uint8_t> buffer);

private:
	std::vector<std::uint8_t> buffer_;

	void verify();
};

} // ember