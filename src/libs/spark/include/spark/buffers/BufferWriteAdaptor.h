/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <spark/buffers/BufferWrite.h>
#include <spark/buffers/Utility.h>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::spark {

template<byte_oriented buf_type>
class BufferWriteAdaptor : public BufferWrite {
	buf_type& buffer_;
	std::size_t write_;

public:
	BufferWriteAdaptor(buf_type& buffer) : buffer_(buffer), write_(0) {}

	void write(const void* source, std::size_t length) override {
		const auto min_req_size = write_ + length;

		// we don't use std::back_inserter so we can support seeks
		if(buffer_.size() < min_req_size) {
			buffer_.resize(min_req_size);
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	void reserve(const std::size_t length) override {
		buffer_.reserve(length);
	}

	std::byte& operator[](const std::size_t index) override {
		return reinterpret_cast<std::byte&>(buffer_[index]);
	}

	bool can_write_seek() const override {
		return true;
	}

	void write_seek(const SeekDir direction, const std::size_t offset) override {
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
};

} // spark, ember