/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/AllocTrack.h>
#include <shared/utility/Utility.h>
#include <array>
#include <concepts>
#include <memory>
#include <new>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#ifndef NDEBUG
#define EMBER_DEBUG_ALLOCATORS
#endif

namespace ember::allocators {

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
	std::derived_from<NoPageLock> PageLockPolicy = NoPageLock,
	std::derived_from<NoValidateDealloc> ValidatePolicy = NoValidateDealloc,
	typename BaseAllocatorType = std::allocator<_ty>
>
class BlockAllocator {
	static constexpr std::string_view default_tag { "block_allocator" };

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

	using AllocatorType = typename std::allocator_traits<BaseAllocatorType>::template rebind_alloc<Block>;

	Block* head_ = nullptr;
	Block* storage_ = nullptr;
	std::size_t elements_ = 0;
	std::string_view tag_;
	AllocatorType allocator_;
	[[no_unique_address]] tid_type thread_id_;

	void page_lock_conditional() {
		if constexpr(std::is_same_v<PageLockPolicy, PageLock>) {
			utility::page_lock(storage_, sizeof(Block) * elements_);
		}
	}

	void page_unlock_conditional() {
		if constexpr(std::is_same_v<PageLockPolicy, PageLock>) {
			utility::page_unlock(storage_, sizeof(Block) * elements_);
		}
	}

	void initialise_free_list() {
		for(std::size_t i = 0; i < elements_; ++i) {
			push(&storage_[i]);
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

	void allocate_storage() {
		constexpr auto block_size = sizeof(Block);
		const auto storage_size = block_size * elements_;
		storage_ = allocator_.allocate(elements_);

		if(!storage_) {
			throw std::bad_alloc();
		}
	}

	void deallocate_storage() {
		allocator_.deallocate(storage_, elements_);
	}
	
	void initialise() {
		ALLOC_TRACK(tag_, mem_rep_create);
		allocate_storage();
		page_lock_conditional();
		initialise_free_list();
	}

public:
	using value_type = _ty;

	static constexpr std::size_t block_size { sizeof(Block) };

#ifdef EMBER_DEBUG_ALLOCATORS
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	BlockAllocator(std::size_t elements, std::string_view tag = default_tag)
		requires std::same_as<ValidatePolicy, ValidateDealloc>
		: elements_(elements)
		, tag_(tag)
		, thread_id_(std::this_thread::get_id()) {
		initialise();
	}

	BlockAllocator(std::size_t elements, std::string_view tag = default_tag)
		: elements_(elements)
		, tag_(tag) {
		initialise();
	}

	BlockAllocator(BlockAllocator&& other) = delete;
	BlockAllocator& operator=(BlockAllocator&& other) = delete;

	BlockAllocator(BlockAllocator&) = delete;
	BlockAllocator& operator=(BlockAllocator&) = delete;

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		Block* block = pop();

		if(block) [[likely]] {
#ifdef EMBER_DEBUG_ALLOCATORS
			++storage_active_count;
#endif
			block->meta.using_new = false;
			ALLOC_TRACK(tag_, mem_rep_alloc);
		} else {
#ifdef EMBER_DEBUG_ALLOCATORS
			++new_active_count;
#endif
			block = new Block;
			block->meta.using_new = true;
			ALLOC_TRACK(tag_, mem_rep_system_alloc);
		}

		if constexpr(std::is_same_v<ValidatePolicy, ValidateDealloc>) {
			block->meta.thread_id = thread_id_;
		}

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_allocs;
		++active_count;
#endif
		ALLOC_TRACK(tag_, mem_rep_bytes_alloc, sizeof(_ty));
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
			ALLOC_TRACK(tag_, mem_rep_system_dealloc);
		} else {
#ifdef EMBER_DEBUG_ALLOCATORS
			--storage_active_count;
#endif
			t->~_ty();
			push(block);
			ALLOC_TRACK(tag_, mem_rep_dealloc);
		}

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_count;
#endif
		ALLOC_TRACK(tag_, mem_rep_bytes_dealloc, sizeof(_ty));
	}

	std::string_view tag() const {
		return tag_;
	}

	~BlockAllocator() {
		page_unlock_conditional();
		deallocate_storage();

#ifdef EMBER_DEBUG_ALLOCATORS
		assert(active_count == 0);
#endif
		ALLOC_TRACK(tag_, mem_rep_destroy);
	}
};

} // allocators, ember