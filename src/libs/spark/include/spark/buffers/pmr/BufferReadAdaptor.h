/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BufferRead.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io::pmr {

template<byte_oriented buf_type>
requires std::ranges::contiguous_range<buf_type>
class BufferReadAdaptor : public BufferRead {
	buf_type& buffer_;

public:
	BufferReadAdaptor(buf_type& buffer)
		: buffer_(buffer) {
		write_ = buffer_.size();
	}

	BufferReadAdaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer) {}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, std::size_t length) override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, std::size_t length) const override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	void skip(std::size_t length) override {
		read_ += length;
	}

	std::size_t size() const override {
		return write_ - read_;
	}

	[[nodiscard]]
	bool empty() const override {
		return read_ == write_;
	}

	const std::byte& operator[](const std::size_t index) const override {
		return reinterpret_cast<const std::byte*>(buffer_.data() + read_)[index];
	}

	auto read_ptr() const {
		return buffer_.data() + read_;
	}

	auto read_offset() const {
		return read_;
	}

	std::size_t find_first_of(std::byte val) const override {
		const auto data = read_ptr();

		for(std::size_t i = 0, j = size(); i < j; ++i) {
			if(static_cast<std::byte>(data[i]) == val) {
				return i;
			}
		}

		return npos;
	}

	void clear() {
		read_ = 0;

		if constexpr(has_clear<buf_type>) {
			buffer_.clear();
		}
	}
};

} // pmr, io, spark, ember
