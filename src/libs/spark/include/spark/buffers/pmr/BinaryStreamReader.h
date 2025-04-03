/*
 * Copyright (c) 2021 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/StreamBase.h>
#include <spark/buffers/pmr/BufferRead.h>
#include <spark/buffers/Concepts.h>
#include <spark/buffers/Endian.h>
#include <spark/buffers/Exception.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/StreamAdaptors.h>
#include <ranges>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ember::spark::io::pmr {

using namespace detail;

class BinaryStreamReader : virtual public StreamBase {
	BufferRead& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;

	void enforce_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			set_state(StreamState::BUFF_LIMIT_ERR);
			throw buffer_underrun(read_size, total_read_, buffer_.size());
		}

		if(read_limit_) {
			const auto max_read_remaining = read_limit_ - total_read_;

			if(read_size > max_read_remaining) [[unlikely]] {
				set_state(StreamState::READ_LIMIT_ERR);
				throw stream_read_limit(read_size, total_read_, read_limit_);
			}
		}

		total_read_ += read_size;
	}

	inline void read(void* dest, const std::size_t size) {
		if(state() == StreamState::OK) [[likely]] {
			enforce_read_bounds(size);
			buffer_.read(dest, size);
		}
	}

public:
	explicit BinaryStreamReader(BufferRead& source, std::size_t read_limit = 0)
		: StreamBase(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	BinaryStreamReader(BinaryStreamReader&& rhs) noexcept
		: StreamBase(rhs),
		  buffer_(rhs.buffer_), 
		  total_read_(rhs.total_read_),
		  read_limit_(rhs.read_limit_) {
		rhs.total_read_ = static_cast<std::size_t>(-1);
		rhs.set_state(StreamState::INVALID_STREAM);
	}

	BinaryStreamReader& operator=(BinaryStreamReader&&) = delete;
	BinaryStreamReader& operator=(const BinaryStreamReader&) = delete;
	BinaryStreamReader(const BinaryStreamReader&) = delete;

	void deserialise(auto& object) {
		stream_read_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	template<typename T>
	requires has_serialise<T, stream_read_adaptor<BinaryStreamReader>>
	BinaryStreamReader& operator>>(T& data) {
		deserialise(data);
		return *this;
	}

	BinaryStreamReader& operator>>(prefixed<std::string> adaptor) {
		std::uint32_t size = 0;
		*this >> endian::le(size);

		if(state() != StreamState::OK) {
			return *this;
		}

		enforce_read_bounds(size);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}
	
	BinaryStreamReader& operator>>(prefixed_varint<std::string> adaptor) {
		const auto size = varint_decode<std::size_t>(*this);

		// if an error was triggered during decode, we shouldn't reach here
		if(state() != StreamState::OK) {
			std::unreachable();
		}

		enforce_read_bounds(size);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	BinaryStreamReader& operator>>(null_terminated<std::string> adaptor) {
		auto pos = buffer_.find_first_of(std::byte{0});

		if(pos == buffer_.npos) {
			adaptor->clear();
			return *this;
		}

		enforce_read_bounds(pos + 1); // include null terminator

		adaptor->resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, pos);
			return size;
		});

		buffer_.skip(1); // skip null terminator
		return *this;
	}

	BinaryStreamReader& operator >>(std::string& data) {
		return (*this >> prefixed(data));
	}

	BinaryStreamReader& operator>>(has_shr_override<BinaryStreamReader> auto&& data) {
		return data.operator>>(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	BinaryStreamReader& operator>>(endian_func adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
		return *this;
	}

	template<pod T>
	requires (!has_shr_override<T, BinaryStreamReader>)
	BinaryStreamReader& operator>>(T& data) {
		read(&data, sizeof(data));
		return *this;
	}

	BinaryStreamReader& operator>>(pod auto& data) {
		read(&data, sizeof(data));
		return *this;
	}

	/**
	 * @brief Reads a string from the stream.
	 * 
	 * @param dest The destination string.
	 */
	void get(std::string& dest) {
		*this >> dest;
	}

	/**
	 * @brief Reads a fixed-length string from the stream.
	 * 
	 * @param dest The destination string.
	 * @param count The number of bytes to be read.
	 */
	void get(std::string& dest, std::size_t size) {
		enforce_read_bounds(size);

		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest The destination buffer.
	 * @param count The number of bytes to be read into the destination.
	 */
	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		read(dest, read_size);
	}

	/**
	 * @brief Read data from the stream to the destination represented by the iterators.
	 * 
	 * @param begin The beginning iterator.
	 * @param end The end iterator.
	 */
	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest A contiguous range into which the data should be read.
	 */
	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(typename range::value_type);
		read(dest.data(), read_size);
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return t;
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	void get(arithmetic auto& dest) {
		read(&dest, sizeof(dest));
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @param The destination for the read value.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return endian::convert<conversion>(t);
	}

	/**  Misc functions **/ 

	/**
	 * @brief Skip over count bytes
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t count) {
		enforce_read_bounds(count);
		buffer_.skip(count);
	}

	/**
	 * @return The total number of bytes read from the stream.
	 */
	std::size_t total_read() const {
		return total_read_;
	}

	/**
	 * @return If provided to the constructor, the upper limit on how much data
	 * can be read from this stream before an error is triggers.
	 */
	std::size_t read_limit() const {
		return read_limit_;
	}

	/**
	 * @brief Determine the maximum number of bytes that can be
	 * safely read from this stream.
	 * 
	 * The value returned may be lower than the amount of data
	 * available in the buffer if a read limit was set during
	 * the stream's construction.
	 * 
	 * @return The number of bytes available for reading.
	 */
	std::size_t read_max() const {
		if(read_limit_) {
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	/**
	 * @return Pointer to stream's underlying buffer.
	 */
	BufferRead* buffer() const {
		return &buffer_;
	}
};

} // pmr, io, spark, ember
