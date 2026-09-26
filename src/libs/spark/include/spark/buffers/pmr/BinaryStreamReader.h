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
#include <concepts>
#include <limits>
#include <ranges>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ember::spark::io::pmr {

#ifndef MAX_OBJECTS_DESERIALISE
#define MAX_OBJECTS_DESERIALISE 1024
#endif // MAX_OBJECTS_DESERIALISE

#define STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)            \
	if(state() != StreamState::ok) [[unlikely]] {                 \
		return ret_var;                                           \
	}                                                             \
                                                                  \
	enforce_read_bounds(read_size);                               \
	                                                              \
	if(!allow_throw()) {                                          \
		if(state() != StreamState::ok) [[unlikely]] {             \
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
		if(const auto max = size(); read_size > max) [[unlikely]] {
			if(read_limit_) {
				set_state(StreamState::read_limit_error);

				if(allow_throw()) {
					throw stream_read_limit(read_size, total_read_, read_limit_);
				}
			} else {
				set_state(StreamState::buffer_limit_error);

				if(allow_throw()) {
					throw buffer_underrun(read_size, total_read_, buffer_.size());
				}
			}

			return;
		}

		total_read_ += read_size;
	}

	template<typename container_type, typename count_type>
	void read_container(container_type& container, const count_type count) {
		using c_value_type = typename container_type::value_type;

		// guard against large reserve requests
		if(count > MAX_OBJECTS_DESERIALISE) {
			set_state(StreamState::object_limit);

			if(allow_throw()) {
				throw object_limit(count, MAX_OBJECTS_DESERIALISE);
			}

			return;
		}

		if constexpr(!memcpy_read<container_type, BinaryStreamReader>) {
			container.clear();
		}

		if constexpr(has_reserve<container_type>) {
			container.reserve(count);
		}

		if constexpr(memcpy_read<container_type, BinaryStreamReader>) {
			// ensure there's enough data in the buffer to satisify this request
			// before go ahead and resize the container and begin the read
			if(const auto max = size(); count > max / sizeof(c_value_type)) {
				set_state(StreamState::malformed_read);

				if(allow_throw()) {
					throw malformed_read(count * sizeof(c_value_type), total_read_, max);
				}

				return;
			}

			const auto bytes = count * sizeof(c_value_type);
			container.resize(count);
			SAFE_READ(container.data(), bytes, void());
		} else {
			for(count_type i = 0; i < count; ++i) {
				c_value_type value;
				*this >> value;

				if(state() != StreamState::ok) [[unlikely]] {
					return;
				}

				container.emplace_back(std::move(value));
			}
		}
	}

public:
	explicit BinaryStreamReader(BufferRead& source, std::size_t read_limit = 0)
		: StreamBase(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {
		if(read_limit_ > buffer_.size()) {
			set_state(StreamState::bad_read_limit);

			if(allow_throw()) {
				throw bad_read_limit(read_limit_, buffer_.size());
			}
		}
	}

	explicit BinaryStreamReader(BufferRead& source, no_throw_t)
		: StreamBase(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(0) {
		if(read_limit_ > buffer_.size()) {
			set_state(StreamState::bad_read_limit);

			if(allow_throw()) {
				throw bad_read_limit(read_limit_, buffer_.size());
			}
		}
	}

	explicit BinaryStreamReader(BufferRead& source, std::size_t read_limit, no_throw_t)
		: StreamBase(source, false),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {
		if(read_limit_ > buffer_.size()) {
			set_state(StreamState::bad_read_limit);

			if(allow_throw()) {
				throw bad_read_limit(read_limit_, buffer_.size());
			}
		}
	}

	BinaryStreamReader(BinaryStreamReader&& rhs) noexcept
		: StreamBase(rhs),
		  buffer_(rhs.buffer_),
		  total_read_(rhs.total_read_),
		  read_limit_(rhs.read_limit_) {
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

	template<basic_string string_type, std::integral prefix_type, typename endian_tag>
	BinaryStreamReader& operator>>(prefixed<string_type, prefix_type, endian_tag> adaptor) {
		prefix_type size = 0;
		*this >> size;
		endian::storage_out(size, adaptor.byte_order);

		if(state() != StreamState::ok) [[unlikely]] {
			return *this;
		}

		if constexpr(std::signed_integral<prefix_type>) {
			if(size < 0) {
				set_state(StreamState::malformed_read);

				if(allow_throw()) {
					throw malformed_read(size, total_read_, size());
				}

				return *this;
			}
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](string_type::value_type* strbuf, string_type::size_type size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	template<basic_string string_type, std::integral prefix_type, typename endian_tag>
	BinaryStreamReader& operator>>(prefixed_null_terminated<string_type, prefix_type, endian_tag> adaptor) {
		prefix_type size = 0;
		*this >> size;
		endian::storage_out(size, adaptor.byte_order);

		if(state() != StreamState::ok) [[unlikely]] {
			return *this;
		}

		if constexpr(std::signed_integral<prefix_type>) {
			if(size < 0) {
				set_state(StreamState::malformed_read);

				if(allow_throw()) {
					throw malformed_read(size, total_read_, size());
				}

				return *this;
			}
		}

		if(size == 0) { // prefixed_null_terminated must always be at least one byte
			set_state(StreamState::malformed_read);

			if(allow_throw()) {
				throw malformed_read(size, total_read_, size());
			}

			return *this;
		}
		
		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](string_type::value_type* strbuf, string_type::size_type size) {
			// std::*string is guaranteed to be null terminated, so we don't want to read the
			// null terminator from the buffer (double null bytes)
			buffer_.read(strbuf, size - 1);
			return size;
		});

		// validate the terminator is as expected
		char terminator = '\0';

		if(buffer_.read(&terminator, sizeof(terminator)); terminator != '\0') [[unlikely]] {
			set_state(StreamState::malformed_read);

			if(allow_throw()) {
				throw malformed_read(size, total_read_, size());
			}
		}

		return *this;
	}
	
	template<basic_string string_type>
	BinaryStreamReader& operator>>(prefixed_varint<string_type> adaptor) {
		const auto size = detail::varint_decode<std::size_t>(*this);

		if(state() != StreamState::ok) [[unlikely]] {
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](string_type::value_type* strbuf, string_type::size_type size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	template<basic_string string_type>
	BinaryStreamReader& operator>>(null_terminated<string_type> adaptor) {
		auto pos = buffer_.find_first_of(std::byte{0});

		if(pos == buffer_.npos) {
			set_state(StreamState::malformed_read);

			if(allow_throw()) {
				throw malformed_read(pos, total_read_, size());
			}

			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(pos + 1, *this); // include null terminator

		adaptor->resize_and_overwrite(pos, [&](string_type::value_type* strbuf, string_type::size_type size) {
			buffer_.read(strbuf, pos);
			return size;
		});

		buffer_.skip(1); // skip null terminator
		return *this;
	}

	BinaryStreamReader& operator>>(basic_string auto& data) {
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

	template<non_std_string_iterable type, std::integral prefix_type, typename endian_tag>
	BinaryStreamReader& operator>>(prefixed<type, prefix_type, endian_tag> adaptor) {
		prefix_type count = 0;
		*this >> count;
		endian::storage_out(count, adaptor.byte_order);

		if(state() != StreamState::ok) [[unlikely]] {
			return *this;
		}

		if constexpr(std::signed_integral<prefix_type>) {
			if(count < 0) {
				set_state(StreamState::malformed_read);

				if(allow_throw()) {
					throw malformed_read(count * sizeof(type::value_type), total_read_, size());
				}

				return *this;
			}
		}

		read_container(adaptor.str, count);
		return *this;
	}

	template<non_std_string_iterable T, std::integral prefix_type>
	BinaryStreamReader& operator>>(prefixed_varint<T> adaptor) {
		const auto count = detail::varint_decode<std::size_t>(*this);

		if(state() != StreamState::ok) [[unlikely]] {
			return *this;
		}

		read_container(adaptor.str, count);
		return *this;
	}

	void get(basic_string auto& dest) {
		*this >> dest;
	}

	template<basic_string string_type>
	void get(string_type& dest, std::size_t size) {
		STREAM_READ_BOUNDS_ENFORCE(size, void());

		dest.resize_and_overwrite(size, [&](string_type::value_type* strbuf, string_type::size_type len) {
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

	[[nodiscard]]
	std::size_t size() const {
		if(read_limit_) {
			assert(read_limit_ >= total_read_);
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	[[nodiscard]]
	bool empty() const {
		if(read_limit_ && total_read_ == read_limit_) {
			return true;
		} else {
			return buffer_.empty();
		}
	}

	void skip(std::size_t count) {
		STREAM_READ_BOUNDS_ENFORCE(count, void());
		buffer_.skip(count);
	}

	[[nodiscard]]
	std::size_t total_read() const {
		return total_read_;
	}

	[[nodiscard]]
	std::size_t read_limit() const {
		return read_limit_;
	}

	[[nodiscard]]
	BufferRead* buffer() const {
		return &buffer_;
	}
};

#undef SAFE_READ
#undef STREAM_READ_BOUNDS_ENFORCE

} // pmr, io, spark, ember
