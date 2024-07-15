/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/Buffer.h>
#include <spark/buffers/SharedDefs.h>
#include <spark/buffers/allocators/DefaultAllocator.h>
#include <spark/buffers/detail/IntrusiveStorage.h>
#include <boost/assert.hpp>
#include <algorithm>
#include <concepts>
#include <utility>
#ifdef BUFFER_DEBUG
#include <vector>
#endif
#include <cstddef>
#include <cstdint>

namespace ember::spark {

template<typename BufferType>
class BufferSequence;

template<decltype(auto) BlockSize>
concept int_gt_zero = std::integral<decltype(BlockSize)> && BlockSize > 0;

template<
	decltype(auto) BlockSize,
	byte_type StorageType = std::byte,
	typename Allocator = DefaultAllocator<detail::IntrusiveStorage<BlockSize, StorageType>
>>
requires int_gt_zero<BlockSize>
class DynamicBuffer final : public Buffer {
	using IntrusiveStorage = typename detail::IntrusiveStorage<BlockSize, StorageType>;
	using IntrusiveNode = detail::IntrusiveNode;

public:
	using value_type = StorageType;
	using node_type  = IntrusiveNode;
	using size_type  = std::size_t;
	using contiguous = is_non_contiguous;
	using seeking    = supported;

	static constexpr size_type npos = -1;

private:
	IntrusiveNode root_;
	size_type size_;
	Allocator alloc_;

	void link_tail_node(IntrusiveNode* node) {
		node->next = &root_;
		node->prev = root_.prev;
		root_.prev = root_.prev->next = node;
	}

	void unlink_node(IntrusiveNode* node) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	inline IntrusiveStorage* buffer_from_node(const IntrusiveNode* node) const {
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
	
#ifdef BUFFER_DEBUG
	void offset_buffers(std::vector<IntrusiveStorage*>& buffers, std::size_t offset) {
		std::erase_if(buffers, [&](auto block) {
			if(block->size() > offset) {
				block->read_offset += offset;
				block->write_offset -= offset;
				return false;
			} else {
				return true;
			}
		});
	}
#endif

	value_type& byte_at_index(const size_t index) const {
		BOOST_ASSERT_MSG(index < size_, "Buffer subscript index out of range");

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

	std::size_t abs_seek_offset(std::size_t offset) {
		if(offset < size_) {
			return size_ - offset;
		} else if(offset > size_) {
			return offset - size_;
		} else {
			return 0;
		}
	}

public:
	DynamicBuffer()
		: root_{ .next = &root_, .prev = &root_ },
		  size_(0) {}

	~DynamicBuffer() {
		clear();
	}

	DynamicBuffer& operator=(DynamicBuffer&& rhs) noexcept { move(rhs); return *this;  }
	DynamicBuffer(DynamicBuffer&& rhs) noexcept {  move(rhs); }
	DynamicBuffer(const DynamicBuffer& rhs) { copy(rhs); }
	DynamicBuffer& operator=(const DynamicBuffer& rhs) { clear(); copy(rhs); return *this;  }

	void read(void* destination, std::size_t length) override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer read too large!");
		std::size_t remaining = length;

		do {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->read(
				static_cast<value_type*>(destination) + length - remaining, remaining,
				                         root_.next == root_.prev
			);

			if(remaining) [[unlikely]] {
				unlink_node(root_.next);
				deallocate(buffer);
			}
		} while(remaining);

		size_ -= length;
	}

	void copy(void* destination, const std::size_t length) const override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer copy too large!");
		std::size_t remaining = length;
		auto head = root_.next;

		do {
			const auto buffer = buffer_from_node(head);
			remaining -= buffer->copy(
				static_cast<value_type*>(destination) + length - remaining, remaining
			);

			if(remaining) {
				head = head->next;
			}
		} while(remaining);
	}

#ifdef BUFFER_DEBUG
	std::vector<IntrusiveStorage*> fetch_buffers(const std::size_t length,
	                                             const std::size_t offset = 0) {
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
#endif

	void skip(const std::size_t length) override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer skip too large!");
		std::size_t remaining = length;

		do {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->skip(remaining, root_.next == root_.prev);

			if(remaining) [[unlikely]] {
				unlink_node(root_.next);
				deallocate(buffer);
			}
		} while(remaining);

		size_ -= length;
	}

