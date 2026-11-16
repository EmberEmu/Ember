/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <limits>
#include <new>
#include <cstddef>

namespace ember::allocators {

namespace detail {
	void* huge_page_allocate(std::size_t size, std::size_t alignment);
	void huge_page_deallocate(void* ptr, std::size_t size, std::size_t alignment);
}

/*
 * This exists as its own allocator rather than as an allocator policy
 * to avoid platform-specific handling within the user-facing allocator.
 * 
 * Linux's transparent huge pages allows for existing allocations to be
 * explicitly converted to huge pages but Windows requires that allocations
 * are marked as huge pages at the point of allocation, meaning the
 * underlying allocator must be explicitly designed to support it, rather than
 * allowing a policy within the user-facing allocator to mark returned
 * memory as huge pages.
 * 
 * Additionally, allocators can be composed, meaning that memory allocated
 * by one allocator can be subdivided by other allocators, making it
 * undesirable to attempt to mark a subdivided block for huge page conversion.
 * By ensuring the initial allocation explicitly goes through a huge
 * pages allocator, we can be sure that subdivided allocations will not.
 */
template<typename T>
class HugePageAllocator final {
public:
	using value_type = T;

	HugePageAllocator() = default;

	template<typename U>
	HugePageAllocator(const HugePageAllocator<U>&) noexcept {}

	T* allocate(std::size_t count) const {
		if(count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
			throw std::bad_array_new_length();
		}

		return static_cast<T*>(
			detail::huge_page_allocate(sizeof(T) * count, alignof(T))
		);
	}

	void deallocate(T* ptr, std::size_t count) const {
		detail::huge_page_deallocate(ptr, sizeof(T) * count, alignof(T));
	}
};

template<class T, class U>
constexpr bool operator==(const HugePageAllocator<T>& lhs, const HugePageAllocator<U>& rhs) noexcept {
	return true;
}

} // allocators, ember