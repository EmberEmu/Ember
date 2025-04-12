/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Exception.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <array>
#include <span>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io {

using namespace detail;

template<byte_type storage_type, std::size_t buf_size>
class StaticBuffer final {
	std::array<storage_type, buf_size> buffer_ = {};
	std::size_t read_ = 0;
	std::size_t write_ = 0;

public:
	using size_type       = typename decltype(buffer_)::size_type;
	using offset_type     = size_type;
	using value_type      = storage_type;
	using contiguous      = is_contiguous;

	static constexpr auto npos { static_cast<size_type>(-1) };
	
	StaticBuffer() = default;

	template<typename... T> 
	StaticBuffer(T&&... vals) : buffer_{ std::forward<T>(vals)... } {
		write_ = sizeof... (vals);
	}

	StaticBuffer(StaticBuffer&& rhs) = default;
	StaticBuffer& operator=(StaticBuffer&&) = default;
	StaticBuffer& operator=(const StaticBuffer&) = default;
	StaticBuffer(const StaticBuffer&) = default;

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

		for(size_type i = 0, j = size(); i < j; ++i) {
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

	void resize(size_type size) {
		if(size > buffer_.size()) {
			throw exception("attempted to resize StaticBuffer to larger than capacity");
		}

		write_ = size;
	}

	void clear() {
		read_ = write_ = 0;
	}

	bool defragment() {
		if(read_ == 0) {
			return false;
		}

		write_ = size();
		std::memmove(buffer_.data(), read_ptr(), write_);
		read_ = 0;
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
		return seekable<StaticBuffer>;
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

	auto begin() const {
		return buffer_.begin() + read_;
	}

	auto end() {
		return buffer_.begin() + write_;
	}

	auto end() const {
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
		return read_ptr();
	}

	value_type* data() {
		return read_ptr();
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

	std::span<const value_type> read_span() const {
		return { read_ptr(), size() };
	}

	std::span<value_type> write_span() {
		return { write_ptr(), free() };
	}
};

} // io, spark, ember
