/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

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

namespace detail {

struct FreeBlock {
	FreeBlock* next;
};

template<decltype(auto) size>
concept gt_zero = size > 0;

template<typename T, typename U>
concept sizeof_gte = sizeof(T) >= sizeof(U);

} // detail

using namespace detail;

struct no_validate_dealloc {};
struct validate_dealloc : no_validate_dealloc {};

/**
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
	std::derived_from<no_validate_dealloc> ValidatePolicy = no_validate_dealloc>
requires gt_zero<_elements> && sizeof_gte<_ty, FreeBlock>
class BlockAllocator {
	using tid_type = std::conditional_t<
		std::is_same_v<ValidatePolicy, validate_dealloc>, std::thread::id, std::monostate
	>;

	struct mem_block {
		_ty obj;

		struct {
			[[no_unique_address]] tid_type thread_id;
			bool using_new;
		} meta;
	};

	static constexpr auto block_size = sizeof(mem_block);

	FreeBlock* head_ = nullptr;
	[[no_unique_address]] tid_type thread_id_;
	std::array<char, block_size * _elements> storage_;

	void initialise_free_list() {
		auto storage = storage_.data();

		for(std::size_t i = 0; i < _elements; ++i) {
			auto block = reinterpret_cast<FreeBlock*>(storage + (block_size * i));
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
#ifdef EMBER_DEBUG_ALLOCATORS
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	BlockAllocator() requires std::same_as<ValidatePolicy, validate_dealloc>
		: thread_id_(std::this_thread::get_id()) {
		initialise_free_list();
	}

	BlockAllocator() {
		initialise_free_list();
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		auto block = reinterpret_cast<mem_block*>(pop());

		if(block) [[likely]] {
#ifdef EMBER_DEBUG_ALLOCATORS
			++storage_active_count;
#endif
			block->meta.using_new = false;
		} else {
#ifdef EMBER_DEBUG_ALLOCATORS
			++new_active_count;
#endif
			block = new mem_block;
			block->meta.using_new = true;
		}

		if constexpr(std::is_same_v<ValidatePolicy, validate_dealloc>) {
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
		auto block = reinterpret_cast<mem_block*>(t);

		if constexpr(std::is_same_v<ValidatePolicy, validate_dealloc>) {
			assert(block->meta.thread_id == thread_id_
				&& "thread policy violation or clobbered block");
		}

		if(block->meta.using_new) [[unlikely]] {
#ifdef EMBER_DEBUG_ALLOCATORS
			--new_active_count;
#endif
			t->~_ty();
			delete block;
		} else {
#ifdef EMBER_DEBUG_ALLOCATORS
			--storage_active_count;
#endif
			t->~_ty();
			push(reinterpret_cast<FreeBlock*>(t));
		}

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_count;
#endif
	}

	~BlockAllocator() {
#ifdef EMBER_DEBUG_ALLOCATORS
		assert(active_count == 0);
#endif
	}
};

} // io, spark, ember
