/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <spark/buffers/Exception.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/polyfill/start_lifetime_as>
#include <algorithm>
#include <array>
#include <concepts>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ember::spark::io {

using namespace detail;

#define STREAM_READ_BOUNDS_CHECK(read_size, ret_var)              \
	check_read_bounds(read_size);                                 \
	                                                              \
	if constexpr(std::is_same_v<exceptions, no_throw>) {          \
		if(state_ != StreamState::OK) [[unlikely]] {              \
			return ret_var;                                       \
		}                                                         \
	}

template<byte_oriented buf_type, std::derived_from<except_tag> exceptions = allow_throw>
class BinaryStream final {
public:
	using size_type          = typename buf_type::size_type;
	using seeking            = typename buf_type::seeking;
	using value_type         = typename buf_type::value_type;
	using contiguous_type    = typename buf_type::contiguous;
	
private:
	buf_type& buffer_;
	size_type total_write_ = 0;
	size_type total_read_ = 0;
	StreamState state_ = StreamState::OK;
	const size_type read_limit_;

	inline void check_read_bounds(const size_type read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			state_ = StreamState::BUFF_LIMIT_ERR;

			if constexpr(std::is_same_v<exceptions, allow_throw>) {
				throw buffer_underrun(read_size, total_read_, buffer_.size());
			}

			return;
		}

		const auto req_total_read = total_read_ + read_size;

		if(read_limit_ && req_total_read > read_limit_) [[unlikely]] {
			state_ = StreamState::READ_LIMIT_ERR;

			if constexpr(std::is_same_v<exceptions, allow_throw>) {
				throw stream_read_limit(read_size, total_read_, read_limit_);
			}

			return;
		}

		total_read_ = req_total_read;
	}

	template<size_type size>
	constexpr auto generate_filled(const std::uint8_t value) {
		std::array<std::uint8_t, size> target{};
		std::ranges::fill(target, value);
		return target;
	}

