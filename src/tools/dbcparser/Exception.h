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
 
namespace ember { namespace dbc {

class exception : public std::runtime_error {
public:
	exception() : std::runtime_error("An unknown DBC error occured!") { }
	explicit exception(const std::string& msg) : std::runtime_error(msg) { };
};

class parse_error : public exception {
public:
	parse_error() : exception("An unknown parsing error occured!") { }
	parse_error(const std::string& file, const std::string& msg) : exception(file + ": " + msg) { };
};

}} //dbc, ember