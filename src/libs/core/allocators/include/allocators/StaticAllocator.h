/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/AllocTrack.h>
#include <allocators/BlockAllocator.h>
#include <array>
#include <string_view>
#include <cstddef>
#include <cstdlib>

namespace ember::allocators {

template<std::size_t _size, std::size_t _elements>
class StaticAllocator final {
	static constexpr std::string_view tag { "asio_static" };

	struct alignas(std::max_align_t) AlignedStorage {
		std::byte storage[_size];
	};

	template<typename _alloc = DefaultBlockAllocator>
	using BlockAllocator = BlockAllocator<
		AlignedStorage, NoPageLock, NoValidateDealloc, _alloc
	>;

	class SlabAllocator {
		static constexpr std::string_view tag { "asio_static_slab" };
		static constexpr std::size_t slab_size = BlockAllocator<>::block_size * _elements;

		alignas(std::max_align_t) std::array<std::byte, slab_size> storage;
		bool in_use = false;

	public:
		SlabAllocator() {
			ALLOC_TRACK(tag, mem_rep_create);
		}

		~SlabAllocator() {
			ALLOC_TRACK(tag, mem_rep_destroy);
		}

		void* allocate(std::size_t size) {
			ALLOC_TRACK(tag, mem_rep_bytes_alloc, size);

			if(size <= storage.size() && !in_use) {
				ALLOC_TRACK(tag, mem_rep_alloc);
				in_use = true;
				return storage.data();
			} else {
				ALLOC_TRACK(tag, mem_rep_system_alloc);
				return std::malloc(size);
			}
		}

		void deallocate(void* ptr, std::size_t size) {
			ALLOC_TRACK(tag, mem_rep_bytes_dealloc, size);

			if(ptr == storage.data()) {
				ALLOC_TRACK(tag, mem_rep_dealloc);
				in_use = false;
			} else {
				ALLOC_TRACK(tag, mem_rep_system_dealloc);
				std::free(ptr);
			}
		}
	};

	BlockAllocator<SlabAllocator> allocator;

public:
	StaticAllocator()
		: allocator(_elements, tag) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	StaticAllocator(std::string_view tag)
		: allocator(_elements, tag) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	~StaticAllocator() {
		ALLOC_TRACK(tag, mem_rep_destroy);
	}

	void* allocate(std::size_t size) {
		ALLOC_TRACK(tag, mem_rep_bytes_alloc, size);

		if(size <= _size) {
			ALLOC_TRACK(tag, mem_rep_alloc);
			return allocator.allocate();
		} else {
			ALLOC_TRACK(tag, mem_rep_system_alloc);
			return std::malloc(size);
		}
	}

	void deallocate(void* ptr, std::size_t size) {
		ALLOC_TRACK(tag, mem_rep_bytes_dealloc, size);

		if(size <= _size) {
			ALLOC_TRACK(tag, mem_rep_dealloc);
			allocator.deallocate(static_cast<AlignedStorage*>(ptr));
		} else {
			ALLOC_TRACK(tag, mem_rep_system_dealloc);
			std::free(ptr);
		}
	}
};

} // allocators, ember