public:
	explicit BinaryStream(buf_type& source, size_type read_limit = 0)
		: buffer_(source),
		  read_limit_(read_limit) {};

	/*** Write ***/

	BinaryStream& operator <<(const has_shl_override<BinaryStream> auto& data)
	requires(writeable<buf_type>) {
		return data.operator<<(*this);
	}

	template<pod T>
	requires (!has_shl_override<T, BinaryStream>)
	BinaryStream& operator <<(const T& data) requires(writeable<buf_type>) {
		buffer_.write(&data, sizeof(T));
		total_write_ += sizeof(T);
		return *this;
	}

	BinaryStream& operator <<(const std::string& data) requires(writeable<buf_type>) {
		buffer_.write(data.data(), data.size() + 1); // +1 also writes terminator
		total_write_ += (data.size() + 1);
		return *this;
	}

	BinaryStream& operator <<(const char* data) requires(writeable<buf_type>) {
		assert(data);
		const auto len = std::strlen(data);
		buffer_.write(data, len + 1); // include terminator
		total_write_ += len + 1;
		return *this;
	}

	BinaryStream& operator <<(std::string_view& data) requires(writeable<buf_type>) {
		buffer_.write(data.data(), data.size());
		const char term = '\0';
		buffer_.write(&term, sizeof(term));
		total_write_ += (data.size() + 1);
		return *this;
	}

	BinaryStream& operator <<(cstring_view& data) requires(writeable<buf_type>) {
		buffer_.write(data.data(), data.size() + 1);
		total_write_ += (data.size() + 1);
		return *this;
	}

	template<std::ranges::contiguous_range range>
	void put(const range& data) requires(writeable<buf_type>) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		buffer_.write(data.data(), write_size);
		total_write_ += write_size;
	}

	template<arithmetic T>
	void put(const T& data) requires(writeable<buf_type>) {
		buffer_.write(&data, sizeof(T));
		total_write_ += sizeof(T);
	}

	template<pod T>
	void put(const T* data, size_type count) requires(writeable<buf_type>) {
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

	template<size_type size>
	void fill(const std::uint8_t value) requires(writeable<buf_type>) {
		const auto filled = generate_filled<size>(value);
		buffer_.write(filled.data(), filled.size());
		total_write_ += size;
	}

	/*** Read ***/

	// terminates when it hits a null byte, empty string if none found
	BinaryStream& operator>>(std::string& dest) {
		auto pos = buffer_.find_first_of(value_type(0));

		if(pos == buf_type::npos) {
			dest.clear();
			return *this;
		}

		dest.resize_and_overwrite(pos, [&](char* strbuf, size_type size) {
			buffer_.read(strbuf, size);
			total_read_ += size;
			return size;
		});

		total_read_ += 1;
		buffer_.skip(1); // skip null term
		return *this;
	}

	// terminates when it hits a null byte, empty string_view if none found
	// goes without saying that the buffer must outlive the string_view
	BinaryStream& operator>>(std::string_view& dest) requires(contiguous<buf_type>) {
		dest = view();
		return *this;
	}

	// terminates when it hits a null byte, empty cstring_view if none found
	// goes without saying that the buffer must outlive the cstring_view
	BinaryStream& operator>>(cstring_view& dest) requires(contiguous<buf_type>) {
		dest = cstring_view(cstring_view::null_terminated, view());
		return *this;
	}

	BinaryStream& operator>>(has_shr_override<BinaryStream> auto& data) {
		return data.operator>>(*this);
	}

	template<pod T>
	requires (!has_shr_override<T, BinaryStream>)
	BinaryStream& operator>>(T& data) {
		STREAM_READ_BOUNDS_CHECK(sizeof(data), *this);
		buffer_.read(&data, sizeof(data));
		return *this;
	}

	template<arithmetic T>
	void get(T& dest) {
		STREAM_READ_BOUNDS_CHECK(sizeof(T), void());
		buffer_.read(&dest, sizeof(T));
	}

	template<arithmetic T>
	T get() {
		STREAM_READ_BOUNDS_CHECK(sizeof(T), void());
		T t{};
		buffer_.read(&t, sizeof(T));
		return t;
	}

	void get(std::string& dest) {
		*this >> dest;
	}

	void get(std::string& dest, size_type size) {
		STREAM_READ_BOUNDS_CHECK(size, void());
		dest.resize_and_overwrite(size, [&](char* strbuf, size_type len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	template<typename T>
	void get(T* dest, size_type count) {
		assert(dest);
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
		buffer_.read(dest.data(), read_size);
	}

	void skip(const size_type count) {
		STREAM_READ_BOUNDS_CHECK(count, void());
		buffer_.skip(count);
	}

	// Reads a string_view from the buffer, up to the terminator value
	// Returns an empty string_view if a terminator is not found
	std::string_view view(value_type terminator = value_type(0)) requires(contiguous<buf_type>) {
		const auto pos = buffer_.find_first_of(terminator);

		if(pos == buf_type::npos) {
			return {};
		}

		std::string_view view { reinterpret_cast<char*>(buffer_.read_ptr()), pos };
		buffer_.skip(pos + 1);
		total_read_ += (pos + 1);
		return view;
	}

	// Reads a span<T> from the buffer
	// Fails if buffer length < requested bytes
	template<typename OutType = value_type>
	std::span<OutType> span(size_type count) requires(contiguous<buf_type>) {
		STREAM_READ_BOUNDS_CHECK(sizeof(OutType) * count, {});
		std::span span { std::start_lifetime_as<OutType>(buffer_.read_ptr()), count };
		buffer_.skip(sizeof(OutType) * count);
		return span;
	}

	/**  Misc functions **/

	consteval static bool can_write_seek() requires(writeable<buf_type>) {
		return std::is_same_v<seeking, supported>;
	}

	void write_seek(const StreamSeek direction, const offset_type offset) requires(writeable<buf_type>) {
		if(direction == StreamSeek::SK_STREAM_ABSOLUTE) {
			buffer_.write_seek(BufferSeek::SK_BACKWARD, total_write_ - offset);
		} else {
			buffer_.write_seek(static_cast<BufferSeek>(direction), offset);
		}
	}

	size_type size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	size_type total_write() const requires(writeable<buf_type>) {
		return total_write_;
	}

	const buf_type* buffer() const {
		return &buffer_;
	}

	buf_type* buffer() {
		return &buffer_;
	}

	StreamState state() const {
		return state_;
	}

	size_type total_read() const {
		return total_read_;
	}

	size_type read_limit() const {
		return read_limit_;
	}

	bool good() const {
		return state_ == StreamState::OK;
	}

	void clear_error_state() {
		state_ = StreamState::OK;
	}

	operator bool() const {
		return good();
	}

	void set_error_state() {
		state_ = StreamState::USER_DEFINED_ERR;
	}
};

} // io, spark, ember