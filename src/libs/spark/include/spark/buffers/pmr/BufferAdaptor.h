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
		: BufferReadAdaptor<buf_type>(buffer),
		  BufferWriteAdaptor<buf_type>(buffer, init_empty) {}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		BufferReadAdaptor<buf_type>::read(destination);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, std::size_t length) override {
		BufferReadAdaptor<buf_type>::read(destination, length);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	};

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		BufferWriteAdaptor<buf_type>::write(source);
	};

	/**
	 * @brief Write provided data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, std::size_t length) override {
		BufferWriteAdaptor<buf_type>::write(source, length);
	};

	void copy(auto* destination) const {
		BufferReadAdaptor<buf_type>::copy(destination);
	};

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, std::size_t length) const override {
		BufferReadAdaptor<buf_type>::copy(destination, length);
	};

	/**
	 * @brief Skip over requested number of bytes.
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t length) override {
		BufferReadAdaptor<buf_type>::skip(length);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	};

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const std::byte& operator[](const std::size_t index) const override { 
		return BufferReadAdaptor<buf_type>::operator[](index); 
	};

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	std::byte& operator[](const std::size_t index) override {
		const auto offset = BufferReadAdaptor<buf_type>::read_offset();
		auto buffer = BufferWriteAdaptor<buf_type>::storage();
		return reinterpret_cast<std::byte*>(buffer + offset)[index];
	}

	/**
	 * @brief Reserves a number of bytes within the container for future use.
	 * 
	 * @param length The number of bytes that the container should reserve.
	 */
	void reserve(std::size_t length) override {
		BufferWriteAdaptor<buf_type>::reserve(length);
	};

	/**
	 * @brief Determines whether this container can write seek.
	 * 
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const override { 
		return BufferWriteAdaptor<buf_type>::can_write_seek();
	};

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(BufferSeek direction, std::size_t offset) override {
		BufferWriteAdaptor<buf_type>::write_seek(direction, offset);
	};

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const override { 
		return BufferReadAdaptor<buf_type>::size(); 
	};

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const override {
		return BufferReadAdaptor<buf_type>::read_offset()
			== BufferWriteAdaptor<buf_type>::write_offset();
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	std::size_t find_first_of(std::byte val) const override { 
		return BufferReadAdaptor<buf_type>::find_first_of(val);
	}

	void clear() {
		BufferReadAdaptor<buf_type>::clear();
		BufferWriteAdaptor<buf_type>::clear();
	}
};

} // pmr, io, spark, ember
