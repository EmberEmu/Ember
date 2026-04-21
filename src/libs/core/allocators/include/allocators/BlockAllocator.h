/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/AllocTrack.h>
#include <allocators/HugePages.h>
#include <shared/utility/Utility.h>
#include <array>
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
#if (defined __linux__ || defined __unix__) && defined ENABLE_HUGE_PAGES
#include <sys/mman.h>
#endif

#ifndef NDEBUG
#define EMBER_DEBUG_ALLOCATORS
#endif

namespace ember::allocators {

struct NoPageLock {};
struct PageLock final : NoPageLock {};

struct NoValidateDealloc {};
struct ValidateDealloc final : NoValidateDealloc {};

struct DefaultBlockAllocator {
	void* allocate(std::size_t size) {
		return std::malloc(size);
	}

	void deallocate(void* ptr, std::size_t) {
		std::free(ptr);
	}
};

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
	typename UseAllocator = DefaultBlockAllocator
>
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

	Block* head_ = nullptr;
	Block* storage_ = nullptr;
	std::string_view tag_;
	std::size_t elements_ = 0;
	UseAllocator allocator_;
	[[no_unique_address]] tid_type thread_id_;
	std::size_t alloc_size_ = 0;

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

	void allocate_storage(const std::size_t elements) {
		constexpr auto block_size = sizeof(Block);
		const auto storage_size = block_size * elements;

#if (defined __linux__ || defined __unix__) && defined ENABLE_HUGE_PAGES
		constexpr std::size_t align_to = HUGE_PAGE_MINIMUM;
		const auto rem = storage_size % align_to;
		const auto alloc_size = rem == 0? storage_size : storage_size + (align_to - rem);

		storage_ = static_cast<Block*>(std::aligned_alloc(alloc_size, HUGE_PAGE_MINIMUM));
		madvise(storage_, alloc_size, MADV_HUGEPAGE);
		elements_ = alloc_size / block_size;
		alloc_size_ = alloc_size;
#else
		storage_ = static_cast<Block*>(allocator_.allocate(storage_size));
		elements_ = elements;
		alloc_size_ = storage_size;
#endif
	}

	void deallocate_storage() {
#if (defined __linux__ || defined __unix__) && defined ENABLE_HUGE_PAGES
		std::free(storage_);
#else
		allocator_.deallocate(storage_, alloc_size_);
#endif
	}
	
	void initialise(std::size_t elements) {
		ALLOC_TRACK(tag_, mem_rep_create);
		allocate_storage(elements);
		page_lock_conditional();
		initialise_free_list();
	}

public:
	static constexpr std::size_t block_size { sizeof(Block) };

#ifdef EMBER_DEBUG_ALLOCATORS
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	BlockAllocator(std::size_t elements, std::string_view tag = {}) requires std::same_as<ValidatePolicy, ValidateDealloc>
		: tag_(tag)
		, thread_id_(std::this_thread::get_id()) {
		initialise(elements);
	}

	BlockAllocator(std::size_t elements, std::string_view tag = {})
		: tag_(tag) {
		initialise(elements);
	}

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
		ALLOC_TRACK(tag_, mem_rep_bytes_alloc, sizeof(Block));
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
		ALLOC_TRACK(tag_, mem_rep_bytes_dealloc, sizeof(Block));
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