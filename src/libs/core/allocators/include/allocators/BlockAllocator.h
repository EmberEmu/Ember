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
#include <limits>
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
 * Basic LIFO object allocator that can fulfill requests for a constructor-specified
 * number of objects.
 * 
 * When constructed, a list of chunks is built within the slab and
 * each allocation request is served from the head node. Since the allocations
 * are fixed-size, the list does not need to be traversed for a suitable
 * size and fragmentation is not possible within the initial slab.
 *
 * If the preallocated slab runs out of objects, it will fall back to using the
 * underlying allocator to fulfill the request rather than allocating additional
 * slabs. This means sizing the initial allocation correctly is important for
 * both performance and keeping fragmentation to a minimum, so it's better to be
 * pessimistic. This is a server application and RAM is cheap (edit: now lies). :)
 * 
 * Thread safety: for performance, all functionality is intentionally unsafe.
 *
 * PagePolicy: 'lock' requests that the OS does not page out the memory slab to disk.
 * If the underlying allocator is huge pages, this is likely already the case and the
 * policy will have no additional effect.
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
class BlockAllocator final {
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
			bool fallback;
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

	void initialise_free_list() noexcept {
		for(std::size_t i = 0; i < elements_; ++i) {
			push(&storage_[i]);
		}
	}

	inline void push(Block* block) noexcept {
		assert(block);
		block->next = head_;
		head_ = block;
	}

	[[nodiscard]] inline Block* pop() noexcept {
		if(!head_) {
			return nullptr;
		}

		auto block = head_;
		head_ = block->next;
		return block;
	}

	void allocate_storage() {
		if(elements_ > std::numeric_limits<std::size_t>::max() / sizeof(Block)) {
			throw std::bad_array_new_length{};
		}

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

		try {
			page_lock_conditional();
		} catch(...) {
			deallocate_storage();
			throw;
		}

		initialise_free_list();
	}

public:
	using value_type = _ty;

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

	[[nodiscard]] inline _ty* allocate() {
		Block* block = pop();

		if(block) [[likely]] {
#ifdef EMBER_DEBUG_ALLOCATORS
			++storage_active_count;
#endif
			block->meta.fallback = false;
			ALLOC_TRACK(tag_, mem_rep_alloc);
		} else {
			block = allocator_.allocate(1);
#ifdef EMBER_DEBUG_ALLOCATORS
			++new_active_count;
#endif
			block->meta.fallback = true;
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
		return &block->obj;
	}

	void deallocate(_ty* t) {
		assert(t);
		auto block = reinterpret_cast<Block*>(t);

		if constexpr(std::is_same_v<ValidatePolicy, ValidateDealloc>) {
			assert(block->meta.thread_id == thread_id_
				&& "thread policy violation or clobbered block");
		}

		if(block->meta.fallback) [[unlikely]] {
			allocator_.deallocate(block, 1);
#ifdef EMBER_DEBUG_ALLOCATORS
			--new_active_count;
#endif
			ALLOC_TRACK(tag_, mem_rep_system_dealloc);
		} else {
			push(block);
#ifdef EMBER_DEBUG_ALLOCATORS
			--storage_active_count;
#endif
			ALLOC_TRACK(tag_, mem_rep_dealloc);
		}

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_count;
#endif
		ALLOC_TRACK(tag_, mem_rep_bytes_dealloc, sizeof(_ty));
	}

	template <class... Args>
	static constexpr void construct(_ty* ptr, Args&&... args) {
		std::construct_at(ptr, std::forward<Args>(args)...);
	}

	static constexpr void destroy(_ty* ptr) {
		std::destroy_at(ptr);
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