/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferIn.h>
#include <boost/assert.hpp>
#include <span>
#include <stdexcept>
#include <utility>
#include <cstddef>

namespace ember::spark {

// todo, byte concept 
template<byte_oriented buf_type>
class SpanBufferAdaptor final : public BufferIn {
	std::span<buf_type> buffer_;
	std::size_t read_;

public:
	SpanBufferAdaptor(std::span<buf_type> buffer) : buffer_(buffer), read_(0) {}

	void read(void* destination, std::size_t length) override {
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	void copy(void* destination, std::size_t length) const override {
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	void skip(std::size_t length) override {
		read_ += length;
	}

	std::size_t size() const override {
		return buffer_.size() - read_;
	}

	bool empty() const override {
		return !(buffer_.size() - read_);
	}

	std::byte& operator[](const std::size_t index) override {
		throw std::logic_error("Unsupported operation");
	}
};

} // spark, ember