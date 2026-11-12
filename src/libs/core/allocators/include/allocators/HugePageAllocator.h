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
	void* huge_page_allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
	void huge_page_deallocate(void* ptr, std::size_t size, std::size_t alignment = alignof(std::max_align_t));
}

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
		detail::huge_page_deallocate(ptr, sizeof(T) * count);
	}
};

template<class T, class U>
constexpr bool operator==(const HugePageAllocator<T>& lhs, const HugePageAllocator<U>& rhs) noexcept {
	return true;
}

} // allocators, ember