/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#pragma warning(disable : 4996)

#include <array>
#include <cstddef>

namespace ember::spark {

struct ChainedNode {
	ChainedNode* next;
	ChainedNode* prev;
};

template<typename std::size_t BlockSize>
struct BufferBlock {
	std::array<std::byte, BlockSize> storage;
	std::size_t read_offset = 0;
	std::size_t write_offset = 0;
	ChainedNode node;

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

		std::copy(reinterpret_cast<const std::byte*>(source),
			reinterpret_cast<const std::byte*>(source) + write_len,
			storage.data() + write_offset);
		write_offset += write_len;
		return write_len;
	}

	template<typename OutT>
	std::size_t copy(OutT destination, std::size_t length) const {
		std::size_t read_len = BlockSize - read_offset;

		if(read_len > length) {
			read_len = length;
		}

		std::copy(storage.data() + read_offset, storage.data() + read_offset + read_len, 
			reinterpret_cast<std::byte*>(destination));
		return read_len;
	}

	template<typename InT>
	std::size_t read(InT destination, std::size_t length, bool allow_optimise = false) {
		std::size_t read_len = copy(destination, length);
		read_offset += read_len;

		if(read_offset == write_offset && allow_optimise) {
			reset();
		}

		return read_len;
	}

	std::size_t skip(std::size_t length, bool allow_optimise = false) {
		std::size_t skip_len = BlockSize - read_offset;

		if(skip_len > length) {
			skip_len = length;
		}

		read_offset += skip_len;

		if(read_offset == write_offset && allow_optimise) {
			reset();
		}

		return skip_len;
	}

	std::size_t reserve(std::size_t length) {
		std::size_t reserve_len = BlockSize - write_offset;

		if(reserve_len > length) {
			reserve_len = length;
		}

		write_offset += reserve_len;
		return reserve_len;
	}

	std::size_t size() const {
		return write_offset - read_offset;
	}

	std::size_t free() const {
		return BlockSize - write_offset;
	}

	void write_seek(std::size_t offset, bool rewind) {
		if(rewind) {
			write_offset -= offset; 
		} else {
			write_offset += offset; 
		}
	}

	std::size_t advance_write_cursor(std::size_t size) {
		const std::size_t remaining = free();

		if(remaining < size) {
			size = remaining;
		}

		write_offset += size;
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

} // spark, ember