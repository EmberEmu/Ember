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
#include <utility>
#include <cstddef>
#include <cstdio>

namespace ember::spark::io {

using namespace detail;

class FileBuffer final {
public:
	using size_type       = std::size_t;
	using offset_type     = long;
	using value_type      = char;
	using contiguous      = is_non_contiguous;
	using seeking         = unsupported;

	static constexpr auto npos { static_cast<size_type>(-1) };

private:
	FILE* file_ = nullptr;
	offset_type read_ = 0;
	offset_type write_ = 0;
	bool error_ = false;

	void close() {
		if(file_) {
			std::fclose(file_);
			file_ = nullptr;
		}
	}

public:
	FileBuffer(const std::filesystem::path& path)
		: FileBuffer(path.string().c_str()) { }

	FileBuffer(const char* path) {
		file_ = std::fopen(path, "a+b");
		
		if(!file_) {
			error_ = true;
			return;
		}
		
		if(std::fseek(file_, 0, SEEK_END)) {
			error_ = true;
			return;
		}

		write_ = std::ftell(file_);

		if(write_ == -1) {
			error_ = true;
		}
	}

	FileBuffer(FileBuffer&& rhs) noexcept
		: file_(rhs.file_),
		  read_(rhs.read_),
		  write_(rhs.write_),
		  error_(rhs.error_) {
		rhs.file_ = nullptr;
	}

	FileBuffer& operator=(FileBuffer&& rhs) noexcept {
		std::exchange(file_, rhs.file_);
		read_ = rhs.read_;
		write_ = rhs.write_;
		error_ = rhs.error_;
		return *this;
	}

	FileBuffer& operator=(const FileBuffer&) = delete;
	FileBuffer(const FileBuffer&) = delete;

	~FileBuffer() {
		close();
	}


	void flush() {
		std::fflush(file_);
	}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, size_type length) {
		if(error_) {
			return;
		}

		if(std::fseek(file_, read_, 0)) {
			error_ = true;
			return;
		}

		if(std::fread(destination, length, 1, file_) != 1) {
			error_ = true;
			return;
		}

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
	}

	size_type find_first_of(value_type val) noexcept {
		if(error_) {
			return npos;
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return npos;
		}

		value_type buffer{};

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(std::fread(&buffer, sizeof(value_type), 1, file_) != 1) {
				error_ = true;
				return npos;
			}

			if(buffer == val) {
				return i;
			}
		}

		return npos;
	}

	void skip(const size_type length) {
		read_ += length;
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


	size_type size() const {
		return static_cast<size_type>(write_) - read_;
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
		return !error();
	}
};

} // io, spark, ember
