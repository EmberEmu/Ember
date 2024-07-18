/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Exception.h>
#include <spark/buffers/SharedDefs.h>
#include <array>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io {

template<byte_type StorageType, std::size_t buf_size>
class StaticBuffer {
	std::array<StorageType, buf_size> buffer_ = {};
	std::size_t read_ = 0;
	std::size_t write_ = 0;

public:
	using size_type       = std::size_t;
	using value_type      = StorageType;
	using pointer         = value_type*;
	using const_pointer   = const value_type*;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using contiguous      = is_contiguous;
	using seeking         = supported;

	static constexpr size_type npos = -1;
	
	StaticBuffer() = default;

	template <typename... T> 
	StaticBuffer(T&&... vals) : buffer_{ std::forward<T>(vals)... } {
		write_ = sizeof... (vals);
	}

	size_type capacity() const {
		return buf_size;
	}

	size_type size() const {
		return write_ - read_;
	}

	size_type free() const {
		return buf_size - write_;
	}

	const value_type* data() const {
		return buffer_.data() + read_;
	}

	value_type* data() {
		return buffer_.data() + read_;
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

	value_type* storage() {
		return buffer_.data();
	}

	const value_type* storage() const {
		return buffer_.data();
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
		assert(free() >= bytes);
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
		return (buffer_.data() + read_)[index];
	}

	const value_type& operator[](const size_type index) const {
		return (buffer_.data() + read_)[index];
	}

	bool empty() const {
		return write_ == read_;
	}

	consteval bool can_write_seek() const {
		return true;
	}

	void write(const void* source, size_type length) {
		assert(!region_overlap(source, length, buffer_.data(), buffer_.size()));

		if(free() < length) {
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

	auto begin() {
		return buffer_.begin() + read_;
	}

	const auto begin() const {
		return buffer_.begin() + read_;
	}

	auto end() {
		return buffer_.begin() + write_;
	}

	const auto end() const {
		return buffer_.begin() + write_;
	}
};

} // io, spark, ember