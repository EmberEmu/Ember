/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Buffer.h>
#include <algorithm>
#include <string>
#include <type_traits>
#include <cstddef>
#include <cstring>

namespace ember { namespace spark{

class BinaryStream {
	Buffer& buffer_;

public:
	explicit BinaryStream(Buffer& source) : buffer_(source) {}

	/**  Serialisation **/

	template<typename T>
	BinaryStream& operator <<(const T& data) {
		static_assert(std::is_trivially_copyable<T>::value, "Cannot safely copy this type");
		buffer_.write(reinterpret_cast<const char*>(&data), sizeof(T));
		return *this;
	}

	BinaryStream& operator <<(const char* data) {
		buffer_.write(data, std::strlen(data));
		return *this;
	}

	void put(const void* data, std::size_t size) {
		buffer_.write(data, size);
	}

	/**  Deserialisation **/

	// terminates when it hits a null-byte or consumes all data in the buffer
	BinaryStream& operator >>(std::string& dest) {
		char byte;

		do { // not overly efficient
			buffer_.read(&byte, 1);
			dest.push_back(byte);
		} while(byte && buffer_.size() > 0);
		
		return *this;
	}

	template<typename T>
	BinaryStream& operator >>(T& data) {
		static_assert(std::is_trivially_copyable<T>::value, "Cannot safely copy this type");
		buffer_.read(reinterpret_cast<char*>(&data), sizeof(T));
		return *this;
	}

	void get(std::string& dest, std::size_t size) {
		dest.resize(size);
		buffer_.read(&dest[0], size); // check back in a decade - non-const data should be added by then
	}

	void get(void* dest, std::size_t size) {
		buffer_.read(dest, size);
	}

	/**  Misc functions **/ 

	std::size_t size() {
		return buffer_.size();
	}

	void skip(std::size_t count) {
		buffer_.skip(count);
	}
};

}} // spark, ember