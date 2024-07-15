/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/SharedDefs.h>
#include <algorithm>
#include <ranges>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::v2 {

template <typename T>
concept can_resize = 
	requires(T t) {
		{ t.resize( std::size_t() ) } -> std::same_as<void>;
};

template<byte_oriented buf_type, bool space_optimise = false>
requires std::ranges::contiguous_range<buf_type>
class BufferAdaptor final {
public:
	using value_type = typename buf_type::value_type;
	using size_type  = typename buf_type::size_type;
	using contiguous = is_contiguous;

	static constexpr size_type npos = -1;

private:
	buf_type& buffer_;
	size_type read_;
	size_type write_;

public:
	BufferAdaptor(buf_type& buffer)
		: buffer_(buffer), read_(0), write_(buffer.size()) {}

	void read(void* destination, size_type length) {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	void copy(void* destination, size_type length) const {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	void skip(size_type length) {
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	void write(const void* source, size_type length) requires(can_resize<buf_type>) {
		assert(!region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		// we don't use std::back_inserter so we can support seeks
		if(buffer_.size() < min_req_size) {
			buffer_.resize(min_req_size);
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	size_type find_first_of(value_type val) const {
		const auto data = buffer_.data() + read_;

		for(auto i = 0u; i < size(); ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}
	
	size_type size() const {
		return buffer_.size() - read_;
	}

	bool empty() const {
		return !(buffer_.size() - read_);
	}

	value_type& operator[](const size_type index) {
		return buffer_[index];
	}

	const value_type& operator[](const size_type index) const {
		return buffer_[index];
	}

	bool can_write_seek() const requires(can_resize<buf_type>) {
		return true;
	}

	void write_seek(const BufferSeek direction, const size_type offset) requires(can_resize<buf_type>) {
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

	const auto read_ptr() const {
		return buffer_.data() + read_;
	}
};

} // v2, spark, ember