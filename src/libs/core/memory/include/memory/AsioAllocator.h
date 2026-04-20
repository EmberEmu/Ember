/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PoolAllocator.h"
#include "StaticAllocator.h"
#include <cstdlib>

namespace ember {

template<typename T, typename Allocator>
class AsioAllocator {
	template<typename, typename> friend class AsioAllocator;
	Allocator& allocator_;

public:
	using value_type = T;

	explicit AsioAllocator(Allocator& allocator)
		: allocator_(allocator) {}

	template<typename U, typename V>
	AsioAllocator(const AsioAllocator<U, V>& other) noexcept
		: allocator_(other.allocator_) {}

	bool operator==(const AsioAllocator& other) const noexcept {
		return &allocator_ == &other.allocator_;
	}

	bool operator!=(const AsioAllocator& other) const noexcept {
		return &allocator_ != &other.allocator_;
	}

	value_type* allocate(std::size_t n) const {
		return static_cast<value_type*>(allocator_.allocate(sizeof(value_type) * n));
	}

	void deallocate(value_type* ptr, std::size_t n) const {
		allocator_.deallocate(ptr, sizeof(value_type) * n);
	}
};

} // ember