/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/Utility.h>
#include <shared/utility/polyfill/start_lifetime_as>
#include <array>
#include <memory>
#include <new>
#include <thread>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
	#define _DEBUG_TLS_BLOCK_ALLOCATOR
#endif

namespace ember::spark::io {

enum class PagePolicy {
	no_lock, lock
};

enum class ThreadPolicy {
	none, same_thread
};

namespace {

struct FreeBlock {
	FreeBlock* next;
};

template<std::size_t size>
concept gt_zero = size > 0;

template<typename _ty>
concept gte_freeblock = sizeof(_ty) >= sizeof(FreeBlock);

/*
 * Basic fixed-size block allocator that preallocates a slab of memory
 * capable of holding a compile-time determined number of elements.
 * When constructed, a linked list of chunks is built within the slab and
 * each allocation request will take the head node. Since the allocations
 * are fixed-size, the list does not need to be traversed for a suitable
 * size. Deallocations place the chunk as the new head (LIFO).
 *
 * If the preallocated slab runs out of chunks, it will fall back to using the
 * system allocator rather than allocating additional slabs. This means sizing
 * the initial allocation correctly is important for maximum performance, so
 * it's better to be pessimistic. This is a server application and RAM is cheap. :)
 *
 * PagePolicy: 'lock' requests that the OS does not page out the memory slab to disk.
 *
 * ThreadPolicy: 'same_thread' triggers an assert if an allocated object
 * is deallocated from a different thread. Used by the TLS allocator, since
 * implementing the functionality there is messier (and slower).
 */
template<typename _ty, std::size_t _elements, PagePolicy _policy = PagePolicy::no_lock,
	ThreadPolicy _thread_policy = ThreadPolicy::none>
requires gt_zero<_elements> && gte_freeblock<_ty>
struct Allocator {
	struct Block {
		_ty obj;

		struct Metadata {
			std::thread::id thread_id;
			bool using_new;
		} meta;
	};

	static constexpr auto block_size = sizeof(Block);

	std::array<char, block_size * _elements> storage_;
	FreeBlock* head_ = nullptr;
	std::thread::id thread_id_;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	Allocator()	: thread_id_(std::this_thread::get_id())
	{
		if constexpr(_policy == PagePolicy::lock) {
			util::page_lock(storage_.data(), storage_.size());
		}

		link_blocks();
	}

	void link_blocks() {
		auto storage = storage_.data();

		for(std::size_t i = 0; i < _elements; ++i) {
			auto block = std::start_lifetime_as<FreeBlock>(storage + (block_size * i));
			block->next = reinterpret_cast<FreeBlock*>(storage + (block_size * (i + 1)));
		}

		auto tail = reinterpret_cast<FreeBlock*>(storage + (block_size * (_elements - 1)));
		tail->next = nullptr;
		head_ = reinterpret_cast<FreeBlock*>(storage);
	}

	inline void add_block(FreeBlock* block) {
		assert(block);
		block->next = head_;
		head_ = block;
	}

	inline FreeBlock* remove_block(FreeBlock* block) {
		assert(block);
		head_ = block->next;
		return block;
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++total_allocs;
#endif
		Block* block = nullptr;

		if(head_) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++storage_active_count;
#endif
			block = std::start_lifetime_as<Block>(remove_block(head_));
			block->meta.using_new = false;
		} else {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++new_active_count;
#endif
			block = new Block;
			block->meta.using_new = true;
		}

		if constexpr(_thread_policy == ThreadPolicy::same_thread) {
			block->meta.thread_id = thread_id_;
		}

		return new (&block->obj) _ty(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);
		auto block = std::start_lifetime_as<Block>(t);

		if constexpr(_thread_policy == ThreadPolicy::same_thread) {
			assert(block->meta.thread_id == thread_id_
				&& "thread policy error or clobbered block");
		}

		if(block->meta.using_new) [[unlikely]] {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--new_active_count;
			++total_deallocs;
#endif
			t->~_ty();
			delete block;
		} else {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--storage_active_count;
			++total_deallocs;
#endif

			t->~_ty();
			add_block(std::start_lifetime_as<FreeBlock>(t));
		}
	}

	~Allocator() {
		if constexpr(_policy == PagePolicy::lock) {
			util::page_unlock(storage_.data(), storage_.size());
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(!storage_active_count && !new_active_count);
		assert(total_allocs == total_deallocs);
#endif
	}
};

} // unnamed

template<typename _ty, std::size_t _elements, PagePolicy policy = PagePolicy::lock>
class TLSBlockAllocator final {
	using AllocatorType = Allocator<_ty, _elements, policy, ThreadPolicy::same_thread>;

	static inline thread_local std::unique_ptr<AllocatorType> allocator_;

	inline void init_allocator() {
		if(!allocator_) {
			allocator_ = std::make_unique<AllocatorType>();
		}
	}

public:
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	TLSBlockAllocator() {
		init_allocator();
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		/*
		 * Need to do this here & in ctor unless we can be 100% sure that any object using the
		 * allocator is created on the same thread that ends up using it, otherwise nullptr.
		 * That's probably going to be hassle to keep track of, so we'll just do it here too.
		 */
		init_allocator();

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_allocs;
		++active_allocs;
#endif
		return allocator_->allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_deallocs;
		--active_allocs;
#endif
		allocator_->deallocate(t);
	}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	~TLSBlockAllocator() {
		assert(active_allocs == 0);
	}
#endif

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	auto allocator() {
		init_allocator();
		return allocator_.get();
	}
#endif
};

} // io, spark, ember