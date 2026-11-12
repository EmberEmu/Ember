/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stdexcept>
#include <string>

namespace ember {

class huge_page_allocation_error : public std::runtime_error {
public:
	huge_page_allocation_error()
		: std::runtime_error("huge page creation failed") {}

	huge_page_allocation_error(std::string message) 
		: std::runtime_error(std::move(message)) {}
};

} // allocators, ember