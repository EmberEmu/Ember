/*
 * Copyright (c) 2018 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferOut.h>
#include <stdexcept>
#include <utility>
#include <cstddef>

namespace ember::spark {

class NullBuffer final : public BufferOut {
public:
	void write(const void* source, std::size_t length) override {};
	void reserve(std::size_t length) override {};
	std::size_t size() const override{ return 0; };
	bool empty() const override { return true; };
	bool can_write_seek() const override { return false; }

	void write_seek(SeekDir direction, std::size_t offset = 0) override {
		throw std::logic_error("Don't do this on a NullBuffer"); 
	};

	std::byte& operator[](const std::size_t index) override {
		throw std::logic_error("Don't do this on a NullBuffer"); 
	}
};

} // spark, ember