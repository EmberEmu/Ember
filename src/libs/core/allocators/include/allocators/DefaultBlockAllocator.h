/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <allocators/HugePages.h>
#if (defined __linux__ || defined __unix__) && defined ENABLE_HUGE_PAGES
#include <sys/mman.h>
#endif
#include <cstddef>
#include <cstdlib>

namespace ember::allocators {

struct DefaultBlockAllocator {
	void* allocate(std::size_t size) {
#if (defined __linux__ || defined __unix__) && defined ENABLE_HUGE_PAGES
		constexpr std::size_t align_to = HUGE_PAGE_MINIMUM;
		const auto rem = storage_size % align_to;
		const auto alloc_size = rem == 0? storage_size : storage_size + (align_to - rem);
		auto allocated = std::aligned_alloc(HUGE_PAGE_MINIMUM, alloc_size));

		if(!allocated) {
			return nullptr;
		}

		madvise(allocated, alloc_size, MADV_HUGEPAGE);
		return allocated;
#else
		return std::malloc(size);
#endif
	}

	void deallocate(void* ptr, std::size_t) {
		std::free(ptr);
	}
};

} // allocators, ember