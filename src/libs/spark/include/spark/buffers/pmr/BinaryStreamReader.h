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
#include <spark/buffers/Exception.h>
#include <spark/buffers/Endian.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <algorithm>
#include <ranges>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace ember::spark::io::pmr {

using namespace detail;

class BinaryStreamReader : virtual public StreamBase {
	BufferRead& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;

	void check_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			set_state(StreamState::BUFF_LIMIT_ERR);
			throw buffer_underrun(read_size, total_read_, buffer_.size());
		}

		const auto max_read_remaining = read_limit_ - total_read_;

		if(read_limit_ && read_size > max_read_remaining) [[unlikely]] {
			set_state(StreamState::READ_LIMIT_ERR);
			throw stream_read_limit(read_size, total_read_, read_limit_);
		}

		total_read_ += read_size;
	}

public:
	explicit BinaryStreamReader(BufferRead& source, std::size_t read_limit = 0)
		: StreamBase(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	BinaryStreamReader(BinaryStreamReader&& rhs) = delete;
	BinaryStreamReader& operator=(BinaryStreamReader&&) = delete;
	BinaryStreamReader& operator=(const BinaryStreamReader&) = delete;
	BinaryStreamReader(const BinaryStreamReader&) = delete;

	// terminates when it hits a null byte, empty string if none found
	BinaryStreamReader& operator>>(std::string& dest) {
		check_read_bounds(1); // just to prevent trying to read from an empty buffer
		auto pos = buffer_.find_first_of(std::byte(0));

		if(pos == BufferRead::npos) {
			dest.clear();
			return *this;
		}

		dest.resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			total_read_ += size;
			buffer_.read(strbuf, size);
			return size;
		});

		buffer_.skip(1); // skip null term
		return *this;
	}

	BinaryStreamReader& operator>>(has_shr_override<BinaryStreamReader> auto&& data) {
		return data.operator>>(*this);
	}

	template<pod T>
	requires (!has_shr_override<T, BinaryStreamReader>)
	BinaryStreamReader& operator>>(T& data) {
		check_read_bounds(sizeof(data));
		buffer_.read(&data, sizeof(data));
		return *this;
	}

	BinaryStreamReader& operator>>(pod auto& data) {
		check_read_bounds(sizeof(data));
		buffer_.read(&data, sizeof(data));
		return *this;
	}

	void get(std::string& dest) {
		*this >> dest;
	}

	void get(std::string& dest, std::size_t size) {
		check_read_bounds(size);
		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		check_read_bounds(read_size);
		buffer_.read(dest, read_size);
	}

	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(range::value_type);
		check_read_bounds(read_size);
		buffer_.read(dest.data(), read_size);
	}

	template<arithmetic T>
	T get() {
		check_read_bounds(sizeof(T));
		T t{};
		buffer_.read(&t, sizeof(T));
		return t;
	}

	void get(arithmetic auto& dest) {
		check_read_bounds(sizeof(dest));
		buffer_.read(&dest, sizeof(dest));
	}

	template<endian::Conversion conversion>
	void get(arithmetic auto& dest) {
		check_read_bounds(sizeof(dest));
		buffer_.read(&dest, sizeof(dest));
		dest = endian::convert<conversion>(dest);
	}

	template<arithmetic T, endian::Conversion conversion>
	T get() {
		check_read_bounds(sizeof(T));
		T t{};
		buffer_.read(&t, sizeof(T));
		return endian::convert<conversion>(t);
	}

	/**  Misc functions **/ 

	void skip(std::size_t count) {
		check_read_bounds(count);
		buffer_.skip(count);
	}

	std::size_t total_read() const {
		return total_read_;
	}

	std::size_t read_limit() const {
		return read_limit_;
	}

	BufferRead* buffer() const {
		return &buffer_;
	}
};

} // pmr, io, spark, ember
