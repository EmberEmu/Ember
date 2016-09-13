/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <cstddef>

namespace ember { namespace spark {

class exception : public std::runtime_error {
public:
	exception() : std::runtime_error("An unknown Spark exception occured!") { }
	exception(std::string msg) : std::runtime_error(msg) { };
};

class buffer_underrun : public exception {
public:
	const std::size_t buff_size, read_size;

	buffer_underrun(std::size_t read_size, std::size_t buff_size)
		: exception("Buffer underrun - " + std::to_string(read_size) + " byte read requested, buffer contains "
		            + std::to_string(buff_size) + " bytes"),
		            buff_size(buff_size), read_size(read_size) { }
};

}} //spark, ember