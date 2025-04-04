/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/Buffer.h>
#include <spark/buffers/pmr/BufferReadAdaptor.h>
#include <spark/buffers/pmr/BufferWriteAdaptor.h>
#include <spark/buffers/Concepts.h>
#include <ranges>

namespace ember::spark::io::pmr {

template<byte_oriented buf_type, bool allow_optimise  = true>
requires std::ranges::contiguous_range<buf_type>
class BufferAdaptor final : public BufferReadAdaptor<buf_type>,
                            public BufferWriteAdaptor<buf_type>,
                            public Buffer {
	void conditional_clear() {
		if(BufferReadAdaptor<buf_type>::read_ptr() == BufferWriteAdaptor<buf_type>::write_ptr()) {
			clear();
		}
	}

public:
	explicit BufferAdaptor(buf_type& buffer)
		: BufferReadAdaptor<buf_type>(buffer),
		  BufferWriteAdaptor<buf_type>(buffer) {}

	explicit BufferAdaptor(buf_type& buffer, init_empty_t)
		: BufferReadAdaptor<buf_type>(buffer, init_empty),
		  BufferWriteAdaptor<buf_type>(buffer, init_empty) {}

	template<typename T>
	void read(T* destination) {
		BufferReadAdaptor<buf_type>::read(destination);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	}

	void read(void* destination, std::size_t length) override {
		BufferReadAdaptor<buf_type>::read(destination, length);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	};

	void write(const auto& source) {
		BufferWriteAdaptor<buf_type>::write(source);
	};

	void write(const void* source, std::size_t length) override {
		BufferWriteAdaptor<buf_type>::write(source, length);
	};

	void copy(auto* destination) const {
		BufferReadAdaptor<buf_type>::copy(destination);
	};

	void copy(void* destination, std::size_t length) const override {
		BufferReadAdaptor<buf_type>::copy(destination, length);
	};

	void skip(std::size_t length) override {
		BufferReadAdaptor<buf_type>::skip(length);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	};

	const std::byte& operator[](const std::size_t index) const override { 
		return BufferReadAdaptor<buf_type>::operator[](index); 
	};

	std::byte& operator[](const std::size_t index) override {
		const auto offset = BufferReadAdaptor<buf_type>::read_offset();
		auto buffer = BufferWriteAdaptor<buf_type>::storage();
		return reinterpret_cast<std::byte*>(buffer + offset)[index];
	}

	void reserve(std::size_t length) override {
		BufferWriteAdaptor<buf_type>::reserve(length);
	};

	bool can_write_seek() const override { 
		return BufferWriteAdaptor<buf_type>::can_write_seek();
	};

	void write_seek(BufferSeek direction, std::size_t offset) override {
		BufferWriteAdaptor<buf_type>::write_seek(direction, offset);
	};

	std::size_t size() const override { 
		return BufferReadAdaptor<buf_type>::size(); 
	};

	[[nodiscard]]
	bool empty() const override {
		return BufferReadAdaptor<buf_type>::read_offset()
			== BufferWriteAdaptor<buf_type>::write_offset();
	}

	std::size_t find_first_of(std::byte val) const override { 
		return BufferReadAdaptor<buf_type>::find_first_of(val);
	}

	void clear() {
		BufferReadAdaptor<buf_type>::clear();
		BufferWriteAdaptor<buf_type>::clear();
	}
};

} // pmr, io, spark, ember
