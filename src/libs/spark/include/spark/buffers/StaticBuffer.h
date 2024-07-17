/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdlib>

namespace ember::spark::io {

template<typename StorageType, std::size_t buf_size>
class StaticBuffer {
	std::array<StorageType, buf_size> buffer_;
	size_t read_ = 0;
	size_t write_ = 0;

public:
	using store_type      = typename decltype(buffer_);
	using size_type       = typename store_type::size_type;
	using value_type      = typename store_type::value_type;
	using pointer         = value_type*;
	using const_pointer   = const value_type*;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using contiguous      = is_contiguous;
	using seeking         = supported;

	static constexpr size_type npos = -1;
	
	size_type capacity() const {
		return buf_size;
	}

	size_type size() const {
		return buffer_.size() - read_;
	}

	size_type free() const {
		return buf_size - write_;
	}

	const value_type* data() const {
		return buffer_.data();
	}

	value_type* data() {
		return buffer_.data();
	}

	const value_type* read_ptr() const {
		return buffer_.data() + read_;
	}

	value_type* read_ptr() {
		return buffer_.data() + read_;
	}

	const value_type* write_ptr() const {
		return buffer_.data() + write_;
	}

	value_type* write_ptr() {
		return buffer_.data() + write_;
	}

	void read(void* destination, size_type length) {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	void copy(void* destination, size_type length) const {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	size_type find_first_of(value_type val) const noexcept {
		const auto data = buffer_.data() + read_;

		for(auto i = 0u; i < size(); ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}

	void skip(const size_type length) {
		read_ += length;
	}

	void advance_write(size_type bytes) {
		assert(write_ + bytes > write_ && write_ + bytes < size);
		write_ += bytes;
	}

	void clear() {
		read_ = write_ = 0;
	}

	void shift() {
		if(write_ == read_) {
			return;
		}

		std::memmove(buffer_.data(), read_ptr(), size());
	}

	value_type& operator[](const size_type index) {
		return buffer_[index];
	}

	const value_type& operator[](const size_type index) const {
		return buffer_[index];
	}

	bool empty() const {
		return write_ == read_;
	}

	consteval bool can_write_seek() const {
		return true;
	}

	void write(const void* source, size_type length) {
		assert(!region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) {
			throw buffer_overflow(length, write_, free());
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	void write_seek(const BufferSeek direction, const size_type offset) {
		switch(direction) {
			case BufferSeek::SK_BACKWARD:
				write_ -= offset;
				break;
			case BufferSeek::SK_FORWARD:
				write_ += offset;
				break;
			case BufferSeek::SK_ABSOLUTE:
				write_ = offset;
		}
	}
};

} // io, spark, ember