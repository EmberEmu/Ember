/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <spark/buffers/Exception.h>
#include <spark/buffers/Endian.h>
#include <spark/buffers/StreamAdaptors.h>
#include <shared/utility/polyfill/start_lifetime_as>
#include <concepts>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ember::spark::io {

using namespace detail;

#define STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)            \
	if(state_ != StreamState::ok) [[unlikely]] {                 \
		return ret_var;                                           \
	}                                                             \
                                                                  \
	enforce_read_bounds(read_size);                               \
	                                                              \
	if constexpr(std::is_same_v<exceptions, no_throw_t>) {        \
		if(state_ != StreamState::ok) [[unlikely]] {             \
			return ret_var;                                       \
		}                                                         \
	}

#define SAFE_READ(dest, read_size, ret_var)                       \
	STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)                \
	buffer_.read(dest, read_size);

template<
	byte_oriented buf_type,
	std::derived_from<except_tag> exceptions = allow_throw_t,
	std::derived_from<endian::storage_tag> endianness = endian::as_native_t
>
class BinaryStream final {
public:
	using size_type          = typename buf_type::size_type;
	using offset_type        = typename buf_type::offset_type;
	using value_type         = typename buf_type::value_type;
	using contiguous_type    = typename buf_type::contiguous;
	
	static constexpr endianness byte_order{};

private:
	using cond_size_type = std::conditional_t<writeable<buf_type>, size_type, std::monostate>;

	buf_type& buffer_;
	[[no_unique_address]] cond_size_type total_write_{};
	size_type total_read_ = 0;
	StreamState state_ = StreamState::ok;
	const size_type read_limit_;

	inline void enforce_read_bounds(const size_type read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			state_ = StreamState::buffer_limit_error;

			if constexpr(std::is_same_v<exceptions, allow_throw_t>) {
				throw buffer_underrun(read_size, total_read_, buffer_.size());
			}

			return;
		}

		if(read_limit_) {
			const auto max_read_remaining = read_limit_ - total_read_;

			if(read_size > max_read_remaining) [[unlikely]] {
				state_ = StreamState::read_limit_error;

				if constexpr(std::is_same_v<exceptions, allow_throw_t>) {
					throw stream_read_limit(read_size, total_read_, read_limit_);
				}

				return;
			}
		}

