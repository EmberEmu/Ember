/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Exception.h>
#include <format>
#include <stdexcept>
#include <utility>
#include <cstddef>

namespace ember::spark::io {

class buffer_underrun final : public exception {
public:
	const std::size_t buff_size, read_size, total_read;

	buffer_underrun(std::size_t read_size, std::size_t total_read, std::size_t buff_size)
		: exception(std::format(
			"Buffer underrun: {} byte read requested, buffer contains {} bytes and total bytes read was {}",
			read_size, buff_size, total_read)),
		buff_size(buff_size), read_size(read_size), total_read(total_read) {}
};

class buffer_overflow final : public exception {
public:
	const std::size_t free, write_size, total_write;

	buffer_overflow(std::size_t write_size, std::size_t total_write, std::size_t free)
		: exception(std::format(
			"Buffer overflow: {} byte write requested, free space is {} bytes and total bytes written was {}",
			write_size, free, total_write)),
		free(free), write_size(write_size), total_write(total_write) {}
};

class stream_read_limit final : public exception {
public:
	const std::size_t read_limit, read_size, total_read;

	stream_read_limit(std::size_t read_size, std::size_t total_read, std::size_t read_limit)
		: exception(std::format(
			"Read boundary exceeded: {} byte read requested, read limit was {} bytes and total bytes read was {}",
			read_size, read_limit, total_read)),
		read_limit(read_limit), read_size(read_size), total_read(total_read) {}
};

} // io, spark, ember
