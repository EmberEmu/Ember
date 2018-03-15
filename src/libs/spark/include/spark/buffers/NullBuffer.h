/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Buffer.h"
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::spark {

class NullBuffer final : public Buffer {
public:
	void read(void* destination, std::size_t length) {};
	void copy(void* destination, std::size_t length) const {};
	void skip(std::size_t length) {};
	void write(const void* source, std::size_t length) {};
	void reserve(std::size_t length) {};
	std::size_t size() const { return 0; };
	void clear() {};
	bool empty() { return true; };
	std::byte& operator[](const std::size_t index) {
		throw std::logic_error("Don't do this on a NullBuffer"); 
	}
};

} // spark, ember