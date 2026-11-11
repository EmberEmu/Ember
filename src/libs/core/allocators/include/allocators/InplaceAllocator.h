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
#include <memory>
#include <cstddef>
#include <cstdlib>

namespace ember::allocators {

template<std::size_t _size, std::size_t _elements>
class InplaceAllocator final {
	static constexpr std::string_view default_tag { "static_allocator" };
	static constexpr std::string_view block_tag   { "static_allocator_block" };

	struct alignas(std::max_align_t) AlignedStorage {
		std::byte storage[_size];
	};

	template<typename _alloc = std::allocator<AlignedStorage>>
	using BlockAllocator = BlockAllocator<
		AlignedStorage, NoPageLock, NoValidateDealloc, _alloc
	>;

	struct SlabStorage {
		static constexpr std::size_t slab_size = BlockAllocator<>::block_size * _elements;
		alignas(std::max_align_t) std::array<std::byte, slab_size> storage{};

		bool in_use = false;
	};

	template<typename T>
	class SlabAllocator {
		static constexpr std::string_view tag = "static_slab_allocator";

		SlabStorage slab;

	public:
		using value_type = T;

		SlabAllocator() {
			ALLOC_TRACK(tag, mem_rep_create);
		}

		template<typename U>
		SlabAllocator(const SlabAllocator<U>& u) noexcept
			: slab(u.slab) {
			ALLOC_TRACK(tag, mem_rep_create);
		}

		~SlabAllocator() {
			ALLOC_TRACK(tag, mem_rep_destroy);
		}

		T* allocate(std::size_t count) {
			const auto size = count * sizeof(T);

			ALLOC_TRACK(tag, mem_rep_bytes_alloc, size);

			if(size <= slab.storage.size() && !slab.in_use) {
				ALLOC_TRACK(tag, mem_rep_alloc);
				slab.in_use = true;
				return reinterpret_cast<T*>(slab.storage.data());
			}

			ALLOC_TRACK(tag, mem_rep_system_alloc);
			return static_cast<T*>(std::malloc(size));
		}

		void deallocate(T* ptr, std::size_t count) {
			const auto size = count * sizeof(T);

			ALLOC_TRACK(tag, mem_rep_bytes_dealloc, size);

			if(static_cast<void*>(ptr) == slab.storage.data()) {
				ALLOC_TRACK(tag, mem_rep_dealloc);
				slab.in_use = false;
			} else {
				ALLOC_TRACK(tag, mem_rep_system_dealloc);
				std::free(ptr);
			}
		}

		template<typename U>
		friend class SlabAllocator;
	};

	BlockAllocator<SlabAllocator<AlignedStorage>> allocator;
	std::string_view tag;

public:
	InplaceAllocator(std::string_view tag = default_tag)
		: allocator(_elements, block_tag)
		, tag(tag) {
		ALLOC_TRACK(tag, mem_rep_create);
	}

	~InplaceAllocator() {
		ALLOC_TRACK(tag, mem_rep_destroy);
	}

	InplaceAllocator(const InplaceAllocator&) = delete;
	InplaceAllocator& operator=(const InplaceAllocator&) = delete;

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