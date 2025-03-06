/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Exception.h>
#include <spark/buffers/detail/SharedDefs.h>
#include <array>
#include <span>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io {

template<byte_type StorageType, std::size_t buf_size>
class StaticBuffer final {
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

	template<typename... T> 
	StaticBuffer(T&&... vals) : buffer_{ std::forward<T>(vals)... } {
		write_ = sizeof... (vals);
	}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, size_type length) {
		copy(destination, length);
		read_ += length;

		if(read_ == write_) {
			read_ = write_ = 0;
		}
	}

	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, size_type length) const {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));

		if(length > size()) {
			throw buffer_underrun(length, read_, size());
		}

		std::memcpy(destination, read_ptr(), length);
	}

	size_type find_first_of(value_type val) const noexcept {
		const auto data = read_ptr();

		for(std::size_t i = 0u; i < size(); ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}

	void skip(const size_type length) {
		read_ += length;

		if(read_ == write_) {
			read_ = write_ = 0;
		}
	}

	void advance_write(size_type bytes) {
		assert(free() >= bytes);
		write_ += bytes;
	}

	void clear() {
		read_ = write_ = 0;
	}

	/*
	 * Moves any unread data to the front of the buffer, freeing space at the end.
	 * If a move is performed, pointers obtained from read/write_ptr() will be invalidated.
	 * 
	 * Return true if additional space was made available.
	 */
	bool defragment() {
		if(read_ == 0) {
			return false;
		}

		const auto prev_size = size();
		std::memmove(buffer_.data(), read_ptr(), prev_size);
		read_ = 0;
		write_ = prev_size;
		return true;
	}

	value_type& operator[](const size_type index) {
		return read_ptr()[index];
	}

	const value_type& operator[](const size_type index) const {
		return read_ptr()[index];
	}

	[[nodiscard]]
	bool empty() const {
		return write_ == read_;
	}

	bool full() const {
		return write_ == capacity();
	}

	constexpr static bool can_write_seek() {
		return std::is_same<seeking, supported>::value;
	}

	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	void write(const void* source, size_type length) {
		assert(!region_overlap(source, length, buffer_.data(), buffer_.size()));

		if(free() < length) {
			throw buffer_overflow(length, write_, free());
		}

		std::memcpy(write_ptr(), source, length);
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

	constexpr static size_type capacity() {
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

	std::span<value_type> read_span() {
		return { read_ptr(), size() };
	}

	std::span<value_type> write_span() {
		return { write_ptr(), free() };
	}
};

} // io, spark, ember