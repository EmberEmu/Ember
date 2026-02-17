/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <format>
#include <stdexcept>
#include <string>

namespace ember::connection_pool {

class exception : public std::runtime_error {
public:
	exception() : std::runtime_error("An unknown connection pool exception occurred!") { }
	exception(const std::string& msg) : std::runtime_error(msg) { };
};

class no_free_connections final : public exception {
public:
	no_free_connections() : exception("No more connections are available!") { }
	no_free_connections(const std::string& msg) : exception(msg) { };
};

class active_connections final : public exception {
public:
	active_connections(int active)
		: exception(std::format("Attempted to close the pool with {} connection(s) still in use!", active)) {}
	active_connections(const std::string& msg) : exception(msg) { };
};

} // connection_pool, ember