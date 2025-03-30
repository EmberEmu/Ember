/*
 * Copyright (c) 2021 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/StreamBase.h>
#include <spark/buffers/pmr/BufferWrite.h>
#include <spark/buffers/Endian.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <shared/utility/cstring_view.hpp>
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ember::spark::io::pmr {

using namespace detail;

class BinaryStreamWriter : virtual public StreamBase {
private:
	BufferWrite& buffer_;
	std::size_t total_write_;

public:
	explicit BinaryStreamWriter(BufferWrite& source)
		: StreamBase(source),
		  buffer_(source),
		  total_write_(0) {}

	BinaryStreamWriter(BinaryStreamWriter&& rhs) noexcept
		: StreamBase(rhs),
		  buffer_(rhs.buffer_), 
		  total_write_(rhs.total_write_) {
		 rhs.total_write_ = static_cast<std::size_t>(-1);
		 rhs.set_state(StreamState::INVALID_STREAM);
	}

	BinaryStreamWriter& operator=(BinaryStreamWriter&&) = delete;
	BinaryStreamWriter& operator=(const BinaryStreamWriter&) = delete;
	BinaryStreamWriter(const BinaryStreamWriter&) = delete;

	BinaryStreamWriter& operator<<(has_shl_override<BinaryStreamWriter> auto&& data) {
		return data.operator<<(*this);
	}

	template<pod T>
	requires (!has_shl_override<T, BinaryStreamWriter>)
	BinaryStreamWriter& operator<<(const T& data) {
		buffer_.write(&data, sizeof(data));
		total_write_ += sizeof(data);
		return *this;
	}

	template<typename T>
	BinaryStreamWriter& operator<<(prefixed<T> adaptor) {
		const auto size = endian::native_to_little(adaptor->size());
		buffer_.write(&size, sizeof(size));
		buffer_.write(adaptor->data(), adaptor->size());
		total_write_ += (adaptor->size()) + sizeof(adaptor->size());
		return *this;
	}

	template<typename T>
	BinaryStreamWriter& operator<<(prefixed_varint<T> adaptor) {
		const auto encode_len = varint_encode(*this, adaptor->size());
		buffer_.write(adaptor->data(), adaptor->size());
		total_write_ += (adaptor->size() + encode_len);
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	BinaryStreamWriter& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		buffer_.write(adaptor->data(), adaptor->size());
		const char terminator = '\0';
		buffer_.write(&terminator, 1);
		total_write_ += (adaptor->size() + 1);
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	BinaryStreamWriter& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		buffer_.write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		total_write_ += (adaptor->size() + 1);
		return *this;
	}

	template<typename T>
	BinaryStreamWriter& operator<<(raw<T> adaptor) {
		buffer_.write(adaptor->data(), adaptor->size());
		total_write_ += adaptor->size();
		return *this;
	}

	BinaryStreamWriter& operator<<(std::string_view string) {
		return (*this << prefixed(string));
	}

	BinaryStreamWriter& operator<<(const std::string& string) {
		return (*this << prefixed(string));
	}

	BinaryStreamWriter& operator<<(const char* data) {
		assert(data);
		const auto len = std::strlen(data);
		buffer_.write(data, len + 1); // include terminator
		total_write_ += len + 1;
		return *this;
	}

	BinaryStreamWriter& operator<<(cstring_view& data) {
		buffer_.write(data.data(), data.size() + 1);
		total_write_ += (data.size() + 1);
		return *this;
	}

	template<std::ranges::contiguous_range range>
	void put(range& data) {
		const auto write_size = data.size() * sizeof(range::value_type);
		buffer_.write(data.data(), write_size);
		total_write_ += write_size;
	}

	void put(const arithmetic auto& data) {
		buffer_.write(&data, sizeof(data));
		total_write_ += sizeof(data);
	}

	template<endian::Conversion conversion>
	void put(const arithmetic auto& data) {
		const auto swapped = endian::convert<conversion>(data);
		buffer_.write(&swapped, sizeof(data));
		total_write_ += sizeof(data);
	}

	template<pod T>
	void put(const T* data, std::size_t count) {
		assert(data);
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
		total_write_ += size;
	}

	/**  Misc functions **/ 

	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	void write_seek(const StreamSeek direction, const std::size_t offset) {
		if(direction == StreamSeek::SK_STREAM_ABSOLUTE) {
			buffer_.write_seek(BufferSeek::SK_BACKWARD, total_write_ - offset);
		} else {
			buffer_.write_seek(static_cast<BufferSeek>(direction), offset);
		}
	}

	std::size_t size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	std::size_t total_write() const {
		return total_write_;
	}

	BufferWrite* buffer() {
		return &buffer_;
	}

	const BufferWrite* buffer() const {
		return &buffer_;
	}
};

} // pmr, io, spark, ember