	void write(const void* source, const std::size_t length) override {
		std::size_t remaining = length;
		IntrusiveNode* tail = root_.prev;

		do {
			IntrusiveStorage* buffer;

			if(tail != &root_) [[likely]] {
				buffer = buffer_from_node(tail);
			} else {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			}

			remaining -= buffer->write(
				static_cast<const value_type*>(source) + length - remaining, remaining
			);

			tail = tail->next;
		} while(remaining);

		size_ += length;
	}

	void reserve(const std::size_t length) override {
		std::size_t remaining = length;
		IntrusiveNode* tail = root_.prev;

		do {
			IntrusiveStorage* buffer;

			if(tail == &root_) [[unlikely]] {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			} else {
				buffer = buffer_from_node(tail);
			}

			remaining -= buffer->reserve(remaining);
			tail = tail->next;
		} while(remaining);

		size_ += length;
	}

	std::size_t size() const override {
		return size_;
	}

	IntrusiveStorage* back() const {
		return buffer_from_node(root_.prev);
	}

	IntrusiveStorage* front() const {
		return buffer_from_node(root_.next);
	}

	[[nodiscard]] auto pop_front() {
		auto buffer = buffer_from_node(root_.next);
		size_ -= buffer->size();
		unlink_node(root_.next);
		return buffer;
	}

	void push_back(IntrusiveStorage* buffer) {
		link_tail_node(&buffer->node);
		size_ += buffer->write_offset;
	}

	[[nodiscard]] IntrusiveStorage* allocate() {
		return alloc_.allocate();
	}

	void deallocate(IntrusiveStorage* buffer) {
		alloc_.deallocate(buffer);
	}

	void advance_write_cursor(const std::size_t size) {
		auto buffer = buffer_from_node(root_.prev);
		const auto actual = buffer->advance_write_cursor(size);
		BOOST_ASSERT_MSG(size <= BlockSize && actual <= size,
		                 "Attempted to advance write cursor out of bounds!");
		size_ += size;
	}

	bool can_write_seek() const override {
		return true;
	}

	void write_seek(const BufferSeek mode, std::size_t offset) override {
		// nothing to do in this case
		if(mode == BufferSeek::SK_ABSOLUTE && offset == size_) {
			return;
		}

		auto tail = root_.prev;

		switch(mode) {
			case BufferSeek::SK_BACKWARD:
				size_ -= offset;
				break;
			case BufferSeek::SK_FORWARD:
				size_ += offset;
				break;
			case BufferSeek::SK_ABSOLUTE:
				size_ = offset;
				offset = abs_seek_offset(offset);
				break;
		}

		const bool rewind = (mode == BufferSeek::SK_BACKWARD
							 || (mode == BufferSeek::SK_ABSOLUTE && offset < size_));

		while(offset) {
			auto buffer = buffer_from_node(tail);
			const auto max_seek = rewind? buffer->size() : buffer->free();

			if(max_seek >= offset) {
				buffer->write_seek(mode, offset);
				offset = 0;
			} else {
				buffer->write_seek(mode, max_seek);
				offset -= max_seek;
				tail = rewind? tail->prev : tail->next;
			}
		}

		root_.prev = tail;
	}

	void clear() {
		IntrusiveNode* head = root_.next;

		while(head != &root_) {
			auto next = head->next;
			deallocate(buffer_from_node(head));
			head = next;
		}

		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;
	}

	bool empty() const override {
		return !size_;
	}
	
	consteval std::size_t block_size() const {
		return BlockSize;
	}

	value_type& operator[](const std::size_t index) override {
		return byte_at_index(index);
	}

	const value_type& operator[](const std::size_t index) const override {
		return byte_at_index(index);
	}

	std::size_t block_count() {
		auto node = &root_;
		std::size_t count = 0;

		// not calculating based on block size & size as it
		// wouldn't play nice with seeking or manual push/pop
		while(node->next != root_.prev->next) {
			++count;
			node = node->next;
		}

		return count;
	}

	size_type find_first_of(value_type val) const {
		size_type index = 0;
		auto head = root_.next;

		while(head != &root_) {
			const auto buffer = buffer_from_node(head);
			const auto data = buffer->read_data();
			
			for(size_type i = 0, j = buffer->size(); i < j; ++i, ++index) {
				if(data[i] == val) {
					return index;
				}
			}

			head = head->next;
		}

		return npos;
	}

	template<typename BufferType>
	friend class BufferSequence;
};

} // spark, ember
