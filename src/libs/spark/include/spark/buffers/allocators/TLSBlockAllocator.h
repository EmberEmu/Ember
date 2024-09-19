/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <bitset>
#include <memory>
#include <new>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ember::spark::io {

namespace {

template<typename _ty, std::size_t _elements>
struct Allocator {
	std::unique_ptr<char[]> storage;
	std::size_t index = 0;
	std::bitset<_elements> used_set;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		// lazy allocation to prevent every created thread allocating
		if(!storage) [[unlikely]] {
			storage = std::make_unique<char[]>(sizeof(_ty) * _elements);
		}

		for(std::size_t i = 0; i < _elements; ++i) {
			if(!used_set[index]) {
				used_set[index] = true;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
				++storage_active_count;
				++total_allocs;
#endif

				auto res = &storage[sizeof(_ty) * index];

				++index;

				if(index >= _elements) {
					index = 0;
				}

				return new (res) _ty(std::forward<Args>(args)...);
			}

			++index;

			if(index >= _elements) {
				index = 0;
			}
		}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++new_active_count;
		++total_allocs;
#endif

		return new _ty(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		const auto lower_bound = reinterpret_cast<std::uintptr_t>(storage.get());
		const auto upper_bound = lower_bound + (sizeof(_ty) * _elements);
		const auto t_ptr = reinterpret_cast<std::uintptr_t>(t);

		if(t_ptr < lower_bound || t_ptr >= upper_bound) [[unlikely]] {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--new_active_count;
			++total_deallocs;
#endif
			delete t;
			return;
		}

		const auto offset = t_ptr - lower_bound;
		const auto index = static_cast<std::size_t>(offset / sizeof(_ty));
		assert(used_set[index]);
		used_set[index] = false;
		t->~_ty();

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		--storage_active_count;
		++total_deallocs;
#endif
	}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	~Allocator() {
		assert(!storage_active_count && !new_active_count);
		assert(total_allocs == total_deallocs);
	}
#endif
};

} // unnamed


template<typename _ty, std::size_t _elements>
struct TLSBlockAllocator final {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	template<typename ...Args>
	inline _ty* allocate(Args&&... args) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_allocs;
		++active_allocs;
#endif
		return allocator.allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_deallocs;
		--active_allocs;
#endif
		allocator.deallocate(t);
	}

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	~TLSBlockAllocator() {
		assert(total_allocs == total_deallocs);
		assert(active_allocs == 0);
	}
#endif

#ifndef _DEBUG_TLS_BLOCK_ALLOCATOR
private:
#endif
	static inline thread_local Allocator<_ty, _elements> allocator;
};

} // io, spark, ember