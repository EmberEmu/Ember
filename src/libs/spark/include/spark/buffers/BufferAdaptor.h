/*
 * Copyright (c) 2018 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <spark/buffers/Exception.h>
#include <ranges>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io {

using namespace detail;

template<byte_oriented buf_type, bool space_optimise = true>
requires std::ranges::contiguous_range<buf_type>
class BufferAdaptor final {
public:
	using value_type  = typename buf_type::value_type;
	using size_type   = typename buf_type::size_type;
	using offset_type = typename buf_type::size_type;
	using contiguous  = is_contiguous;

	static constexpr auto npos { static_cast<size_type>(-1) };

private:
	buf_type& buffer_;
	size_type read_;
	size_type write_;

public:
	BufferAdaptor(buf_type& buffer)
		: buffer_(buffer),
		  read_(0),
		  write_(buffer.size()) {}

	BufferAdaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer),
		  read_(0),
		  write_(0) {}

	BufferAdaptor(BufferAdaptor&& rhs) = delete;
	BufferAdaptor& operator=(BufferAdaptor&&) = delete;
	BufferAdaptor& operator=(const BufferAdaptor&) = delete;
	BufferAdaptor(const BufferAdaptor&) = delete;

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, size_type length) {
		assert(destination);
		copy(destination, length);
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, size_type length) const {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, read_ptr(), length);
	}

	void skip(size_type length) {
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	void write(const void* source, size_type length) {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) [[likely]] {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, size_type size) {
					return size;
				});
			} else if constexpr(has_resize<buf_type>) {
				buffer_.resize(min_req_size);
			} else {
				throw buffer_overflow(length, write_, free());
			}
		}

		std::memcpy(write_ptr(), source, length);
		write_ += length;
	}

	void reserve(const size_type length) {
		if constexpr(has_reserve<buf_type>) {
			buffer_.reserve(length);
		}
	}

	size_type find_first_of(value_type val) const {
		const auto data = read_ptr();

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}
	
	size_type size() const {
		return write_ - read_;
	}

	[[nodiscard]]
	bool empty() const {
		return read_ == write_;
	}

	value_type& operator[](const size_type index) {
		return read_ptr()[index];
	}

	const value_type& operator[](const size_type index) const {
		return read_ptr()[index];
	}

	constexpr static bool can_write_seek() {
		return seekable<BufferAdaptor>;
	}

	void write_seek(const BufferSeek direction, const offset_type offset) {
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

	auto read_ptr() const {
		return buffer_.data() + read_;
	}

	auto read_ptr() {
		return buffer_.data() + read_;
	}

	auto write_ptr() const {
		return buffer_.data() + write_;
	}

	auto write_ptr() {
		return buffer_.data() + write_;
	}

	auto data() const {
		return buffer_.data() + read_;
	}

	auto data() {
		return buffer_.data() + read_;
	}

	auto storage() const {
		return buffer_.data();
	}

	auto storage() {
		return buffer_.data();
	}

	void advance_write(size_type bytes) {
		assert(buffer_.size() >= (write_ + bytes));
		write_ += bytes;
	}

	auto free() const {
		return buffer_.size() - write_;
	}

	void clear() {
		read_ = write_ = 0;
	}
};

} // io, spark, ember
