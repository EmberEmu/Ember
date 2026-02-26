/*
 * Copyright (c) 2021 - 2026 Ember
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
#include <spark/buffers/StringAdaptors.h>
#include <ranges>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ember::spark::io::pmr {

using namespace detail;

#define STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)            \
	if(state() != StreamState::ok) [[unlikely]] {                \
		return ret_var;                                           \
	}                                                             \
                                                                  \
	enforce_read_bounds(read_size);                               \
	                                                              \
	if(!allow_throw()) {                                          \
		if(state() != StreamState::ok) [[unlikely]] {            \
			return ret_var;                                       \
		}                                                         \
	}

#define SAFE_READ(dest, read_size, ret_var)                       \
	STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)                \
	buffer_.read(dest, read_size);


class BinaryStreamReader : virtual public StreamBase {
	BufferRead& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;

	inline void enforce_read_bounds(const std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			set_state(StreamState::buffer_limit_error);

			if(allow_throw()) {
				throw buffer_underrun(read_size, total_read_, buffer_.size());
			}

			return;
		}

		if(read_limit_) {
			const auto max_read_remaining = read_limit_ - total_read_;

			if(read_size > max_read_remaining) [[unlikely]] {
				set_state(StreamState::read_limit_error);

				if(allow_throw()) {
					throw stream_read_limit(read_size, total_read_, read_limit_);
				}

				return;
			}
		}

		total_read_ += read_size;
	}

	template<typename container_type, typename count_type>
	void read_container(container_type& container, const count_type count) {
		using c_value_type = typename container_type::value_type;

		if constexpr(!memcpy_read<container_type, BinaryStreamReader>) {
			container.clear();
		}

		if constexpr(has_reserve<container_type>) {
			container.reserve(count);
		}

		if constexpr(memcpy_read<container_type, BinaryStreamReader>) {
			container.resize(count);

			const auto bytes = count * sizeof(c_value_type);
			SAFE_READ(container.data(), bytes, void());
		} else {
			for(count_type i = 0; i < count; ++i) {
				c_value_type value;
				*this >> value;
				container.emplace_back(std::move(value));
			}
		}
	}

public:
	explicit BinaryStreamReader(BufferRead& source, std::size_t read_limit = 0)
		: StreamBase(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	explicit BinaryStreamReader(BufferRead& source, no_throw_t, std::size_t read_limit = 0)
		: StreamBase(source, false),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	BinaryStreamReader(BinaryStreamReader&& rhs) noexcept
		: StreamBase(rhs),
		  buffer_(rhs.buffer_),
		  total_read_(rhs.total_read_),
		  read_limit_(rhs.read_limit_) {
		rhs.total_read_ = static_cast<std::size_t>(-1);
		rhs.set_state(StreamState::invalid_stream);
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

	template<std::integral prefix_type, typename endian_tag>
	BinaryStreamReader& operator>>(prefixed<std::string, prefix_type, endian_tag> adaptor) {
		prefix_type size = 0;
		*this >> size;
		endian::storage_out(size, adaptor.byte_order);

		if(state() != StreamState::ok) {
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}
	
	BinaryStreamReader& operator>>(prefixed_varint<std::string> adaptor) {
		const auto size = varint_decode<std::size_t>(*this);

		// if an error was triggered during decode, we shouldn't reach here
		if(state() != StreamState::ok) {
			std::unreachable();
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

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

		STREAM_READ_BOUNDS_ENFORCE(pos + 1, *this); // include null terminator

		adaptor->resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, pos);
			return size;
		});

		buffer_.skip(1); // skip null terminator
		return *this;
	}

	BinaryStreamReader& operator>>(std::string& data) {
		return (*this >> prefixed(data));
	}

	BinaryStreamReader& operator>>(has_shr_override<BinaryStreamReader> auto&& data) {
		return data.operator>>(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	BinaryStreamReader& operator>>(endian_func adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor.value), *this);
		adaptor.value = adaptor.from();
		return *this;
	}

	template<pod T>
	requires (!has_shr_override<T, BinaryStreamReader>)
	BinaryStreamReader& operator>>(T& data) {
		SAFE_READ(&data, sizeof(data), *this);
		return *this;
	}

	BinaryStreamReader& operator>>(pod auto& data) {
		read(&data, sizeof(data));
		return *this;
	}

	template<is_iterable type, std::integral prefix_type, typename endian_tag>
	requires (!std::is_same_v<std::decay_t<type>, std::string>
		&& !std::is_same_v<std::decay_t<type>, std::string_view>)
	BinaryStreamReader& operator>>(prefixed<type, prefix_type, endian_tag> adaptor) {
		prefix_type count = 0;
		*this >> count;
		endian::storage_out(count, adaptor.byte_order);

		read_container(adaptor.str, count);
		return *this;
	}

	template<is_iterable T, std::integral prefix_type>
	requires (!std::is_same_v<std::decay_t<T>, std::string>
		&& !std::is_same_v<std::decay_t<T>, std::string_view>)
	BinaryStreamReader& operator>>(prefixed_varint<T> adaptor) {
		const auto count = varint_decode<std::size_t>(*this);
		read_container(adaptor.str, count);
		return *this;
	}

	void get(std::string& dest) {
		*this >> dest;
	}

	void get(std::string& dest, std::size_t size) {
		STREAM_READ_BOUNDS_ENFORCE(size, void());

		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		SAFE_READ(dest, read_size, void());
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
		SAFE_READ(dest.data(), read_size, void());
	}

	template<arithmetic T>
	T get() {
		T t{};
		SAFE_READ(&t, sizeof(T), t);
		return t;
	}

	void get(arithmetic auto& dest) {
		SAFE_READ(&dest, sizeof(dest), void());
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor.value), void());
		adaptor.value = adaptor.from();
	}

	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		SAFE_READ(&t, sizeof(T), t);
		return endian::convert<conversion>(t);
	}

	/**  Misc functions **/ 

	void skip(std::size_t count) {
		STREAM_READ_BOUNDS_ENFORCE(count, void());
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

#undef SAFE_READ
#undef STREAM_READ_BOUNDS_ENFORCE

} // pmr, io, spark, ember
