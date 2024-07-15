/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/SharedDefs.h>
#include <spark/Exception.h>
#include <algorithm>
#include <array>
#include <concepts>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ember::spark::v2 {

#define STREAM_READ_BOUNDS_CHECK(read_size, ret_var) \
	check_read_bounds(read_size);                    \
	                                                 \
	if constexpr(!enable_exceptions) {               \
		if(state_ != StreamState::OK) [[unlikely]] { \
			return ret_var;                          \
		}                                            \
	}

template <typename buf_type>
concept writeable =
	requires(buf_type t, void* v, std::size_t s) {
		{ t.write(v, s) } -> std::same_as<void>;
};

template<typename buf_type>
concept contiguous = requires(buf_type t) {
	std::is_same<typename buf_type::contiguous, is_contiguous>::value;
};

template<byte_oriented buf_type, bool enable_exceptions = true>
class BinaryStream final {
	buf_type& buffer_;
	std::size_t total_write_ = 0;
	std::size_t total_read_ = 0;
	const std::size_t read_limit_;
	StreamState state_ = StreamState::OK;

	inline void check_read_bounds(const std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			state_ = StreamState::BUFF_LIMIT_ERR;

			if constexpr(enable_exceptions) {
				throw buffer_underrun(read_size, total_read_, buffer_.size());
			}

			return;
		}

		const auto req_total_read = total_read_ + read_size;

		if(read_limit_ && req_total_read > read_limit_) [[unlikely]] {
			state_ = StreamState::READ_LIMIT_ERR;

			if constexpr(enable_exceptions) {
				throw stream_read_limit(read_size, total_read_, read_limit_);
			}

			return;
		}

		total_read_ = req_total_read;
	}

	template<std::size_t size>
	constexpr auto generate_filled(const std::uint8_t value) {
		std::array<std::uint8_t, size> target{};
		std::fill(target.begin(), target.end(), value);
		return target;
	}

public:
	using State = StreamState;
	using value_type = typename buf_type::value_type;
	using contiguous_type = typename buf_type::contiguous;

	explicit BinaryStream(buf_type& source, const std::size_t read_limit = 0)
		: buffer_(source), read_limit_(read_limit) {};

	/*** Write ***/

	BinaryStream& operator <<(const is_pod auto& data) requires(writeable<buf_type>) {
		buffer_.write(&data, sizeof(data));
		total_write_ += sizeof(data);
		return *this;
	}

	BinaryStream& operator <<(const std::string& data) requires(writeable<buf_type>) {
		buffer_.write(data.data(), data.size() + 1); // +1 also writes terminator
		total_write_ += (data.size() + 1);
		return *this;
	}

	BinaryStream& operator <<(const char* data) requires(writeable<buf_type>) {
		const auto len = std::strlen(data);
		buffer_.write(data, len);
		total_write_ += len;
		return *this;
	}

	template<std::ranges::contiguous_range range>
	void put(range& data) requires(writeable<buf_type>) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		buffer_.write(data.data(), write_size);
		total_write_ += write_size;
	}

	template<typename T>
	void put(const T* data, std::size_t count) requires(writeable<buf_type>) {
		const auto write_size = count * sizeof(T);
		buffer_.write(data, write_size);
		total_write_ += write_size;
	}

	template<typename It>
	void put(It begin, const It end) requires(writeable<buf_type>) {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}

	template<std::size_t size>
	void fill(const std::uint8_t value) requires(writeable<buf_type>) {
		const auto filled = generate_filled<size>(value);
		buffer_.write(filled.data(), filled.size());
		total_write_ += size;
	}

	/*** Read ***/

	// terminates when it hits a null-byte or consumes all data in the buffer
	BinaryStream& operator >>(std::string& dest) {
		STREAM_READ_BOUNDS_CHECK(1, *this);
		dest.clear();
		auto pos = buffer_.find_first_of(typename buf_type::value_type(0x00));
		auto read_len = pos == buf_type::npos? buffer_.size() : pos;
		dest.resize(read_len);
		buffer_.read(dest.data(), read_len);

		if(pos != buf_type::npos) {
			buffer_.skip(1); // skip null term
		}

		return *this;
	}

	BinaryStream& operator >>(is_pod auto& data) {
		STREAM_READ_BOUNDS_CHECK(sizeof(data), *this);
		buffer_.read(&data, sizeof(data));
		return *this;
	}

	void get(std::string& dest, std::size_t size) {
		STREAM_READ_BOUNDS_CHECK(size, void());
		dest.resize(size);
		buffer_.read(dest.data(), size);
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
		const auto read_size = count * sizeof(T);
		STREAM_READ_BOUNDS_CHECK(read_size, void());
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
		STREAM_READ_BOUNDS_CHECK(read_size, void());
		buffer_.read(dest, read_size);
	}

	void skip(const std::size_t count) {
		STREAM_READ_BOUNDS_CHECK(count, void());
		buffer_.skip(count);
	}

	// Reads a string_view from the buffer, up to the terminator value
	// Returns an empty string_view if a terminator is not found
	std::string_view view(value_type terminator = 0x00) requires(contiguous<buf_type>) {
		STREAM_READ_BOUNDS_CHECK(1, {});
		const auto pos = buffer_.find_first_of(terminator);

		if(pos == buf_type::npos) {
			return {};
		}

		std::string_view view { span<char, true>(pos) };
		buffer_.skip(1);
		return view;
	}

	// Reads a span from the buffer, up to the length value
	// Returns an empty span if buffer length < length
	template<typename OutType = value_type, bool skip_bounds = false>
	std::span<OutType> span(std::size_t count) requires(contiguous<buf_type>) {
		if constexpr(!skip_bounds) {
			STREAM_READ_BOUNDS_CHECK(sizeof(OutType) * count, {});
		}

		std::span span { reinterpret_cast<OutType*>(buffer_.read_ptr()), count };
		buffer_.skip(sizeof(OutType) * count);
		return span;
	}

	/**  Misc functions **/

	bool can_write_seek() const requires(writeable<buf_type>) {
		return buffer_.can_write_seek();
	}

	void write_seek(const StreamSeek direction, const std::size_t offset) requires(writeable<buf_type>) {
		if(direction == StreamSeek::SK_STREAM_ABSOLUTE) {
			buffer_.write_seek(BufferSeek::SK_BACKWARD, total_write_ - offset);
		} else {
			buffer_.write_seek(static_cast<BufferSeek>(direction), offset);
		}
	}

	std::size_t size() const {
		return buffer_.size();
	}

	bool empty() const {
		return buffer_.empty();
	}

	std::size_t total_write() const requires(writeable<buf_type>) {
		return total_write_;
	}

	buf_type* buffer() const {
		return &buffer_;
	}

	StreamState state() const {
		return state_;
	}

	std::size_t total_read() const {
		return total_read_;
	}

	std::size_t read_limit() const {
		return read_limit_;
	}

	bool good() const {
		return state_ == StreamState::OK;
	}

	void clear_state() {
		state_ = StreamState::OK;
	}
};

} // v2, spark, ember