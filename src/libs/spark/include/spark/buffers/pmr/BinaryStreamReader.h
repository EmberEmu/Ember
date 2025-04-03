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

	void get(std::string& dest) {
		*this >> dest;
	}

	void get(std::string& dest, std::size_t size) {
		enforce_read_bounds(size);

		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		read(dest, read_size);
	}

	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(typename range::value_type);
		read(dest.data(), read_size);
	}

	template<arithmetic T>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return t;
	}

	void get(arithmetic auto& dest) {
		read(&dest, sizeof(dest));
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
	}

	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return endian::convert<conversion>(t);
	}

	/**  Misc functions **/ 

	void skip(std::size_t count) {
		enforce_read_bounds(count);
		buffer_.skip(count);
	}

	std::size_t total_read() const {
		return total_read_;
	}

	std::size_t read_limit() const {
		return read_limit_;
	}

	std::size_t read_max() const {
		if(read_limit_) {
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	BufferRead* buffer() const {
		return &buffer_;
	}
};

} // pmr, io, spark, ember
