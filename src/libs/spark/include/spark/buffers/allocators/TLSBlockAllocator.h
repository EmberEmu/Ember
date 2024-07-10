/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ember::spark {

namespace {

template<typename T, std::size_t _elements>
struct Allocator {
	T* storage = nullptr;
	std::size_t index = 0;
	std::bitset<_elements> used_set;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	[[nodiscard]] inline T* allocate() {
		// lazy allocation to prevent every created thread allocating
		if(!storage) [[unlikely]] {
			storage = new T[_elements];
		}

		for(std::size_t i = 0; i < _elements; ++i) {
			if(!used_set[index]) {
				used_set[index] = true;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
				++storage_active_count;
				++total_allocs;
#endif

				auto res = &storage[index];

				++index;

				if(index >= _elements) {
					index = 0;
				}

				return res;
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

		return new T;
	}

	inline void deallocate(T* t) {
		const auto lower_bound = reinterpret_cast<std::uintptr_t>(storage);
		const auto upper_bound = lower_bound + (sizeof(T) * _elements);
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
		const auto index = static_cast<std::size_t>(offset / sizeof(T));
		assert(used_set[index]);
		used_set[index] = false;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		--storage_active_count;
		++total_deallocs;
#endif
	}

	~Allocator() {
		delete[] storage;
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(!storage_active_count && !new_active_count);
		assert(total_allocs == total_deallocs);
#endif
	}
};

} // unnamed


template<typename T, std::size_t _elements>
struct TLSBlockAllocator final {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	inline T* allocate() {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		++total_allocs;
		++active_allocs;
#endif
		return allocator.allocate();
	}

	inline void deallocate(T* t) {
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
	static inline thread_local Allocator<T, _elements> allocator;
};

} // spark, ember