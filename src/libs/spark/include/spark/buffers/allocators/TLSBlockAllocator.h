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

enum class PagePolicy {
	no_lock, lock
};

namespace {

struct FreeBlock {
	FreeBlock* next;
};

template<decltype(auto) size>
concept gt_zero = size > 0;

template<typename T, typename U>
concept sizeof_gte = sizeof(T) >= sizeof(U);

struct thread_tag{};
struct enforce_same : thread_tag{};
struct no_thread_policy: thread_tag{};

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
	PagePolicy _policy = PagePolicy::no_lock,
	std::derived_from<thread_tag> ThreadPolicy = no_thread_policy
>
requires gt_zero<_elements> && sizeof_gte<_ty, FreeBlock>
class Allocator {
	using tid_type = std::conditional<
		std::is_same_v<ThreadPolicy, enforce_same>, std::thread::id, std::monostate
	>::type;

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

	void initialise() {
		if constexpr(_policy == PagePolicy::lock) {
			util::page_lock(storage_.data(), storage_.size());
		}

		auto storage = storage_.data();

		for(std::size_t i = 0; i < _elements; ++i) {
			auto block = std::start_lifetime_as<FreeBlock>(storage + (block_size * i));
			block->next = reinterpret_cast<FreeBlock*>(storage + (block_size * (i + 1)));
		}

		auto tail = reinterpret_cast<FreeBlock*>(storage + (block_size * (_elements - 1)));
		tail->next = nullptr;
		head_ = reinterpret_cast<FreeBlock*>(storage);
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

	Allocator()	requires requires { std::is_same_v<ThreadPolicy, enforce_same>; }
		: thread_id_(std::this_thread::get_id()) {
		initialise();
	}

	Allocator()	{
		initialise();
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

		if constexpr(std::is_same_v<ThreadPolicy, enforce_same>) {
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

		if constexpr(std::is_same_v<ThreadPolicy, enforce_same>) {
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
		if constexpr(_policy == PagePolicy::lock) {
			util::page_unlock(storage_.data(), storage_.size());
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(!storage_active_count && !new_active_count);
#endif
	}
};

} // unnamed

struct EntrantPolicy{};
struct SafeEntrant : EntrantPolicy{};
struct UnsafeEntrant : EntrantPolicy{};

template<typename _ty,
	std::size_t _elements,
	std::derived_from<EntrantPolicy> entrant = SafeEntrant,
	PagePolicy policy = PagePolicy::lock
>
class TLSBlockAllocator final {
	using AllocatorType = Allocator<_ty, _elements, policy, enforce_same>;

	static inline thread_local std::unique_ptr<AllocatorType> allocator_;

	// Compiler will optimise calls to this out when using UnsafeEntrant
	inline void initialise() {
		if constexpr(std::is_same_v<entrant, SafeEntrant>) {
			if(!allocator_) {
				allocator_ = std::make_unique<AllocatorType>();
			}
		}
	}

public:
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	TLSBlockAllocator() {
		initialise();
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
	auto allocator() {
		initialise();
		return allocator_.get();
	}

	~TLSBlockAllocator() {
		assert(active_allocs == 0);
	}
#endif
};

} // io, spark, ember