/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <cstdint>

namespace ember {

class PacketBuffer {
	const static std::size_t BUFFER_SIZE = 512;
	std::array<char, BUFFER_SIZE> buffer_;
	std::size_t free_ = BUFFER_SIZE;
	char* location_ = buffer_.data();

public:
	std::size_t capacity() {
		return BUFFER_SIZE;
	}

	std::size_t size() {
		return BUFFER_SIZE - free_;
	}

	std::size_t free() {
		return free_;
	}

	void advance(std::size_t bytes) {
		std::size_t old = free_;
		free_ -= bytes;
		location_ += bytes;

		if(free_ > old) {
			//assert
		}
	}

	void clear() {
		free_ = BUFFER_SIZE;
		location_ = buffer_.data();
	}

	void* data() {
		return buffer_.data();
	}

	void* store() {
		return location_;
	}

	template<typename T>
	T* data() {
		return static_cast<T*>(data());
	}

	PacketBuffer operator=(const PacketBuffer& rhs) = delete;
	PacketBuffer::PacketBuffer(const PacketBuffer& rhs) = delete;
	PacketBuffer::PacketBuffer() = default;
};

} //ember