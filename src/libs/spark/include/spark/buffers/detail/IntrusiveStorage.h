/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#pragma warning(disable : 4996)

#include <spark/buffers/Buffer.h>
#include <array>
#include <concepts>
#include <type_traits>
#include <cstring>
#include <cstddef>

namespace ember::spark::detail {

struct IntrusiveNode {
	IntrusiveNode* next;
	IntrusiveNode* prev;
};

template<decltype(auto) BlockSize>
requires std::unsigned_integral<decltype(BlockSize)>
struct IntrusiveStorage {
	using OffsetType = std::remove_const_t<decltype(BlockSize)>;
	using value_type = std::byte;

	OffsetType read_offset = 0;
	OffsetType write_offset = 0;
	IntrusiveNode node {};
	std::array<value_type, BlockSize> storage;

	void reset() {
		read_offset = 0;
		write_offset = 0;
	}

	template<typename InT>
	std::size_t write(const InT source, std::size_t length) {
		std::size_t write_len = BlockSize - write_offset;

		if(write_len > length) {
			write_len = length;
		}

		std::memcpy(storage.data() + write_offset, source, write_len);
		write_offset += static_cast<OffsetType>(write_len);
		return write_len;
	}

	template<typename OutT>
	std::size_t copy(OutT destination, const std::size_t length) const {
		std::size_t read_len = BlockSize - read_offset;

		if(read_len > length) {
			read_len = length;
		}

		std::memcpy(destination, storage.data() + read_offset, read_len);
		return read_len;
	}

	template<typename InT>
	std::size_t read(InT destination, const std::size_t length, const bool allow_optimise = false) {
		std::size_t read_len = copy(destination, length);
		read_offset += static_cast<OffsetType>(read_len);

		if(read_offset == write_offset && allow_optimise) {
			reset();
		}

		return read_len;
	}

	std::size_t skip(const std::size_t length, const bool allow_optimise = false) {
		std::size_t skip_len = BlockSize - read_offset;

		if(skip_len > length) {
			skip_len = length;
		}

		read_offset += static_cast<OffsetType>(skip_len);

		if(read_offset == write_offset && allow_optimise) {
			reset();
		}

		return skip_len;
	}

	std::size_t reserve(const std::size_t length) {
		std::size_t reserve_len = BlockSize - write_offset;

		if(reserve_len > length) {
			reserve_len = length;
		}

		write_offset += static_cast<OffsetType>(reserve_len);
		return reserve_len;
	}

	std::size_t size() const {
		return write_offset - read_offset;
	}

	std::size_t free() const {
		return BlockSize - write_offset;
	}

	void write_seek(const SeekDir direction, const std::size_t offset) {
		if(direction == SeekDir::SD_START) {
			write_offset = 0;
		} else if(direction == SeekDir::SD_BACK) {
			write_offset -= static_cast<OffsetType>(offset);
		} else {
			write_offset += static_cast<OffsetType>(offset);
		}
	}

	std::size_t advance_write_cursor(std::size_t size) {
		const std::size_t remaining = free();

		if(remaining < size) {
			size = remaining;
		}

		write_offset += static_cast<OffsetType>(size);
		return size;
	}

	const std::byte* read_data() const {
		return storage.data() + read_offset;
	}

	std::byte* write_data() {
		return storage.data() + write_offset;
	}

	std::byte& operator[](const std::size_t index) {
		return *(storage.data() + index);
	}

	std::byte& operator[](const std::size_t index) const {
		return *(storage.data() + index);
	}
};

} // detail, spark, ember