/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/Utility.h>
#include <array>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
#define EMBER_DEBUG_ALLOCATORS
#endif

namespace ember::spark::io {

struct NoPageLock {};
struct PageLock final : NoPageLock {};

struct NoValidateDealloc {};
struct ValidateDealloc final : NoValidateDealloc {};

/*
 * Basic fixed-size block stack allocator that preallocates a slab of memory
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
template<typename _ty, 
	std::size_t _elements,
	std::derived_from<NoPageLock> PageLockPolicy = NoPageLock,
	std::derived_from<NoValidateDealloc> ValidatePolicy = NoValidateDealloc>
requires (_elements > 0)
class BlockAllocator {
	using tid_type = std::conditional_t<
		std::is_same_v<ValidatePolicy, ValidateDealloc>, std::thread::id, std::monostate
	>;

	struct Block {
		Block() {};
		~Block() {};

		// this is fine because we're always reading from the member that was last assigned to
		union {
			Block* next;
			_ty obj;
		};

		struct {
			[[no_unique_address]] tid_type thread_id;
			bool using_new;
		} meta;
	};

	static constexpr auto block_size = sizeof(Block);

	Block* head_ = nullptr;
	[[no_unique_address]] tid_type thread_id_;
	std::array<Block, _elements> storage_;

	void page_lock_conditional() {
		if constexpr(std::is_same_v<PageLockPolicy, PageLock>) {
			utility::page_lock(storage_.data(), storage_.size());
		}
	}

	void page_unlock_conditional() {
		if constexpr(std::is_same_v<PageLockPolicy, PageLock>) {
			utility::page_unlock(storage_.data(), storage_.size());
		}
	}

	void initialise_free_list() {
		for(auto& element : storage_) {
			push(&element);
		}
	}

	inline void push(Block* block) {
		assert(block);
		block->next = head_;
		head_ = block;
	}

	[[nodiscard]] inline Block* pop() {
		if(!head_) {
			return nullptr;
		}

		auto block = head_;
		head_ = block->next;
		return block;
	}

public:
#ifdef EMBER_DEBUG_ALLOCATORS
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	BlockAllocator() requires std::same_as<ValidatePolicy, ValidateDealloc>
		: thread_id_(std::this_thread::get_id()) {
		page_lock_conditional();
		initialise_free_list();
	}

	BlockAllocator() {
		page_lock_conditional();
		initialise_free_list();
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		Block* block = pop();

		if(block) [[likely]] {
#ifdef EMBER_DEBUG_ALLOCATORS
			++storage_active_count;
#endif
			block->meta.using_new = false;
		} else {
#ifdef EMBER_DEBUG_ALLOCATORS
			++new_active_count;
#endif
			block = new Block;
			block->meta.using_new = true;
		}

		if constexpr(std::is_same_v<ValidatePolicy, ValidateDealloc>) {
			block->meta.thread_id = thread_id_;
		}

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_allocs;
		++active_count;
#endif
		return new (&block->obj) _ty(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);
		auto block = reinterpret_cast<Block*>(t);

		if constexpr(std::is_same_v<ValidatePolicy, ValidateDealloc>) {
			assert(block->meta.thread_id == thread_id_
				&& "thread policy violation or clobbered block");
		}

		if(block->meta.using_new) [[unlikely]] {
#ifdef EMBER_DEBUG_ALLOCATORS
			--new_active_count;
#endif
			t->~_ty();
			operator delete(block);
		} else {
#ifdef EMBER_DEBUG_ALLOCATORS
			--storage_active_count;
#endif
			t->~_ty();
			push(block);
		}

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_count;
#endif
	}

	~BlockAllocator() {
		page_unlock_conditional();

#ifdef EMBER_DEBUG_ALLOCATORS
		assert(active_count == 0);
#endif
	}
};

} // io, spark, ember