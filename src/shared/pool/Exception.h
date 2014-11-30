/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stdexcept>
#include <string>
 
namespace ember { namespace connection_pool {

class exception : public std::runtime_error {
public:
	exception() : std::runtime_error("An unknown connection pool exception occured!") { }
	exception(std::string msg) : std::runtime_error(msg) { };
};

class no_free_connections : public exception {
public:
	no_free_connections() : exception("No more connections are available!") { }
	no_free_connections(std::string msg) : exception(msg) { };
};

}} //connection_pool, ember