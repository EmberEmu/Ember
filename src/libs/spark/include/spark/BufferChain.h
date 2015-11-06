/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "BufferChainNode.h"
#include "Buffer.h"
#include <boost/assert.hpp>
#include <boost/asio/buffer.hpp>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember { namespace spark {

template<typename std::size_t BlockSize = 1024>
class BufferChain final : public Buffer {
	BufferChainNode root_;
	std::size_t size_;

	void link_tail_node(BufferChainNode* node) {
		node->next = &root_;
		node->prev = root_.prev;
		root_.prev = root_.prev->next = node;
	}

	void unlink_node(BufferChainNode* node) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	BufferBlock<BlockSize>* buffer_from_node(BufferChainNode* node) const {
		return reinterpret_cast<BufferBlock<BlockSize>*>(std::size_t(node)
			- offsetof(BufferBlock<BlockSize>, node));
	}

	BufferBlock<BlockSize>* buffer_from_node(const BufferChainNode* node) const {
		return reinterpret_cast<BufferBlock<BlockSize>*>(std::size_t(node)
			- offsetof(BufferBlock<BlockSize>, node));
	}

	void move(BufferChain& rhs) {
		if(this == &rhs) { // self-assignment
			return;
		}

		clear(); //clear our current blocks rather than swapping them

		size_ = rhs.size_;
		root_.next = rhs.root_.next;
		root_.prev = rhs.root_.prev;
		root_.next->prev = &root_;
		root_.prev->next = &root_;
		rhs.size_ = 0;
		rhs.root_.next = &rhs.root_;
		rhs.root_.prev = &rhs.root_;
	}

	void copy(const BufferChain& rhs) {
		const BufferChainNode* head = rhs.root_.next;
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
	
	void offset_buffers(std::vector<BufferBlock<BlockSize>*>& buffers, std::size_t offset) {
		for(auto i = buffers.begin(); i != buffers.end();) {
			if((*i)->size() > offset) {
				(*i)->read_offset += offset;
				(*i)->write_offset -= offset;
				break;
			} else {
				i = buffers.erase(i);
			}
		}
	}

public:
	BufferChain() { // todo - change in VS2015
		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;
		attach(allocate());
	}

	~BufferChain() {
		clear();
	}

	BufferChain& operator=(BufferChain&& rhs) { move(rhs); return *this;  }
	BufferChain(BufferChain&& rhs) {  move(rhs); }
	BufferChain(const BufferChain& rhs) { copy(rhs); }
	BufferChain& operator=(const BufferChain& rhs) { clear(); copy(rhs); return *this;  }

	void read(void* destination, std::size_t length) override {
		BOOST_ASSERT_MSG(length <= size_, "Chained buffer read too large!");
		std::size_t remaining = length;

		while(remaining) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->read(static_cast<char*>(destination) + length - remaining, remaining, 
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
			auto buffer = buffer_from_node(head);
			remaining -= buffer->copy(static_cast<char*>(destination) + length - remaining, remaining);

			if(remaining) {
				head = head->next;
			}
		}
	}

	std::vector<BufferBlock<BlockSize>*> fetch_buffers(std::size_t length, std::size_t offset = 0) {
		std::size_t total = length + offset;
		BOOST_ASSERT_MSG(total <= size_, "Chained buffer fetch too large!");
		std::vector<BufferBlock<BlockSize>*> buffers;
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
		BufferChainNode* tail = root_.prev;

		while(remaining) {
			BufferBlock<BlockSize>* buffer;

			if(tail == &root_) {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			} else {
				buffer = buffer_from_node(tail);
			}

			remaining -= buffer->write(static_cast<const char*>(source) + length - remaining, remaining);
			tail = tail->next;
		}

		size_ += length;
	}

	void reserve(std::size_t length) override {
		std::size_t remaining = length;
		BufferChainNode* tail = root_.prev;

		while(remaining) {
			BufferBlock<BlockSize>* buffer;

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

	BufferBlock<BlockSize>* tail() {
		return buffer_from_node(root_.prev);
	}

	void attach(BufferBlock<BlockSize>* buffer) {
		link_tail_node(&buffer->node);
		size_ += buffer->write_offset;
	}

	BufferBlock<BlockSize>* allocate() const {
		return new BufferBlock<BlockSize>(); // todo, actual allocator
	}

	void deallocate(BufferBlock<BlockSize>* buffer) const {
		delete buffer; // todo, actual allocator
	}

	void advance_write_cursor(std::size_t size) {
		auto buffer = buffer_from_node(root_.prev);
		auto actual = buffer->advance_write_cursor(size);
		BOOST_ASSERT_MSG(size <= BlockSize && actual == size,
		                 "Attempted to advance write cursor out of bounds!");
		size_ += size;
	}

	void clear() override {
		BufferChainNode* head;

		while((head = root_.next) != &root_) {
			unlink_node(head);
			deallocate(buffer_from_node(head));
		}

		size_ = 0;
	}

	bool empty() override {
		return !size_;
	}

	class const_iterator {
	public:
		const_iterator(const BufferChain<BlockSize>& chain, const BufferChainNode* curr_node)
		               : chain_(chain), curr_node_(curr_node) {}

		const_iterator& operator++() {
			curr_node_ = curr_node_->next;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator current(*this);
			curr_node_ = curr_node_->next;
			return current;
		}

		boost::asio::const_buffer operator*() const {
			auto buffer = chain_.buffer_from_node(curr_node_);
			return boost::asio::const_buffer(buffer->data(), buffer->size());
		}

		bool operator==(const const_iterator& rhs) const {
			return curr_node_ == rhs.curr_node_;
		}

		bool operator!=(const const_iterator& rhs) const {
			return curr_node_ != rhs.curr_node_;
		}

		const_iterator& operator=(const_iterator&) = delete;

#ifdef BUFFER_CHAIN_DEBUG
		std::pair<char*, std::size_t> get_buffer() {
			auto buffer = chain_.buffer_from_node(curr_node_);
			return std::make_pair<char*, std::size_t>(buffer->data(), buffer->size());
		}
#endif

	private:
		const BufferChain<BlockSize>& chain_;
		const BufferChainNode* curr_node_;
	};

	const_iterator begin() const {
		return const_iterator(*this, root_.next);
	}

	const_iterator end() const {
		return const_iterator(*this, &root_);
	}

	friend class const_iterator;
};

}} // spark, ember