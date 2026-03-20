/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BufferWrite.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <spark/buffers/Exception.h>
#include <ranges>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io::pmr {

template<byte_oriented buf_type>
requires std::ranges::contiguous_range<buf_type>
class BufferWriteAdaptor : public BufferWrite {
	buf_type& buffer_;

public:
	BufferWriteAdaptor(buf_type& buffer)
		: buffer_(buffer) {
		write_ = buffer.size();
	}

	BufferWriteAdaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer) {}

	void write(auto& source) {
		write(&source, sizeof(source));
	}

	void write(const void* source, std::size_t length) override {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) [[likely]] {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, std::size_t size) {
					return size;
				});
			} else if constexpr(has_resize<buf_type>) {
				buffer_.resize(min_req_size);
			} else {
				throw buffer_overflow(free(), length, write_);
			}
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	void reserve(const std::size_t length) override {
		if constexpr(has_reserve<buf_type>) {
			buffer_.reserve(length);
		}
	}

	bool can_write_seek() const override {
		return true;
	}

	void write_seek(const BufferSeek direction, const std::size_t offset) override {
		switch(direction) {
			case BufferSeek::sk_backward:
				write_ -= offset;
				break;
			case BufferSeek::sk_forward:
				write_ += offset;
				break;
			case BufferSeek::sk_absolute:
				write_ = offset;
		}
	}

	auto storage() const {
		return buffer_.data();
	}

	auto storage() {
		return buffer_.data();
	}

	auto write_ptr() {
		return buffer_.data() + write_;
	}

	auto write_ptr() const {
		return buffer_.data() + write_;
	}

	auto write_offset() const {
		return write_;
	}

	void clear() {
		write_ = 0;

		if constexpr(has_clear<buf_type>) {
			buffer_.clear();
		}
	}

	void advance_write(std::size_t bytes) {
		assert(buffer_.size() >= (write_ + bytes));
		write_ += bytes;
	}

	std::size_t free() const {
		return buffer_.size() - write_;
	}
};

} // pmr, io, spark, ember