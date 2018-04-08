/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Buffer.h"
#include <boost/assert.hpp>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::spark {

class VectorBufferAdaptor : public Buffer {
	std::vector<std::uint8_t>& buffer_;
	std::size_t read_;
	std::size_t write_;

public:
	VectorBufferAdaptor(std::vector<std::uint8_t>& buffer) : buffer_(buffer), read_(0), write_(0) {}

	void read(void* destination, std::size_t length) override {
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	void copy(void* destination, std::size_t length) const override {
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	void skip(std::size_t length) override {
		read_ += length;
	}

	void write(const void* source, std::size_t length) override {
		const auto min_req_size = write_ + length;

		// we don't use std::back_inserter so we can support seeks
		if(buffer_.size() < min_req_size) {
			buffer_.resize(min_req_size);
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}
	
	void reserve(std::size_t length) override {
		buffer_.reserve(length);
	}

	std::size_t size() const override {
		return buffer_.size() - read_;
	}

	void clear() override {
		buffer_.clear();
	}

	bool empty() override {
		return !(buffer_.size() - read_);
	}

	std::byte& operator[](const std::size_t index) override {
		return reinterpret_cast<std::byte&>(buffer_[index]);
	}

	//bool can_write_seek() /*override - todo*/ {
	//	return true;
	//}

	//void write_seek(int val, SeekMode mode) /*override - todo*/ {
	//	switch(mode) {
	//		case SeekMode::SM_OFFSET:
	//			write_ += val;
	//			break;
	//		case SeekMode::SM_ABSOLUTE:
	//			write_ = val;
	//			break;
	//	}
	//}
};

} // spark, ember