/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/StreamBase.h>
#include <spark/buffers/BufferWrite.h>
#include <spark/buffers/Utility.h>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::spark::v2 {

template <typename T>
concept can_resize = 
	requires(T t) {
		{ t.resize( std::size_t() ) } -> std::same_as<void>;
};

template<byte_oriented buf_type>
class BufferAdaptor final {
	buf_type& buffer_;
	std::size_t read_;
	std::size_t write_;

public:
	using value_type = buf_type::value_type;

	BufferAdaptor(buf_type& buffer)
		: buffer_(buffer), read_(0), write_(0) {}

	void read(void* destination, std::size_t length) {
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	void copy(void* destination, std::size_t length) const {
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	void skip(std::size_t length) {
		read_ += length;
	}

	void write(const void* source, std::size_t length) requires(can_resize<buf_type>) {
		const auto min_req_size = write_ + length;

		// we don't use std::back_inserter so we can support seeks
		if(buffer_.size() < min_req_size) {
			buffer_.resize(min_req_size);
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}
	
	std::size_t size() const {
		return buffer_.size() - read_;
	}

	bool empty() const {
		return !(buffer_.size() - read_);
	}

	value_type& operator[](const std::size_t index) {
		return buffer_[index];
	}

	const value_type& operator[](const std::size_t index) const {
		return buffer_[index];
	}

	bool can_write_seek() const requires(can_resize<buf_type>) {
		return true;
	}

	void write_seek(const SeekDir direction, const std::size_t offset) requires(can_resize<buf_type>) {
		switch(direction) {
			case SeekDir::SD_BACK:
				write_ -= offset;
				break;
			case SeekDir::SD_FORWARD:
				write_ += offset;
				break;
			case SeekDir::SD_START:
				write_ = 0 + offset;
		}
	}

	const auto read_ptr() const {
		return buffer_.data() + read_;
	}
};

} // v2, spark, ember