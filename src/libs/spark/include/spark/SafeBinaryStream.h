/*
 * Copyright (c) 2015-2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Buffer.h>
#include <spark/Exception.h>
#include <algorithm>
#include <string>
#include <type_traits>
#include <cstddef>
#include <cstring>

namespace ember { namespace spark {

class SafeBinaryStream {
	Buffer& buffer_;

public:
	explicit SafeBinaryStream(Buffer& source) : buffer_(source) {}

	void check_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) {
			throw buffer_underrun(read_size, buffer_.size());
		}
	}

	/**  Serialisation **/

	template<typename T>
	SafeBinaryStream& operator <<(const T& data) {
		static_assert(std::is_trivially_copyable<T>::value, "Cannot safely copy this type");
		buffer_.write(reinterpret_cast<const char*>(&data), sizeof(T));
		return *this;
	}

	SafeBinaryStream& operator <<(const std::string& data) {
		buffer_.write(data.data(), data.size());
		char term = '\0';
		buffer_.write(&term, 1);
		return *this;
	}

	SafeBinaryStream& operator <<(const char* data) {
		buffer_.write(data, std::strlen(data));
		return *this;
	}

	void put(const void* data, std::size_t size) {
		buffer_.write(data, size);
	}

	/**  Deserialisation **/

	// terminates when it hits a null-byte or consumes all data in the buffer
	SafeBinaryStream& operator >>(std::string& dest) {
		check_read_bounds(1);
		char byte;

		do { // not overly efficient
			buffer_.read(&byte, 1);
			if(byte) {
				dest.push_back(byte);
			}
		} while(byte && buffer_.size() > 0);
		
		return *this;
	}

	template<typename T>
	SafeBinaryStream& operator >>(T& data) {
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

	std::size_t size() const {
		return buffer_.size();
	}

	void skip(std::size_t count) {
		buffer_.skip(count);
	}

	void clear() {
		buffer_.clear();
	}

	bool empty() {
		return buffer_.empty();
	}
};

}} // spark, ember