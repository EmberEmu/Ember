/*
 * Copyright (c) 2018 - 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Buffer.h>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::spark {

class NullBuffer final : public Buffer {
public:
	void read(void* destination, std::size_t length) override {};
	void copy(void* destination, std::size_t length) const override {};
	void skip(std::size_t length) override {};
	void write(const void* source, std::size_t length) override {};
	void reserve(std::size_t length) override {};
	std::size_t size() const override{ return 0; };
	void clear() override {};
	bool empty() const override { return true; };
	bool can_write_seek() const override { return false; }

	void write_seek(SeekDir direction, std::size_t offset = 0) override {
		throw std::logic_error("Don't do this on a NullBuffer"); 
	};

	std::byte& operator[](const std::size_t index) {
		throw std::logic_error("Don't do this on a NullBuffer"); 
	}
};

} // spark, ember