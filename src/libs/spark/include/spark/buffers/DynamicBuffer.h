/*
 * Copyright (c) 2015 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/detail/IntrusiveStorage.h>
#include <spark/buffers/Buffer.h>
#include <boost/assert.hpp>
#include <algorithm>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::spark {

template<typename std::size_t BlockSize>
class BufferSequence;

template<typename std::size_t BlockSize>
class DynamicBuffer final : public Buffer {
	using IntrusiveStorage = typename detail::IntrusiveStorage<BlockSize>;
	using IntrusiveNode = detail::IntrusiveNode;

	IntrusiveNode root_;
	std::size_t size_;

	void link_tail_node(IntrusiveNode* node) {
		node->next = &root_;
		node->prev = root_.prev;
		root_.prev = root_.prev->next = node;
	}

	void unlink_node(IntrusiveNode* node) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	IntrusiveStorage* buffer_from_node(IntrusiveNode* node) const {
		return reinterpret_cast<IntrusiveStorage*>(std::uintptr_t(node)
			- offsetof(IntrusiveStorage, node));
	}

	IntrusiveStorage* buffer_from_node(const IntrusiveNode* node) const {
		return reinterpret_cast<IntrusiveStorage*>(std::uintptr_t(node)
			- offsetof(IntrusiveStorage, node));
	}

	void move(DynamicBuffer& rhs) {
		if(this == &rhs) { // self-assignment
			return;
		}

		clear(); // clear our current blocks rather than swapping them

		size_ = rhs.size_;
		root_.next = rhs.root_.next;
		root_.prev = rhs.root_.prev;
		root_.next->prev = &root_;
		root_.prev->next = &root_;
		rhs.size_ = 0;
		rhs.root_.next = &rhs.root_;
		rhs.root_.prev = &rhs.root_;
	}

	void copy(const DynamicBuffer& rhs) {
		const IntrusiveNode* head = rhs.root_.next;
		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;

		while(head != &rhs.root_) {
			auto buffer = allocate();
			*buffer = *buffer_from_node(head);
			link_tail_node(&buffer->node);
			size_ += buffer->write_offset;
			head = head->next;
		}
	}
	
	void offset_buffers(std::vector<IntrusiveStorage*>& buffers, std::size_t offset) {
		buffers.erase(std::remove_if(buffers.begin(), buffers.end(), [&](IntrusiveStorage* block) {
			if(block->size() > offset) {
				block->read_offset += offset;
				block->write_offset -= offset;
				return false;
			} else {
				return true;
			}
		}), buffers.end());
	}

public:
	DynamicBuffer() { // todo - change in VS2015
		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;
		push_back(allocate());
	}

	~DynamicBuffer() {
		clear();
	}

	DynamicBuffer& operator=(DynamicBuffer&& rhs) { move(rhs); return *this;  }
	DynamicBuffer(DynamicBuffer&& rhs) {  move(rhs); }
	DynamicBuffer(const DynamicBuffer& rhs) { copy(rhs); }
	DynamicBuffer& operator=(const DynamicBuffer& rhs) { clear(); copy(rhs); return *this;  }

	void read(void* destination, std::size_t length) override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer read too large!");
		std::size_t remaining = length;

		while(remaining) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->read(static_cast<std::byte*>(destination) + length - remaining, remaining,
			                          root_.next == root_.prev);

			if(remaining) {
				unlink_node(root_.next);
				deallocate(buffer);
			}
		}

		size_ -= length;
	}

	void copy(void* destination, std::size_t length) const override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer copy too large!");
		std::size_t remaining = length;
		auto head = root_.next;

		while(remaining) {
			const auto buffer = buffer_from_node(head);
			remaining -= buffer->copy(static_cast<std::byte*>(destination) + length - remaining, remaining);

			if(remaining) {
				head = head->next;
			}
		}
	}

	std::vector<IntrusiveStorage*> fetch_buffers(std::size_t length, std::size_t offset = 0) {
		std::size_t total = length + offset;
		BOOST_ASSERT_MSG(total <= size_, "Chained buffer fetch too large!");
		std::vector<IntrusiveStorage*> buffers;
		auto head = root_.next;

		while(total) {
			auto buffer = buffer_from_node(head);
			std::size_t read_size = BlockSize - buffer->read_offset;
			
			// guard against overflow - buffer may have more content than requested
			if(read_size > total) {
				read_size = total;
			}
		
			buffers.emplace_back(buffer);
			total -= read_size;
			head = head->next;
		}

		if(offset) {
			offset_buffers(buffers, offset);
		}

		return buffers;
	}

	void skip(std::size_t length) override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer skip too large!");
		std::size_t remaining = length;

		while(remaining) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->skip(remaining, root_.next == root_.prev);

			if(remaining) {
				unlink_node(root_.next);
				deallocate(buffer);
			}
		}

		size_ -= length;
	}

	void write(const void* source, std::size_t length) override {
		std::size_t remaining = length;
		IntrusiveNode* tail = root_.prev;

		while(remaining) {
			IntrusiveStorage* buffer;

			if(tail == &root_) {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			} else {
				buffer = buffer_from_node(tail);
			}

			remaining -= buffer->write(static_cast<const std::byte*>(source) + length - remaining, remaining);
			tail = tail->next;
		}

		size_ += length;
	}

	void reserve(std::size_t length) override {
		std::size_t remaining = length;
		IntrusiveNode* tail = root_.prev;

		while(remaining) {
			IntrusiveStorage* buffer;

			if(tail == &root_) {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			} else {
				buffer = buffer_from_node(tail);
			}

			remaining -= buffer->reserve(remaining);
			tail = tail->next;
		}

		size_ += length;
	}

	std::size_t size() const override {
		return size_;
	}

	IntrusiveStorage* back() {
		return buffer_from_node(root_.prev);
	}

	IntrusiveStorage* front() {
		return buffer_from_node(root_.next);
	}

	auto pop_front() {
		auto buffer = buffer_from_node(root_.next);
		size_ -= buffer->size();
		unlink_node(root_.next);
		return buffer;
	}

	void push_back(IntrusiveStorage* buffer) {
		link_tail_node(&buffer->node);
		size_ += buffer->write_offset;
	}

	IntrusiveStorage* allocate() const {
		return new IntrusiveStorage(); // todo, actual allocator
	}

	void deallocate(IntrusiveStorage* buffer) const {
		delete buffer; // todo, actual allocator
	}

	void advance_write_cursor(std::size_t size) {
		auto buffer = buffer_from_node(root_.prev);
		const auto actual = buffer->advance_write_cursor(size);
		BOOST_ASSERT_MSG(size <= BlockSize && actual <= size,
		                 "Attempted to advance write cursor out of bounds!");
		size_ += size;
	}

	bool can_write_seek() const override {
		return true;
	}

	void write_seek(SeekDir direction, std::size_t offset = 0) override {
		const bool rewind = direction == SeekDir::SD_BACK || direction == SeekDir::SD_START;
		auto tail = root_.prev;

		if(direction == SeekDir::SD_START) {
			size_ = 0;
		}

		size_ = rewind? size_ - offset : size_ + offset;

		while(offset) {
			auto buffer = buffer_from_node(tail);
			const auto max_seek = rewind? buffer->size() : buffer->free();

			if(max_seek >= offset) {
				buffer->write_seek(direction, offset);
				offset = 0;
			} else {
				buffer->write_seek(direction, max_seek);
				offset -= max_seek;
				tail = rewind? tail->prev : tail->next;
			}
		}

		root_.prev = tail;
	}

	void clear() override {
		IntrusiveNode* head;

		while((head = root_.next) != &root_) {
			unlink_node(head);
			deallocate(buffer_from_node(head));
		}

		size_ = 0;
	}

	bool empty() const override {
		return !size_;
	}
	
	constexpr std::size_t block_size() const {
		return BlockSize;
	}

	std::byte& operator[](const std::size_t index) const {
		BOOST_ASSERT_MSG(index <= size_, "Buffer subscript index out of range");

		auto head = root_.next;
		auto buffer = buffer_from_node(head);
		const auto offset_index = index + buffer->read_offset;
		const auto node_index = offset_index / BlockSize;

		for(std::size_t i = 0; i < node_index; ++i) {
			head = head->next;
		}

		buffer = buffer_from_node(head);
		return (*buffer)[offset_index % BlockSize];
	}

	std::byte& operator[](const std::size_t index) override {
		return const_cast<std::byte&>(static_cast<const DynamicBuffer<BlockSize>&>(*this)[index]);
	}

	template<typename std::size_t T>
	friend class BufferSequence;
};

} // spark, ember