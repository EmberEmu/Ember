/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace ember::spark {

namespace {

template<typename T, std::size_t _elements>
struct Allocator {
	std::size_t index = 0;
	std::bitset<_elements> used_set;
	T* storage;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	Allocator() : storage(static_cast<T*>(new T[_elements])) {}

	[[nodiscard]] inline T* allocate() {
		for(std::size_t i = 0; i < _elements; ++i) {
			if(!used_set[index]) {
				used_set[index] = true;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
				++storage_active_count;
				++total_allocs;
#endif

				return &storage[index];
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
		if(t < storage || t > (storage + sizeof(T) * _elements)) [[unlikely]] {
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
			--new_active_count;
			++total_deallocs;
#endif
			delete t;
			return;
		}

		const auto base = t - storage;
		const auto index = base % sizeof(T);
		used_set[index] = false;

#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		--storage_active_count;
		++total_deallocs;
#endif
	}

	~Allocator() {
		delete[] storage;
#ifdef _DEBUG_TLS_BLOCK_ALLOCATOR
		assert(!storage_active_count && !storage_use_count);
#endif
	}
};

} // unnamed


template<typename T, std::size_t _elements>
struct TLSBlockAllocator final {
	static inline thread_local Allocator<T, _elements> allocator;

	inline T* allocate() {
		return allocator.allocate();
	}

	inline void deallocate(T* t) {
		allocator.deallocate(t);
	}
};

} // spark, ember