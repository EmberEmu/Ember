/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/allocators/BlockAllocator.h>
#include <array>
#include <string_view>
#include <cstddef>
#include <cstdlib>

namespace ember {

template<std::size_t _size, std::size_t _elements>
class StaticAllocator final {
	static constexpr std::string_view tag { "asio_static_allocator" };

	struct alignas(std::max_align_t) AlignedStorage {
		std::byte storage[_size];
	};

	template<typename _alloc = spark::io::DefaultBlockAllocator>
	using BlockAllocator = spark::io::BlockAllocator<
		AlignedStorage, spark::io::NoPageLock, spark::io::NoValidateDealloc, _alloc
	>;

	class SlabAllocator {
		static constexpr std::size_t slab_size = BlockAllocator<>::block_size * _elements;

		std::array<std::byte, slab_size> storage;
		bool in_use = false;

	public:
		void* allocate(std::size_t size) {
			if(size <= storage.size() && !in_use) {
				in_use = true;
				return storage.data();
			} else {
				return std::malloc(size);
			}
		}

		void deallocate(void* ptr) {
			if(ptr == storage.data()) {
				in_use = false;
			} else {
				std::free(ptr);
			}
		}
	};

	BlockAllocator<SlabAllocator> allocator;

public:
	StaticAllocator() : allocator(_elements, tag) {}

	void* allocate(std::size_t size) {
		if(size <= _size) {
			return allocator.allocate();
		} else {
			return std::malloc(size);
		}
	}

	void deallocate(void* ptr, std::size_t size) {
		if(size <= _size) {
			allocator.deallocate(static_cast<AlignedStorage*>(ptr));
		} else {
			std::free(ptr);
		}
	}
};

} // ember