/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Buffer.h>
#include <spark/buffers/StreamBase.h>
#include <spark/Exception.h>
#include <algorithm>
#include <concepts>
#include <string>
#include <type_traits>
#include <cstddef>
#include <cstring>

namespace ember::spark {

class BinaryInStream final {
private:
	Buffer& buffer_;
	std::size_t total_write_;

public:
	explicit BinaryInStream(Buffer& source) : buffer_(source), total_write_(0) {}

	BinaryInStream& operator <<(const trivially_copyable auto& data) {
		buffer_.write(reinterpret_cast<const char*>(&data), sizeof(data));
		total_write_ += sizeof(data);
		return *this;
	}

	BinaryInStream& operator <<(const std::string& data) {
		buffer_.write(data.data(), data.size());
		const char term = '\0';
		buffer_.write(&term, 1);
		total_write_ += (data.size() + 1);
		return *this;
	}

	BinaryInStream& operator <<(const char* data) {
		const auto len = std::strlen(data);
		buffer_.write(data, len);
		total_write_ += len;
		return *this;
	}

	template<typename T>
	void put(const T* data, std::size_t count) {
		const auto write_size = count * sizeof(T);
		buffer_.write(data, write_size);
		total_write_ += write_size;
	}

	template<typename It>
	void put(It begin, const It end) {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}

	/**  Misc functions **/ 

	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	void write_seek(SeekDir direction, std::size_t offset = 0) {
		buffer_.write_seek(direction, offset);
	}

	std::size_t size() const {
		return buffer_.size();
	}

	void clear() {
		buffer_.clear();
	}

	bool empty() {
		return buffer_.empty();
	}

	std::size_t total_write() {
		return total_write_;
	}

	Buffer* buffer() {
		return &buffer_;
	}
};

} // spark, ember
