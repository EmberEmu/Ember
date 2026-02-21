/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/Exception.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <cstddef>

namespace ember::commands {

class exception : public ember::exception {
public:
	exception()
		: ember::exception("An unknown command handling exception occurred") { }

	exception(std::string msg)
		: ember::exception(std::move(msg)) { };
};

class parse_error : public exception {
public:
	parse_error()
		: parse_error("An exception occurred during command argument parsing") { }

	explicit parse_error(std::string msg)
		: exception(std::move(msg)) { };
};

class invalid_type : public exception {
public:
	invalid_type()
		: invalid_type("Encountered an unexpected argument type") { }

	explicit invalid_type(std::string msg)
		: exception(std::move(msg)) { };
};

} // commands, ember