/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/StreamBase.h>
#include <spark/buffers/BufferRead.h>
#include <spark/Exception.h>
#include <algorithm>
#include <concepts>
#include <ranges>
#include <string>
#include <cstddef>
#include <cstring>

namespace ember::spark {

class BinaryStreamReader : virtual public StreamBase {
	constexpr static std::size_t string_copy_block = 32;

	BufferRead& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;
	StreamState state_;

	void check_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			state_ = StreamState::BUFF_LIMIT_ERR;
			throw buffer_underrun(read_size, total_read_, buffer_.size());
		}

		const auto req_total_read = total_read_ + read_size;

		if(read_limit_ && req_total_read > read_limit_) [[unlikely]] {
			state_ = StreamState::READ_LIMIT_ERR;
			throw stream_read_limit(read_size, total_read_, read_limit_);
		}

		total_read_ = req_total_read ;
	}

public:
	explicit BinaryStreamReader(BufferRead& source, std::size_t read_limit = 0)
                            : StreamBase(source), buffer_(source), total_read_(0),
                              read_limit_(read_limit), state_(StreamState::OK) {}

	// terminates when it hits a null-byte or consumes all data in the buffer
	BinaryStreamReader& operator >>(std::string& dest) {
		check_read_bounds(1); // just to prevent trying to read from an empty buffer
		dest.clear();

		do {
			const auto copy_len = std::min(string_copy_block, buffer_.size());
			const auto dest_size = dest.size();
			dest.resize(dest_size + copy_len);
			buffer_.copy(dest.data() + dest_size, copy_len);

			if(auto pos = dest.find_first_of('\0'); pos != std::string::npos) {
				const auto skip_len = copy_len - (dest.size() - (pos + 1));
				buffer_.skip(skip_len); // skip only the string data, not any trailing
				dest.resize(pos);       // shrink down so as to ignore trailing data
				break;
			} else {
				buffer_.skip(copy_len); // skip all copied data
			}
		} while(!buffer_.empty());

		return *this;
	}

	BinaryStreamReader& operator >>(is_pod auto& data) {
		check_read_bounds(sizeof(data));
		buffer_.read(&data, sizeof(data));
		return *this;
	}

	void get(std::string& dest, std::size_t size) {
		check_read_bounds(size);
		dest.resize(size);
		buffer_.read(dest.data(), size);
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
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
		buffer_.read(dest, read_size);
	}

	/**  Misc functions **/ 

	void skip(std::size_t count) {
		check_read_bounds(count);
		buffer_.skip(count);
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

	BufferRead* buffer() const {
		return &buffer_;
	}

	bool good() const {
		return state_ == StreamState::OK;
	}

	void clear_state() {
		state_ = StreamState::OK;
	}
};

} // spark, ember
