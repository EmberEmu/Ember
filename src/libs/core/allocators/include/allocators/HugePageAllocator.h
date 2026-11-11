/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

namespace ember::allocators {

namespace detail {
	void* huge_page_allocate(std::size_t size);
	void huge_page_deallocate(void* ptr);
}

template<typename T>
class HugePageAllocator {
public:
	using value_type = T;

	HugePageAllocator() = default;

	template<typename U>
	HugePageAllocator(const HugePageAllocator<U>&) noexcept {}

	T* allocate(std::size_t count) {
		return static_cast<T*>(detail::huge_page_allocate(sizeof(T) * count));
	}

	void deallocate(void* ptr, std::size_t) {
		detail::huge_page_deallocate(ptr);
	}
};

} // allocators, ember