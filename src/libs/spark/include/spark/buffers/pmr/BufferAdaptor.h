/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <spark/buffers/pmr/Buffer.h>
#include <spark/buffers/pmr/BufferReadAdaptor.h>
#include <spark/buffers/pmr/BufferWriteAdaptor.h>

namespace ember::spark::io::pmr {

template<byte_oriented buf_type, bool allow_optimise  = true>
class BufferAdaptor final : public BufferReadAdaptor<buf_type>,
                            public BufferWriteAdaptor<buf_type>,
                            public Buffer {
	void reset() {
		if(BufferReadAdaptor<buf_type>::read_ptr() == BufferWriteAdaptor<buf_type>::write_ptr()) {
			BufferReadAdaptor<buf_type>::reset();
			BufferWriteAdaptor<buf_type>::reset();
		}
	}
public:
	explicit BufferAdaptor(buf_type& buffer)
		: BufferReadAdaptor<buf_type>(buffer), BufferWriteAdaptor<buf_type>(buffer) {}

	template<typename T>
	void read(T* destination) {
		BufferReadAdaptor<buf_type>::read(destination);

		if constexpr(allow_optimise) {
			reset();
		}
	}

	void read(void* destination, std::size_t length) override {
		BufferReadAdaptor<buf_type>::read(destination, length);

		if constexpr(allow_optimise) {
			reset();
		}
	};

	template<typename T>
	void write(const T& source) {
		BufferWriteAdaptor<buf_type>::write(source);
	};

	void write(const void* source, std::size_t length) {
		BufferWriteAdaptor<buf_type>::write(source, length);
	};

	template<typename T>
	void copy(T* destination) const {
		BufferReadAdaptor<buf_type>::copy(destination);
	};

	void copy(void* destination, std::size_t length) const override {
		BufferReadAdaptor<buf_type>::copy(destination, length);
	};

	void skip(std::size_t length) override {
		BufferReadAdaptor<buf_type>::skip(length);

		if constexpr(allow_optimise) {
			reset();
		}
	};

	const std::byte& operator[](const std::size_t index) const override { 
		return BufferReadAdaptor<buf_type>::operator[](index); 
	};

	std::byte& operator[](const std::size_t index) override {
		const auto offset = BufferReadAdaptor<buf_type>::read_offset();
		auto buffer = BufferWriteAdaptor<buf_type>::underlying_data();
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
		return BufferReadAdaptor<buf_type>::empty();
	}

	std::size_t find_first_of(std::byte val) const override { 
		return BufferReadAdaptor<buf_type>::find_first_of(val);
	}
};

} // pmr, io, spark, ember