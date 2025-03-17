/*
 * Copyright (c) 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Exception.h>
#include <spark/buffers/Shared.h>
#include <spark/buffers/Concepts.h>
#include <filesystem>
#include <cstddef>
#include <cstdio>

namespace ember::spark::io {

using namespace detail;

class FileBuffer final {
public:
public:
	using size_type       = std::size_t;
	using offset_type     = long;
	using value_type      = char;
	using pointer         = value_type*;
	using const_pointer   = const value_type*;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using contiguous      = is_non_contiguous;
	using seeking         = supported;

	static constexpr auto npos { static_cast<size_type>(-1) };

private:
	FILE* file_ = nullptr;
	offset_type read_ = 0;
	offset_type write_ = 0;
	bool error_ = false;

public:
	FileBuffer(const std::string& path) {
		file_ = std::fopen(path.c_str(), "a+b");
		
		if(!file_) {
			error_ = true;
			return;
		}

		write_ = std::ftell(file_);
	}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, size_type length) {
		if(error_) {
			return;
		}

		error_ = std::fseek(file_, read_, 0);

		if(error_) {
			return;
		}

		std::fread(destination, length, 1, file_);
		read_ += length;
	}

	template<typename T>
	void copy(T* destination) {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, size_type length) {
		if(error_) {
			return;
		} else if(length > size()) {
			error_ = true;
			throw buffer_underrun(length, read_, size());
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return;
		}
		
		if(std::fread(destination, length, 1, file_) != 1) {
			error_ = true;
			return;
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
		}
	}

	size_type find_first_of(value_type val) noexcept {
		if(error_) {
			return;
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return npos;
		}

		value_type buffer{};

		for(std::size_t i = 0u; i < size(); ++i) {
			if(std::fread(&buffer, sizeof(value_type), 1, file_) != 1) {
				error_ = true;
				return npos;
			}

			if(buffer == val) {
				if(std::fseek(file_, read_, SEEK_SET)) {
					error_ = true;
					return npos;
				}

				return i;
			}
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
		}

		return npos;
	}

	void skip(const size_type length) {
		read_ += length;
	}

	void advance_write(size_type bytes) {
		write_ += bytes;
	}

	[[nodiscard]]
	bool empty() const {
		return write_ == read_;
	}

	constexpr static bool can_write_seek() {
		return std::is_same_v<seeking, supported>;
	}

	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	void write(const void* source, size_type length) {
		if(error_) {
			return;
		}
		
		if(std::fseek(file_, write_, SEEK_SET)) {
			error_ = true;
			return;
		}

		if(std::fwrite(source, length, 1, file_) != 1) {
			error_ = true;
			return;
		}

		write_ += length;
	}

	void write_seek(const BufferSeek direction, const offset_type offset) {
		switch(direction) {
			case BufferSeek::SK_BACKWARD:
				write_ -= offset;
				break;
			case BufferSeek::SK_FORWARD:
				write_ += offset;
				break;
			case BufferSeek::SK_ABSOLUTE:
				write_ = offset;
		}
	}

	size_type size() const {
		return write_ - read_;
	}

	FILE* handle() {
		return file_;
	}

	const FILE* handle() const {
		return file_;
	}

	bool error() const {
		return error_;
	}

	operator bool() const {
		return error();
	}
};

} // io, spark, ember