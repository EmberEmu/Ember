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
#include <spark/buffers/Concepts.h>
#include <spark/buffers/Endian.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/StreamAdaptors.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ember::spark::io::pmr {

using namespace detail;

class BinaryStreamWriter : virtual public StreamBase {
	BufferWrite& buffer_;
	std::size_t total_write_;

	inline void write(const void* data, const std::size_t size) {
		if(state() == StreamState::OK) [[likely]] {
			buffer_.write(data, size);
			total_write_ += size;
		}
	}

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

	void serialise(auto&& object) {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	BinaryStreamWriter& operator<<(has_shl_override<BinaryStreamWriter> auto&& data) {
		return data.operator<<(*this);
	}

	template<typename T>
	requires has_serialise<T, stream_write_adaptor<BinaryStreamWriter>>
	BinaryStreamWriter& operator<<(T& data) {
		serialise(data);
		return *this;
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	BinaryStreamWriter& operator<<(endian_func adaptor) {
		const auto converted = adaptor.to();
		write(&converted, sizeof(converted));
		return *this;
	}

	template<pod T>
	requires (!has_shl_override<T, BinaryStreamWriter>)
	BinaryStreamWriter& operator<<(const T& data) {
		write(&data, sizeof(data));
		return *this;
	}

	template<typename T>
	BinaryStreamWriter& operator<<(prefixed<T> adaptor) {
		auto size = static_cast<std::uint32_t>(adaptor->size());
		endian::native_to_little_inplace(size);
		write(&size, sizeof(size));
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	BinaryStreamWriter& operator<<(prefixed_varint<T> adaptor) {
		varint_encode(*this, adaptor->size());
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	BinaryStreamWriter& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size());
		const char terminator = '\0';
		write(&terminator, 1);
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	BinaryStreamWriter& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		return *this;
	}

	template<typename T>
	BinaryStreamWriter& operator<<(raw<T> adaptor) {
		write(adaptor->data(), adaptor->size());
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
		write(data, len + 1); // include terminator
		return *this;
	}

	template<std::ranges::contiguous_range range>
	requires pod<typename range::value_type>
	BinaryStreamWriter& operator <<(const range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
		return *this;
	}

	template<is_iterable T>
	requires (!pod<typename T::value_type> || !std::ranges::contiguous_range<T>)
	BinaryStreamWriter& operator<<(T& data) {
		for(auto& element : data) {
			*this << element;
		}

		return *this;
	}

	/**
	 * @brief Writes a contiguous range to the stream.
	 * 
	 * @param data The contiguous range to be written to the stream.
	 */
	template<std::ranges::contiguous_range range>
	void put(range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
	}

	/**
	 * @brief Writes a the provided value to the stream.
	 * 
	 * @param data The value to be written to the stream.
	 */
	void put(const arithmetic auto& data) {
		write(&data, sizeof(data));
	}

	/**
	 * @brief Writes data to the stream.
	 * 
	 * @param data The element to be written to the stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	/**
	 * @brief Writes count elements from the provided buffer to the stream.
	 * 
	 * @param data Pointer to the buffer from which data will be copied to the stream.
	 * @param count The number of elements to write.
	 */
	template<pod T>
	void put(const T* data, std::size_t count) {
		assert(data);
		const auto write_size = count * sizeof(T);
		write(data, write_size);
	}

	/**
	 * @brief Writes the data from the iterator range to the stream.
	 * 
	 * @param begin Iterator to the beginning of the data.
	 * @param end Iterator to the end of the data.
	 */
	template<typename It>
	void put(It begin, const It end) {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}
	/**
	 * @brief Allows for writing a provided byte value a specified number of times to
	 * the stream.
	 * 
	 * @param The byte value that will fill the specified number of bytes.
	 */
	template<std::size_t size>
	void fill(const std::uint8_t value) {
		const auto filled = generate_filled<size>(value);
		write(filled.data(), filled.size());
	}

	/**  Misc functions **/ 

	/**
	 * @brief Determines whether this container can write seek.
	 * 
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const StreamSeek direction, const std::size_t offset) {
		if(direction == StreamSeek::SK_STREAM_ABSOLUTE) {
			if(offset >= total_write_) {
				buffer_.write_seek(BufferSeek::SK_FORWARD, offset - total_write_);
			} else {
				buffer_.write_seek(BufferSeek::SK_BACKWARD, total_write_ - offset);
			}

			total_write_ = offset;
		} else {
			buffer_.write_seek(static_cast<BufferSeek>(direction), offset);
		}
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const {
		return buffer_.size();
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	/**
	 * @return The total number of bytes written to the stream.
	 */
	std::size_t total_write() const {
		return total_write_;
	}

	/** 
	 * @brief Get a pointer to the buffer.
	 *
	 * @return Pointer to the underlying buffer. 
	 */
	BufferWrite* buffer() {
		return &buffer_;
	}

	/** 
	 * @brief Get a pointer to the buffer.
	 *
	 * @return Pointer to the underlying buffer. 
	 */
	const BufferWrite* buffer() const {
		return &buffer_;
	}
};

} // pmr, io, spark, ember
