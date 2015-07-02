/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <array>
#include <cstdint>
#include <string>

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

	std::size_t size() const {
		return BUFFER_SIZE - free_;
	}

	std::size_t free() const {
		return free_;
	}

	void advance(std::size_t bytes) {
		std::size_t old = free_;
		free_ -= bytes;
		location_ += bytes;

		if(free_ > old) {
			LOG_FATAL_GLOB << "Overflow in packet buffer: " << __FILE__ << ":" << __LINE__ << LOG_SYNC;
			std::abort();
		}
	}

	void clear() {
		free_ = BUFFER_SIZE;
		location_ = buffer_.data();
	}

	void* data() {
		return buffer_.data();
	}

	const void* data() const {
		return buffer_.data();
	}

	void* store() const {
		return location_;
	}

	template<typename T>
	T* data() {
		return static_cast<T*>(data());
	}

	template<typename T>
	const T* data() const {
		return static_cast<T*>(data());
	}

	
	PacketBuffer(const PacketBuffer& rhs) = delete;
	PacketBuffer& operator=(const PacketBuffer& rhs) = delete;
	PacketBuffer(PacketBuffer&& rhs) = delete;
	PacketBuffer& operator=(PacketBuffer&& rhs) = delete;
	PacketBuffer() = default;
};

} //ember