		total_read_ += read_size;
	}

	template<typename T>
	inline void advance_write(T&& arg) {
		total_write_ += sizeof(T);
	}

	template<typename T, typename U>
	inline void advance_write(T&&, U&& size) {
		total_write_ += size;
	}

	template<typename... Ts>
	inline void write(Ts&&... args) {
		try {
			if(state_ == StreamState::ok) [[likely]] {
				buffer_.write(std::forward<Ts>(args)...);
				advance_write(std::forward<Ts>(args)...);                            
			}
		} catch(...) {
			state_ = StreamState::buffer_write_error;

			if constexpr(std::is_same_v<exceptions, allow_throw_t>) {
				throw;
			}
		}
	}

	template<typename container_type>
	void write_container(container_type& container) {
		using c_value_type = typename container_type::value_type;

		if constexpr(memcpy_write<container_type, BinaryStream>) {
			const auto bytes = container.size() * sizeof(c_value_type);
			write(container.data(), static_cast<size_type>(bytes));
		} else {
			for(auto& element : container) {
				*this << element;
			}
		}
	}

	template<typename container_type, typename count_type>
	void read_container(container_type& container, const count_type count) {
		using c_value_type = typename container_type::value_type;

		if constexpr(!memcpy_read<container_type, BinaryStream>) {
			container.clear();
		}

		if constexpr(has_reserve<container_type>) {
			container.reserve(count);
		}

		if constexpr(memcpy_read<container_type, BinaryStream>) {
			container.resize(count);

			const auto bytes = static_cast<size_type>(count * sizeof(c_value_type));
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
	explicit BinaryStream(buf_type& source, size_type read_limit = 0)
		: buffer_(source),
		  read_limit_(read_limit) {};

	explicit BinaryStream(buf_type& source, exceptions)
		: BinaryStream(source, 0) {}

	explicit BinaryStream(buf_type& source, endianness)
		: BinaryStream(source, 0) {}

	explicit BinaryStream(buf_type& source, exceptions, endianness)
		: BinaryStream(source, 0) {}

	explicit BinaryStream(buf_type& source, size_type read_limit, exceptions)
		: BinaryStream(source, read_limit) {}

	explicit BinaryStream(buf_type& source, size_type read_limit, endianness)
		: BinaryStream(source, read_limit) {}

	explicit BinaryStream(buf_type& source, size_type read_limit, exceptions, endianness)
		: BinaryStream(source, read_limit) {}

	BinaryStream(BinaryStream&& rhs) noexcept
		: buffer_(rhs.buffer_), 
		  total_write_(rhs.total_write_),
		  total_read_(rhs.total_read_),
		  state_(rhs.state_),
		  read_limit_(rhs.read_limit_) {
		rhs.total_read_ = static_cast<size_type>(-1);
		rhs.state_ = StreamState::invalid_stream;
	}

	BinaryStream& operator=(BinaryStream&&) = delete;
	BinaryStream& operator=(BinaryStream&) = delete;
	BinaryStream(BinaryStream&) = delete;

	/*** Write ***/

	void serialise(auto&& object) requires writeable<buf_type> {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	template<typename T>
	requires has_serialise<T, stream_write_adaptor<BinaryStream>>
	BinaryStream& operator<<(T& data) requires writeable<buf_type> {
		serialise(data);
		return *this;
	}

	BinaryStream& operator<<(const has_shl_override<BinaryStream> auto& data) requires writeable<buf_type> {
		return data.operator<<(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	BinaryStream& operator<<(endian_func adaptor) requires writeable<buf_type> {
		const auto converted = adaptor.to();
		write(&converted, sizeof(converted));
		return *this;
	}

	BinaryStream& operator<<(const arithmetic auto& data) requires writeable<buf_type> {
		const auto converted = endian::storage_in(data, byte_order);
		write(&converted, sizeof(converted));
		return *this;
	}

	template<pod T>
	requires (!has_shl_override<T, BinaryStream> && !arithmetic<T> && !is_iterable<T>)
	BinaryStream& operator<<(const T& data) requires writeable<buf_type> {
		write(&data, sizeof(T));
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	BinaryStream& operator<<(null_terminated<T> adaptor) requires writeable<buf_type> {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size());
		write('\0');
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	BinaryStream& operator<<(null_terminated<T> adaptor) requires writeable<buf_type> {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		return *this;
	}

	template<typename T>
	BinaryStream& operator<<(raw<T> adaptor) requires writeable<buf_type> {
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	BinaryStream& operator<<(const std::string_view string) requires writeable<buf_type> {
		return (*this << prefixed(string));
	}

	BinaryStream& operator<<(const std::string& string) requires writeable<buf_type> {
		return (*this << prefixed(string));
	}

	BinaryStream& operator<<(const char* data) requires writeable<buf_type> {
		assert(data);
		const auto len = std::strlen(data);
		write(data, len + 1); // include terminator
		return *this;
	}

	BinaryStream& operator<<(const is_iterable auto& data) requires writeable<buf_type> {
		write_container(data);
		return *this;
	}

	template<is_iterable type, std::integral prefix_type>
	BinaryStream& operator<<(prefixed<type, prefix_type> adaptor) requires writeable<buf_type> {
		const auto count = static_cast<prefix_type>(adaptor->size());
		write(endian::native_to_little(count));
		write_container(adaptor.str);
		return *this;
	}

	template<is_iterable T>
	BinaryStream& operator<<(prefixed_varint<T> adaptor) requires writeable<buf_type> {
		varint_encode(*this, adaptor->size());
		write_container(adaptor.str);
		return *this;
	}

	template<std::ranges::contiguous_range range>
	void put(const range& data) requires writeable<buf_type> {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
	}

	void put(const arithmetic auto& data) requires writeable<buf_type> {
		write(&data, sizeof(data));
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) requires writeable<buf_type> {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	template<pod T>
	void put(const T* data, size_type count) requires writeable<buf_type> {
		const auto write_size = count * sizeof(T);
		write(data, write_size);
	}

	template<typename It>
	void put(It begin, const It end) requires writeable<buf_type> {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}

	template<size_type size>
	constexpr void fill(const std::uint8_t value) requires writeable<buf_type> {
		const auto filled = generate_filled<size>(value);
		write(filled.data(), filled.size());
	}

	/*** Read ***/

	void deserialise(auto& object) {
		stream_read_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	template<typename T>
	requires has_serialise<T, stream_read_adaptor<BinaryStream>>
	BinaryStream& operator>>(T& data) requires writeable<buf_type> {
		deserialise(data);
		return *this;
	}

	template<std::integral prefix_type>
	BinaryStream& operator>>(prefixed<std::string, prefix_type> adaptor) {
		prefix_type size = 0;
		*this >> endian::le(size);

		if(state_ != StreamState::ok) {
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	template<std::integral prefix_type>
	BinaryStream& operator>>(prefixed<std::string_view, prefix_type> adaptor) {
		prefix_type size = 0;
		*this >> endian::le(size);

		if(state_ != StreamState::ok) {
			return *this;
		}

		adaptor.str = std::string_view { span<char>(size) };
		return *this;
	}
	
	BinaryStream& operator>>(prefixed_varint<std::string> adaptor) {
		const auto size = varint_decode<size_type>(*this);

		// if an error was triggered during decode
		if(state_ != StreamState::ok) {
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	BinaryStream& operator>>(prefixed_varint<std::string_view> adaptor) {
		const auto size = varint_decode<size_type>(*this);

		// if an error was triggered during decode
		if(state_ != StreamState::ok) {
			return *this;
		}
		
		adaptor.str = std::string_view { span<char>(size) };
		return *this;
	}

	BinaryStream& operator>>(null_terminated<std::string> adaptor) {
		auto pos = buffer_.find_first_of(value_type(0));

		if(pos == buf_type::npos) {
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

	BinaryStream& operator>>(null_terminated<std::string_view> adaptor) {
		adaptor.str = view();
		return *this;
	}

	BinaryStream& operator>>(std::string_view& data) {
		return (*this >> prefixed(data));
	}

	BinaryStream& operator>>(std::string& data) {
		return (*this >> prefixed(data));
	}

	BinaryStream& operator>>(has_shr_override<BinaryStream> auto&& data) {
		return data.operator>>(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	BinaryStream& operator>>(endian_func adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor.value), *this);
		adaptor.value = adaptor.from();
		return *this;
	}

	BinaryStream& operator>>(arithmetic auto& data) {
		SAFE_READ(&data, sizeof(data), *this);
		endian::storage_out(data, byte_order);
		return *this;
	}

	template<pod T>
	requires (!has_shr_override<T, BinaryStream> && !arithmetic<T>)
	BinaryStream& operator>>(T& data) {
		SAFE_READ(&data, sizeof(data), *this);
		return *this;
	}

	template<is_iterable type, std::integral prefix_type>
	BinaryStream& operator>>(prefixed<type, prefix_type> adaptor) {
		prefix_type count = 0;
		*this >> endian::le(count);
		read_container(adaptor.str, count);
		return *this;
	}

	template<is_iterable type>
	BinaryStream& operator>>(prefixed_varint<type> adaptor) {
		const auto count = varint_decode<size_type>(*this);
		read_container(adaptor.str, count);
		return *this;
	}

	void get(arithmetic auto& dest) {
		SAFE_READ(&dest, sizeof(dest), void());
	}

	template<arithmetic T>
	T get() {
		T t{};
		SAFE_READ(&t, sizeof(T), t);
		return t;
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor), void());
		adaptor.value = adaptor.from();
	}

	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		SAFE_READ(&t, sizeof(T), t);
		return endian::convert<conversion>(t);
	}

	void get(std::string& dest) {
		*this >> dest;
	}

	void get(std::string& dest, size_type size) {
		STREAM_READ_BOUNDS_ENFORCE(size, void());

		dest.resize_and_overwrite(size, [&](char* strbuf, size_type len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	template<typename T>
	void get(T* dest, size_type count) {
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

	void skip(const size_type count) {
		STREAM_READ_BOUNDS_ENFORCE(count, void());
		buffer_.skip(count);
	}

	std::string_view view(value_type terminator = value_type(0)) requires contiguous<buf_type> {
		const auto pos = buffer_.find_first_of(terminator);

		if(pos == buf_type::npos) {
			return {};
		}

		std::string_view view { reinterpret_cast<char*>(buffer_.read_ptr()), pos };

		// no need to enforce bounds, we know there's enough data
		buffer_.skip(pos + 1);
		total_read_ += (pos + 1);
		return view;
	}

	template<typename out_type = value_type>
	std::span<out_type> span(size_type count) requires contiguous<buf_type> {
		std::span view { std::start_lifetime_as<out_type>(buffer_.read_ptr()), count };
		skip(sizeof(out_type) * count);
		return (state_ == StreamState::ok? view : std::span<out_type>());
	}

	/**  Misc functions **/

	constexpr static bool can_write_seek() {
		return seekable<buf_type>;
	}

	void write_seek(const StreamSeek direction, const offset_type offset) requires(seekable<buf_type>) {
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

	size_type size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	size_type total_write() const requires writeable<buf_type> {
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

	size_type read_max() const {
		if(read_limit_) {
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	bool good() const {
		return state_ == StreamState::ok;
	}

	void clear_error_state() {
		state_ = StreamState::ok;
	}

	operator bool() const {
		return good();
	}

	void set_error_state() {
		state_ = StreamState::user_defined_error;
	}
};

#undef SAFE_READ
#undef STREAM_READ_BOUNDS_ENFORCE

} // io, spark, ember
