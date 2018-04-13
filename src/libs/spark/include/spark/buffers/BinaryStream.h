/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Buffer.h>
#include <spark/Exception.h>
#include <algorithm>
#include <string>
#include <type_traits>
#include <cstddef>
#include <cstring>

namespace ember::spark {

class BinaryStream {
public:
	enum class State {
		OK, READ_LIMIT_ERR, BUFF_LIMIT_ERR
	};

private:
	Buffer& buffer_;
	std::size_t total_read_;
	std::size_t total_write_;
	const std::size_t read_limit_;
	State state_;

public:
	explicit BinaryStream(Buffer& source, std::size_t read_limit = 0)
                          : buffer_(source), total_read_(0), total_write_(0),
                            read_limit_(read_limit), state_(State::OK) {}

	void check_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) {
			state_ = State::BUFF_LIMIT_ERR;
			throw buffer_underrun(read_size, total_read_, buffer_.size());
		}

		const auto req_total_read = total_read_ + read_size;

		if(read_limit_ && req_total_read > read_limit_) {
			state_ = State::READ_LIMIT_ERR;
			throw stream_read_limit(read_size, total_read_, read_limit_);
		}

		total_read_ = req_total_read ;
	}

	/**  Serialisation **/

	template<typename T>
	BinaryStream& operator <<(const T& data) {
		static_assert(std::is_trivially_copyable<T>::value, "Cannot safely copy this type");
		buffer_.write(reinterpret_cast<const char*>(&data), sizeof(T));
		total_write_ += sizeof(T);
		return *this;
	}

	BinaryStream& operator <<(const std::string& data) {
		buffer_.write(data.data(), data.size());
		const char term = '\0';
		buffer_.write(&term, 1);
		total_write_ += (data.size() + 1);
		return *this;
	}

	BinaryStream& operator <<(const char* data) {
		const auto len = std::strlen(data);
		buffer_.write(data, len);
		total_write_ += len;
		return *this;
	}

	void put(const void* data, std::size_t size) {
		buffer_.write(data, size);
		total_write_ += size;
	}

	/**  Deserialisation **/

	// terminates when it hits a null-byte or consumes all data in the buffer
	BinaryStream& operator >>(std::string& dest) {
		char byte;

		do { // not overly efficient
			check_read_bounds(1);
			buffer_.read(&byte, 1);

			if(byte) {
				dest.push_back(byte);
			}
		} while(byte && buffer_.size() > 0);
		
		return *this;
	}

	template<typename T>
	BinaryStream& operator >>(T& data) {
		static_assert(std::is_trivially_copyable<T>::value, "Cannot safely copy this type");
		check_read_bounds(sizeof(T));
		buffer_.read(reinterpret_cast<char*>(&data), sizeof(T));
		return *this;
	}

	void get(std::string& dest, std::size_t size) {
		check_read_bounds(size);
		dest.resize(size);
		buffer_.read(dest.data(), size);
	}

	void get(void* dest, std::size_t size) {
		check_read_bounds(size);
		buffer_.read(dest, size);
	}

	/**  Misc functions **/ 

	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	void write_seek(std::size_t offset, SeekDir direction) {
		buffer_.write_seek(offset, direction);
	}

	std::size_t size() const {
		return buffer_.size();
	}

	void skip(std::size_t count) {
		check_read_bounds(count);
		buffer_.skip(count);
	}

	void clear() {
		buffer_.clear();
	}

	bool empty() {
		return buffer_.empty();
	}

	State state() {
		return state_;
	}

	std::size_t total_read() {
		return total_read_;
	}

	std::size_t read_limit() {
		return read_limit_;
	}

	std::size_t total_write() {
		return total_write_;
	}
};

} // spark, ember