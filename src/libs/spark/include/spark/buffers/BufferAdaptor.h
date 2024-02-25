/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <spark/buffers/Buffer.h>
#include <spark/buffers/BufferReadAdaptor.h>
#include <spark/buffers/BufferWriteAdaptor.h>

namespace ember::spark {

template<byte_oriented buf_type>
class BufferAdaptor final : public BufferReadAdaptor<buf_type>,
                            public BufferWriteAdaptor<buf_type>,
                            public Buffer {
public:
	explicit BufferAdaptor(buf_type& buffer)
		: BufferReadAdaptor<buf_type>(buffer), BufferWriteAdaptor<buf_type>(buffer) {}

	void read(void* destination, std::size_t length) override {
		BufferReadAdaptor<buf_type>::read(destination, length);
	};

	void copy(void* destination, std::size_t length) const override {
		BufferReadAdaptor<buf_type>::copy(destination, length);
	};

	void skip(std::size_t length) override {
		BufferReadAdaptor<buf_type>::skip(length);
	};

	const std::byte& operator[](const std::size_t index) const override { 
		return BufferReadAdaptor<buf_type>::operator[](index); 
	};

	void write(const void* source, std::size_t length) override {
		BufferWriteAdaptor<buf_type>::write(source, length);
	};

	void reserve(std::size_t length) override {
		BufferWriteAdaptor<buf_type>::reserve(length);
	};

	 bool can_write_seek() const override { 
		return BufferWriteAdaptor<buf_type>::can_write_seek();
	};

	void write_seek(SeekDir direction, std::size_t offset) override {
		BufferWriteAdaptor<buf_type>::write_seek(direction, offset);
	};

	std::byte& operator[](const std::size_t index) override { 
		return BufferWriteAdaptor<buf_type>::operator[](index); 
	};

	std::size_t size() const override { 
		return BufferReadAdaptor<buf_type>::size(); 
	};

	bool empty() const override { 
		return BufferReadAdaptor<buf_type>::empty(); 
	}
};


} // spark, ember