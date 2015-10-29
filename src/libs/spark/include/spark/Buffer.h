/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "BufferChainNode.h"
#include <boost/assert.hpp>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember { namespace spark {

class Buffer {
public:
	virtual ~Buffer() = default;
	virtual void read(void* destination, std::size_t length) = 0;
	virtual void copy(void* destination, std::size_t length) const = 0;
	virtual	void skip(std::size_t length) = 0;
	virtual void write(const void* source, std::size_t length) = 0;
	virtual void reserve(std::size_t length) = 0;
	virtual std::size_t size() const = 0;
	virtual void clear() = 0;
};

}} // spark, ember