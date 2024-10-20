/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Exception.h>
#include <stdexcept>
#include <string>
#include <cstddef>

namespace ember::spark::io {

class buffer_underrun final : public exception {
public:
	const std::size_t buff_size, read_size, total_read;

	buffer_underrun(std::size_t read_size, std::size_t total_read, std::size_t buff_size)
		: exception("Buffer underrun: " + std::to_string(read_size) + " byte read requested, buffer contains "
					+ std::to_string(buff_size) + " bytes and total bytes read was " + std::to_string(total_read)),
		buff_size(buff_size), read_size(read_size), total_read(total_read) {}
};

class buffer_overflow final : public exception {
public:
	const std::size_t free, write_size, total_write;

	buffer_overflow(std::size_t write_size, std::size_t total_write, std::size_t free)
		: exception("Buffer overflow: " + std::to_string(write_size) + " byte write requested, free space is "
					+ std::to_string(free) + " bytes and total bytes written was " + std::to_string(total_write)),
		free(free), write_size(write_size), total_write(total_write) {}
};

class stream_read_limit final : public exception {
public:
	const std::size_t read_limit, read_size, total_read;

	stream_read_limit(std::size_t read_size, std::size_t total_read, std::size_t read_limit)
		: exception("Read boundary exceeded: " + std::to_string(read_size) + " byte read requested, read limit was "
					+ std::to_string(read_limit) + " bytes and total bytes read was " + std::to_string(total_read)),
		read_limit(read_limit), read_size(read_size), total_read(total_read) {}
};

} // io, spark, ember
