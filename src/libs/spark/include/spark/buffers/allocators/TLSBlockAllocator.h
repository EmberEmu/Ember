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
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
	#define _DEBUG_TLS_BLOCK_ALLOCATOR
#endif

namespace ember::spark::io {

struct NoPageLock {};
struct PageLock : NoPageLock {};

namespace {

struct FreeBlock {
	FreeBlock* next;
};

template<decltype(auto) size>
concept gt_zero = size > 0;

template<typename T, typename U>
concept sizeof_gte = sizeof(T) >= sizeof(U);

struct NoValidateDealloc {};
struct ValidateDealloc : NoValidateDealloc {};

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
template<typename _ty, std::size_t _elements,
	std::derived_from<NoPageLock> PageLockPolicy = NoPageLock,
	std::derived_from<NoValidateDealloc> ValidatePolicy = NoValidateDealloc>
requires gt_zero<_elements> && sizeof_gte<_ty, FreeBlock>
class Allocator {
	using tid_type = std::conditional_t<
		std::is_same_v<ValidatePolicy, ValidateDealloc>, std::thread::id, std::monostate
	>;

	struct Block {
		_ty obj;

		struct {
			[[no_unique_address]] tid_type thread_id;
			bool using_new;
		} meta;
	};

	static constexpr auto block_size = sizeof(Block);

	FreeBlock* head_ = nullptr;
	[[no_unique_address]] tid_type thread_id_;
	std::array<char, block_size * _elements> storage_;

	void page_lock_conditional() {
		if constexpr(std::is_same_v<PageLockPolicy, PageLock>) {
			util::page_lock(storage_.data(), storage_.size());
		}
	}

	void page_unlock_conditional() {
		if constexpr(std::is_same_v<PageLockPolicy, PageLock>) {
			util::page_unlock(storage_.data(), storage_.size());
		}
	}

	void initialise_free_list() {
		auto storage = storage_.data();

		for(std::size_t i = 0; i < _elements; ++i) {
			auto block = std::start_lifetime_as<FreeBlock>(storage + (block_size * i));
			push(block);
		}
	}

	inline void push(FreeBlock* block) {
		assert(block);
		block->next = head_;
		head_ = block;
	}

	[[nodiscard]] inline FreeBlock* pop() {
		if(!head_) {
			return nullptr;
		}

		auto block = head_;
		head_ = block->next;
		return block;
	}

public:
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	Allocator()	requires std::same_as<ValidatePolicy, ValidateDealloc>
		: thread_id_(std::this_thread::get_id()) {
		page_lock_conditional();
		initialise_free_list();
	}

	Allocator()	{
		page_lock_conditional();
		initialise_free_list();
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		Block* block = std::start_lifetime_as<Block>(pop());

		if(block) [[likely]] {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++storage_active_count;
#endif
			block->meta.using_new = false;
		} else {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			++new_active_count;
#endif
			block = new Block;
			block->meta.using_new = true;
		}

		if constexpr(std::is_same_v<ValidatePolicy, ValidateDealloc>) {
			block->meta.thread_id = thread_id_;
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_allocs;
#endif
		return new (&block->obj) _ty(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);
		auto block = std::start_lifetime_as<Block>(t);

		if constexpr(std::is_same_v<ValidatePolicy, ValidateDealloc>) {
			assert(block->meta.thread_id == thread_id_
				&& "thread policy error or clobbered block");
		}

		if(block->meta.using_new) [[unlikely]] {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--new_active_count;
#endif
			t->~_ty();
			delete block;
		} else {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--storage_active_count;
#endif
			t->~_ty();
			push(std::start_lifetime_as<FreeBlock>(t));
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_deallocs;
#endif
	}

	~Allocator() {
		page_unlock_conditional();

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(!storage_active_count && !new_active_count);
#endif
	}
};

} // unnamed

struct SafeEntrant {};
struct NoRefCounting {};

struct UnsafeEntrant : SafeEntrant {};
struct RefCounting : NoRefCounting {};

template<typename _ty,
	std::size_t _elements,
	std::derived_from<NoRefCounting> RefCountPolicy = NoRefCounting,
	std::derived_from<SafeEntrant> EntrantPolicy = SafeEntrant,
	std::derived_from<NoPageLock> PageLockPolicy = NoPageLock
>
class TLSBlockAllocator final {
	using AllocatorType = Allocator<_ty, _elements, PageLockPolicy>;

	using RefCount = std::conditional_t<
		std::is_same_v<RefCountPolicy, RefCounting>, int, std::monostate
	>;

	using TLSHandleCache = std::conditional_t<
		std::is_same_v<EntrantPolicy, UnsafeEntrant>, AllocatorType*, std::monostate
	>;

	static inline thread_local std::unique_ptr<AllocatorType> allocator_;
	static inline thread_local RefCount ref_count_{};

	[[no_unique_address]] TLSHandleCache cached_handle_{};

	// Compiler will optimise calls to this out when using UnsafeEntrant
	inline void initialise() {
		if constexpr(!std::is_same_v<EntrantPolicy, UnsafeEntrant>) {
			if(!allocator_) {
				allocator_ = std::make_unique<AllocatorType>();
			}
		}
	}

	inline AllocatorType* allocator_handle() {
		if constexpr(std::is_same_v<EntrantPolicy, UnsafeEntrant>) {
			return cached_handle_;
		} else {
			return allocator_.get();
		}
	}

public:
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	TLSBlockAllocator() {
		if constexpr(std::is_same_v<RefCountPolicy, RefCounting>) {
			++ref_count_;
		}

		thread_enter();
	}

	/*
	 * When used in conjunction with UnsafeMigration, allows the owning object
	 * to be executed on another thread without paying for checks on every
	 * allocation
	 */
	inline void thread_enter() {
		if(!allocator_) {
			allocator_ = std::make_unique<AllocatorType>();
		}

		if constexpr(std::is_same_v<EntrantPolicy, UnsafeEntrant>) {
			cached_handle_ = allocator_.get();
		}
 	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		/*
		 * When SafeEntrant is set, need to do this here & in ctor unless
		 * we can be 100% sure that any object using the allocator is created
		 * on the same thread that ends up using it.
		 */
		initialise();

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_allocs;
		++active_allocs;
#endif
		return allocator_handle()->allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_deallocs;
		--active_allocs;
#endif
		allocator_handle()->deallocate(t);
	}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	auto allocator() {
		initialise();
		return allocator_handle();
	}
#endif

	~TLSBlockAllocator() {
		if constexpr(std::is_same_v<RefCountPolicy, RefCounting>) {
			--ref_count_;

			if(ref_count_ == 0) {
				allocator_.reset();
			}
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(active_allocs == 0);
#endif
	}
};

} // io, spark, ember