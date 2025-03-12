/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/allocators/BlockAllocator.h>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
#define EMBER_DEBUG_ALLOCATORS
#endif

namespace ember::spark::io {

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
	using AllocatorType = BlockAllocator<_ty, _elements, PageLockPolicy>;

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
		if constexpr(std::is_same_v<EntrantPolicy, SafeEntrant>) {
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
#ifdef EMBER_DEBUG_ALLOCATORS
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
	 * When used in conjunction with UnsafeEntrant, allows the owning object
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

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_allocs;
		++active_allocs;
#endif
		return allocator_handle()->allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);

#ifdef EMBER_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_allocs;
#endif
		allocator_handle()->deallocate(t);
	}

#ifdef EMBER_DEBUG_ALLOCATORS
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

#ifdef EMBER_DEBUG_ALLOCATORS
		assert(active_allocs == 0);
#endif
	}
};

} // io, spark, ember