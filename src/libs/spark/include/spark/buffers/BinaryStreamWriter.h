/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/StreamBase.h>
#include <spark/buffers/BufferWrite.h>
#include <spark/Exception.h>
#include <algorithm>
#include <array>
#include <concepts>
#include <string>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ember::spark {

class BinaryStreamWriter : virtual public StreamBase {
private:
	BufferWrite& buffer_;
	std::size_t total_write_;

	template<std::size_t size>
	constexpr auto generate_filled(const std::uint8_t value) {
		std::array<std::uint8_t, size> target{};

		for(std::size_t i = 0; i < size; ++i) {
			target[i] = value;
		}

		return target;
	}

public:
	explicit BinaryStreamWriter(BufferWrite& source)
		: StreamBase(source), buffer_(source), total_write_(0) {}

	BinaryStreamWriter& operator <<(const is_pod auto& data) {
		buffer_.write(&data, sizeof(data));
		total_write_ += sizeof(data);
		return *this;
	}

	BinaryStreamWriter& operator <<(const std::string& data) {
		buffer_.write(data.data(), data.size());
		const char term = '\0';
		buffer_.write(&term, 1);
		total_write_ += (data.size() + 1);
		return *this;
	}

	BinaryStreamWriter& operator <<(const char* data) {
		const auto len = std::strlen(data);
		buffer_.write(data, len);
		total_write_ += len;
		return *this;
	}

	template<std::ranges::contiguous_range range>
	void put(range& data) {
		const auto write_size = data.size() * sizeof(range::value_type);
		buffer_.write(data.data(), write_size);
		total_write_ += write_size;
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

	template<std::size_t size>
	void fill(const std::uint8_t value) {
		const auto filled = generate_filled<size>(value);
		buffer_.write(filled.data(), filled.size());
	}

	/**  Misc functions **/ 

	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	void write_seek(const SeekDir direction, const std::size_t offset) {
		buffer_.write_seek(direction, offset);
	}

	std::size_t size() const {
		return buffer_.size();
	}

	bool empty() {
		return buffer_.empty();
	}

	std::size_t total_write() {
		return total_write_;
	}

	BufferWrite* buffer() {
		return &buffer_;
	}
};

} // spark, ember
