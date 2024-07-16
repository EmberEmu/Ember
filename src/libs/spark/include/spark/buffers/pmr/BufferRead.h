/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BufferBase.h>
#include <cstddef>

namespace ember::spark::io::pmr {

class BufferRead : virtual public BufferBase {
public:
	constexpr static std::size_t npos = -1;

	virtual ~BufferRead() = default;
	virtual void read(void* destination, std::size_t length) = 0;
	virtual void copy(void* destination, std::size_t length) const = 0;
	virtual	void skip(std::size_t length) = 0;
	virtual const std::byte& operator[](const std::size_t index) const = 0;
	virtual std::size_t find_first_of(std::byte val) const = 0;
};

} // pmr, io, spark, ember