/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BinaryInStream.h>
#include <spark/buffers/BinaryOutStream.h>
#include <spark/buffers/StreamBase.h>
#include <spark/buffers/Buffer.h>
#include <spark/Exception.h>
#include <algorithm>
#include <concepts>
#include <string>
#include <type_traits>
#include <cstddef>
#include <cstring>

namespace ember::spark {

class BinaryStream final {
public:
	using State = StreamStateBase;

private:
	BinaryInStream bin_;
	BinaryOutStream bout_;
	Buffer& buffer_;

public:
	explicit BinaryStream(Buffer& source, std::size_t read_limit = 0)
                          : buffer_(source), bin_(source), bout_(source, read_limit) {}

	/**  Serialisation **/

	BinaryStream& operator <<(const trivially_copyable auto& data) {
		bin_ << data;
		return *this;
	}

	BinaryStream& operator <<(const std::string& data) {
		bin_ << data;
		return *this;
	}

	BinaryStream& operator <<(const char* data) {
		bin_ << data;
		return *this;
	}

	template<typename T>
	void put(const T* data, std::size_t count) {
		bin_.put(data, count);
;	}

	template<typename It>
	void put(It begin, const It end) {
		bin_.put(begin, end);
	}

	/**  Deserialisation **/

	BinaryStream& operator >>(std::string& dest) {
		bout_ >> dest;
		return *this;
	}

	BinaryStream& operator >>(trivially_copyable auto& data) {
		bout_ >> data;
		return *this;
	}

	void get(std::string& dest, std::size_t size) {
		bout_.get(dest, size);
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
		bout_.get(dest, count);
	}

	template<typename It>
	void get(It begin, const It end) {
		bout_.get(begin, end);
	}

	/**  Misc functions **/ 

	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	void write_seek(SeekDir direction, std::size_t offset = 0) {
		buffer_.write_seek(direction, offset);
	}

	std::size_t size() const {
		return buffer_.size();
	}

	void skip(std::size_t count) {
		bout_.skip(count);
	}

	void clear() {
		buffer_.clear();
	}

	bool empty() {
		return buffer_.empty();
	}

	State state() {
		return bout_.state();
	}

	std::size_t total_read() {
		return bout_.total_read();
	}

	std::size_t read_limit() {
		return bout_.read_limit();
	}

	std::size_t total_write() {
		return bin_.total_write();
	}

	Buffer* buffer() {
		return &buffer_;
	}
};

} // spark, ember