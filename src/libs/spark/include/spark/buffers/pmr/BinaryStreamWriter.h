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

	inline void write(const void* data, const std::size_t size) try {
		if(state() == StreamState::ok) [[likely]] {
			buffer_.write(data, size);
			total_write_ += size;
		}

	} catch(...) {
		set_state(StreamState::buffer_write_error);

		if(allow_throw()) {
			throw;
		}
	}

	template<typename container_type>
	void write_container(container_type& container) {
		using c_value_type = typename container_type::value_type;

		if constexpr(memcpy_write<container_type, BinaryStreamWriter>) {
			const auto bytes = container.size() * sizeof(c_value_type);
			write(container.data(), bytes);
		} else {
			for(auto& element : container) {
				*this << element;
			}
		}
	}

public:
	explicit BinaryStreamWriter(BufferWrite& source)
		: StreamBase(source),
		  buffer_(source),
		  total_write_(0) {}

	explicit BinaryStreamWriter(BufferWrite& source, no_throw_t)
		: StreamBase(source, false),
		  buffer_(source),
		  total_write_(0) {}

	BinaryStreamWriter(BinaryStreamWriter&& rhs) noexcept
		: StreamBase(rhs),
		  buffer_(rhs.buffer_), 
		  total_write_(rhs.total_write_) {
		rhs.total_write_ = static_cast<std::size_t>(-1);
		rhs.set_state(StreamState::invalid_stream);
	}

	BinaryStreamWriter& operator=(BinaryStreamWriter&&) = delete;
	BinaryStreamWriter& operator=(const BinaryStreamWriter&) = delete;
	BinaryStreamWriter(const BinaryStreamWriter&) = delete;

	void serialise(auto&& object) {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	template<typename T>
	requires has_serialise<T, stream_write_adaptor<BinaryStreamWriter>>
	BinaryStreamWriter& operator<<(T& data) {
		serialise(data);
		return *this;
	}

	BinaryStreamWriter& operator<<(has_shl_override<BinaryStreamWriter> auto&& data) {
		return *this << data;
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

	BinaryStreamWriter& operator<<(const is_iterable auto& data) {
		write_container(data);
		return *this;
	}

	template<is_iterable T>
	BinaryStreamWriter& operator<<(prefixed<T> adaptor) {
		const auto count = endian::native_to_little(static_cast<std::uint32_t>(adaptor->size()));
		write(&count, sizeof(count));
		write_container(adaptor.str);
		return *this;
	}

	template<is_iterable T>
	BinaryStreamWriter& operator<<(prefixed_varint<T> adaptor) {
		varint_encode(*this, adaptor->size());
		write_container(adaptor.str);
		return *this;
	}

	template<std::ranges::contiguous_range range>
	void put(range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
	}

	void put(const arithmetic auto& data) {
		write(&data, sizeof(data));
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	template<pod T>
	void put(const T* data, std::size_t count) {
		assert(data);
		const auto write_size = count * sizeof(T);
		write(data, write_size);
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
		write(filled.data(), filled.size());
	}

	/**  Misc functions **/ 

	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	void write_seek(const StreamSeek direction, const std::size_t offset) {
		if(direction == StreamSeek::sk_stream_absolute) {
			if(offset >= total_write_) {
				buffer_.write_seek(BufferSeek::sk_forward, offset - total_write_);
			} else {
				buffer_.write_seek(BufferSeek::sk_backward, total_write_ - offset);
			}

			total_write_ = offset;
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
